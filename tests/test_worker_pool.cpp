#ifdef CU_USE_LIBUV
extern "C" {
#include "concurrency/worker_pool.h"
#include "memory/allocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
}
#include <atomic>
#include <gtest/gtest.h>
#include <unistd.h>
#include <signal.h>
#include <uv.h>

static cu_Allocator create_allocator(
    cu_GPAllocator *gpa, cu_PageAllocator *page) {
  cu_Allocator page_alloc = cu_Allocator_PageAllocator(page);
  cu_GPAllocator_Config cfg = {};
  cfg.bucketSize = CU_GPA_BUCKET_SIZE;
  cfg.backingAllocator = cu_Allocator_Optional_some(page_alloc);
  return cu_Allocator_GPAllocator(gpa, cfg);
}

struct TimerData {
  uv_timer_t timer;
  std::atomic_int *counter;
};

static void timer_close_cb(uv_handle_t *handle) {
  auto *data = static_cast<TimerData *>(handle->data);
  delete data;
}

static void timer_cb(uv_timer_t *t) {
  auto *data = static_cast<TimerData *>(t->data);
  (*data->counter)++;
  uv_close(reinterpret_cast<uv_handle_t *>(&data->timer), timer_close_cb);
}

static void start_timer(uv_loop_t *loop, void *ptr) {
  auto *data = static_cast<TimerData *>(ptr);
  uv_timer_init(loop, &data->timer);
  data->timer.data = data;
  uv_timer_start(&data->timer, timer_cb, 5, 0);
}

struct SignalData {
  uv_signal_t signal;
  std::atomic_int *counter;
};

static void signal_close_cb(uv_handle_t *handle) {
  auto *data = static_cast<SignalData *>(handle->data);
  delete data;
}

static void signal_cb(uv_signal_t *sig, int /*signum*/) {
  auto *data = static_cast<SignalData *>(sig->data);
  (*data->counter)++;
  uv_signal_stop(sig);
  uv_close(reinterpret_cast<uv_handle_t *>(&data->signal), signal_close_cb);
}

static void start_signal(uv_loop_t *loop, void *ptr) {
  auto *data = static_cast<SignalData *>(ptr);
  uv_signal_init(loop, &data->signal);
  data->signal.data = data;
  uv_signal_start(&data->signal, signal_cb, SIGINT);
}

TEST(WorkerPool, AsyncScheduling) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_Scheduler sched;
  auto res = cu_Scheduler_create(&sched, alloc, 2, 16, 4);
  ASSERT_TRUE(cu_Scheduler_result_is_ok(&res));
  sched = cu_Scheduler_result_unwrap(&res);

  std::atomic_int timers{0};
  std::atomic_int signals{0};

  auto *t1 = new TimerData{ {}, &timers };
  auto *t2 = new TimerData{ {}, &timers };
  auto *sig = new SignalData{ {}, &signals };

  cu_Scheduler_schedule(&sched, start_timer, t1);
  cu_Scheduler_schedule(&sched, start_timer, t2);
  cu_Scheduler_schedule(&sched, start_signal, sig);

  usleep(50000);
  kill(getpid(), SIGINT);
  usleep(100000);

  EXPECT_EQ(timers.load(), 2);
  EXPECT_EQ(signals.load(), 1);

  cu_Scheduler_destroy(&sched);
  cu_GPAllocator_destroy(&gpa);
}
#else
#include <gtest/gtest.h>
TEST(WorkerPool, AsyncScheduling) {
  GTEST_SKIP() << "libuv disabled";
}
#endif
