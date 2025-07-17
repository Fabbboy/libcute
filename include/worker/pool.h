#pragma once

/** @file pool.h Simple worker pool with work stealing. */

#include "collection/ring_buffer.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

typedef void (*cu_TaskFn)(void *data);

typedef struct cu_WorkerPool cu_WorkerPool;

#define CU_WORKERPOOL_DEFAULT_QUEUE_CAPACITY 64

typedef struct {
  size_t workers;
  size_t queueCapacity;
  cu_Allocator_Optional allocator;
} cu_WorkerPool_Config;

/** Task executed by a worker. */
typedef struct {
  cu_TaskFn fn;
  void *data;
} cu_Task;

/** Worker executing tasks from its queue. */
typedef struct cu_Worker {
  pthread_t thread;      /**< worker thread */
  cu_RingBuffer queue;   /**< pending tasks */
  pthread_mutex_t mutex; /**< queue lock */
  pthread_cond_t cond;   /**< wake signal */
  bool stop;             /**< request termination */
  cu_WorkerPool *pool;   /**< owning pool */
  size_t index;          /**< worker index */
} cu_Worker;

/** Pool managing multiple workers and scheduling tasks. */
typedef struct cu_WorkerPool {
  cu_Worker *workers;
  size_t count;
  cu_Allocator allocator;
  size_t next;
  size_t queueCapacity;
} cu_WorkerPool;

/** Error codes returned by worker pool operations. */
typedef enum {
  CU_WORKERPOOL_ERROR_NONE = 0,
  CU_WORKERPOOL_ERROR_OOM,
  CU_WORKERPOOL_ERROR_INVALID,
} cu_WorkerPool_Error;

CU_OPTIONAL_DECL(cu_WorkerPool_Error, cu_WorkerPool_Error)

cu_WorkerPool_Error cu_WorkerPool_create(
    cu_WorkerPool *pool, cu_WorkerPool_Config config);
void cu_WorkerPool_destroy(cu_WorkerPool *pool);
cu_WorkerPool_Error_Optional cu_WorkerPool_schedule(
    cu_WorkerPool *pool, cu_Task task);
