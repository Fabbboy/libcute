#include "memory/allocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
#include "string/string.h"
#include "unity.h"
#include <unity_internals.h>
#include <string.h>
#include <assert.h>

static void small_string_ops(cu_Allocator alloc) {
  for (int i = 0; i < 100; ++i) {
    cu_String_Result res = cu_String_from_cstr(alloc, "hello world!");
    assert(cu_String_Result_is_ok(&res));
    cu_String str = res.value;
    assert(CU_STRING_ERROR_NONE == cu_String_append_cstr(&str, "abc"));
    cu_String_destroy(&str);
  }
}

static void run_small_string_test(cu_Allocator alloc) {
  small_string_ops(alloc);
}

static void GPA_SmallStrings(void) {
  cu_PageAllocator page;
  cu_Allocator backing = cu_Allocator_PageAllocator(&page);
  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_some(backing);
  cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);
  run_small_string_test(alloc);
  cu_GPAllocator_destroy(&gpa);
}

static void CAllocator_SmallStrings(void) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  run_small_string_test(alloc);
}

static void PageAllocator_SmallStrings(void) {
  cu_PageAllocator page;
  cu_Allocator alloc = cu_Allocator_PageAllocator(&page);
  run_small_string_test(alloc);
}

int main(int argc, char **argv) {
  if (argc > 1) {
    if (strcmp(argv[1], "gpa") == 0) {
      cu_PageAllocator page;
      cu_Allocator backing = cu_Allocator_PageAllocator(&page);
      cu_GPAllocator gpa;
      cu_GPAllocator_Config cfg = {0};
      cfg.backingAllocator = cu_Allocator_Optional_some(backing);
      cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);
      small_string_ops(alloc);
      cu_GPAllocator_destroy(&gpa);
      return 0;
    } else if (strcmp(argv[1], "c") == 0) {
      cu_Allocator alloc = cu_Allocator_CAllocator();
      small_string_ops(alloc);
      return 0;
    } else if (strcmp(argv[1], "page") == 0) {
      cu_PageAllocator page;
      cu_Allocator alloc = cu_Allocator_PageAllocator(&page);
      small_string_ops(alloc);
      return 0;
    }
  }
  UNITY_BEGIN();
  RUN_TEST(GPA_SmallStrings);
  RUN_TEST(CAllocator_SmallStrings);
  RUN_TEST(PageAllocator_SmallStrings);
  return UNITY_END();
}
