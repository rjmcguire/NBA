#ifndef __NBA_KNAPP_MICTYPES_HH__
#define __NBA_KNAPP_MICTYPES_HH__

#ifndef __MIC__
#error "This header should be used by MIC-side codes only."
#endif
#ifdef __NBA_KNAPP_HOSTTYPES_HH__
#error "Mixed use of MIC/host headers!"
#endif

#include <cstdint>
#include <atomic>
#include <vector>
#include <scif.h>
#include <nba/engines/knapp/sharedtypes.hh>
#include <nba/engines/knapp/micintrinsic.hh>
#include <nba/engines/knapp/micbarrier.hh>

namespace nba { namespace knapp {

/* Forward decls. */
struct worker;

/* Callback defs. */
typedef void (*worker_func_t)(struct worker *work);


// TODO: deprecate
union u_worker {
    void *kernel_specific_data;
};

/* MIC-side vDevice context */
struct vdevice {
    int device_id;

    scif_epd_t data_epd;
    scif_epd_t ctrl_epd;
    uint32_t pipeline_depth;
    size_t ht_per_core;

    scif_epd_t master_epd;
    scif_epd_t data_listen_epd;
    scif_epd_t ctrl_listen_epd;
    struct scif_portID master_port;
    struct scif_portID local_data_port;
    struct scif_portID local_ctrl_port;
    struct scif_portID remote_data_port;
    struct scif_portID remote_ctrl_port;
    struct poll_ring poll_ring;

    off_t remote_writebuf_base_ra;
    off_t remote_poll_ring_window;

    // ---
    uint8_t *ctrlbuf;
    Barrier **data_ready_barriers;
    Barrier **task_done_barriers;

    pthread_t master_thread;
    pthread_t *worker_threads;

    struct worker_thread_info *thread_info_array;
    int master_cpu;
    int num_worker_threads;
    int next_task_id;
    int cur_task_id;
    int num_packets_in_cur_task;

    struct bufarray inputbuf_array;
    struct bufarray resultbuf_array;

    struct worker **per_thread_work_info;
    worker_func_t worker_func;

    uint32_t offload_batch_size;
    union u_worker u;
    std::atomic<bool> exit;
    std::vector<int> cores;

    /* stats-related */
    bool first_entry;
    uint64_t ts_laststat;
    uint64_t ts_curstat;
    uint64_t ts_batch_begin;
    uint64_t ts_batch_end;
    uint64_t total_packets_processed;
    uint64_t total_batches_processed;
    uint64_t acc_batch_process_us;
    uint64_t acc_batch_process_us_sq;
    uint64_t acc_batch_transfer_us;
    uint64_t acc_batch_transfer_us_sq;
};

struct worker {
    // To be used by all threads:
    int thread_id;
    struct vdevice *vdev;
    Barrier *data_ready_barrier;
    Barrier *task_done_barrier;

    uint32_t max_num_packets;
    volatile int num_packets;
    std::atomic<bool> exit;
    union u_worker u;
} __cache_aligned;

struct worker_thread_info {
     int thread_id;
     struct vdevice *vdev;
     worker_func_t pktproc_func;
} CACHE_ALIGNED;

}} // endns(nba::knapp)

#endif // __NBA_KNAPP_MICTYPES_HH__

// vim: ts=8 sts=4 sw=4 et
