#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/wasmallocator.h"
#include "nostd.h"
#include "string/string.h"
#include "unity.h"
#include <unity_internals.h>

static void String_AppendAndSubstring(void) {
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
  cu_String_Result res = cu_String_from_cstr(alloc, "hello");
  TEST_ASSERT_TRUE(cu_String_Result_is_ok(&res));
  cu_String str = res.value;

  TEST_ASSERT_EQUAL(str.length, 5u);
  TEST_ASSERT_EQUAL_STRING(str.data, "hello");

  TEST_ASSERT_EQUAL(
      CU_STRING_ERROR_NONE, cu_String_append_cstr(&str, ", world"));
  TEST_ASSERT_EQUAL(str.length, 12u);
  TEST_ASSERT_EQUAL_STRING(str.data, "hello, world");

  cu_String_Result sub = cu_String_substring(&str, 7, 5);
  TEST_ASSERT_TRUE(cu_String_Result_is_ok(&sub));
  cu_String part = sub.value;
  TEST_ASSERT_EQUAL_STRING(part.data, "world");

  cu_String_destroy(&part);
  cu_String_destroy(&str);
}

static void String_Clear(void) {
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
  cu_String_Result res = cu_String_from_cstr(alloc, "data");
  TEST_ASSERT_TRUE(cu_String_Result_is_ok(&res));
  cu_String str = res.value;
  TEST_ASSERT_EQUAL(str.length, 4u);
  cu_String_clear(&str);
  TEST_ASSERT_EQUAL(str.length, 0u);
  TEST_ASSERT_EQUAL_STRING(str.data, "");
  cu_String_destroy(&str);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(String_AppendAndSubstring);
  RUN_TEST(String_Clear);
  return UNITY_END();
}
