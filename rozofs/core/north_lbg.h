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


#ifndef NORTH_LBG_H
#define NORTH_LBG_H
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/un.h>
#include <errno.h>

#include <rozofs/common/types.h>

#include "ruc_common.h"
#include "ruc_list.h"
#include "af_unix_socket_generic_api.h"
#include "af_unix_socket_generic.h"
#include "rozofs_socket_family.h"
#include "ppu_trace.h"
#include "uma_dbg_api.h"
#include "north_lbg_timer.h"
#include "north_lbg_timer_api.h"

#define NORTH__LBG_MAX_ENTRY    (2<<6) /**< max number of entries on a north load-balancer  */
#define NORTH__LBG_TB_MAX_ENTRY  (NORTH__LBG_MAX_ENTRY/sizeof(uint8_t))
/**
* state of a loab balancer eny
*/
typedef enum _north_lbg_entry_state_e
{
   NORTH_LBG_DEPENDENCY = 0,
   NORTH_LBG_UP,
   NORTH_LBG_DOWN,

} north_lbg_entry_state_e;


#define NORTH_LBG_MAX_NAME    64 /**< name of of a north load bamancer */
#define NORTH_LBG_MAX_XMIT_ON_UP_TRANSITION 4

#define NORTH_LBG_MAX_RETRY 3  /**< max number of time that we can reselet a target */
/**
* That structure contains the statistics of a socket : might be common for all
* sockets
*/
typedef struct _north_lbg_stats_t
{

   uint64_t xmitQueuelen;     /**< global xmit queue len */
   uint64_t totalXmitBytes;     /**< total number of bytes that has been sent */
   uint64_t totalXmitRetries;             /**< total number of messages that have been retransmiited    */
   uint64_t totalXmitAborts;             /**< total number of messages that have been retransmiited    */
   uint64_t totalXmitError;             /**< direct error on message send after entry selection    */
   uint64_t totalXmit;             /**< total number of messages submitted       */
   uint64_t totalConnectAttempts;      /**< total number of connect request  */
   uint64_t totalUpDownTransition; /**< total number of messages submitted for with EWOULDBLOCK is returned  */
   uint64_t totalRecv;             /**< total number of messages received       */

} north_lbg_stats_t;


/**
* context associated wih an enry a load balancer entry
*/
typedef struct _north_lbg_entry_ctx_t
{
  ruc_obj_desc_t    link;          /**< To be able to chain the MS context on any list */
  uint32_t            index;         /**<Index of the entry within the load balancer     */
  uint32_t            free;          /**< Is the context free or allocated TRUE/FALSE    */
  /*
  ** specific part
  */
  int               sock_ctx_ref;  /**< socket contex reference                        */
  int               state;         /**< see north_lbg_entry_state_e                    */
  north_lbg_stats_t stats;         /**< entry statistics                               */
  void              *parent;       /**< pointer to the parent load balancer context    */
  north_lbg_tmr_cell_t  rpc_guard_timer;   /**< guard timer associated with a pending connect */
  ruc_obj_desc_t xmitList;       /**< link list of the xmit buffer that have been sent on that interface  */

} north_lbg_entry_ctx_t;


#define NORTH_LBG_MAX_PRIO  2
/**
*  Load abalancer conex
*/
typedef struct _north_lbg_ctx_t
{
  ruc_obj_desc_t    link;          /**< To be able to chain the MS context on any list */
  uint32_t            index;         /**<Index of the MS */
  uint32_t            free;          /**< Is the context free or allocated TRUE/FALSE */
  /*
  ** specific part
  */
  int               state;                   /**< aggregator state: see north_lbg_entry_state_e */
  int               family;                   /**< load balancer family            */
  char              name[NORTH_LBG_MAX_NAME];  /**< name of the load balancer       */
  int               nb_entries_conf;          /**< number of configured entries      */
  int               nb_active_entries;        /**< number of active entries          */
  int               next_entry_idx;           /**< index of the next enry to select  */
  generic_disc_CBK_t  userDiscCallBack;    /**< user disconnect call back (optional)        */
  generic_recv_CBK_t  userRcvCallBack;    /**< user receive call back (mandatory)        */
  uint8_t           entry_bitmap_state[NORTH__LBG_TB_MAX_ENTRY];
  ruc_obj_desc_t xmitList[NORTH_LBG_MAX_PRIO]; /* pending xmit list        */
  north_lbg_stats_t stats;                     /**< load balancing group statistics  */
  north_lbg_entry_ctx_t entry_tb[NORTH__LBG_MAX_ENTRY];
  af_unix_socket_conf_t  lbg_conf;
} north_lbg_ctx_t;


/**
* Prototypes
*/
char * lbg_north_state2String (int x);
void north_lbg_debug_show(uint32_t tcpRef, void *bufRef);
void north_lbg_debug(char * argv[], uint32_t tcpRef, void *bufRef);
void north_lbg_debug_init();
north_lbg_ctx_t *north_lbg_getObjCtx_p(uint32_t north_lbg_ctx_id);
uint32_t north_lbg_getObjCtx_ref(north_lbg_ctx_t *p);
void north_lbg_init();
void north_lbg_entry_init(void *parent,north_lbg_entry_ctx_t *entry_p,uint32_t index);
void  north_lbg_ctxInit(north_lbg_ctx_t *p,uint8_t creation);
north_lbg_ctx_t *north_lbg_alloc();
uint32_t north_lbg_createIndex(uint32_t north_lbg_ctx_id);
uint32_t north_lbg_free_from_idx(uint32_t north_lbg_ctx_id);
uint32_t north_lbg_free_from_ptr(north_lbg_ctx_t *p);

/*__________________________________________________________________________
*/
/**
*  set a bit in a bitmap

  @param entry_idx : index of the entry that must be set
  @param *p  : pointer to the bitmap array

  @retval none
*/
static inline void north_lbg_set_bit(int entry_idx,uint8_t *p)
{
     (p[entry_idx/8] |=(1<<entry_idx%8));
}
/*__________________________________________________________________________
*/
/**
*  clear a bit in a bitmap

  @param entry_idx : index of the entry that must be set
  @param *p  : pointer to the bitmap array

  @retval none
*/
static inline void north_lbg_clear_bit(int entry_idx,uint8_t *p)
{
     (p[entry_idx/8] &=~(1<<entry_idx%8));

}
/*__________________________________________________________________________
*/
/**
* Check a bit in a bitmap

  @param entry_idx : index of the entry that must be set
  @param *p  : pointer to the bitmap array

  @retval 0 if bit is cleared
  @retval <>0 if bit is asserted
 */
static inline int north_lbg_test_bit(int entry_idx,uint8_t *p)
{
     return ((p[entry_idx/8] &(1<<entry_idx%8)));

}


/*__________________________________________________________________________
*/
/**
*  that function checks if the stats of a context are empty

@param stats_p : poiinter to the stats context

@retval 0 non empty
@retval 1 empty
*/
static inline int north_lbg_eval_global_state(north_lbg_ctx_t *lbg_p)
{
   int i;

   north_lbg_entry_ctx_t *entry_p;

   entry_p = lbg_p->entry_tb;

   int state = NORTH_LBG_DEPENDENCY;
   for (i = 0 ; i < lbg_p->nb_entries_conf; i++,entry_p++)
   {
     if (entry_p->state == NORTH_LBG_DOWN)
     {
       if (state == NORTH_LBG_DEPENDENCY) state = NORTH_LBG_DOWN;
       continue;
     }
     if (entry_p->state == NORTH_LBG_UP)
     {
       state = NORTH_LBG_UP;
       break;
     }
   }
   return state;
}


/*__________________________________________________________________________
*/
/**
*  load balanncing group entry state change

  @param entry_p : pointer to the load balancing greoup entry
  @param new_state: new state of the interface

  @retval none
*/
static inline void north_lbg_entry_state_change(north_lbg_entry_ctx_t *entry_p,int new_state)
{
  north_lbg_ctx_t *lbg_p = entry_p->parent;
  int  state;

  if (entry_p->state != new_state)
  {
     entry_p->stats.totalUpDownTransition++;
     entry_p->state = new_state;
     if (entry_p->state == NORTH_LBG_UP)
     {
       north_lbg_set_bit(entry_p->index,lbg_p->entry_bitmap_state);
       if (lbg_p->state != NORTH_LBG_UP)
       {
         lbg_p->state =  NORTH_LBG_UP;
         /*
         ** here we can trigger the polling of the global xmit pending queue of the
         ** load balancer
         */
         lbg_p->stats.totalUpDownTransition++;
//#warning Put code to trigger the polling of the global pending xmit queue
       }
       return;
     }
     /*
     ** the element is going down-> so need to re-evaluate the status of the load balancing group
     ** by check the state of each configured entry
     */
     north_lbg_clear_bit(entry_p->index,lbg_p->entry_bitmap_state);
     state = north_lbg_eval_global_state(lbg_p);
     if (lbg_p->state != state)
     {
       lbg_p->state = state;
       //lbg_p->stats.totalUpDownTransition++;
     }
  }
}

/*__________________________________________________________________________
*/
/**
*  That function returns the index of the next active entry

@param lbg_p : poiinter to load balancing group

@retval -1 : not entry available
@retval <>-1 index of the entry that will be used for sending the message
*/
static inline int north_lbg_get_next_valid_entry(north_lbg_ctx_t *lbg_p)
{
  int start_idx;
  int check_idx;

  check_idx = lbg_p->next_entry_idx;
  for (start_idx = 0; start_idx < lbg_p->nb_entries_conf; start_idx++)
  {
     if (check_idx >= lbg_p->nb_entries_conf) check_idx = 0;
     if (north_lbg_test_bit(check_idx,lbg_p->entry_bitmap_state) == 0)
     {
       check_idx +=1;
       continue;
     }
     /*
     ** update for the next run
     */
     lbg_p->next_entry_idx = check_idx+1;
     return check_idx;
  }
  /*
  ** all the interfaces are down!!
  */
  return -1;
}
/**
*  API that provide the current state of a load balancing Group

 @param lbg_id : index of the load balancing group
 
 @retval    NORTH_LBG_DEPENDENCY : lbg is idle or the reference is out of range
 @retval    NORTH_LBG_UP : at least one connection is UP
 @retval    NORTH_LBG_DOWN : all the connection are down
*/
int north_lbg_get_state(int lbg_id);
#endif
