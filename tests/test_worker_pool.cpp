#include <atomic>
extern "C" {
#include "macro.h"
#include "memory/allocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
#include "worker/pool.h"
}
#include <gtest/gtest.h>

static cu_Allocator create_allocator(
    cu_GPAllocator *gpa, cu_PageAllocator *page) {
  cu_Allocator page_alloc = cu_Allocator_PageAllocator(page);
  cu_GPAllocator_Config cfg = {};
  cfg.bucketSize = CU_GPA_BUCKET_SIZE;
  cfg.backingAllocator = cu_Allocator_Optional_some(page_alloc);
  return cu_Allocator_GPAllocator(gpa, cfg);
}

static std::atomic_int counter;

static void inc_task(void *data) {
  CU_UNUSED(data);
  counter.fetch_add(1);
}

TEST(WorkerPool, BasicRun) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_WorkerPool_Config cfg = {};
  cfg.workers = 2;
  cfg.queueCapacity = 4;
  cfg.allocator = cu_Allocator_Optional_some(alloc);
  cu_WorkerPool pool;
  cu_WorkerPool_Error err = cu_WorkerPool_create(&pool, cfg);
  ASSERT_EQ(err, CU_WORKERPOOL_ERROR_NONE);

  counter.store(0);
  for (int i = 0; i < 8; ++i) {
    cu_Task t = {inc_task, NULL};
    cu_WorkerPool_Error_Optional err = cu_WorkerPool_schedule(&pool, t);
    ASSERT_TRUE(cu_WorkerPool_Error_Optional_is_none(&err));
  }
  cu_WorkerPool_destroy(&pool);
  EXPECT_EQ(counter.load(), 8);

  cu_GPAllocator_destroy(&gpa);
}

TEST(WorkerPool, Invalid) {
  cu_WorkerPool_Config cfg = {};
  cfg.workers = 0;
  cfg.queueCapacity = 1;
  cfg.allocator = cu_Allocator_Optional_some(cu_Allocator_CAllocator());
  cu_WorkerPool pool;
  cu_WorkerPool_Error err = cu_WorkerPool_create(&pool, cfg);
  EXPECT_NE(err, CU_WORKERPOOL_ERROR_NONE);
}
