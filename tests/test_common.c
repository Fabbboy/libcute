#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
#include "memory/wasmallocator.h"

#include "test_common.h"

static cu_GPAllocator gpa;
cu_Allocator test_allocator;

#if CU_PLAT_WASM
/* WebAssembly uses its dedicated page allocator. */
#elif CU_FREESTANDING
static unsigned char fixed_buf[64 * 1024];
static cu_FixedAllocator fixed_alloc;
#else
static cu_PageAllocator page_alloc;
#endif

void setUp(void) { // NOLINT(readability-identifier-naming)
  cu_Allocator backing = {0};
#if CU_PLAT_WASM
  backing = cu_Allocator_WasmAllocator();
#elif CU_FREESTANDING
  backing = cu_Allocator_FixedAllocator(
      &fixed_alloc, cu_Slice_create(fixed_buf, sizeof(fixed_buf)));
#else
  backing = cu_Allocator_PageAllocator(&page_alloc);
#endif

  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_some(backing);
  cfg.bucketSize = CU_GPA_BUCKET_SIZE;
  test_allocator = cu_Allocator_GPAllocator(&gpa, cfg);
}

void tearDown(void) { // NOLINT(readability-identifier-naming)
  cu_GPAllocator_destroy(&gpa);
#if CU_FREESTANDING
  cu_FixedAllocator_reset(&fixed_alloc);
#endif
}
