/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _STORCLI_PROTO_H_RPCGEN
#define _STORCLI_PROTO_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif

#include <rozofs/rozofs.h>

typedef u_char storcli_uuid_t[ROZOFS_UUID_SIZE];

enum storcli_status_t {
	STORCLI_SUCCESS = 0,
	STORCLI_FAILURE = 1,
};
typedef enum storcli_status_t storcli_status_t;

struct storcli_status_ret_t {
	storcli_status_t status;
	union {
		int error;
	} storcli_status_ret_t_u;
};
typedef struct storcli_status_ret_t storcli_status_ret_t;

struct storcli_write_arg_t {
	uint16_t cid;
	uint8_t sid;
	uint8_t empty_file;
	uint32_t layout;
	uint8_t dist_set[ROZOFS_SAFE_MAX];
	storcli_uuid_t fid;
	uint64_t off;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct storcli_write_arg_t storcli_write_arg_t;

struct storcli_write_arg_no_data_t {
	uint16_t cid;
	uint8_t sid;
	uint8_t empty_file;
	uint32_t layout;
	uint8_t dist_set[ROZOFS_SAFE_MAX];
	storcli_uuid_t fid;
	uint64_t off;
	uint32_t len;
};
typedef struct storcli_write_arg_no_data_t storcli_write_arg_no_data_t;

struct storcli_read_arg_t {
	uint16_t cid;
	uint8_t sid;
	uint32_t layout;
	uint8_t spare;
	uint8_t dist_set[ROZOFS_SAFE_MAX];
	storcli_uuid_t fid;
	uint8_t proj_id;
	uint64_t bid;
	uint32_t nb_proj;
};
typedef struct storcli_read_arg_t storcli_read_arg_t;

struct storcli_truncate_arg_t {
	uint16_t cid;
	uint8_t sid;
	uint32_t layout;
	uint8_t spare;
	uint8_t dist_set[ROZOFS_SAFE_MAX];
	storcli_uuid_t fid;
	uint16_t last_seg;
	uint64_t bid;
};
typedef struct storcli_truncate_arg_t storcli_truncate_arg_t;

struct storcli_read_no_data_ret_t {
	uint32_t alignment;
	uint32_t len;
};
typedef struct storcli_read_no_data_ret_t storcli_read_no_data_ret_t;

struct storcli_read_ret_no_data_t {
	storcli_status_t status;
	union {
		storcli_read_no_data_ret_t len;
		int error;
	} storcli_read_ret_no_data_t_u;
};
typedef struct storcli_read_ret_no_data_t storcli_read_ret_no_data_t;

struct storcli_read_data_ret_t {
	uint32_t alignment;
	struct {
		u_int dara_len;
		char *dara_val;
	} dara;
};
typedef struct storcli_read_data_ret_t storcli_read_data_ret_t;

struct storcli_read_ret_t {
	storcli_status_t status;
	union {
		struct storcli_read_data_ret_t data;
		int error;
	} storcli_read_ret_t_u;
};
typedef struct storcli_read_ret_t storcli_read_ret_t;

#define STORCLI_PROGRAM 0x20000007
#define STORCLI_VERSION 1

#if defined(__STDC__) || defined(__cplusplus)
#define STORCLI_NULL 0
extern  void * storcli_null_1(void *, CLIENT *);
extern  void * storcli_null_1_svc(void *, struct svc_req *);
#define STORCLI_WRITE 1
extern  storcli_status_ret_t * storcli_write_1(storcli_write_arg_t *, CLIENT *);
extern  storcli_status_ret_t * storcli_write_1_svc(storcli_write_arg_t *, struct svc_req *);
#define STORCLI_READ 2
extern  storcli_read_ret_t * storcli_read_1(storcli_read_arg_t *, CLIENT *);
extern  storcli_read_ret_t * storcli_read_1_svc(storcli_read_arg_t *, struct svc_req *);
#define STORCLI_TRUNCATE 3
extern  storcli_status_ret_t * storcli_truncate_1(storcli_truncate_arg_t *, CLIENT *);
extern  storcli_status_ret_t * storcli_truncate_1_svc(storcli_truncate_arg_t *, struct svc_req *);
extern int storcli_program_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define STORCLI_NULL 0
extern  void * storcli_null_1();
extern  void * storcli_null_1_svc();
#define STORCLI_WRITE 1
extern  storcli_status_ret_t * storcli_write_1();
extern  storcli_status_ret_t * storcli_write_1_svc();
#define STORCLI_READ 2
extern  storcli_read_ret_t * storcli_read_1();
extern  storcli_read_ret_t * storcli_read_1_svc();
#define STORCLI_TRUNCATE 3
extern  storcli_status_ret_t * storcli_truncate_1();
extern  storcli_status_ret_t * storcli_truncate_1_svc();
extern int storcli_program_1_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_storcli_uuid_t (XDR *, storcli_uuid_t);
extern  bool_t xdr_storcli_status_t (XDR *, storcli_status_t*);
extern  bool_t xdr_storcli_status_ret_t (XDR *, storcli_status_ret_t*);
extern  bool_t xdr_storcli_write_arg_t (XDR *, storcli_write_arg_t*);
extern  bool_t xdr_storcli_write_arg_no_data_t (XDR *, storcli_write_arg_no_data_t*);
extern  bool_t xdr_storcli_read_arg_t (XDR *, storcli_read_arg_t*);
extern  bool_t xdr_storcli_truncate_arg_t (XDR *, storcli_truncate_arg_t*);
extern  bool_t xdr_storcli_read_no_data_ret_t (XDR *, storcli_read_no_data_ret_t*);
extern  bool_t xdr_storcli_read_ret_no_data_t (XDR *, storcli_read_ret_no_data_t*);
extern  bool_t xdr_storcli_read_data_ret_t (XDR *, storcli_read_data_ret_t*);
extern  bool_t xdr_storcli_read_ret_t (XDR *, storcli_read_ret_t*);

#else /* K&R C */
extern bool_t xdr_storcli_uuid_t ();
extern bool_t xdr_storcli_status_t ();
extern bool_t xdr_storcli_status_ret_t ();
extern bool_t xdr_storcli_write_arg_t ();
extern bool_t xdr_storcli_write_arg_no_data_t ();
extern bool_t xdr_storcli_read_arg_t ();
extern bool_t xdr_storcli_truncate_arg_t ();
extern bool_t xdr_storcli_read_no_data_ret_t ();
extern bool_t xdr_storcli_read_ret_no_data_t ();
extern bool_t xdr_storcli_read_data_ret_t ();
extern bool_t xdr_storcli_read_ret_t ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_STORCLI_PROTO_H_RPCGEN */
