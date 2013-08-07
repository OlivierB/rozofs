
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



#include <rozofs/rozofs.h>
#include <rozofs/common/log.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <rozofs/core/rozofs_tx_api.h>
#include <rozofs/core/uma_dbg_api.h>
#include "rozofs_rw_load_balancing.h"


/**
* hash table associated with the load balancing amoung the 
* available storcli associated with the rozofs client
*/
ruc_obj_desc_t stclbg_hash_table[STCLBG_HASH_SIZE];

int stclbg_next_idx = 0; /**< index of the next storcli to use */
int stclbg_init_done = 0;
int stclbg_storcli_count = STORCLI_PER_FSMOUNT;

uint64_t stclbg_storcli_stats[STORCLI_PER_FSMOUNT];
/*
 **____________________________________________________
 */
static char localBuf[4096];

#define TRAFFIC_SHAPER_COUNTER(name) pchar += sprintf(pchar," %-20s : %llu\n",#name,(long long unsigned int)p->stats.name);

void show_stclbg(char * argv[], uint32_t tcpRef, void *bufRef) 
{
    char *pchar = localBuf;
    int storcli_count = 0;
    int i;
    
    if (argv[1] != NULL)
    {
      if (strcmp(argv[1],"reset")==0) 
      {
        memset(stclbg_storcli_stats,0,sizeof(stclbg_storcli_stats));  
        uma_dbg_send(tcpRef, bufRef, TRUE, "Reset Done\n");    
        return;      
      }
      if (strcmp(argv[1],"set")==0) 
      {
        errno = 0;       
        storcli_count = (int) strtol(argv[2], (char **) NULL, 10);   
        if (errno != 0) {
         uma_dbg_send(tcpRef, bufRef, TRUE, "bad storcli count (%s set <value>) %s\n",argv[0],strerror(errno));    
         return;     
        } 
        if ( storcli_count > STORCLI_PER_FSMOUNT)
        {
         uma_dbg_send(tcpRef, bufRef, TRUE, "bad storcli count (max is %d)\n",STORCLI_PER_FSMOUNT);    
         return;
        } 
        if ( storcli_count ==  0)
        {
         uma_dbg_send(tcpRef, bufRef, TRUE, "bad storcli count: unsupported value(range: 1..%d)\n",1,STORCLI_PER_FSMOUNT);    
         return;             
        }
        stclbg_storcli_count = storcli_count; 
        stclbg_next_idx = 0;
        memset(stclbg_storcli_stats,0,sizeof(stclbg_storcli_stats));  
        uma_dbg_send(tcpRef, bufRef, TRUE, "Done\n"); 
        return;   
      }      
    }
   pchar += sprintf(pchar,"number of configured storcli: %d\n",  stclbg_storcli_count);
   for (i = 0; i < STORCLI_PER_FSMOUNT; i++)
   pchar += sprintf(pchar,"storcli %d: %llu\n",i, (long long unsigned int) stclbg_storcli_stats[i]);

  uma_dbg_send(tcpRef, bufRef, TRUE, localBuf);
}
/*
 **____________________________________________________
 */
/**
* local function that performs an exact match on fid

  @param key1: fid source
  @param key2 : fid to search
  
  @retval 0: match
  @retval <> 0 no match
*/
static inline int fid_cmp(void *key1, void *key2) {
    return memcmp(key1, key2, sizeof (fid_t));
}
/*
 **____________________________________________________
 */
 /**
 * local function that compute the hash of a fid
 
  @param key: fid
  @retval hash : hash value
 */
static unsigned int fid_hash(void *key) {
    uint32_t hash = 0;
    uint8_t *c;
    for (c = key; c != key + 16; c++)
        hash = *c + (hash << 6) + (hash << 16) - hash;
    return hash;
}


/*
*________________________________________________________
*/
/**
  Search for a context with the same fid.
  That service is intended to be used for read/write
  and truncate service prior to send the request to a
  storcli.

  @param fid: file id to search
   
  @retval <>NULL pointer to searched context
  @retval NULL context is not found
*/
rozofs_tx_rw_lbg_t *stclbg_hash_table_search_ctx(fid_t fid)
{
   unsigned int       hashIdx;
   ruc_obj_desc_t   * phead;
   ruc_obj_desc_t   * elt;
   ruc_obj_desc_t   * pnext;
   rozofs_tx_rw_lbg_t  * p;
   
   if (stclbg_init_done == 0)
   {
     stclbg_init();
   }
   
   /*
   *  Compute the hash from the file handle
   */

   hashIdx = fid_hash((void*)fid);
   hashIdx = hashIdx%STCLBG_HASH_SIZE;   
   /*
   ** Get the head of list
   */
   phead = &stclbg_hash_table[hashIdx];   
   pnext = (ruc_obj_desc_t*)NULL;
   while ((elt = ruc_objGetNext(phead, &pnext)) != NULL) 
   {
      p = (rozofs_tx_rw_lbg_t*) elt;  
      /*
      ** Check fid value
      */      
      if (memcmp(p->fid, fid, sizeof (fid_t)) == 0) 
      {
        /* 
        ** This is our guy. Refresh this entry now
        */
//        stclbg_hash_lookup_hit_count++;
        return p;
      }      
   } 
//   nfs_lbg_cache_stats_table.lookup_miss_count++;
   return NULL;
}

/*
*________________________________________________________
*/
/**
  insert a storcli load balancing context in the hash
  table (stclbg_hash_table).
  

  @param p: pointer to the context to insert
  @param storcli_idx: index of the storcli that has been selected
  @param fid : fid of the file 
   
 
  @retval none
*/
void stclbg_hash_table_insert_ctx(rozofs_tx_rw_lbg_t *p, fid_t fid, int storcli_idx)
{
   unsigned int       hashIdx;
   ruc_obj_desc_t   * phead; 
   
   memcpy(p->fid, fid, sizeof(fid));
   p->storcli_idx = storcli_idx;

   if (stclbg_init_done == 0)
   {
     stclbg_init();
   }     
   /*
   *  Compute the hash from the file handle
   */
   hashIdx = fid_hash((void*)p->fid);
   hashIdx = hashIdx%STCLBG_HASH_SIZE;   

   /*
   ** Get the head of list and insert the context at the tail of the queue
   */
   phead = &stclbg_hash_table[hashIdx];  
   ruc_objInsertTail(phead,(ruc_obj_desc_t*)p);
}
/*
*________________________________________________________
*/
/**
* That service is intended to be called prior
  sending a read/write or truncate request to a storcli
  The fid is the key to search.
  If there is already a request with the same fid in the
  hash table (stclbg_hash_table), then the function returns
  the reference of the storcli found in the entry that matches.
  
  Otherwise, the service gets the next storlci index to use 
  and then inserts an entry in the hash table.
  
  The entry is removed at the end of the transaction (success or error)
  
  @param fid: fid of the file
  
  @retval local index of the storcli to use
  
*/
int stclbg_storcli_idx_from_fid(fid_t fid)
{
  rozofs_tx_rw_lbg_t *p = stclbg_hash_table_search_ctx(fid);
  if ( p == NULL)
  {
    stclbg_next_idx +=1;
    stclbg_storcli_stats[stclbg_next_idx%stclbg_storcli_count]++;
    return (stclbg_next_idx%stclbg_storcli_count);
  }
  stclbg_storcli_stats[p->storcli_idx]++;
  return p->storcli_idx;
}
/*
*________________________________________________________
*/
/**
*  Init service
*
  @param none
  @retval none
  
*/
void stclbg_init()
{

  uint32_t idxCur;

  /*
  ** Initialie the cache table entries
  */
  for (idxCur=0; idxCur<STCLBG_HASH_SIZE; idxCur++) 
  {
     ruc_listHdrInit(&stclbg_hash_table[idxCur]);
  }
  memset(stclbg_storcli_stats,0,sizeof(stclbg_storcli_stats));  

  stclbg_init_done = 1;
  
}