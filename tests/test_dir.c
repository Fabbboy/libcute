#include "cute.h"
#include "unity.h"
#include <nostd.h>
#include "io/dir.h"

static void Dir_OpenClose(void) {
  const char path_c[] = ".";
  cu_Slice path = cu_Slice_create((void *)path_c, sizeof(path_c) - 1);
  cu_Dir_Result res = cu_Dir_open(path, false);
  TEST_ASSERT_TRUE(cu_Dir_Result_is_ok(&res));
  cu_Dir dir = cu_Dir_Result_unwrap(&res);
  TEST_ASSERT_TRUE(dir.handle != CU_INVALID_HANDLE);
  TEST_ASSERT_EQUAL(dir.stat.kind, CU_FILE_TYPE_DIRECTORY);
  cu_Dir_close(&dir);
  TEST_ASSERT_EQUAL(dir.handle, CU_INVALID_HANDLE);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(Dir_OpenClose);
  return UNITY_END();
}
