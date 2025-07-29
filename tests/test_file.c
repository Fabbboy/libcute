#include "cute.h"
#include "unity.h"
#include <nostd.h>
#include "io/dir.h"
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

static void File_OpenAt(void) {
  cu_Dir_Result dres =
      cu_Dir_open(cu_Slice_create((void *)"somename", 8), true, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_Dir_Result_is_ok(&dres));
  cu_Dir dir = cu_Dir_Result_unwrap(&dres);

  cu_File_Options opt = {0};
  cu_File_Options_write(&opt);
  cu_File_Options_create(&opt);
  cu_File_Options_truncate(&opt);

  const char fname[] = "config.toml";
  cu_Slice fp = cu_Slice_create((void *)fname, sizeof(fname) - 1);
  cu_File_Result fres = cu_Dir_openat(&dir, fp, opt);
  TEST_ASSERT_TRUE(cu_File_Result_is_ok(&fres));
  cu_File file = cu_File_Result_unwrap(&fres);
  TEST_ASSERT_EQUAL_STRING(file.stat.path.data, fname);

  const char data[] = "ok";
  cu_Io_Error_Optional err =
      cu_File_write(&file, cu_Slice_create((void *)data, sizeof(data) - 1));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  cu_File_close(&file);

  cu_File_Options ropt = {0};
  cu_File_Options_read(&ropt);
  fres = cu_Dir_openat(&dir, fp, ropt);
  TEST_ASSERT_TRUE(cu_File_Result_is_ok(&fres));
  file = cu_File_Result_unwrap(&fres);

  char buf[3] = {0};
  err = cu_File_read(&file, cu_Slice_create(buf, sizeof(data) - 1));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  buf[sizeof(data) - 1] = '\0';
  TEST_ASSERT_EQUAL_STRING(buf, data);
  cu_File_close(&file);

  cu_Dir_close(&dir);
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
  RUN_TEST(File_OpenAt);
  RUN_TEST(File_InvalidOptions);
  return UNITY_END();
}
