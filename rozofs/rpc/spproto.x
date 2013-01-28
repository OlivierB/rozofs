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

/* Storaged profiling protocol
 * profile storaged main process (aka internal monitor process)
 */
%#include <rozofs/rozofs.h>

enum spp_status_t {
    SPP_SUCCESS = 0,
    SPP_FAILURE = 1
};

union spp_status_ret_t switch (spp_status_t status) {
    case SPP_FAILURE:    int error;
    default:            void;
};


struct spp_profiler_t {
    uint64_t    uptime;
    uint64_t    now;
    uint8_t     vers[20];
    uint64_t    stat[2];
    uint64_t    ports[2];
    uint64_t    remove[2];
    /* io processes only */
    uint64_t    read[3];
    uint64_t    write[3];
    uint64_t    truncate[3];
    uint16_t    nb_io_processes;
    uint16_t    io_process_ports[32];
};

union spp_profiler_ret_t switch (spp_status_t status) {
    case SPP_SUCCESS:    spp_profiler_t profiler;
    case SPP_FAILURE:    int error;
    default:            void;
};

program STORAGED_PROFILE_PROGRAM {
    version STORAGED_PROFILE_VERSION {
        void
        SPP_NULL(void)          = 0;

        spp_profiler_ret_t
        SPP_GET_PROFILER(void)  = 1;

        spp_status_ret_t
        SPP_CLEAR(void)         = 2;
    }=1;
} = 0x20000004;
