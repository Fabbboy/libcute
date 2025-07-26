#include <gtest/gtest.h>
extern "C" {
#include "memory/allocator.h"
#include "memory/wasmallocator.h"
#include "memory/fixedallocator.h"
#include "string/fmt.h"
}

TEST(StrBuilder, AppendFormatted) {
#if CU_PLAT_WASM
  cu_Allocator alloc = cu_Allocator_WasmAllocator();
#elif CU_FREESTANDING
  static char buf[1024];
  cu_FixedAllocator fa;
  cu_Allocator alloc =
      cu_Allocator_FixedAllocator(&fa, cu_Slice_create(buf, sizeof(buf)));
#else
  cu_Allocator alloc = cu_Allocator_CAllocator();
#endif
  cu_StrBuilder builder = cu_StrBuilder_init(alloc);
  EXPECT_EQ(
      CU_STRING_ERROR_NONE, cu_StrBuilder_appendf(&builder, "number %d", 10));
  EXPECT_EQ(
      CU_STRING_ERROR_NONE, cu_StrBuilder_appendf(&builder, " %s", "items"));

  cu_String_Result result = cu_StrBuilder_finalize(&builder);
  ASSERT_TRUE(cu_String_result_is_ok(&result));
  EXPECT_STREQ(result.value.data, "number 10 items");
  cu_String_destroy(&result.value);
  cu_StrBuilder_destroy(&builder);
}

TEST(StrBuilder, AppendAndFinalize) {
#if CU_PLAT_WASM
  cu_Allocator alloc = cu_Allocator_WasmAllocator();
#elif CU_FREESTANDING
  static char buf2[1024];
  cu_FixedAllocator fa2;
  cu_Allocator alloc =
      cu_Allocator_FixedAllocator(&fa2, cu_Slice_create(buf2, sizeof(buf2)));
#else
  cu_Allocator alloc = cu_Allocator_CAllocator();
#endif
  cu_StrBuilder builder = cu_StrBuilder_init(alloc);
  cu_StrBuilder_append_slice(&builder, cu_Slice_create((void *)"ab", 2));
  cu_StrBuilder_append_cstr(&builder, "cd");
  cu_String_Result tmp = cu_String_from_cstr(alloc, "ef");
  ASSERT_TRUE(cu_String_result_is_ok(&tmp));
  cu_StrBuilder_append(&builder, &tmp.value);
  cu_String_destroy(&tmp.value);

  cu_Slice view = cu_StrBuilder_as_slice(&builder);
  EXPECT_EQ(view.length, 6u);

  cu_String_Result result = cu_StrBuilder_finalize(&builder);
  ASSERT_TRUE(cu_String_result_is_ok(&result));
  EXPECT_STREQ(result.value.data, "abcdef");
  cu_String_destroy(&result.value);
  cu_StrBuilder_destroy(&builder);
}
