#include "worker/pool.h"
#include "macro.h"
#include "utility.h"
#include <stdalign.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

CU_OPTIONAL_IMPL(cu_WorkerPool_Error, cu_WorkerPool_Error)

static bool steal_task(cu_Worker *worker, cu_Task *out) {
  cu_WorkerPool *pool = worker->pool;
  for (size_t i = 0; i < pool->count; ++i) {
    cu_Worker *victim = &pool->workers[(worker->index + i + 1) % pool->count];
    if (victim == worker) {
      continue;
    }
    if (pthread_mutex_trylock(&victim->mutex) == 0) {
      if (!cu_RingBuffer_is_empty(&victim->queue)) {
        cu_RingBuffer_pop(&victim->queue, out);
        pthread_mutex_unlock(&victim->mutex);
        return true;
      }
      pthread_mutex_unlock(&victim->mutex);
    }
  }
  return false;
}

static void *worker_main(void *arg) {
  cu_Worker *worker = arg;
  for (;;) {
    cu_Task task;
    bool have_task = false;

    pthread_mutex_lock(&worker->mutex);
    if (!cu_RingBuffer_is_empty(&worker->queue)) {
      cu_RingBuffer_pop(&worker->queue, &task);
      have_task = true;
    } else if (worker->stop) {
      pthread_mutex_unlock(&worker->mutex);
      break;
    }
    pthread_mutex_unlock(&worker->mutex);

    if (!have_task) {
      if (steal_task(worker, &task)) {
        have_task = true;
      }
    }

    if (have_task) {
      task.fn(task.data);
      continue;
    }

    pthread_mutex_lock(&worker->mutex);
    if (!worker->stop && cu_RingBuffer_is_empty(&worker->queue)) {
      pthread_cond_wait(&worker->cond, &worker->mutex);
    }
    bool quit = worker->stop && cu_RingBuffer_is_empty(&worker->queue);
    pthread_mutex_unlock(&worker->mutex);
    if (quit) {
      break;
    }
  }
  return NULL;
}

static cu_WorkerPool_Error worker_init(cu_Worker *worker,
    cu_Allocator allocator, cu_WorkerPool *pool, size_t index,
    size_t capacity) {
  if (pthread_mutex_init(&worker->mutex, NULL) != 0) {
    return CU_WORKERPOOL_ERROR_OOM;
  }
  if (pthread_cond_init(&worker->cond, NULL) != 0) {
    pthread_mutex_destroy(&worker->mutex);
    return CU_WORKERPOOL_ERROR_OOM;
  }
  cu_RingBuffer_Result qr =
      cu_RingBuffer_create(allocator, CU_LAYOUT(cu_Task), capacity);
  if (!cu_RingBuffer_result_is_ok(&qr)) {
    pthread_cond_destroy(&worker->cond);
    pthread_mutex_destroy(&worker->mutex);
    return CU_WORKERPOOL_ERROR_OOM;
  }
  worker->queue = qr.value;
  worker->stop = false;
  worker->pool = pool;
  worker->index = index;
  if (pthread_create(&worker->thread, NULL, worker_main, worker) != 0) {
    cu_RingBuffer_destroy(&worker->queue);
    pthread_cond_destroy(&worker->cond);
    pthread_mutex_destroy(&worker->mutex);
    return CU_WORKERPOOL_ERROR_OOM;
  }
  return CU_WORKERPOOL_ERROR_NONE;
}

cu_WorkerPool_Error cu_WorkerPool_create(
    cu_WorkerPool *pool, cu_WorkerPool_Config config) {
  if (!pool || config.workers == 0) {
    return CU_WORKERPOOL_ERROR_INVALID;
  }
  cu_Allocator allocator = cu_Allocator_Optional_is_some(&config.allocator)
                               ? config.allocator.value
                               : cu_Allocator_CAllocator();
  size_t capacity = config.queueCapacity ? config.queueCapacity
                                         : CU_WORKERPOOL_DEFAULT_QUEUE_CAPACITY;
  size_t size = sizeof(cu_Worker) * config.workers;
  cu_Slice_Optional mem =
      cu_Allocator_Alloc(allocator, size, alignof(cu_Worker));
  if (cu_Slice_Optional_is_none(&mem)) {
    return CU_WORKERPOOL_ERROR_OOM;
  }
  pool->workers = mem.value.ptr;
  pool->count = config.workers;
  pool->allocator = allocator;
  pool->next = 0;
  pool->queueCapacity = capacity;
  memset(pool->workers, 0, size);
  for (size_t i = 0; i < config.workers; ++i) {
    cu_WorkerPool_Error err =
        worker_init(&pool->workers[i], allocator, pool, i, capacity);
    if (err != CU_WORKERPOOL_ERROR_NONE) {
      for (size_t j = 0; j < i; ++j) {
        pthread_mutex_lock(&pool->workers[j].mutex);
        pool->workers[j].stop = true;
        pthread_cond_signal(&pool->workers[j].cond);
        pthread_mutex_unlock(&pool->workers[j].mutex);
        pthread_join(pool->workers[j].thread, NULL);
        cu_RingBuffer_destroy(&pool->workers[j].queue);
        pthread_cond_destroy(&pool->workers[j].cond);
        pthread_mutex_destroy(&pool->workers[j].mutex);
      }
      cu_Allocator_Free(allocator, mem.value);
      return err;
    }
  }
  return CU_WORKERPOOL_ERROR_NONE;
}

void cu_WorkerPool_destroy(cu_WorkerPool *pool) {
  if (!pool || !pool->workers) {
    return;
  }
  for (size_t i = 0; i < pool->count; ++i) {
    pthread_mutex_lock(&pool->workers[i].mutex);
    pool->workers[i].stop = true;
    pthread_cond_signal(&pool->workers[i].cond);
    pthread_mutex_unlock(&pool->workers[i].mutex);
    pthread_join(pool->workers[i].thread, NULL);
    cu_RingBuffer_destroy(&pool->workers[i].queue);
    pthread_cond_destroy(&pool->workers[i].cond);
    pthread_mutex_destroy(&pool->workers[i].mutex);
  }
  cu_Allocator_Free(pool->allocator,
      cu_Slice_create(pool->workers, sizeof(cu_Worker) * pool->count));
  pool->workers = NULL;
  pool->count = 0;
}

cu_WorkerPool_Error_Optional cu_WorkerPool_schedule(
    cu_WorkerPool *pool, cu_Task task) {
  if (!pool || pool->count == 0) {
    return cu_WorkerPool_Error_Optional_some(CU_WORKERPOOL_ERROR_INVALID);
  }
  size_t start =
      atomic_fetch_add((atomic_size_t *)&pool->next, 1) % pool->count;
  for (size_t i = 0; i < pool->count; ++i) {
    cu_Worker *w = &pool->workers[(start + i) % pool->count];
    pthread_mutex_lock(&w->mutex);
    if (!cu_RingBuffer_is_full(&w->queue)) {
      cu_RingBuffer_push(&w->queue, &task);
      pthread_cond_signal(&w->cond);
      pthread_mutex_unlock(&w->mutex);
      return cu_WorkerPool_Error_Optional_none();
    }
    pthread_mutex_unlock(&w->mutex);
  }
  return cu_WorkerPool_Error_Optional_some(CU_WORKERPOOL_ERROR_OOM);
}
