#include "memory/allocator.h"
#include "string/fmt.h"
#include "test_common.h"
#include "unity.h"
#include <unity_internals.h>

static void StrBuilder_AppendFormatted(void) {
  cu_Allocator alloc = test_allocator;
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
  cu_Allocator alloc = test_allocator;
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
