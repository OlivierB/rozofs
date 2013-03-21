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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <sys/resource.h>
#include <unistd.h>
#include <libintl.h>
#include <sys/poll.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <libconfig.h>
#include <getopt.h>
#include <inttypes.h>
#include <time.h>
#include <limits.h>
#include <semaphore.h>
#include "config.h"

#include <rozofs/common/log.h>
#include <rozofs/common/xmalloc.h>
#include <rozofs/common/list.h>
#include <rozofs/rozofs_srv.h>
#include <rozofs/common/daemon.h>
#include <rozofs/common/profile.h>
#include <rozofs/rpc/mproto.h>
#include <rozofs/rpc/sproto.h>
#include <rozofs/rpc/spproto.h>
#include <rozofs/rpc/mclient.h>
#include <rozofs/rpc/eclient.h>
#include "storcli_main.h"
#include <rozofs/core/north_lbg_api.h>
#include "rozofs_storcli_lbg_cnf_supervision.h"
#include <rozofs/rpc/storcli_lbg_prototypes.h>
#include "rozofs_storcli.h"
#include <rozofs/rpc/stcpproto.h>
#define STORCLI_PID_FILE "storcli.pid"

int rozofs_storcli_non_blocking_init(uint16_t dbg_port,uint16_t rozofsmount_instance);

DEFINE_PROFILING(stcpp_profiler_t) = {0};

/**
* data structure used to store the configuration parameter of a storcli process
*/
typedef struct storcli_conf {
    char *host;       /**< hostname of the export from which the storcli will get the mstorage configuration  */
    char *export;     /**< pathname of the exportd (unique) */
    char *passwd;     /**< user password */
    char *mount;     /**< mount point */
    int   module_index;  /**< storcli instance number within the exportd: more that one storcli processes can be started */
    unsigned buf_size;
    unsigned max_retry;
    unsigned dbg_port;
    unsigned rozofsmount_instance;
} storcli_conf;

static char localBuf[4096];


#define SHOW_PROFILER_PROBE(probe) pChar += sprintf(pChar," %14s | %15lu | %9lu | %18lu |\n",\
					#probe,\
					gprofiler.probe[P_COUNT],\
					gprofiler.probe[P_COUNT]?gprofiler.probe[P_ELAPSE]/gprofiler.probe[P_COUNT]:0,\
					gprofiler.probe[P_ELAPSE]);

#define SHOW_PROFILER_PROBE_BYTE(probe) pChar += sprintf(pChar," %14s | %15lu | %9lu | %18lu | %15lu\n",\
					#probe,\
					gprofiler.probe[P_COUNT],\
					gprofiler.probe[P_COUNT]?gprofiler.probe[P_ELAPSE]/gprofiler.probe[P_COUNT]:0,\
					gprofiler.probe[P_ELAPSE],\
                    gprofiler.probe[P_BYTES]);

void show_profiler(char * argv[], uint32_t tcpRef, void *bufRef) {
    char           *pChar=localBuf;
    
    pChar += sprintf(pChar,"GPROFILER version %s uptime = %llu\n",gprofiler.vers, (long long unsigned int)gprofiler.uptime);
    pChar += sprintf(pChar,"   procedure    |     count       |  time(us) | cumulated time(us) |     bytes       \n");
    pChar += sprintf(pChar,"----------------+-----------------+-----------+--------------------+-----------------\n"); 

    SHOW_PROFILER_PROBE_BYTE(read);
    SHOW_PROFILER_PROBE_BYTE(read_prj);
    SHOW_PROFILER_PROBE(read_prj_err);
    SHOW_PROFILER_PROBE(read_prj_tmo);
    SHOW_PROFILER_PROBE_BYTE(write);
    SHOW_PROFILER_PROBE_BYTE(write_prj);
    SHOW_PROFILER_PROBE(write_prj_tmo);
    SHOW_PROFILER_PROBE(write_prj_err);

    uma_dbg_send(tcpRef,bufRef,TRUE,localBuf);
}

/*__________________________________________________________________________
*/ 
/**
*  Global and local datas
*/
static storcli_conf conf;
 exportclt_t exportclt;  /**< structure associated to exportd, needed for communication */
 uint32_t  *rozofs_storcli_cid_table[ROZOFS_CLUSTERS_MAX];


/*__________________________________________________________________________
*/ 


 /*__________________________________________________________________________
*/ 
 /**
 * init of the cid table. That table contains pointers to
 * a sid/lbg_id association table, the primary key being the sid index
 */
 
 void rozofs_storcli_cid_table_init()
 {
    memset(rozofs_storcli_cid_table,0,ROZOFS_CLUSTERS_MAX*sizeof(uint32_t *)); 
 }

/*__________________________________________________________________________
*/ 
/**
*  insert an entry in the  rozofs_storcli_cid_table table

   @param cid : cluster index
   @param sid : storage index
   @param lbg_id : load balancing group index
   
   @retval 0 on success
   @retval < 0 on error
*/
int rozofs_storcli_cid_table_insert(cid_t cid,sid_t sid,uint32_t lbg_id)
{
  uint32_t *sid_lbg_id_p;

  if (cid >= ROZOFS_CLUSTERS_MAX)
  { 
    /*
    ** out of range
    */
    return -1;
  }
  
  sid_lbg_id_p = rozofs_storcli_cid_table[cid-1];
  if (sid_lbg_id_p == NULL)
  {
     /*
     ** allocate the cid/lbg_id association table if it does not
     ** exist
     */
     sid_lbg_id_p = xmalloc(sizeof(uint32_t)*(SID_MAX+1));
     memset(sid_lbg_id_p,-1,sizeof(uint32_t)*(SID_MAX+1)); 
     rozofs_storcli_cid_table[cid-1] = sid_lbg_id_p ;
  }
  sid_lbg_id_p[sid-1] = lbg_id;
  return 0;

}


#if 0

/**
*  Entry point for deamon start
*/

static void on_start() {
    int i = 0;
    int sock;
    int one = 1;

    DEBUG_FUNCTION;

    /* Initialization of the storage configuration */
    if (storaged_initialize() != 0) {
        fatal("can't initialize storaged: %s.", strerror(errno));
        return;
    }

    SET_PROBE_VALUE(uptime, time(0));
    strcpy((char*) gprofiler.vers, VERSION);
    SET_PROBE_VALUE(nb_io_processes, storaged_nb_io_processes);

    /* Create io processes */
    for (i = 0; i < storaged_nb_io_processes; i++) {
        int pid;

        /* Create child process */
        if (!(pid = fork())) {

            /* Associates STORAGE_PROGRAM, STORAGE_VERSION and
             * STORAGED_IO_PROFILE_PROGRAM, STORAGED_IO_PROFILE_VERSION
             * with their service dispatch procedure.
             * Here protocol is zero, the service is not registered with
             *  the portmap service
             */
            if ((storaged_svc = storaged_create_rpc_service(storaged_storage_ports[i])) == NULL) {
                fatal("can't create storaged service on port: %d",
                        storaged_storage_ports[i]);
            }

            if (!svc_register(storaged_svc, STORAGE_PROGRAM, STORAGE_VERSION,
                    storage_program_1, 0)) {
                fatal("can't register service : %s", strerror(errno));
            }

            if ((storaged_profile_svc = storaged_create_rpc_service(storaged_storage_ports[i] + 1000)) == NULL) {
                fatal("can't create monitoring service on port: %d",
                        storaged_storage_ports[i] + 1000);
            }

            if (!svc_register(storaged_profile_svc, STORAGED_PROFILE_PROGRAM,
                    STORAGED_PROFILE_VERSION, storaged_profile_program_1, 0)) {
                fatal("can't register service : %s", strerror(errno));
            }

            /* Waits for RPC requests to arrive ! */
            info("running io service (pid=%d, port=%d).", getpid(), storaged_storage_ports[i]);
            svc_run();
            /* NOT REACHED */
        } else {
            SET_PROBE_VALUE(io_process_ports[i], storaged_storage_ports[i] + 1000);
        }
    }

    /* Create internal monitoring service */
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof (int));
    setsockopt(sock, SOL_TCP, TCP_DEFER_ACCEPT, (char *) &one, sizeof (int));
    setsockopt(sock, SOL_TCP, TCP_NODELAY, (char *) &one, sizeof (int));

    if ((storaged_monitoring_svc = svctcp_create(sock,
            ROZOFS_RPC_BUFFER_SIZE, ROZOFS_RPC_BUFFER_SIZE)) == NULL) {
        fatal("can't create internal monitoring service.");
        return;
    }

    /* Destroy portmap mapping */
    pmap_unset(MONITOR_PROGRAM, MONITOR_VERSION); // in case !

    /* Associates MONITOR_PROGRAM and MONITOR_VERSION
     * with the service dispatch procedure, monitor_program_1.
     * Here protocol is no zero, the service is registered with
     * the portmap service */
    if (!svc_register(storaged_monitoring_svc, MONITOR_PROGRAM,
            MONITOR_VERSION, monitor_program_1, IPPROTO_TCP)) {
        fatal("can't register service : %s", strerror(errno));
        return;
    }

    /* Create profiling service for main process */
    /*
    mon_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    setsockopt(mon_sock, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(int));
    setsockopt(mon_sock, SOL_TCP, TCP_DEFER_ACCEPT, (char *) &one, sizeof(int));
    setsockopt(mon_sock, SOL_TCP, TCP_NODELAY, (char *) &one, sizeof(int));
     */
    if ((storaged_profile_svc = svctcp_create(RPC_ANYSOCK, 0, 0)) == NULL) {
        severe("can't create profiling service.");
    }
    pmap_unset(STORAGED_PROFILE_PROGRAM, STORAGED_PROFILE_VERSION); // in case !

    if (!svc_register(storaged_profile_svc, STORAGED_PROFILE_PROGRAM,
            STORAGED_PROFILE_VERSION, storaged_profile_program_1, IPPROTO_TCP)) {
        severe("can't register service : %s", strerror(errno));
    }
    /* Waits for RPC requests to arrive ! */
    info("running.");
    svc_run();
    /* NOT REACHED */
}

static void on_stop() {
    DEBUG_FUNCTION;

    svc_exit();
    svc_unregister(MONITOR_PROGRAM, MONITOR_VERSION);
    pmap_unset(MONITOR_PROGRAM, MONITOR_VERSION);

    if (storaged_monitoring_svc)
        svc_destroy(storaged_monitoring_svc);
    if (storaged_profile_svc)
        svc_destroy(storaged_profile_svc);

    storaged_release();

    info("stopped.");
    closelog();
}

#endif

/*__________________________________________________________________________
*/ 
/** Send a request to a storage node for get the list of TCP ports this storage
 *
 * @param storage: the storage node
 *
 * @return 0 on success otherwise -1
 */
static int get_storage_ports(mstorage_t *s) {
    int status = -1;
    int i = 0;
    mclient_t mclt;

    uint32_t ports[STORAGE_NODE_PORTS_MAX];
    memset(ports, 0, sizeof (uint32_t) * STORAGE_NODE_PORTS_MAX);
    strncpy(mclt.host, s->host, ROZOFS_HOSTNAME_MAX);

    struct timeval timeo;
    timeo.tv_sec = ROZOFS_MPROTO_TIMEOUT_SEC;
    timeo.tv_usec = 0;

    /* Initialize connection with storage (by mproto) */
    if (mclient_initialize(&mclt, timeo) != 0) {
        severe("Warning: failed to join storage (host: %s), %s.\n",
                s->host, strerror(errno));
        goto out;
    } else {
        /* Send request to get storage TCP ports */
        if (mclient_ports(&mclt, ports) != 0) {
            severe("Warning: failed to get ports for storage (host: %s).\n",
                    s->host);
            goto out;
        }
    }

    /* Copy each TCP ports */
    for (i = 0; i < STORAGE_NODE_PORTS_MAX; i++) {
        if (ports[i] != 0) {
            strncpy(s->sclients[i].host, s->host, ROZOFS_HOSTNAME_MAX);
            s->sclients[i].port = ports[i];
            s->sclients[i].status = 0;
            s->sclients_nb++;
        }
    }

    /* Release mclient*/
    mclient_release(&mclt);

    status = 0;
out:
    return status;
}

/*__________________________________________________________________________
*/ 
/** Thread : Check if the connections for one storage node are active or not
 *
 * @param storage: the storage node
 */
#define CONNECTION_THREAD_TIMESPEC  2

static void *connect_storage(void *v) {
    mstorage_t *mstorage = (mstorage_t*) v;
    int configuration_done = 0;
    
    struct timespec ts = {CONNECTION_THREAD_TIMESPEC, 0};
    
    if (mstorage->sclients_nb != 0) {
      configuration_done = 1;
      ts.tv_sec = CONNECTION_THREAD_TIMESPEC*20;    
    }

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    for (;;) {
        if (configuration_done == 0)
        {
          /* We don't have the ports for this storage node */
          if (mstorage->sclients_nb == 0) {
              /* Get ports for this storage node */
              if (get_storage_ports(mstorage) != 0) {
                  DEBUG("Cannot get ports for host: %s", mstorage->host);
              }
          }
          if (mstorage->sclients_nb != 0)
          {
            /*
            ** configure the load balancing group is not yet done:
            ** here we have to address the race competition case since
            ** that thread runs in parallel with the socket controller, so
            ** we cannot configure the load balancing group from that thread
            ** we just can assert a flag to indicate that the configuration
            ** data of the lbg are available.S
            */   
            storcli_sup_send_lbg_port_configuration(STORCLI_LBG_ADD,( void *)mstorage );
            configuration_done = 1;

            ts.tv_sec = CONNECTION_THREAD_TIMESPEC*20;    

          }
        }

        nanosleep(&ts, NULL);
    }
    return 0;
}
/*__________________________________________________________________________
*/ 
/**
*  API to start the connect_storage thread
*
*   The goal of that thread is to retrieve the port configuration for the mstorage
   that were not accessible at the time storcli has started.
   That function is intended to be called for the following cases:
     - after the retrieving of the storage configuration from the exportd
     - after the reload of the configuration from the exportd
    
 @param none
 
 @retval none
 */
void  rozofs_storcli_start_connect_storage_thread()
 {
    list_t *p = NULL;
 
     list_for_each_forward(p, &exportclt.storages) {

        mstorage_t *storage = list_entry(p, mstorage_t, list);
        if (storage->thread_started  == 1)
        {
           /*
           ** thread has already been started
           */
           continue;
        }
        pthread_t thread;

        if ((errno = pthread_create(&thread, NULL, connect_storage, storage)) != 0) {
            severe("can't create connexion thread: %s", strerror(errno));
        }
        storage->thread_started = 1;
    }
} 
/*__________________________________________________________________________
*/ 
/**
*  Get the exportd configuration:
  The goal of that procedure is to get the list of the mstorages
  that are associated with the exportd that is referenced as input
  argument of the storage client process
  
  that API uses the parameters stored in the conf structure

  @param none
  
  @retval 0 on success
  @retval -1 on error
*/
int rozofs_storcli_get_export_config(storcli_conf *conf)
{

    int i = 0;
    int ret;
    list_t *iterator = NULL;
    
    /* Initialize rozofs */
    rozofs_layout_initialize();
    
    struct timeval timeout_exportd;
    
    #warning SYLVAIN NEED TO CHANGE TIMEOUT
    //timeout_exportd.tv_sec = conf.export_timeout;
    timeout_exportd.tv_usec = 0;

    /* Initiate the connection to the export and get informations
     * about exported filesystem */
    if (exportclt_initialize(
            &exportclt,
            conf->host,
            conf->export,
            conf->passwd,
            conf->buf_size * 1024,
            conf->max_retry,
            timeout_exportd) != 0) {
        fprintf(stderr,
                "storcli failed for:\n" "export directory: %s\n"
                "export hostname: %s\n"  "error: %s\n"
                "See log for more information\n", conf->export, conf->host,
                 strerror(errno));
        return -1;
    }

    /* Initiate the connection to each storage node (with mproto),
     *  get the list of ports and
     *  establish a connection with each storage socket (with sproto) */
    list_for_each_forward(iterator, &exportclt.storages) {

        mstorage_t *s = list_entry(iterator, mstorage_t, list);

        mclient_t mclt;
        strcpy(mclt.host, s->host);
        uint32_t ports[STORAGE_NODE_PORTS_MAX];
        memset(ports, 0, sizeof (uint32_t) * STORAGE_NODE_PORTS_MAX);
        /*
        ** allocate the load balancing group for the mstorage
        */
        s->lbg_id = north_lbg_create_no_conf();
        if (s->lbg_id < 0)
        {
          severe(" out of lbg contexts");
          goto fatal;        
        }
        
        struct timeval timeout_mproto;
        timeout_mproto.tv_sec = ROZOFS_MPROTO_TIMEOUT_SEC;
        timeout_mproto.tv_usec = 0;
        
        /* Initialize connection with storage (by mproto) */
        if (mclient_initialize(&mclt, timeout_mproto) != 0) {
            fprintf(stderr, "Warning: failed to join storage (host: %s), %s.\n",
                    s->host, strerror(errno));
        } else {
            /* Send request to get storage TCP ports */
            if (mclient_ports(&mclt, ports) != 0) {
                fprintf(stderr,
                        "Warning: failed to get ports for storage (host: %s).\n"
                        , s->host);
            }
        }

        /* Initialize each TCP ports connection with this storage node
         *  (by sproto) */
        for (i = 0; i < STORAGE_NODE_PORTS_MAX; i++) {
            if (ports[i] != 0) {
                strcpy(s->sclients[i].host, s->host);
                s->sclients[i].port = ports[i];
                s->sclients[i].status = 0;
                s->sclients_nb++;
            }
        }
        /*
        ** proceed with storage configuration if the number of port is different from 0
        */
        if (s->sclients_nb != 0)
        {
          ret = storaged_lbg_initialize(s);
          if (ret < 0)
          {
            goto fatal;                       
          }
        }
        /*
        ** init of the cid/sid<-->lbg_id association table
        */
        for (i = 0; i < s->sids_nb;i++)
        {
          rozofs_storcli_cid_table_insert(s->cids[i],s->sids[i],s->lbg_id);       
        }
        /* Release mclient*/
        mclient_release(&mclt);
    }
    return 0;
    

fatal:
    return -1;
}



void usage() {
    printf("Rozofs storage client daemon - %s\n", VERSION);
    printf("Usage: storcli -i <instance> [OPTIONS]\n\n");
    printf("\t-h, --help\tprint this message.\n");
    printf("\t-H,--host EXPORT_HOST\t\tdefine address (or dns name) where exportd deamon is running (default: rozofsexport) \n");
    printf("\t-E,--path EXPORT_PATH\t\tdefine path of an export see exportd (default: /srv/rozofs/exports/export)\n");
    printf("\t-M,--mount MOUNT_POINT\t\tmount point\n");
    printf("\t-P,--pwd EXPORT_PASSWD\t\tdefine passwd used for an export see exportd (default: none) \n");
    printf("\t-D,--dbg DEBUG_PORT\t\tdebug port (default: none) \n");
    printf("\t-R,--rozo_instance ROZO_INSTANCE\t\trozofsmount instance number \n");
    printf("\t-i,--instance index\t\t unique index of the module instance related to export \n");

}





int main(int argc, char *argv[]) {
    int c;
    int ret;
    int val;
    static struct option long_options[] = {
        { "help", no_argument, 0, 'h'},
        { "host", required_argument, 0, 'H'},
        { "path", required_argument, 0, 'E'},
        { "pwd", required_argument, 0, 'P'},
        { "dbg", required_argument, 0, 'D'},
        { "mount", required_argument, 0, 'M'},
        { "instance", required_argument, 0, 'i'},
        { "rozo_instance", required_argument, 0, 'R'},
        { 0, 0, 0, 0}
    };

    conf.host = NULL;
    conf.passwd = NULL;
    conf.export = NULL;
    conf.mount = NULL;
    conf.module_index = -1;
    conf.buf_size = 256;
    conf.max_retry = 3;
    conf.dbg_port  = 0;
    conf.rozofsmount_instance  = 0;
    
    while (1) {

        int option_index = 0;
        c = getopt_long(argc, argv, "hH:E:P:i:D:M:R:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {

            case 'h':
                usage();
                exit(EXIT_SUCCESS);
                break;
            case 'H':
                conf.host=strdup(optarg);
                break;
            case 'E':
                conf.export=strdup(optarg);
                break;
            case 'P':
                conf.passwd=strdup(optarg);
                break;
            case 'M':
                conf.mount=strdup(optarg);
                break;
            case 'i':
                errno = 0;
                val = (int) strtol(optarg, (char **)NULL, 10);
                if (errno != 0)
                {
                   strerror(errno);
                   usage();
                   exit(EXIT_FAILURE);               
                }
                conf.module_index = val;
                break;

            case 'D':
                errno = 0;
                val = (int) strtol(optarg, (char **)NULL, 10);
                if (errno != 0)
                {
                   strerror(errno);
                   usage();
                   exit(EXIT_FAILURE);               
                }
                conf.dbg_port = val;
                break;
            case 'R':
                errno = 0;
                val = (int) strtol(optarg, (char **)NULL, 10);
                if (errno != 0)
                {
                   strerror(errno);
                   usage();
                   exit(EXIT_FAILURE);               
                }
                conf.rozofsmount_instance = val;
                break;
            case '?':
                usage();
                exit(EXIT_SUCCESS);
                break;
            default:
                usage();
                exit(EXIT_FAILURE);
                break;
        }
    }
    /*
    ** Check the parameters
    */
    if (conf.module_index == -1)
    {
       printf("instance number is mandatory!\n");
       usage();
       exit(EXIT_FAILURE);           
    }

    if (conf.host == NULL) {
        conf.host = strdup("rozofsexport");
    }

    if (conf.export == NULL) {
        conf.export = strdup("/srv/rozofs/exports/export");
    }

    if (conf.passwd == NULL) {
        conf.passwd = strdup("none");
    }
    openlog("storcli", LOG_PID, LOG_DAEMON);
    /*
    ** init of the non blocking part
    */
    ret = rozofs_storcli_non_blocking_init(conf.dbg_port,conf.rozofsmount_instance);
    if (ret < 0) 
    {
        fprintf(stderr, "Fatal error while initializing non blocking entity\n");
        goto error;                
    }
    {
       char name[32];
       sprintf(name,"storcli %d.%d",conf.rozofsmount_instance,conf.module_index);
       uma_dbg_set_name(name);
    } 
      
    
    /*
    ** Get the configuration from the export
    */
    ret = rozofs_storcli_get_export_config(&conf);
    if (ret < 0) 
    {
        fprintf(stderr, "Fatal error on rozofs_storcli_get_export_config()\n");
        goto error;                
    }    

    /*
    ** Init of the north interface (read/write request processing
    */
//#warning buffer size and count must not be hardcoded
    ret = rozofs_storcli_north_interface_init(exportclt.eid,conf.rozofsmount_instance,conf.module_index,STORCLI_NORTH_LBG_BUF_RECV_CNT,STORCLI_NORTH_LBG_BUF_RECV_SZ);
    if (ret < 0) 
    {
        fprintf(stderr, "Fatal error on rozofs_storcli_get_export_config()\n");
        goto error;                
    }       
    rozofs_storcli_start_connect_storage_thread();
    /*
    ** add the topic for the local profiler
    */
    uma_dbg_addTopic("profiler",show_profiler);
 /*
 ** main loop
 */
  fprintf(stderr, "Entering the main loop of the non blocking entity\n");

   while(1)
 {
   ruc_sockCtrl_selectWait();
 }

#if 0
    openlog("storcli", LOG_PID, LOG_DAEMON);
    daemon_start(STORCLI_PID_FILE, on_start, on_stop, NULL);
    exit(0);
#endif
error:
    fprintf(stderr, "see log for more details.\n");
    exit(EXIT_FAILURE);


}
