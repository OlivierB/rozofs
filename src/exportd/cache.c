/*
 Copyright (c) 2010 Fizians SAS. <http://www.fizians.com>
 This file is part of Rozofs.

 Rozofs is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation, version 2.

 Rozofs is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see
 <http://www.gnu.org/licenses/>.
 */

#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <errno.h>
#include <string.h>
 
#include <rozofs/common/xmalloc.h>
#include <rozofs/common/profile.h>
#include <rozofs/rpc/epproto.h>

#include "cache.h"


DECLARE_PROFILING(epp_profiler_t);

/*
**___________________________FILE LOCK SERVICE_____________________________
*/

typedef struct _file_lock_stat_t {
  uint64_t    nb_file_lock;
  uint64_t    nb_client_file_lock;
  uint64_t    nb_lock_unlink;
  uint64_t    nb_lock_allocate;
  uint64_t    nb_remove_client;
  uint64_t    nb_add_client;
} file_lock_stat_t;

static file_lock_stat_t file_lock_stat;
/*
** List of the client owning a lock 
*/
static list_t  file_lock_client_list;
/*
** Context of a client
*/
typedef struct _rozofs_file_lock_client_t {
  uint64_t         client_ref;         /**< reference of the client */
  uint64_t         last_poll_time;     /**< time stamp of the last poll received */
  uint64_t         nb_lock;            /**< Number of lock owned by this client */
  list_t           next_client;        /**< Link to the next client in the list of clients */
  list_t           file_lock_list;     /**< List of the lock owned by this client */
} rozofs_file_lock_client_t;


/*
*___________________________________________________________________
* Display lock list
*
* BE CARE FULL. YOU CAN NOT EXECUTE THIS FUNCTION  
* IN THE NON BLOCKING THREAD WHILE THE BLOCKING ONE 
* IS MODIFYING THE LIST !!! 
*___________________________________________________________________
*/
char * debug_file_lock_list(char * pChar) {
  rozofs_file_lock_client_t * client;
  rozofs_file_lock_t        * lock;
  list_t                    * p, * q;
  
  *pChar = 0;
  
  /* Search the given client */
  list_for_each_forward(p, &file_lock_client_list) {
     
    client = list_entry(p, rozofs_file_lock_client_t, next_client);
 
    pChar += sprintf(pChar," Client %16llx has %d locks\n", 
                     (long long unsigned int)client->client_ref,
		     (int)client->nb_lock);
         
    /* loop on the locks */
    list_for_each_forward(q, &client->file_lock_list) {
      char * size;
      lock = list_entry(q,rozofs_file_lock_t, next_client_lock);
  
      switch(lock->lock.size) {
        case EP_LOCK_TOTAL:      size = "TOTAL"; break;
	case EP_LOCK_FROM_START: size = "START"; break;
	case EP_LOCK_TO_END:     size = "END"; break;
	case EP_LOCK_PARTIAL:    size = "PARTIAL"; break;
	default:                 size = "??";
      }      
      pChar += sprintf(pChar,"    client %16llx owner %16llx %-5s %-7s [%llu:%llu]\n", 
	       (long long unsigned int)lock->lock.client_ref, 
	       (long long unsigned int)lock->lock.owner_ref,
	       (lock->lock.mode==EP_LOCK_WRITE)?"WRITE":"READ",	
	       size,       
	       (long long unsigned int)lock->lock.offset_start,
	       (long long unsigned int)lock->lock.offset_stop);
    }       
  } 
  return pChar;
}
/*
*___________________________________________________________________
* Display file lock statistics
*___________________________________________________________________
*/
#define DISPLAY_LOCK_STAT(name) pChar += sprintf(pChar, "  %-20s = %llu\n", #name, (long long unsigned int) file_lock_stat.name); 
char * display_file_lock(char * pChar) {  
  pChar += sprintf(pChar,"\nFile lock statistics:\n");
  DISPLAY_LOCK_STAT(nb_file_lock);
  DISPLAY_LOCK_STAT(nb_client_file_lock);
  DISPLAY_LOCK_STAT(nb_lock_allocate);
  DISPLAY_LOCK_STAT(nb_lock_unlink);
  DISPLAY_LOCK_STAT(nb_add_client);  
  DISPLAY_LOCK_STAT(nb_remove_client);
  return pChar;
}
/*
*___________________________________________________________________
* Check whether two locks are compatible
*
* @param lock1   1rst lock
* @param lock2   2nd lock
*
* @retval 1 when locks are compatible, 0 else
*___________________________________________________________________
*/
int are_file_locks_compatible(struct ep_lock_t * lock1, struct ep_lock_t * lock2) {
  int key;

  if ((lock1->mode == EP_LOCK_READ)&&(lock2->mode == EP_LOCK_READ)) return 1;


  /*
  ** If one of the 2 locks is a write, it must overlap with the other one
  */
  
  key = lock1->size << 8 | lock2->size;
  switch(key) {
  

      return -1;
    
    case (EP_LOCK_FROM_START<<8|EP_LOCK_TO_END): 
    case (EP_LOCK_FROM_START<<8|EP_LOCK_PARTIAL): 
    case (EP_LOCK_PARTIAL<<8|EP_LOCK_TO_END):
      if (lock1->offset_stop <= lock2->offset_start) return 1;
      return 0;  
      
    
    case (EP_LOCK_TO_END<<8|EP_LOCK_FROM_START): 
    case (EP_LOCK_TO_END<<8|EP_LOCK_PARTIAL):   
    case (EP_LOCK_PARTIAL<<8|EP_LOCK_FROM_START):
      if (lock1->offset_start >= lock2->offset_stop) return 1;
      return 0;

    case (EP_LOCK_PARTIAL<<8|EP_LOCK_PARTIAL):    
      if (lock1->offset_start <= lock2->offset_start) {
	if (lock1->offset_stop <= lock2->offset_start) return 1;
	return 0;
      }
      if (lock2->offset_stop <= lock1->offset_start) return 1;
      return 0;
             
                
    default:
    //case (EP_LOCK_TOTAL<<8|EP_LOCK_TOTAL):
    //case (EP_LOCK_TOTAL<<8|EP_LOCK_FROM_START):
    //case (EP_LOCK_TOTAL<<8|EP_LOCK_TO_END):   
    //case (EP_LOCK_TOTAL<<8|EP_LOCK_PARTIAL): 
           
    //case (EP_LOCK_FROM_START<<8|EP_LOCK_TOTAL):
    //case (EP_LOCK_FROM_START<<8|EP_LOCK_FROM_START):
    
    //case (EP_LOCK_TO_END<<8|EP_LOCK_TOTAL):
    //case (EP_LOCK_TO_END<<8|EP_LOCK_TO_END):
    
    //case (EP_LOCK_PARTIAL<<8|EP_LOCK_TOTAL):    
      return 0;   
  }   
}
/*
*___________________________________________________________________
* Check whether two locks are overlapping
*
* @param lock1   1rst lock
* @param lock2   2nd lock
*
* @retval 1 when locks overlap, 0 else
*___________________________________________________________________
*/
int are_file_locks_overlapping(struct ep_lock_t * lock1, struct ep_lock_t * lock2) {
  int key;
  
  
  key = lock1->size << 8 | lock2->size;
  switch(key) {

      return 1;
    
    case (EP_LOCK_FROM_START<<8|EP_LOCK_TO_END): 
    case (EP_LOCK_FROM_START<<8|EP_LOCK_PARTIAL): 
    case (EP_LOCK_PARTIAL<<8|EP_LOCK_TO_END):
      if (lock1->offset_stop < lock2->offset_start) return 0;
      return 1;

    case (EP_LOCK_TO_END<<8|EP_LOCK_FROM_START): 
    case (EP_LOCK_TO_END<<8|EP_LOCK_PARTIAL):   
    case (EP_LOCK_PARTIAL<<8|EP_LOCK_FROM_START):
      if (lock1->offset_start > lock2->offset_stop) return 0;
      return 1;        

    case (EP_LOCK_PARTIAL<<8|EP_LOCK_PARTIAL):    
      if (lock1->offset_start <= lock2->offset_start) {
	if (lock1->offset_stop < lock2->offset_start) return 0;
	return 1;
      }
      if (lock2->offset_stop < lock1->offset_start) return 0;
      return 1;
             
                
    default:
    //case (EP_LOCK_FROM_START<<8|EP_LOCK_FROM_START):
    //case (EP_LOCK_TO_END<<8|EP_LOCK_TO_END):  
    //case (EP_LOCK_TOTAL<<8|EP_LOCK_TOTAL):  
    //case (EP_LOCK_TOTAL<<8|EP_LOCK_TO_END):  
    //case (EP_LOCK_TOTAL<<8|EP_LOCK_FROM_START):  
    //case (EP_LOCK_TOTAL<<8|EP_LOCK_PARTIAL):  
    //case (EP_LOCK_TOTAL<<8|EP_LOCK_TOTAL):  
    //case (EP_LOCK_TO_END<<8|EP_LOCK_TOTAL):  
    //case (EP_LOCK_FROM_START<<8|EP_LOCK_TOTAL):  
    //case (EP_LOCK_PARTIAL<<8|EP_LOCK_TOTAL):           
      return 1;   
  }   
}
/*
*___________________________________________________________________
* Try to concatenate overlapping locks in lock1
*
* @param lock1   1rst lock
* @param lock2   2nd lock
*
* @retval 1 when locks overlap, 0 else
*___________________________________________________________________
*/
#define max(a,b) (a>b?a:b)
#define min(a,b) (a>b?b:a)
int try_file_locks_concatenate(struct ep_lock_t * lock1, struct ep_lock_t * lock2) {
  int key;
  
  
  key = lock1->size << 8 | lock2->size;
  switch(key) {

      return 1;
    
    case (EP_LOCK_FROM_START<<8|EP_LOCK_TO_END): 
      if (lock1->offset_stop < lock2->offset_start) return 0;
      lock1->size = EP_LOCK_TOTAL;
      lock1->offset_stop = lock2->offset_stop;
      return 1;    
      
    case (EP_LOCK_FROM_START<<8|EP_LOCK_PARTIAL): 
      if (lock1->offset_stop < lock2->offset_start) return 0;
      lock1->offset_stop = max(lock2->offset_stop,lock1->offset_stop);
      return 1; 
      
    case (EP_LOCK_PARTIAL<<8|EP_LOCK_TO_END):
      if (lock1->offset_stop < lock2->offset_start) return 0;
      lock1->size = EP_LOCK_TO_END;
      lock1->offset_start = min(lock2->offset_start,lock1->offset_start);      
      lock1->offset_stop = lock2->offset_stop;
      return 1;

    case (EP_LOCK_TO_END<<8|EP_LOCK_FROM_START): 
      if (lock1->offset_start > lock2->offset_stop) return 0;
      lock1->size = EP_LOCK_TOTAL;
      lock1->offset_start = lock2->offset_start;
      return 1;

    case (EP_LOCK_TO_END<<8|EP_LOCK_PARTIAL): 
      if (lock1->offset_start > lock2->offset_stop) return 0;
      lock1->offset_start = min(lock2->offset_start,lock1->offset_start);      
      return 1;
            
    case (EP_LOCK_PARTIAL<<8|EP_LOCK_FROM_START):
      if (lock1->offset_start > lock2->offset_stop) return 0;
      lock1->size = EP_LOCK_FROM_START;
      lock1->offset_stop = max(lock2->offset_stop,lock1->offset_stop);
      lock1->offset_start = lock2->offset_start;      
      return 1;        

    case (EP_LOCK_PARTIAL<<8|EP_LOCK_PARTIAL):    
      if (lock1->offset_start <= lock2->offset_start) {
	if (lock1->offset_stop < lock2->offset_start) return 0;
        lock1->offset_stop = max(lock2->offset_stop,lock1->offset_stop);      	
	return 1;
      }
      if (lock2->offset_stop < lock1->offset_start) return 0;
      lock1->offset_start = lock2->offset_start;      
      lock1->offset_stop = max(lock2->offset_stop,lock1->offset_stop);          
      return 1;
      
    case (EP_LOCK_FROM_START<<8|EP_LOCK_FROM_START):             
      lock1->offset_stop = max(lock2->offset_stop,lock1->offset_stop);          
      return 1;
             
    case (EP_LOCK_TO_END<<8|EP_LOCK_TO_END):  
      lock1->offset_start = min(lock2->offset_start,lock1->offset_start);      
      return 1;
      
    case (EP_LOCK_TOTAL<<8|EP_LOCK_TOTAL): 
    case (EP_LOCK_TOTAL<<8|EP_LOCK_TO_END): 
    case (EP_LOCK_TOTAL<<8|EP_LOCK_FROM_START):  
    case (EP_LOCK_TOTAL<<8|EP_LOCK_PARTIAL):    
      return 1;
             	              
    case (EP_LOCK_TO_END<<8|EP_LOCK_TOTAL):  
    case (EP_LOCK_FROM_START<<8|EP_LOCK_TOTAL):  
    case (EP_LOCK_PARTIAL<<8|EP_LOCK_TOTAL):  
      lock1->size = EP_LOCK_TOTAL;
      lock1->offset_start = lock2->offset_start;      
      lock1->offset_stop  = lock2->offset_stop;                     
      return 1;   
  }   
  return 0;
}
/*
*___________________________________________________________________
* Check whether two lock2 must :free or update lock1
*
* @param lock_free   The free lock operation
* @param lock_set    The set lock that must be checked
*
* @retval 1 when locks are compatible, 0 else
*___________________________________________________________________
*/
int must_file_lock_be_removed(struct ep_lock_t * lock_free, struct ep_lock_t * lock_set, rozofs_file_lock_t ** new_lock_ctx) {
  int       key;
  ep_lock_t new_lock;
  
  *new_lock_ctx = NULL;

  if (lock_free->client_ref != lock_set->client_ref) return 0;
  if (lock_free->owner_ref != lock_set->owner_ref)   return 0;  
  
  key = lock_free->size << 8 | lock_set->size;
  switch(key) {  
    case (EP_LOCK_TOTAL<<8|EP_LOCK_TOTAL):
    case (EP_LOCK_TOTAL<<8|EP_LOCK_FROM_START):
    case (EP_LOCK_TOTAL<<8|EP_LOCK_TO_END):   
    case (EP_LOCK_TOTAL<<8|EP_LOCK_PARTIAL):
      return 1;

    //   FREE #_______...........# 
    //   LOCK #__________________#   
    //        #.......___________#
    case (EP_LOCK_FROM_START<<8|EP_LOCK_TOTAL):
      lock_set->offset_start = lock_free->offset_stop; 
      lock_set->size = EP_LOCK_TO_END;
      return 0;

    //   FREE #_______...........#    FREE #_______...........# 
    //   LOCK #__________........#    LOCK #_____.............#   
    //        #.......___........#         #..................#
    case (EP_LOCK_FROM_START<<8|EP_LOCK_FROM_START):
      if (lock_free->offset_stop >= lock_set->offset_stop) return 1;
      lock_set->offset_start = lock_free->offset_stop; 
      lock_set->size = EP_LOCK_PARTIAL;  
      return 0;    

    //   FREE #_______...........#    FREE #_______...........# 
    //   LOCK #....______________#    LOCK #........._________#  
    //        #.......___________#         #........._________#
    case (EP_LOCK_FROM_START<<8|EP_LOCK_TO_END): 
      if (lock_free->offset_stop <= lock_set->offset_start) return 0;
      lock_set->offset_start = lock_free->offset_stop; 
      return 0;
      
    //   FREE #_______...........#    FREE #_______...........#  FREE #_______...........# 
    //   LOCK #..__..............#    LOCK #....______........#  LOCK #.........______...#  
    //        #..................#         #.......___........#       #.........______...#
    case (EP_LOCK_FROM_START<<8|EP_LOCK_PARTIAL): 
      if (lock_free->offset_stop <= lock_set->offset_start) return 0;
      if (lock_set->offset_stop <= lock_free->offset_stop) return 1;
      lock_set->offset_start = lock_free->offset_stop; 
      return 0;

    //   FREE #..........._______# 
    //   LOCK #__________________#   
    //        #___________.......#            
    case (EP_LOCK_TO_END<<8|EP_LOCK_TOTAL):  
      lock_set->offset_stop = lock_free->offset_start; 
      lock_set->size = EP_LOCK_FROM_START;
      return 0;    

    //   FREE #..........._______#    FREE #..........._______# 
    //   LOCK #_______________...#    LOCK #________..........#
    //        #___________.......#         #________..........#
    case (EP_LOCK_TO_END<<8|EP_LOCK_FROM_START): 
      if (lock_free->offset_start >= lock_set->offset_stop) return 0;
      lock_set->offset_stop = lock_free->offset_start; 
      return 0;
      
    //   FREE #..........._______#    FREE #.......___________# 
    //   LOCK #....______________#    LOCK #........._________#  
    //        #...._______.......#         #..................#
    case (EP_LOCK_TO_END<<8|EP_LOCK_TO_END):
      if (lock_free->offset_start <= lock_set->offset_start) return 1;
      lock_set->offset_stop = lock_free->offset_start; 
      lock_set->size = EP_LOCK_PARTIAL;
      return 0;
      
    //   FREE #..........._______#    FREE #.......___________#  FREE #.......___________# 
    //   LOCK #..__..............#    LOCK #....______........#  LOCK #.........______...#  
    //   LOCK #..__..............#    LOCK #....___...........#  LOCK #..................#  
    case (EP_LOCK_TO_END<<8|EP_LOCK_PARTIAL):   
      if (lock_set->offset_stop <= lock_free->offset_start) return 0;
      if (lock_set->offset_start >= lock_free->offset_start) return 1;
      lock_set->offset_stop = lock_free->offset_start;
      return 0;      

    //   FREE #.........____.....#    
    //   LOCK #__________________#   
    //        #_________...._____#   
    case (EP_LOCK_PARTIAL<<8|EP_LOCK_TOTAL):
      memcpy(&new_lock,lock_set, sizeof(new_lock));
      new_lock.size = EP_LOCK_FROM_START;
      new_lock.offset_stop = lock_free->offset_start;
      *new_lock_ctx = lv2_cache_allocate_file_lock(&new_lock); 
      lock_set->offset_start = lock_free->offset_stop; 
      lock_set->size = EP_LOCK_TO_END;
      return 0;

      
    //   FREE #.........____.....#    FREE #..____............#    FREE #...._______.......# 
    //   LOCK #....______________#    LOCK #........._________#    LOCK #........._________#  
    //        #...._____...._____#         #........._________#         #..........._______#
    case (EP_LOCK_PARTIAL<<8|EP_LOCK_TO_END):
      if (lock_free->offset_stop <= lock_set->offset_start) return 0;
      if (lock_free->offset_start > lock_set->offset_start) {
	memcpy(&new_lock,lock_set, sizeof(new_lock));
	new_lock.size = EP_LOCK_PARTIAL;
	new_lock.offset_stop = lock_free->offset_start;
	*new_lock_ctx = lv2_cache_allocate_file_lock(&new_lock); 
      }
      lock_set->offset_start = lock_free->offset_stop;       
      return 0;  

    //   FREE #...____...........#    FREE #........____......#    FREE #......_______.....# 
    //   LOCK #__________........#    LOCK #_____.............#    LOCK #__________........# 
    case (EP_LOCK_PARTIAL<<8|EP_LOCK_FROM_START):
      if (lock_free->offset_start >= lock_set->offset_stop) return 0;
      if (lock_set->offset_stop <= lock_free->offset_stop) {
        lock_set->offset_stop = lock_free->offset_start;
	return 0;
      }
      memcpy(&new_lock,lock_set, sizeof(new_lock));
      new_lock.size = EP_LOCK_PARTIAL;
      new_lock.offset_start = lock_free->offset_stop;
      *new_lock_ctx = lv2_cache_allocate_file_lock(&new_lock);
      lock_set->offset_stop = lock_free->offset_start;
      return 0;    

    //   FREE #.......___..#    FREE #..____.........#    FREE #..._____.....#  FREE #.....______..#
    //   LOCK #..__........#    LOCK #.........____..#    LOCK #.....______..#  LOCK #..._____.....#
    case (EP_LOCK_PARTIAL<<8|EP_LOCK_PARTIAL):    
      if (lock_free->offset_start >= lock_set->offset_stop) return 0;
      if (lock_set->offset_start >= lock_free->offset_stop) return 0;      
      if (lock_free->offset_start <= lock_set->offset_start) {
        if (lock_free->offset_stop >= lock_set->offset_stop) return 1;
	lock_set->offset_start = lock_free->offset_stop;
	return 0;
      }
      if (lock_free->offset_stop >= lock_set->offset_stop) {
	lock_set->offset_stop = lock_free->offset_start;
	return 0;
      }      
      memcpy(&new_lock,lock_set, sizeof(new_lock));
      new_lock.offset_stop = lock_free->offset_start;
      *new_lock_ctx = lv2_cache_allocate_file_lock(&new_lock);
      lock_set->offset_start = lock_free->offset_stop;
      return 0;  
             
    default:
      return 0;   
  }   
}
/*
*___________________________________________________________________
* initialize the lock service
*
*___________________________________________________________________
*/
void file_lock_service_init(void) {
  memset(&file_lock_stat,0, sizeof(file_lock_stat));
  list_init(&file_lock_client_list);
}
/*
*___________________________________________________________________
* Unlink a lock. This consist in downgrading the global number of lock 
* and unlinking the lock from every list it is in
*
* @param lock The lock to unlink
*
*___________________________________________________________________
*/
static inline void file_lock_unlink(rozofs_file_lock_t * lock) {

  file_lock_stat.nb_lock_unlink++;
  file_lock_stat.nb_file_lock--;
  
  /* Unlink lock from the FID */  
  list_remove(&lock->next_fid_lock);
  /* Unlink lock from the client */  
  list_remove(&lock->next_client_lock);
  
}
/*
*___________________________________________________________________
* Remove all the locks of a client and then remove the client 
*
* @param client_ref reference of the client to remove
*___________________________________________________________________
*/
void file_lock_remove_client(uint64_t client_ref) {
  list_t * p;
  rozofs_file_lock_client_t * client;
  rozofs_file_lock_t        * lock;
  
  /* Search the given client */
  list_for_each_forward(p, &file_lock_client_list) {
     
    client = list_entry(p, rozofs_file_lock_client_t, next_client);
 
    if (client->client_ref == client_ref) {
       
      file_lock_stat.nb_remove_client++;
       
      /* loop on the locks */
      while (!list_empty(&client->file_lock_list)) {
        lock = list_first_entry(&client->file_lock_list,rozofs_file_lock_t, next_client_lock);
        file_lock_unlink(lock);
	free(lock);
      }
      
      /* No more lock on this client. Let's unlink this client */
      list_remove(&client->next_client);
      free(client);
      file_lock_stat.nb_client_file_lock--;
      return;
    }       
  } 
}
/*
*___________________________________________________________________
* Receive a poll request from a client
*
* @param client_ref reference of the client to remove
*___________________________________________________________________
*/
void file_lock_poll_client(uint64_t client_ref) {
  list_t * p;
  rozofs_file_lock_client_t * client;
  uint64_t                    now;
  
  now = time(0);
  
  /* Search the given client */
  list_for_each_forward(p, &file_lock_client_list) {
     
    client = list_entry(p, rozofs_file_lock_client_t, next_client);
 
    if (client->client_ref == client_ref) {
      client->last_poll_time = now;
      break;;
    }   
  }
  
  /* Search for one old client to remove */
  list_for_each_forward(p, &file_lock_client_list) {
     
    client = list_entry(p, rozofs_file_lock_client_t, next_client);
    
    if ((now - client->last_poll_time) > FILE_LOCK_POLL_DELAY_MAX) {
      /* This client has not been polling us for a long time */
      file_lock_remove_client(client->client_ref);
      break;
    }       
  } 
  
}
/*
*___________________________________________________________________
* Remove all the locks of a FID
*
* @param client_ref reference of the client to remove
*___________________________________________________________________
*/
void file_lock_remove_fid_locks(list_t * lock_list) {
  rozofs_file_lock_t        * lock;  

  /* loop on the locks */
  while (!list_empty(lock_list)) {
    lock = list_first_entry(lock_list,rozofs_file_lock_t, next_fid_lock);
    file_lock_unlink(lock);
    free(lock);
  }
}
/*
*___________________________________________________________________
* put lock on a client
* eventualy create the client when it does not exist
*
* @param lock the lock to be set
*___________________________________________________________________
*/
void file_lock_add_lock_to_client(rozofs_file_lock_t * lock) {
  list_t * p;
  rozofs_file_lock_client_t * client;
  
  /* Search the given client */
  list_for_each_forward(p, &file_lock_client_list) {
     
    client = list_entry(p, rozofs_file_lock_client_t, next_client);
 
    if (client->client_ref == lock->lock.client_ref) {
      /* Put the lock in the list of lock of this client */
      list_push_front(&client->file_lock_list,&lock->next_client_lock);
      client->nb_lock++;
      return;
    }       
  }  

  /*
  ** Client does not exist. Allocate a client structure
  */
  file_lock_stat.nb_add_client++;
  client = xmalloc(sizeof(rozofs_file_lock_client_t));
  client->nb_lock = 0;
  client->client_ref = lock->lock.client_ref;
  list_init(&client->next_client);
  list_init(&client->file_lock_list);
  file_lock_stat.nb_client_file_lock++;
  
  /* Put the client in the list of clients */
  list_push_front(&file_lock_client_list,&client->next_client);
  /* Put the lock in the list of lock of this client */
  list_push_front(&client->file_lock_list,&lock->next_client_lock);
  client->nb_lock++;   
}
/*
*___________________________________________________________________
* Create a new lock
*
* @param lock the lock to be set
*
* retval the lock structure
*___________________________________________________________________
*/
rozofs_file_lock_t * lv2_cache_allocate_file_lock(ep_lock_t * lock) {

  /*
  ** Allocate a lock structure
  */
  rozofs_file_lock_t * new_lock = xmalloc(sizeof(rozofs_file_lock_t));
  file_lock_stat.nb_file_lock++;
  file_lock_stat.nb_lock_allocate++;
  /* 
  ** Initialize the lock content
  */
  list_init(&new_lock->next_fid_lock);
  list_init(&new_lock->next_client_lock);
  memcpy(&new_lock->lock, lock, sizeof(ep_lock_t));

  /*
  ** Put the lock on the client 
  */
  file_lock_add_lock_to_client(new_lock);
  
  return new_lock;
}
/*
*___________________________________________________________________
* Remove a lock
*
* @param lock the lock to be removed
*___________________________________________________________________
*/
void lv2_cache_free_file_lock(rozofs_file_lock_t * lock) {
  list_t * p;
  rozofs_file_lock_client_t * client;
  
  file_lock_unlink(lock);
  
  /*
  ** Check whether the client has still a lock set 
  */
  
  /* Search the given client */
  list_for_each_forward(p, &file_lock_client_list) {
     
    client = list_entry(p, rozofs_file_lock_client_t, next_client);
 
    if (client->client_ref == lock->lock.client_ref) {
      client->nb_lock--;
      if (client->nb_lock == 0) file_lock_remove_client(client->client_ref);
      break;
    }       
  }
  
  /*
  ** Free the lock
  */  
  free(lock);
}
/*
**___________________________END OF FILE LOCK SERVICE_____________________________
*/










/**
 * hashing function used to find lv2 entry in the cache
 */
static inline uint32_t lv2_hash(void *key) {
    uint32_t hash = 0;
    uint8_t *c;

    for (c = key; c != key + 16; c++)
        hash = *c + (hash << 6) + (hash << 16) - hash;
    return hash;
}

static inline int lv2_cmp(void *k1, void *k2) {
    return uuid_compare(k1, k2);
}

#define LV2_BUKETS 128
#define LV2_MAX_ENTRIES 256

char * lv2_cache_display(lv2_cache_t *cache, char * pChar) {

  pChar += sprintf(pChar, "lv2 attributes cache : current/max %u/%u\n",cache->size, cache->max);
  pChar += sprintf(pChar, "hit %llu / miss %llu / lru_del %llu\n",
                   (long long unsigned int) cache->hit, 
		   (long long unsigned int)cache->miss,
		   (long long unsigned int)cache->lru_del);
  pChar += sprintf(pChar, "entry size %u - current size %u - maximum size %u\n", 
                   (unsigned int) sizeof(lv2_entry_t), 
		   (unsigned int)sizeof(lv2_entry_t)*cache->size, 
		   (unsigned int)sizeof(lv2_entry_t)*cache->max); 
  return pChar;		   
}
static inline void lv2_cache_unlink(lv2_cache_t *cache,lv2_entry_t *entry) {
  file_lock_remove_fid_locks(&entry->file_lock);
  mattr_release(&entry->attributes);
  list_remove(&entry->list);
  free(entry);
  cache->size--;  
}
void lv2_cache_initialize(lv2_cache_t *cache) {
    cache->max = LV2_MAX_ENTRIES;
    cache->size = 0;
    cache->hit  = 0;
    cache->miss = 0;
    cache->lru_del = 0;
    list_init(&cache->entries);
    htable_initialize(&cache->htable, LV2_BUKETS, lv2_hash, lv2_cmp);
    
    /* 
    ** Lock service initalize 
    */
    file_lock_service_init();
}

void lv2_cache_release(lv2_cache_t *cache) {
    list_t *p, *q;

    list_for_each_forward_safe(p, q, &cache->entries) {
        lv2_entry_t *entry = list_entry(p, lv2_entry_t, list);
        htable_del(&cache->htable, entry->attributes.fid);
        if (S_ISDIR(entry->attributes.mode)) {
            mdir_close(&entry->container.mdir);
        } else if (S_ISREG(entry->attributes.mode)) {
            mreg_close(&entry->container.mreg);
        } else if (S_ISLNK(entry->attributes.mode)) {
            mslnk_close(&entry->container.mslnk);
        }
	lv2_cache_unlink(cache,entry);
    }
}

lv2_entry_t *lv2_cache_put(lv2_cache_t *cache, fid_t fid, const char *path) {
    lv2_entry_t *entry;
    struct stat st;

    START_PROFILING(lv2_cache_put);

    // maybe already cached.
    if ((entry = htable_get(&cache->htable, fid)) != 0) {
        goto out;
    }

    if (stat(path, &st) != 0) {
        goto error;
    }

    entry = xmalloc(sizeof(lv2_entry_t));
    
    /*
    ** Initialize filoe locking 
    */
    list_init(&entry->file_lock);
    entry->nb_locks = 0;
    
    if (S_ISDIR(st.st_mode)) {
        if (mdir_open(&entry->container.mdir, path) != 0) {
            goto error;
        }
        if (mdir_read_attributes(&entry->container.mdir, &entry->attributes) != 0) {
            goto error;
        }
    } else if (S_ISREG(st.st_mode)) {
        if (mreg_open(&entry->container.mreg, path) != 0) {
            goto error;
        }
        if (mreg_read_attributes(&entry->container.mreg, &entry->attributes) != 0) {
            goto error;
        }
    } else if (S_ISLNK(st.st_mode)) {
        if (mslnk_open(&entry->container.mslnk, path) != 0) {
            goto error;
        }
        if (mslnk_read_attributes(&entry->container.mslnk, &entry->attributes) != 0) {
            goto error;
        }
    } else {
        errno = ENOTSUP;
        goto error;
    }

    list_push_front(&cache->entries, &entry->list);
    htable_put(&cache->htable, entry->attributes.fid, entry);
    
    if (cache->size++ >= cache->max) { // remove the lru
        lv2_entry_t *lru;
	
        /*
	** Do not remove entries with a file lock set
	*/
	while (1) {
	  lru = list_entry(cache->entries.prev, lv2_entry_t, list);
	  if (lru->nb_locks == 0) break; /* This guy can be removed */
	  /* This guy should be put at the beginning of the list */
          list_push_front(&cache->entries, &lru->list);	  
	}
	
        if (S_ISDIR(lru->attributes.mode)) {
            mdir_close(&lru->container.mdir);
        } else if (S_ISREG(lru->attributes.mode)) {
            mreg_close(&lru->container.mreg);
        } else if (S_ISLNK(lru->attributes.mode)) {
            mslnk_close(&lru->container.mslnk);
        } else {
            errno = ENOTSUP;
            goto error;
        }
        htable_del(&cache->htable, lru->attributes.fid);
	lv2_cache_unlink(cache,lru);
	cache->lru_del++;
    }

    goto out;
error:
    if (entry) {
        if (S_ISDIR(st.st_mode)) {
            mdir_close(&entry->container.mdir);
        } else if (S_ISREG((st.st_mode))) {
            mreg_close(&entry->container.mreg);
        } else if (S_ISLNK(st.st_mode)) {
            mslnk_close(&entry->container.mslnk);
        }
        free(entry);
        entry = 0;
    }
out:
    STOP_PROFILING(lv2_cache_put);
    return entry;
}

lv2_entry_t *lv2_cache_get(lv2_cache_t *cache, fid_t fid) {
    lv2_entry_t *entry = 0;

    START_PROFILING(lv2_cache_get);

    if ((entry = htable_get(&cache->htable, fid)) != 0) {
        // push the lru
        list_remove(&entry->list);
        list_push_front(&cache->entries, &entry->list);
	cache->hit++;
    }
    else {
      cache->miss++;
    }

    STOP_PROFILING(lv2_cache_get);
    return entry;
}

void lv2_cache_del(lv2_cache_t *cache, fid_t fid) {
    lv2_entry_t *entry = 0;
    START_PROFILING(lv2_cache_del);

    if ((entry = htable_del(&cache->htable, fid)) != 0) {
        if (S_ISDIR(entry->attributes.mode)) {
            mdir_close(&entry->container.mdir);
        } else if (S_ISREG(entry->attributes.mode)) {
            mreg_close(&entry->container.mreg);
        } else if (S_ISLNK(entry->attributes.mode)) {
            mslnk_close(&entry->container.mslnk);
        }
	lv2_cache_unlink(cache,entry);
    }
    STOP_PROFILING(lv2_cache_del);
}
