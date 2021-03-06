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
 
 #ifndef EXPGW_FID_CACHE_H
#define EXPGW_FID_CACHE_H


#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include <rozofs/rozofs.h>
#include <rozofs/common/log.h>
#include <rozofs/common/list.h>
#include <rozofs/common/xmalloc.h>
#include <rozofs/common/profile.h>
#include <rozofs/common/mattr.h>
#include <rozofs/core/com_cache.h>
/**
* FID cache constants
*/
#define EXPGW_FID_CACHE_LVL0_SZ_POWER_OF_2  16 
#define EXPGW_FID_CACHE_MAX_ENTRIES  (1024*256)

#define EXPGW_FID_CACHE_LVL0_SZ  (1 << EXPGW_FID_CACHE_LVL0_SZ_POWER_OF_2) 
#define EXPGW_FID_CACHE_LVL0_MASK  (EXPGW_FID_CACHE_LVL0_SZ-1)

/**
* search key for the fid
*/
typedef struct _expgw_fid_key_t
{
  fid_t pfid;  /**< parent unique file id          */
  int name_len; /**< length of the name            */
  char *name;  /**< name of the object             */
} expgw_fid_key_t;


typedef struct _expgw_fid_cache_t
{
  com_cache_entry_t   cache;   /** < common cache structure */
  expgw_fid_key_t     key;
  fid_t fid;                   /**< unique file id          */
} expgw_fid_cache_t;

/*
**______________________________________________________________________________

      FID LOOKUP SECTION
**______________________________________________________________________________
*/
extern com_cache_main_t  *expgw_fid_cache_p; /**< pointer to the fid cache  */

/*
**______________________________________________________________________________
*/
/**
* allocate an entry for the fid cache

  @param pfid : fid of the parent
  @param name: name to search within the parent
  @param fid : fid associated with <pfid,name>
  
  @retval <>NULL: pointer to the cache entry
  @retval NULL : out of memory
*/
com_cache_entry_t *expgw_fid_alloc_entry(fid_t pfid,char *name,unsigned char * fid);


/*
**______________________________________________________________________________
*/
/**
* release an entry of the fid cache

  @param p : pointer to the user cache entry 
  
*/
void expgw_fid_release_entry(void *entry_p);

/*
**______________________________________________________________________________
*/
/**
* creation of the FID cache
 That API is intented to be called during the initialization of the module
 
 The max number of entries is given the EXPGW_FID_CACHE_MAX_ENTRIES constant
 and the size of the level 0 entry set is given by EXPGW_FID_CACHE_LVL0_SZ_POWER_OF_2 constant
 
 retval 0 on success
 retval < 0 on error
*/
 
uint32_t expgw_fid_cache_init();


#ifdef __cplusplus
}
#endif /*__cplusplus */




#endif

