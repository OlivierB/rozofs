/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "sproto.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif
#include "rozofs.h"

void
storage_program_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
	union {
		sp_remove_arg_t sp_remove_1_arg;
		sp_write_arg_t sp_write_1_arg;
		sp_read_arg_t sp_read_1_arg;
		sp_truncate_arg_t sp_truncate_1_arg;
		uint16_t sp_stat_1_arg;
	} argument;
	char *result;
	xdrproc_t _xdr_argument, _xdr_result;
	char *(*local)(char *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case SP_NULL:
		_xdr_argument = (xdrproc_t) xdr_void;
		_xdr_result = (xdrproc_t) xdr_void;
		local = (char *(*)(char *, struct svc_req *)) sp_null_1_svc;
		break;

	case SP_REMOVE:
		_xdr_argument = (xdrproc_t) xdr_sp_remove_arg_t;
		_xdr_result = (xdrproc_t) xdr_sp_status_ret_t;
		local = (char *(*)(char *, struct svc_req *)) sp_remove_1_svc;
		break;

	case SP_WRITE:
		_xdr_argument = (xdrproc_t) xdr_sp_write_arg_t;
		_xdr_result = (xdrproc_t) xdr_sp_status_ret_t;
		local = (char *(*)(char *, struct svc_req *)) sp_write_1_svc;
		break;

	case SP_READ:
		_xdr_argument = (xdrproc_t) xdr_sp_read_arg_t;
		_xdr_result = (xdrproc_t) xdr_sp_read_ret_t;
		local = (char *(*)(char *, struct svc_req *)) sp_read_1_svc;
		break;

	case SP_TRUNCATE:
		_xdr_argument = (xdrproc_t) xdr_sp_truncate_arg_t;
		_xdr_result = (xdrproc_t) xdr_sp_status_ret_t;
		local = (char *(*)(char *, struct svc_req *)) sp_truncate_1_svc;
		break;

	case SP_STAT:
		_xdr_argument = (xdrproc_t) xdr_uint16_t;
		_xdr_result = (xdrproc_t) xdr_sp_stat_ret_t;
		local = (char *(*)(char *, struct svc_req *)) sp_stat_1_svc;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	memset ((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		svcerr_decode (transp);
		return;
	}
	result = (*local)((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		exit (1);
	}
	return;
}
