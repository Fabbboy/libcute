#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/wasmallocator.h"
#include "string/fmt.h"
#include "unity.h"
#include <unity_internals.h>

static void StrBuilder_AppendFormatted(void) {
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
  TEST_ASSERT_EQUAL(
      CU_STRING_ERROR_NONE, cu_StrBuilder_appendf(&builder, "number %d", 10));
  TEST_ASSERT_EQUAL(
      CU_STRING_ERROR_NONE, cu_StrBuilder_appendf(&builder, " %s", "items"));

  cu_String_Result result = cu_StrBuilder_finalize(&builder);
  TEST_ASSERT_TRUE(cu_String_Result_is_ok(&result));
  TEST_ASSERT_EQUAL_STRING(result.value.data, "number 10 items");
  cu_String_destroy(&result.value);
  cu_StrBuilder_destroy(&builder);
}

static void StrBuilder_AppendAndFinalize(void) {
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
  TEST_ASSERT_TRUE(cu_String_Result_is_ok(&tmp));
  cu_StrBuilder_append(&builder, &tmp.value);
  cu_String_destroy(&tmp.value);

  cu_Slice view = cu_StrBuilder_as_slice(&builder);
  TEST_ASSERT_EQUAL(view.length, 6u);

  cu_String_Result result = cu_StrBuilder_finalize(&builder);
  TEST_ASSERT_TRUE(cu_String_Result_is_ok(&result));
  TEST_ASSERT_EQUAL_STRING(result.value.data, "abcdef");
  cu_String_destroy(&result.value);
  cu_StrBuilder_destroy(&builder);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(StrBuilder_AppendFormatted);
  RUN_TEST(StrBuilder_AppendAndFinalize);
  return UNITY_END();
}
