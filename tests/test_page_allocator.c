#include "test_common.h"
#include "memory/allocator.h"
#include "memory/page.h"

#if CU_FREESTANDING
static void PageAllocator_Unsupported(void) { }
#else
static void PageAllocator_Basic(void) {
  cu_PageAllocator palloc;
  cu_Allocator a = cu_Allocator_PageAllocator(&palloc);
  cu_Slice_Result mem_res =
      cu_Allocator_Alloc(a, cu_Layout_create(4096, 4096));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&mem_res));
  cu_Slice mem = mem_res.value;
  cu_Memory_memset(mem.ptr, 0, mem.length);
  cu_Allocator_Free(a, mem);
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
