#include "concurrency/worker_pool.h"
#ifdef CU_USE_LIBUV
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "macro.h"
#include <stdalign.h>
#include <stdlib.h>

CU_RESULT_IMPL(cu_Scheduler, cu_Scheduler, cu_Scheduler_Error)
CU_OPTIONAL_IMPL(cu_Scheduler_Error, cu_Scheduler_Error)

static bool cu_worker_fill_local(cu_Worker *worker) {
  cu_Scheduler *sched = worker->sched;
  cu_WorkerTask item;
  bool got = false;
  uv_mutex_lock(&sched->mutex);
  while (!cu_RingBuffer_is_full(&worker->local) &&
         !cu_RingBuffer_is_empty(&sched->queue)) {
    cu_RingBuffer_pop(&sched->queue, &item);
    cu_RingBuffer_push(&worker->local, &item);
    got = true;
  }
  uv_mutex_unlock(&sched->mutex);
  return got;
}

static void cu_worker_async_cb(uv_async_t *handle) {
  cu_Worker *worker = (cu_Worker *)handle->data;
  cu_WorkerTask task;
  while (true) {
    if (cu_RingBuffer_is_empty(&worker->local)) {
      if (!cu_worker_fill_local(worker)) {
        break;
      }
    }
    cu_RingBuffer_pop(&worker->local, &task);
    if (task.fn) {
      task.fn(worker->loop, task.data);
    }
  }
  if (worker->stop) {
    uv_stop(worker->loop);
  }
}

static void cu_worker_thread(void *arg) {
  cu_Worker *worker = (cu_Worker *)arg;
  uv_loop_t loop;
  worker->loop = &loop;
  uv_loop_init(worker->loop);
  uv_async_init(worker->loop, &worker->async, cu_worker_async_cb);
  worker->async.data = worker;
  uv_sem_post(&worker->startSem);
  uv_run(worker->loop, UV_RUN_DEFAULT);
  uv_close((uv_handle_t *)&worker->async, NULL);
  uv_loop_close(worker->loop);
}

cu_Scheduler_Result cu_Scheduler_create(cu_Scheduler *sched,
    cu_Allocator allocator, size_t workers, size_t queueCapacity,
    size_t localCapacity) {
  if (!sched || workers == 0) {
    return cu_Scheduler_result_error(CU_SCHEDULER_ERROR_INVALID);
  }
  cu_RingBuffer_Result qres =
      cu_RingBuffer_create(allocator, CU_LAYOUT(cu_WorkerTask), queueCapacity);
  if (!cu_RingBuffer_result_is_ok(&qres)) {
    return cu_Scheduler_result_error(CU_SCHEDULER_ERROR_OOM);
  }
  sched->queue = cu_RingBuffer_result_unwrap(&qres);
  sched->allocator = allocator;
  sched->workerCount = workers;
  sched->localCapacity = localCapacity;
  sched->nextWorker = 0;
  uv_mutex_init(&sched->mutex);
  cu_Slice_Optional mem = cu_Allocator_Alloc(
      allocator, workers * sizeof(cu_Worker), alignof(cu_Worker));
  if (cu_Slice_Optional_is_none(&mem)) {
    cu_RingBuffer_destroy(&sched->queue);
    return cu_Scheduler_result_error(CU_SCHEDULER_ERROR_OOM);
  }
  sched->workers = (cu_Worker *)mem.value.ptr;
  for (size_t i = 0; i < workers; ++i) {
    cu_RingBuffer_Result lres = cu_RingBuffer_create(
        allocator, CU_LAYOUT(cu_WorkerTask), localCapacity);
    if (!cu_RingBuffer_result_is_ok(&lres)) {
      sched->workerCount = i;
      cu_Scheduler_destroy(sched);
      return cu_Scheduler_result_error(CU_SCHEDULER_ERROR_OOM);
    }
    uv_sem_init(&sched->workers[i].startSem, 0);
    sched->workers[i].local = cu_RingBuffer_result_unwrap(&lres);
    sched->workers[i].stop = false;
    sched->workers[i].sched = sched;
    int r = uv_thread_create(
        &sched->workers[i].thread, cu_worker_thread, &sched->workers[i]);
    if (r != 0) {
      sched->workerCount = i + 1;
      cu_Scheduler_destroy(sched);
      return cu_Scheduler_result_error(CU_SCHEDULER_ERROR_LIBUV);
    }
    uv_sem_wait(&sched->workers[i].startSem);
  }
  return cu_Scheduler_result_ok(*sched);
}

void cu_Scheduler_destroy(cu_Scheduler *sched) {
  if (!sched) {
    return;
  }
  for (size_t i = 0; i < sched->workerCount; ++i) {
    sched->workers[i].stop = true;
    uv_async_send(&sched->workers[i].async);
    uv_thread_join(&sched->workers[i].thread);
    cu_RingBuffer_destroy(&sched->workers[i].local);
    uv_sem_destroy(&sched->workers[i].startSem);
  }
  if (sched->workers) {
    cu_Allocator_Free(
        sched->allocator, cu_Slice_create(sched->workers,
                              sched->workerCount * sizeof(cu_Worker)));
    sched->workers = NULL;
  }
  uv_mutex_destroy(&sched->mutex);
  cu_RingBuffer_destroy(&sched->queue);
  sched->workerCount = 0;
}

void cu_Scheduler_schedule(
    cu_Scheduler *sched, cu_WorkerTaskFn fn, void *data) {
  if (!sched || !fn) {
    return;
  }
  cu_WorkerTask task = {fn, data};
  uv_mutex_lock(&sched->mutex);
  cu_RingBuffer_Error_Optional err = cu_RingBuffer_push(&sched->queue, &task);
  uv_mutex_unlock(&sched->mutex);
  if (cu_RingBuffer_Error_Optional_is_some(&err)) {
    return;
  }
  size_t idx = sched->nextWorker++ % sched->workerCount;
  uv_async_send(&sched->workers[idx].async);
}

#endif /* CU_USE_LIBUV */
