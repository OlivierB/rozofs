/*
 Copyright (c) 2010 Fizians SAS. <http://www.fizians.com>
 This file is part of Rozofs.

 Rozofs is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation; either version 2 of the License,
 or (at your option) any later version.

 Rozofs is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see
 <http://www.gnu.org/licenses/>.
 */

#ifndef ROZOFS_SRV_H
#define ROZOFS_SRV_H

#include <errno.h>

#include <rozofs/common/log.h>
#include <rozofs/common/xmalloc.h>
#include <rozofs/rozofs_srv.h>
#include "rozofs.h"

#define LAYOUT_MAX 1

/**
 * structure used to store the parameters relative to a given layout
 */
typedef struct _rozofs_conf_layout_t {
    uint8_t rozofs_safe; /**< max number of selectable storages       */
    uint8_t rozofs_forward; /**< number of projection to forward         */
    uint8_t rozofs_inverse; /**< number of projections needed to rebuild */
    uint16_t rozofs_psizes_max; /**< size of the larger projection           */
    angle_t *rozofs_angles; /**< p and q angle for each projection       */
    uint16_t *rozofs_psizes; /**< size in bins of each projection         */
} rozofs_conf_layout_t;

extern rozofs_conf_layout_t rozofs_conf_layout_table[];
extern rozofs_layout;

/**
 * Initialize the layout table

 @param none
 @retval none
 */
void rozofs_layout_initialize();


/**
 * Release the layout table

 @param none
 @retval none
 */
void rozofs_layout_release();

/**
 * Set the layout value

 @param layout
 @retval 0 if error
 */
int set_layout(int32_t layout);

/**
  Get the rozofs_inverse for a given layout
  
  @param layout : layout association with the file
  
  @retval rozofs_inverse associated with the layout
 */
static inline uint8_t rozofs_get_rozofs_inverse(uint32_t layout) {
    if (set_layout(layout) == 0) return 0;
    return rozofs_conf_layout_table.rozofs_inverse;

}

/**
  Get the rozofs_forward for a given layout
  
  @param layout : layout association with the file
  
  @retval rozofs_inverse associated with the layout
 */
static inline uint8_t rozofs_get_rozofs_forward(uint32_t layout) {
    if (set_layout(layout) == 0) return 0;
    return rozofs_conf_layout_table.rozofs_forward;

}

/**
  Get the rozofs_safe for a given layout
  
  @param layout : layout association with the file
  
  @retval rozofs_inverse associated with the layout
 */
static inline uint8_t rozofs_get_rozofs_safe(uint32_t layout) {
    if (set_layout(layout) == 0) return 0;
    return rozofs_conf_layout_table.rozofs_safe;

}

/**
  Get the angle "p" for a given layout
  
  @param layout : layout association with the file
  @param projection_id : projection index in the layout
  
  @retval angle value
 */
static inline int rozofs_get_angles_p(uint32_t layout, uint8_t projection_id) {
    if (set_layout(layout) == 0) return 0;
    return rozofs_conf_layout_table.rozofs_angles[projection_id].p;
}

/**
  Get the angle "q" for a given layout
  
  @param layout : layout association with the file
  @param projection_id : projection index in the layout
  
  @retval angle value
 */
static inline int rozofs_get_angles_q(uint32_t layout, uint8_t projection_id) {
    if (set_layout(layout) == 0) return 0;
    return rozofs_conf_layout_table.rozofs_angles[projection_id].q;
}

/**
  Get the projection size for a given projection_id in the layout 
  
  @param layout : layout association with the file
  @param projection_id : projection index in the layout
  
  @retval projection size
 */
static inline int rozofs_get_psizes(uint32_t layout, uint8_t projection_id) {
    if (set_layout(layout) == 0) return 0;
    return rozofs_conf_layout_table.rozofs_psizes[projection_id];
}

/**
  Get the projection max size for a given  layout 
  
  @param layout : layout association with the file
  
  @retval projection size
 */
static inline int rozofs_get_max_psize(uint32_t layout) {
    if (set_layout(layout) == 0) return 0;
    return rozofs_conf_layout_table.rozofs_psizes_max;
}


#endif
