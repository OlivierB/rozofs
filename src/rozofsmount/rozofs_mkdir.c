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

/* need for crypt */
#define _XOPEN_SOURCE 500
#define FUSE_USE_VERSION 26



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <assert.h>
#include <netinet/tcp.h>

#include <fuse/fuse_lowlevel.h>
#include <fuse/fuse_opt.h>

#include <rozofs/rozofs.h>
#include <rozofs/common/list.h>
#include <rozofs/common/log.h>
#include <rozofs/common/htable.h>
#include <rozofs/common/xmalloc.h>
#include <rozofs/common/profile.h>
#include <rozofs/rpc/sclient.h>
#include <rozofs/rpc/mclient.h>
#include <rozofs/rpc/mpproto.h>
#include <rozofs/rpc/eproto.h>
#include "config.h"
#include "file.h"
#include "rozofs_fuse.h"
#include "rozofs_fuse_api.h"
#include "rozofsmount.h"
#include <rozofs/core/rozofs_tx_common.h>
#include <rozofs/core/rozofs_tx_api.h>

DECLARE_PROFILING(mpp_profiler_t);



/**
* Create a directory
*
* Valid replies:
*   fuse_reply_entry
*   fuse_reply_err
*
* @param req request handle
* @param parent inode number of the parent directory
* @param name to create
* @param mode with which to create the new file
*/
void rozofs_ll_mkdir_cbk(void *this,void *param);

void rozofs_ll_mkdir_nb(fuse_req_t req, fuse_ino_t parent, const char *name,
        mode_t mode) {
    ientry_t *ie = 0;
    const struct fuse_ctx *ctx;
    ep_mkdir_arg_t arg;
    int    ret;        
    void *buffer_p = NULL;
    /*
    ** allocate a context for saving the fuse parameters
    */
    buffer_p = rozofs_fuse_alloc_saved_context();
    if (buffer_p == NULL)
    {
      severe("out of fuse saved context");
      errno = ENOMEM;
      goto error;
    }
    SAVE_FUSE_PARAM(buffer_p,req);
    SAVE_FUSE_PARAM(buffer_p,parent);
    SAVE_FUSE_STRING(buffer_p,name);
    SAVE_FUSE_PARAM(buffer_p,mode);

    START_PROFILING_NB(buffer_p,rozofs_ll_mkdir);

    DEBUG("mkdir (%lu,%s,%04o)\n", (unsigned long int) parent, name,
            (unsigned int) mode);

    ctx = fuse_req_ctx(req);
    mode = (mode | S_IFDIR);

    if (strlen(name) > ROZOFS_FILENAME_MAX) {
        errno = ENAMETOOLONG;
        goto error;
    }
    if (!(ie = get_ientry_by_inode(parent))) {
        errno = ENOENT;
        goto error;
    }
    /*
    ** fill up the structure that will be used for creating the xdr message
    */    
    arg.eid = exportclt.eid;
    memcpy(arg.parent,ie->fid, sizeof (uuid_t));
    arg.name = (char*)name;    
    arg.uid  = ctx->uid;
    arg.gid  = ctx->gid;
    arg.mode = mode;
    /*
    ** now initiates the transaction towards the remote end
    */
    ret = rozofs_export_send_common(&exportclt,EXPORT_PROGRAM, EXPORT_VERSION,
                              EP_MKDIR,(xdrproc_t) xdr_ep_mkdir_arg_t,(void *)&arg,
                              rozofs_ll_mkdir_cbk,buffer_p); 
    if (ret < 0) goto error;
    
    /*
    ** no error just waiting for the answer
    */
    return;
error:
    fuse_reply_err(req, errno);
    /*
    ** release the buffer if has been allocated
    */
    STOP_PROFILING_NB(buffer_p,rozofs_ll_mkdir);
    if (buffer_p != NULL) rozofs_fuse_release_saved_context(buffer_p);
    return;
}

/**
*  Call back function call upon a success rpc, timeout or any other rpc failure
*
 @param this : pointer to the transaction context
 @param param: pointer to the associated rozofs_fuse_context
 
 @return none
 */

void rozofs_ll_mkdir_cbk(void *this,void *param) 
{
   struct fuse_entry_param fep;
   ientry_t *nie = 0;
   struct stat stbuf;
   fuse_req_t req; 
   fuse_ino_t parent;
   ep_mattr_ret_t ret ;
   struct rpc_msg  rpc_reply;

   
   int status;
   uint8_t  *payload;
   void     *recv_buf = NULL;   
   XDR       xdrs;    
   int      bufsize;
   mattr_t  attrs;
   xdrproc_t decode_proc = (xdrproc_t)xdr_ep_mattr_ret_t;

   rpc_reply.acpted_rply.ar_results.proc = NULL;
   RESTORE_FUSE_PARAM(param,req);
   RESTORE_FUSE_PARAM(param,parent);
    /*
    ** get the pointer to the transaction context:
    ** it is required to get the information related to the receive buffer
    */
    rozofs_tx_ctx_t      *rozofs_tx_ctx_p = (rozofs_tx_ctx_t*)this;     
    /*    
    ** get the status of the transaction -> 0 OK, -1 error (need to get errno for source cause
    */
    status = rozofs_tx_get_status(this);
    if (status < 0)
    {
       /*
       ** something wrong happened
       */
       errno = rozofs_tx_get_errno(this);  
       goto error; 
    }
    /*
    ** get the pointer to the receive buffer payload
    */
    recv_buf = rozofs_tx_get_recvBuf(this);
    if (recv_buf == NULL)
    {
       /*
       ** something wrong happened
       */
       errno = EFAULT;  
       goto error;         
    }
    payload  = (uint8_t*) ruc_buf_getPayload(recv_buf);
    payload += sizeof(uint32_t); /* skip length*/
    /*
    ** OK now decode the received message
    */
    bufsize = (int) ruc_buf_getPayloadLen(recv_buf);
    xdrmem_create(&xdrs,(char*)payload,bufsize,XDR_DECODE);
    /*
    ** decode the rpc part
    */
    if (rozofs_xdr_replymsg(&xdrs,&rpc_reply) != TRUE)
    {
     TX_STATS(ROZOFS_TX_DECODING_ERROR);
     errno = EPROTO;
     goto error;
    }
    /*
    ** ok now call the procedure to encode the message
    */
    memset(&ret,0, sizeof(ret));                        
    if (decode_proc(&xdrs,&ret) == FALSE)
    {
       TX_STATS(ROZOFS_TX_DECODING_ERROR);
       errno = EPROTO;
       xdr_free((xdrproc_t) decode_proc, (char *) &ret);
       goto error;
    }   
    if (ret.status == EP_FAILURE) {
        errno = ret.ep_mattr_ret_t_u.error;
        xdr_free((xdrproc_t) decode_proc, (char *) &ret);    
        goto error;
    }
    memcpy(&attrs, &ret.ep_mattr_ret_t_u.attrs, sizeof (mattr_t));
    xdr_free((xdrproc_t) decode_proc, (char *) &ret);    
    /*
    ** end of decoding section
    */
    if (!(nie = get_ientry_by_fid(attrs.fid))) {
        nie = xmalloc(sizeof (ientry_t));
        memcpy(nie->fid, attrs.fid, sizeof (fid_t));
        nie->inode = next_inode_idx();
        list_init(&nie->list);
        nie->db.size = 0;
        nie->db.p = NULL;
        nie->db.eof = 0;
        nie->db.cookie = 0;
        nie->nlookup = 1;
        put_ientry(nie);
    }

    memset(&fep, 0, sizeof (fep));
    fep.ino = nie->inode;
    mattr_to_stat(&attrs, &stbuf);
    stbuf.st_ino = nie->inode;
    fep.attr_timeout = attr_cache_timeo;
    fep.entry_timeout = direntry_cache_timeo;
    memcpy(&fep.attr, &stbuf, sizeof (struct stat));
    nie->nlookup++;

    fuse_reply_entry(req, &fep);
    goto out;
error:
    fuse_reply_err(req, errno);
out:
    /*
    ** release the transaction context and the fuse context
    */
    STOP_PROFILING_NB(param,rozofs_ll_mkdir);
    rozofs_fuse_release_saved_context(param);
    if (rozofs_tx_ctx_p != NULL) rozofs_tx_free_from_ptr(rozofs_tx_ctx_p);    
    if (recv_buf != NULL) ruc_buf_freeBuffer(recv_buf);   
    return;
}
