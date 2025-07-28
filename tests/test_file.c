#include "test_common.h"
#include <nostd.h>
#include "cute.h"

static void File_OpenAndClose(void) {
  cu_File_OpenOptions options = {0};
  cu_File_OpenOptions_read(&options);
  cu_File_OpenOptions_create(&options);

  const char lpath[] = "test.txt";
  cu_Slice path = cu_Slice_create((void *)lpath, sizeof(lpath) - 1);
  cu_File_Result res = cu_File_open(path, options);
  TEST_ASSERT_TRUE(cu_File_Result_is_ok(&res));
  cu_File file = cu_File_Result_unwrap(&res);
  TEST_ASSERT_TRUE((file.handle) != (CU_INVALID_HANDLE));

  cu_File_close(&file);
  TEST_ASSERT_EQUAL(file.handle, CU_INVALID_HANDLE);
}

static void File_WriteAndRead(void) {
  cu_File_OpenOptions options = {0};
  cu_File_OpenOptions_write(&options);
  cu_File_OpenOptions_create(&options);
  cu_File_OpenOptions_truncate(&options);

  const char lpath[] = "io_test.txt";
  cu_Slice path = cu_Slice_create((void *)lpath, sizeof(lpath) - 1);
  cu_File_Result res = cu_File_open(path, options);
  TEST_ASSERT_TRUE(cu_File_Result_is_ok(&res));
  cu_File file = cu_File_Result_unwrap(&res);

  const char data[] = "hello";
  cu_Slice data_slice = cu_Slice_create((void *)data, sizeof(data) - 1);
  cu_Io_Error_Optional write_err = cu_File_write(&file, data_slice);
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&write_err));

  cu_File_close(&file);

  cu_File_OpenOptions rd = {0};
  cu_File_OpenOptions_read(&rd);
  res = cu_File_open(path, rd);
  TEST_ASSERT_TRUE(cu_File_Result_is_ok(&res));
  file = cu_File_Result_unwrap(&res);

  char buffer[6] = {0};
  cu_Slice buffer_slice = cu_Slice_create(buffer, sizeof(data) - 1);
  cu_Io_Error_Optional read_err = cu_File_read(&file, buffer_slice);
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&read_err));
  buffer[sizeof(data) - 1] = '\0';
  TEST_ASSERT_EQUAL_STRING(buffer, data);

  cu_File_close(&file);
}

static void File_InvalidOptions(void) {
  cu_File_OpenOptions options = {0};
  const char lpath[] = "invalid.txt";
  cu_Slice path = cu_Slice_create((void *)lpath, sizeof(lpath) - 1);
  cu_File_Result res = cu_File_open(path, options);
  TEST_ASSERT_FALSE(cu_File_Result_is_ok(&res));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(File_OpenAndClose);
    RUN_TEST(File_WriteAndRead);
    RUN_TEST(File_InvalidOptions);
    return UNITY_END();
}
