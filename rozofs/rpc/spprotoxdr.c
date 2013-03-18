/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "spproto.h"
#include <rozofs/rozofs.h>

bool_t
xdr_spp_status_t (XDR *xdrs, spp_status_t *objp)
{
	//register int32_t *buf;

	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_spp_status_ret_t (XDR *xdrs, spp_status_ret_t *objp)
{
	//register int32_t *buf;

	 if (!xdr_spp_status_t (xdrs, &objp->status))
		 return FALSE;
	switch (objp->status) {
	case SPP_FAILURE:
		 if (!xdr_int (xdrs, &objp->spp_status_ret_t_u.error))
			 return FALSE;
		break;
	default:
		break;
	}
	return TRUE;
}

bool_t
xdr_spp_profiler_t (XDR *xdrs, spp_profiler_t *objp)
{
	//register int32_t *buf;

	//int i;
	 if (!xdr_uint64_t (xdrs, &objp->uptime))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->now))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->vers, 20,
		sizeof (uint8_t), (xdrproc_t) xdr_uint8_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->stat, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ports, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->remove, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->read, 3,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->write, 3,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->truncate, 3,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_uint16_t (xdrs, &objp->nb_io_processes))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->io_process_ports, STORAGE_NODE_PORTS_MAX,
		sizeof (uint16_t), (xdrproc_t) xdr_uint16_t))
		 return FALSE;
	 if (!xdr_uint16_t (xdrs, &objp->nb_rb_processes))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->rb_process_ports, STORAGES_MAX_BY_STORAGE_NODE,
		sizeof (uint16_t), (xdrproc_t) xdr_uint16_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->rbs_cids, STORAGES_MAX_BY_STORAGE_NODE,
		sizeof (uint16_t), (xdrproc_t) xdr_uint16_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->rbs_sids, STORAGES_MAX_BY_STORAGE_NODE,
		sizeof (uint8_t), (xdrproc_t) xdr_uint8_t))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->rb_files_current))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->rb_files_total))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_spp_profiler_ret_t (XDR *xdrs, spp_profiler_ret_t *objp)
{
	//register int32_t *buf;

	 if (!xdr_spp_status_t (xdrs, &objp->status))
		 return FALSE;
	switch (objp->status) {
	case SPP_SUCCESS:
		 if (!xdr_spp_profiler_t (xdrs, &objp->spp_profiler_ret_t_u.profiler))
			 return FALSE;
		break;
	case SPP_FAILURE:
		 if (!xdr_int (xdrs, &objp->spp_profiler_ret_t_u.error))
			 return FALSE;
		break;
	default:
		break;
	}
	return TRUE;
}
