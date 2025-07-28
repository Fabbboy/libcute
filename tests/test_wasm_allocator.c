#include "test_common.h"
#include "memory/allocator.h"
#include "memory/wasmallocator.h"

#if CU_PLAT_WASM
static void WasmAllocator_Basic(void) {
  cu_Allocator alloc = cu_Allocator_WasmAllocator();
  cu_Slice_Result res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(64, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&res));
  cu_Slice mem = res.value;
  cu_Memory_memset(mem.ptr, 0xAA, mem.length);
  cu_Allocator_Free(alloc, mem);
}
#else
static void WasmAllocator_Unsupported(void) { }
#endif

int main(void) {
    UNITY_BEGIN();
#if CU_PLAT_WASM
    RUN_TEST(WasmAllocator_Basic);
#else
    RUN_TEST(WasmAllocator_Unsupported);
#endif
    return UNITY_END();
}
