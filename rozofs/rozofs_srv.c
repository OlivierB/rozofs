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

#include <errno.h>
#include <string.h>
#include <rozofs/common/log.h>
#include <rozofs/common/xmalloc.h>
#include "rozofs.h"
#include "rozofs_srv.h"

rozofs_conf_layout_t rozofs_conf_layout_table;
uint32_t rozofs_layout;

// void rozofs_layout_initialize() {
//     int i;
//     uint32_t layout;
//     rozofs_conf_layout_t *p;

//     p = rozofs_conf_layout_table;
//     memset(p, 0, sizeof (rozofs_conf_layout_t) * LAYOUT_MAX);
//     for (layout = 0; layout < LAYOUT_MAX; layout++, p++) {
//         switch (layout) {
//             case LAYOUT_2_3_4:
//                 p->rozofs_safe = 4;
//                 p->rozofs_forward = 3;
//                 p->rozofs_inverse = 2;
//                 break;
//             case LAYOUT_3_5_7:
//                 p->rozofs_safe = 7;
//                 p->rozofs_forward = 5;
//                 p->rozofs_inverse = 3;
//                 break;
//             case LAYOUT_4_6_8:
//                 p->rozofs_safe = 8;
//                 p->rozofs_forward = 6;
//                 p->rozofs_inverse = 4;
//                 break;
//             case LAYOUT_8_12_16:
//                 p->rozofs_safe = 16;
//                 p->rozofs_forward = 12;
//                 p->rozofs_inverse = 8;
//                 break;
//             default:
//                 break;
//         }

//         DEBUG("initialize rozofs with inverse: %u, forward: %u, safe: %u",
//                 p->rozofs_inverse, p->rozofs_forward, p->rozofs_safe);

//         p->rozofs_angles = xmalloc(sizeof (angle_t) * p->rozofs_forward);
//         p->rozofs_psizes = xmalloc(sizeof (uint16_t) * p->rozofs_forward);

//         for (i = 0; i < p->rozofs_forward; i++) {
//             p->rozofs_angles[i].p = i - p->rozofs_forward / 2;
//             p->rozofs_angles[i].q = 1;
//             p->rozofs_psizes[i] = abs(i - p->rozofs_forward / 2) * (p->rozofs_inverse - 1)
//                     + (ROZOFS_BSIZE / sizeof (pxl_t) / p->rozofs_inverse - 1) + 1;
//             if (p->rozofs_psizes[i] > p->rozofs_psizes_max) p->rozofs_psizes_max = p->rozofs_psizes[i];
//         }
//     }
// }

void rozofs_layout_initialize() {
    rozofs_layout = -1;
}

int set_layout(uint32_t layout) {
    int i;
    
    if (rozofs_layout == layout) {
        return 1;
    } else if (rozofs_layout == -1) {
        DEBUG("Le layout : %u", layout);
	rozofs_layout = layout;

        rozofs_conf_layout_t *p;
        p = &rozofs_conf_layout_table;

        p->rozofs_safe = layout >> 16;
        p->rozofs_forward = (layout >> 8) & 0xFF;
        p->rozofs_inverse = (layout & 0xFF);

        DEBUG("initialize rozofs with inverse: %u, forward: %u, safe: %u",
                p->rozofs_inverse, p->rozofs_forward, p->rozofs_safe);

        p->rozofs_angles = xmalloc(sizeof (angle_t) * p->rozofs_forward);
        p->rozofs_psizes = xmalloc(sizeof (uint16_t) * p->rozofs_forward);

        for (i = 0; i < p->rozofs_forward; i++) {
            p->rozofs_angles[i].p = i - p->rozofs_forward / 2;
            p->rozofs_angles[i].q = 1;
            p->rozofs_psizes[i] = abs(i - p->rozofs_forward / 2) * (p->rozofs_inverse - 1)
                    + (ROZOFS_BSIZE / sizeof (pxl_t) / p->rozofs_inverse - 1) + 1;
            if (p->rozofs_psizes[i] > p->rozofs_psizes_max) p->rozofs_psizes_max = p->rozofs_psizes[i];
        }
        return 1;
    } else {
        return 0;
    }
}


// void rozofs_layout_release() {
//     uint32_t layout;
//     rozofs_conf_layout_t *p;

//     p = rozofs_conf_layout_table;

//     for (layout = 0; layout < LAYOUT_MAX; layout++, p++) {

//         if (p->rozofs_angles)
//             free(p->rozofs_angles);
//         if (p->rozofs_psizes)
//             free(p->rozofs_psizes);
//     }
// }

void rozofs_layout_release() {
    rozofs_conf_layout_t *p;

    p = &rozofs_conf_layout_table;

    if (p->rozofs_angles)
        free(p->rozofs_angles);
    if (p->rozofs_psizes)
        free(p->rozofs_psizes);
}
