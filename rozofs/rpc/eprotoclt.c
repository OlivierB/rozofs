/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "eproto.h"
#include <rozofs/rozofs.h>

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

void *
ep_null_1(void *argp, CLIENT *clnt)
{
	static char clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_NULL,
		(xdrproc_t) xdr_void, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((void *)&clnt_res);
}

epgw_mount_ret_t *
ep_mount_1(ep_path_t *argp, CLIENT *clnt)
{
	static epgw_mount_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_MOUNT,
		(xdrproc_t) xdr_ep_path_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_mount_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_status_ret_t *
ep_umount_1(uint32_t *argp, CLIENT *clnt)
{
	static epgw_status_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_UMOUNT,
		(xdrproc_t) xdr_uint32_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_status_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_statfs_ret_t *
ep_statfs_1(uint32_t *argp, CLIENT *clnt)
{
	static epgw_statfs_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_STATFS,
		(xdrproc_t) xdr_uint32_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_statfs_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_mattr_ret_t *
ep_lookup_1(epgw_lookup_arg_t *argp, CLIENT *clnt)
{
	static epgw_mattr_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_LOOKUP,
		(xdrproc_t) xdr_epgw_lookup_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_mattr_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_mattr_ret_t *
ep_getattr_1(epgw_mfile_arg_t *argp, CLIENT *clnt)
{
	static epgw_mattr_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_GETATTR,
		(xdrproc_t) xdr_epgw_mfile_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_mattr_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_mattr_ret_t *
ep_setattr_1(epgw_setattr_arg_t *argp, CLIENT *clnt)
{
	static epgw_mattr_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_SETATTR,
		(xdrproc_t) xdr_epgw_setattr_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_mattr_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_readlink_ret_t *
ep_readlink_1(epgw_mfile_arg_t *argp, CLIENT *clnt)
{
	static epgw_readlink_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_READLINK,
		(xdrproc_t) xdr_epgw_mfile_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_readlink_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_mattr_ret_t *
ep_mknod_1(epgw_mknod_arg_t *argp, CLIENT *clnt)
{
	static epgw_mattr_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_MKNOD,
		(xdrproc_t) xdr_epgw_mknod_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_mattr_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_mattr_ret_t *
ep_mkdir_1(epgw_mkdir_arg_t *argp, CLIENT *clnt)
{
	static epgw_mattr_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_MKDIR,
		(xdrproc_t) xdr_epgw_mkdir_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_mattr_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_fid_ret_t *
ep_unlink_1(epgw_unlink_arg_t *argp, CLIENT *clnt)
{
	static epgw_fid_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_UNLINK,
		(xdrproc_t) xdr_epgw_unlink_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_fid_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_fid_ret_t *
ep_rmdir_1(epgw_rmdir_arg_t *argp, CLIENT *clnt)
{
	static epgw_fid_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_RMDIR,
		(xdrproc_t) xdr_epgw_rmdir_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_fid_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_mattr_ret_t *
ep_symlink_1(epgw_symlink_arg_t *argp, CLIENT *clnt)
{
	static epgw_mattr_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_SYMLINK,
		(xdrproc_t) xdr_epgw_symlink_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_mattr_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_fid_ret_t *
ep_rename_1(epgw_rename_arg_t *argp, CLIENT *clnt)
{
	static epgw_fid_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_RENAME,
		(xdrproc_t) xdr_epgw_rename_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_fid_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_readdir_ret_t *
ep_readdir_1(epgw_readdir_arg_t *argp, CLIENT *clnt)
{
	static epgw_readdir_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_READDIR,
		(xdrproc_t) xdr_epgw_readdir_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_readdir_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_read_block_ret_t *
ep_read_block_1(epgw_io_arg_t *argp, CLIENT *clnt)
{
	static epgw_read_block_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_READ_BLOCK,
		(xdrproc_t) xdr_epgw_io_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_read_block_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_io_ret_t *
ep_write_block_1(epgw_write_block_arg_t *argp, CLIENT *clnt)
{
	static epgw_io_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_WRITE_BLOCK,
		(xdrproc_t) xdr_epgw_write_block_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_io_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_mattr_ret_t *
ep_link_1(epgw_link_arg_t *argp, CLIENT *clnt)
{
	static epgw_mattr_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_LINK,
		(xdrproc_t) xdr_epgw_link_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_mattr_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_status_ret_t *
ep_setxattr_1(epgw_setxattr_arg_t *argp, CLIENT *clnt)
{
	static epgw_status_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_SETXATTR,
		(xdrproc_t) xdr_epgw_setxattr_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_status_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_getxattr_ret_t *
ep_getxattr_1(epgw_getxattr_arg_t *argp, CLIENT *clnt)
{
	static epgw_getxattr_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_GETXATTR,
		(xdrproc_t) xdr_epgw_getxattr_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_getxattr_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_status_ret_t *
ep_removexattr_1(epgw_removexattr_arg_t *argp, CLIENT *clnt)
{
	static epgw_status_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_REMOVEXATTR,
		(xdrproc_t) xdr_epgw_removexattr_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_status_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_listxattr_ret_t *
ep_listxattr_1(epgw_listxattr_arg_t *argp, CLIENT *clnt)
{
	static epgw_listxattr_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_LISTXATTR,
		(xdrproc_t) xdr_epgw_listxattr_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_listxattr_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_cluster_ret_t *
ep_list_cluster_1(uint16_t *argp, CLIENT *clnt)
{
	static epgw_cluster_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_LIST_CLUSTER,
		(xdrproc_t) xdr_uint16_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_cluster_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_conf_ret_t *
ep_conf_storage_1(ep_path_t *argp, CLIENT *clnt)
{
	static epgw_conf_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_CONF_STORAGE,
		(xdrproc_t) xdr_ep_path_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_conf_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

epgw_status_ret_t *
ep_poll_conf_1(ep_gateway_t *argp, CLIENT *clnt)
{
	static epgw_status_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_POLL_CONF,
		(xdrproc_t) xdr_ep_gateway_t, (caddr_t) argp,
		(xdrproc_t) xdr_epgw_status_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

ep_gw_gateway_configuration_ret_t *
ep_conf_expgw_1(ep_path_t *argp, CLIENT *clnt)
{
	static ep_gw_gateway_configuration_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, EP_CONF_EXPGW,
		(xdrproc_t) xdr_ep_path_t, (caddr_t) argp,
		(xdrproc_t) xdr_ep_gw_gateway_configuration_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}
