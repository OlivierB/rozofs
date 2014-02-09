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

#ifndef RBS_TRANSFORM_H
#define RBS_TRANSFORM_H

typedef enum {
    BLK_TRANSFORM_REQ = 0, /**<inverse transformation is requested  */
    BLK_TRANSFORM_DONE, /**< storaqe read request issued          */
    BLK_ERROR, /**< unable to perform the inverse transformation      */
    BLK_MAX
} rbs_blk_transform_state_e;

/**
 * structure that tracks the invere transformation state of a block for a set of projection
 */
typedef struct rbs_inverse_block {
    uint8_t state;
    uint64_t timestamp; /**< timestamp association with the block -> must be the same on each projection */
    uint16_t effective_length;
} rbs_inverse_block_t;

typedef enum {
    PRJ_READ_IDLE = 0, /**< no read initiated for the projection  */
    PRJ_READ_IN_PRG, /**< storaqe read request issued          */
    PRJ_READ_DONE, /**< storage has answered                 */
    PRJ_READ_ERROR, /**< read error reported by storage      */
    PRJ_READ_MAX
} rbs_read_req_state_e;

typedef enum {
    PRJ_WR_IDLE = 0, /**< no write initiated for the prjection  */
    PRJ_WR_IN_PRG, /**< storaqe write request issued          */
    PRJ_WR_DONE, /**< storage has answered                 */
    PRJ_WR_ERROR, /**< write error reported by storage      */
    PRJ_WR_MAX
} rbs_write_req_state_e;

typedef struct rbs_projection_ctx {
    uint8_t valid_stor_idx : 1; /**< assert to 1 if the relative storage idx is valid */
    uint8_t inuse_valid : 1; /**< assert to 1 if the inuse counter of the buffer has been incremented: just for write case */
    uint8_t stor_idx : 6; /**< relative index of the storage                    */

    uint8_t retry_cpt : 7; /**< current retry counter                                      */
    uint8_t rebuild_req : 1; /**< a rebuild is needed for that storage for this projection   */

    uint8_t prj_state; /**< read state see rozofs_read_req_state_e enum      */
    void *prj_buf; /**< ruc buffer that contains the payload             */
    bin_t *bins; /**< pointer to the payload (data read)               */
} rbs_projection_ctx_t;

/**
 * structure used to keep track of the projections that must be used to rebuild a block
 */
typedef struct rbs_timestamp_ctx {
    uint64_t timestamp; /**< key */
    uint8_t count; /**< number of projection with the same timestamp */
    uint8_t prj_idx_tb[ROZOFS_SAFE_MAX]; /**< table of the projection index that have the same timestamp */
} rbs_timestamp_ctx_t;


/**
 * Local variables
 */
extern rbs_timestamp_ctx_t rbs_timestamp_tb[];
extern uint8_t rbs_timestamp_next_free_idx;

extern projection_t rbs_projections[];
extern angle_t rbs_angles[];
extern uint16_t rbs_psizes[];
extern uint8_t rbs_prj_idx_table[];

/** 
  Apply the transform (to generate only one projection) to a buffer starting
  at "data". That buffer MUST be ROZOFS_BSIZE aligned.
  The first_block_idx is the index of a ROZOFS_BSIZE array in the output buffer
  The number_of_blocks is the number of ROZOFS_BSIZE that must be transform
  Notice that the first_block_idx offset applies to the output transform buffer
  only not to the input buffer pointed by "data".
  
 * @param *prj_ctx_p: pointer to the working array of the projection
 * @param *block_ctx_p: pointer to the working array of blocks
 * @param layout: rozofs layout used
 * @param first_block_idx: index of the first block to transform
 * @param number_of_blocks: number of blocks to transform
 * @param projection_id: id of projection to generate
 * @param *data: pointer to the source data that must be transformed
 *
 * @return: 0 on success -1 otherwise
 */
int rbs_transform_forward_one_proj(rbs_projection_ctx_t *prj_ctx_p,
        rbs_inverse_block_t * block_ctx_p, uint32_t layout,
        uint32_t first_block_idx, uint32_t number_of_blocks,
        tid_t projection_id, char *data);

/** 
  Apply the transform to a buffer starting at "data". That buffer MUST be
  ROZOFS_BSIZE aligned.
  The first_block_idx is the index of a ROZOFS_BSIZE array in the output buffer
  The number_of_blocks is the number of ROZOFS_BSIZE that must be transform
  Notice that the first_block_idx offset applies to the output transform buffer
  only not to the input buffer pointed by "data".
  
 * 
 * @param *prj_ctx_p: pointer to the working array of the projections set
 * @param layout: rozofs layout used for store data
 * @param first_block_idx: index of the first block to transform
 * @param number_of_blocks: number of blocks to write
 * @param *block_ctx_p: pointer to the working array of blocks
 * @param *data: pointer to the source data that must be transformed
 *
 * @return: 0 on success -1 otherwise
 */
int rbs_transform_inverse(rbs_projection_ctx_t *prj_ctx_p, uint32_t layout,
        uint32_t first_block_idx, uint32_t number_of_blocks,
        rbs_inverse_block_t *block_ctx_p, char *data);

#endif
