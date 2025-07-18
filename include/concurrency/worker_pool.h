#pragma once

/** @file worker_pool.h Optional libuv worker pool. */

#ifdef CU_USE_LIBUV
#include "collection/ring_buffer.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include "object/result.h"
#include <stdbool.h>
#include <uv.h>

/** Task callback signature providing the worker loop. */
typedef void (*cu_WorkerTaskFn)(uv_loop_t *loop, void *data);

/** Single task item. */
typedef struct {
  cu_WorkerTaskFn fn; /**< task function */
  void *data;         /**< user data */
} cu_WorkerTask;

struct cu_Scheduler;

/** Worker state. */
typedef struct {
  uv_thread_t thread;         /**< worker thread */
  uv_loop_t *loop;            /**< event loop */
  uv_async_t async;           /**< async notifier */
  uv_sem_t startSem;          /**< thread start barrier */
  cu_RingBuffer local;        /**< local buffer */
  bool stop;                  /**< exit flag */
  struct cu_Scheduler *sched; /**< owning scheduler */
} cu_Worker;

/** Streaming task scheduler. */
typedef struct cu_Scheduler {
  cu_RingBuffer queue;    /**< task queue */
  uv_mutex_t mutex;       /**< queue guard */
  cu_Allocator allocator; /**< allocator */
  size_t workerCount;     /**< number of workers */
  size_t localCapacity;   /**< per worker buffer */
  cu_Worker *workers;     /**< worker array */
  size_t nextWorker;      /**< round-robin counter */
} cu_Scheduler;

/** Scheduler error codes. */
typedef enum {
  CU_SCHEDULER_ERROR_NONE = 0,
  CU_SCHEDULER_ERROR_INVALID,
  CU_SCHEDULER_ERROR_OOM,
  CU_SCHEDULER_ERROR_LIBUV,
} cu_Scheduler_Error;

CU_RESULT_DECL(cu_Scheduler, cu_Scheduler, cu_Scheduler_Error)
CU_OPTIONAL_DECL(cu_Scheduler_Error, cu_Scheduler_Error)

cu_Scheduler_Result cu_Scheduler_create(cu_Scheduler *sched,
    cu_Allocator allocator, size_t workers, size_t queueCapacity,
    size_t localCapacity);

void cu_Scheduler_destroy(cu_Scheduler *sched);

void cu_Scheduler_schedule(cu_Scheduler *sched, cu_WorkerTaskFn fn, void *data);

#endif /* CU_USE_LIBUV */
