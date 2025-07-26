#include <gtest/gtest.h>
extern "C" {
#include "memory/allocator.h"
#include "memory/wasmallocator.h"
}

#if CU_PLAT_WASM
TEST(WasmAllocator, Basic) {
  cu_Allocator alloc = cu_Allocator_WasmAllocator();
  cu_Slice_Result res = cu_Allocator_Alloc(alloc, 64, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&res));
  cu_Slice mem = res.value;
  cu_Memory_memset(mem.ptr, 0xAA, mem.length);
  cu_Allocator_Free(alloc, mem);
}
#else
TEST(WasmAllocator, Unsupported) { SUCCEED(); }
#endif
