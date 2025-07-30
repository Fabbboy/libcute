#include "memory/allocator.h"
#include "memory/page.h"
#include "unity.h"
#include <unity_internals.h>

#if CU_FREESTANDING
static void PageAllocator_Unsupported(void) {}
#else
static void PageAllocator_Basic(void) {
  cu_PageAllocator palloc;
  cu_Allocator a = cu_Allocator_PageAllocator(&palloc);
  cu_IoSlice_Result mem_res =
      cu_Allocator_Alloc(a, cu_Layout_create(4096, 4096));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&mem_res));
  cu_Slice mem = mem_res.value;
  cu_Memory_memset(mem.ptr, 0, mem.length);

  cu_IoSlice_Result grown = cu_Allocator_Grow(
      a, mem, cu_Layout_create(mem.length * 2, 4096));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&grown));

  cu_IoSlice_Result shrunk = cu_Allocator_Shrink(
      a, grown.value, cu_Layout_create(1024, 4096));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&shrunk));
  cu_Allocator_Free(a, shrunk.value);
}
#endif

int main(void) {
  UNITY_BEGIN();
#if CU_FREESTANDING
  RUN_TEST(PageAllocator_Unsupported);
#else
  RUN_TEST(PageAllocator_Basic);
#endif
  return UNITY_END();
}
