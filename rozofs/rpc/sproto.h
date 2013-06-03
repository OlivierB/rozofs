/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _SPROTO_H_RPCGEN
#define _SPROTO_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif

#include <rozofs/rozofs.h>

typedef uint32_t sp_uuid_t[ROZOFS_UUID_SIZE_RPC];

enum sp_status_t {
	SP_SUCCESS = 0,
	SP_FAILURE = 1,
};
typedef enum sp_status_t sp_status_t;

struct sp_status_ret_t {
	sp_status_t status;
	union {
		int error;
	} sp_status_ret_t_u;
};
typedef struct sp_status_ret_t sp_status_ret_t;

struct sp_write_arg_t {
	uint16_t cid;
	uint8_t sid;
	uint8_t layout;
	uint8_t spare;
	uint32_t dist_set[ROZOFS_SAFE_MAX_RPC];
	sp_uuid_t fid;
	uint8_t proj_id;
	uint64_t bid;
	uint32_t nb_proj;
	uint32_t alignment;
	struct {
		u_int bins_len;
		char *bins_val;
	} bins;
};
typedef struct sp_write_arg_t sp_write_arg_t;

struct sp_write_arg_no_bins_t {
	uint16_t cid;
	uint8_t sid;
	uint8_t layout;
	uint8_t spare;
	uint32_t dist_set[ROZOFS_SAFE_MAX_RPC];
	sp_uuid_t fid;
	uint8_t proj_id;
	uint64_t bid;
	uint32_t nb_proj;
	uint32_t alignment;
	uint32_t len;
};
typedef struct sp_write_arg_no_bins_t sp_write_arg_no_bins_t;

struct sp_read_arg_t {
	uint16_t cid;
	uint8_t sid;
	uint8_t layout;
	uint8_t spare;
	uint32_t dist_set[ROZOFS_SAFE_MAX_RPC];
	sp_uuid_t fid;
	uint64_t bid;
	uint32_t nb_proj;
};
typedef struct sp_read_arg_t sp_read_arg_t;

struct sp_truncate_arg_t {
	uint16_t cid;
	uint8_t sid;
	uint8_t layout;
	uint8_t spare;
	uint8_t dist_set[ROZOFS_SAFE_MAX];
	sp_uuid_t fid;
	uint8_t proj_id;
	uint64_t bid;
};
typedef struct sp_truncate_arg_t sp_truncate_arg_t;

struct sp_read_t {
	uint32_t filler;
	struct {
		u_int bins_len;
		char *bins_val;
	} bins;
	uint64_t file_size;
};
typedef struct sp_read_t sp_read_t;

struct sp_read_ret_t {
	sp_status_t status;
	union {
		sp_read_t rsp;
		int error;
	} sp_read_ret_t_u;
};
typedef struct sp_read_ret_t sp_read_ret_t;

struct sp_write_ret_t {
	sp_status_t status;
	union {
		uint64_t file_size;
		int error;
	} sp_write_ret_t_u;
};
typedef struct sp_write_ret_t sp_write_ret_t;

#define STORAGE_PROGRAM 0x20000002
#define STORAGE_VERSION 1

#if defined(__STDC__) || defined(__cplusplus)
#define SP_NULL 0
extern  void * sp_null_1(void *, CLIENT *);
extern  void * sp_null_1_svc(void *, struct svc_req *);
#define SP_WRITE 1
extern  sp_write_ret_t * sp_write_1(sp_write_arg_t *, CLIENT *);
extern  sp_write_ret_t * sp_write_1_svc(sp_write_arg_t *, struct svc_req *);
#define SP_READ 2
extern  sp_read_ret_t * sp_read_1(sp_read_arg_t *, CLIENT *);
extern  sp_read_ret_t * sp_read_1_svc(sp_read_arg_t *, struct svc_req *);
#define SP_TRUNCATE 3
extern  sp_status_ret_t * sp_truncate_1(sp_truncate_arg_t *, CLIENT *);
extern  sp_status_ret_t * sp_truncate_1_svc(sp_truncate_arg_t *, struct svc_req *);
extern int storage_program_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define SP_NULL 0
extern  void * sp_null_1();
extern  void * sp_null_1_svc();
#define SP_WRITE 1
extern  sp_write_ret_t * sp_write_1();
extern  sp_write_ret_t * sp_write_1_svc();
#define SP_READ 2
extern  sp_read_ret_t * sp_read_1();
extern  sp_read_ret_t * sp_read_1_svc();
#define SP_TRUNCATE 3
extern  sp_status_ret_t * sp_truncate_1();
extern  sp_status_ret_t * sp_truncate_1_svc();
extern int storage_program_1_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_sp_uuid_t (XDR *, sp_uuid_t);
extern  bool_t xdr_sp_status_t (XDR *, sp_status_t*);
extern  bool_t xdr_sp_status_ret_t (XDR *, sp_status_ret_t*);
extern  bool_t xdr_sp_write_arg_t (XDR *, sp_write_arg_t*);
extern  bool_t xdr_sp_write_arg_no_bins_t (XDR *, sp_write_arg_no_bins_t*);
extern  bool_t xdr_sp_read_arg_t (XDR *, sp_read_arg_t*);
extern  bool_t xdr_sp_truncate_arg_t (XDR *, sp_truncate_arg_t*);
extern  bool_t xdr_sp_read_t (XDR *, sp_read_t*);
extern  bool_t xdr_sp_read_ret_t (XDR *, sp_read_ret_t*);
extern  bool_t xdr_sp_write_ret_t (XDR *, sp_write_ret_t*);

#else /* K&R C */
extern bool_t xdr_sp_uuid_t ();
extern bool_t xdr_sp_status_t ();
extern bool_t xdr_sp_status_ret_t ();
extern bool_t xdr_sp_write_arg_t ();
extern bool_t xdr_sp_write_arg_no_bins_t ();
extern bool_t xdr_sp_read_arg_t ();
extern bool_t xdr_sp_truncate_arg_t ();
extern bool_t xdr_sp_read_t ();
extern bool_t xdr_sp_read_ret_t ();
extern bool_t xdr_sp_write_ret_t ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_SPROTO_H_RPCGEN */
