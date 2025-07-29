#include "cute.h"
#include "unity.h"
#include <nostd.h>
#include <unity_internals.h>

static void File_OpenAndClose(void) {
  cu_File_Options options = {0};
  cu_File_Options_read(&options);
  cu_File_Options_create(&options);

  const char lpath[] = "test.txt";
  cu_Slice path = cu_Slice_create((void *)lpath, sizeof(lpath) - 1);
  cu_File_Result res = cu_File_open(path, options, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_File_Result_is_ok(&res));
  cu_File file = cu_File_Result_unwrap(&res);
  TEST_ASSERT_TRUE((file.handle) != (CU_INVALID_HANDLE));
  TEST_ASSERT_EQUAL(file.stat.path.length, sizeof(lpath) - 1);
  TEST_ASSERT_EQUAL_STRING(file.stat.path.data, lpath);

  cu_File_close(&file);
  TEST_ASSERT_EQUAL(file.handle, CU_INVALID_HANDLE);
  TEST_ASSERT_NULL(file.stat.path.data);
}

static void File_WriteAndRead(void) {
  cu_File_Options options = {0};
  cu_File_Options_write(&options);
  cu_File_Options_create(&options);
  cu_File_Options_truncate(&options);

  const char lpath[] = "io_test.txt";
  cu_Slice path = cu_Slice_create((void *)lpath, sizeof(lpath) - 1);
  cu_File_Result res = cu_File_open(path, options, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_File_Result_is_ok(&res));
  cu_File file = cu_File_Result_unwrap(&res);
  TEST_ASSERT_EQUAL_STRING(file.stat.path.data, lpath);

  const char data[] = "hello";
  cu_Slice data_slice = cu_Slice_create((void *)data, sizeof(data) - 1);
  cu_Io_Error_Optional write_err = cu_File_write(&file, data_slice);
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&write_err));

  cu_File_close(&file);

  cu_File_Options rd = {0};
  cu_File_Options_read(&rd);
  res = cu_File_open(path, rd, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_File_Result_is_ok(&res));
  file = cu_File_Result_unwrap(&res);
  TEST_ASSERT_EQUAL_STRING(file.stat.path.data, lpath);

  char buffer[6] = {0};
  cu_Slice buffer_slice = cu_Slice_create(buffer, sizeof(data) - 1);
  cu_Io_Error_Optional read_err = cu_File_read(&file, buffer_slice);
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&read_err));
  buffer[sizeof(data) - 1] = '\0';
  TEST_ASSERT_EQUAL_STRING(buffer, data);

  cu_File_close(&file);
  TEST_ASSERT_NULL(file.stat.path.data);
}

static void File_InvalidOptions(void) {
  cu_File_Options options = {0};
  const char lpath[] = "invalid.txt";
  cu_Slice path = cu_Slice_create((void *)lpath, sizeof(lpath) - 1);
  cu_File_Result res = cu_File_open(path, options, cu_Allocator_CAllocator());
  TEST_ASSERT_FALSE(cu_File_Result_is_ok(&res));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(File_OpenAndClose);
  RUN_TEST(File_WriteAndRead);
  RUN_TEST(File_InvalidOptions);
  return UNITY_END();
}
