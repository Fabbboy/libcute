#include <gtest/gtest.h>
#include <nostd.h>
extern "C" {
#include "cute.h"
}

TEST(File, OpenAndClose) {
  cu_File_OpenOptions options{};
  cu_File_OpenOptions_read(&options);
  cu_File_OpenOptions_create(&options);

  const char lpath[] = "test.txt";
  cu_Slice path = cu_Slice_create((void *)lpath, sizeof(lpath) - 1);
  cu_File_Result res = cu_File_open(path, options);
  ASSERT_TRUE(cu_File_Result_is_ok(&res));
  cu_File file = cu_File_Result_unwrap(&res);
  EXPECT_NE(file.handle, CU_INVALID_HANDLE);

  cu_File_close(&file);
  EXPECT_EQ(file.handle, CU_INVALID_HANDLE);
}

TEST(File, WriteAndRead) {
  cu_File_OpenOptions options{};
  cu_File_OpenOptions_write(&options);
  cu_File_OpenOptions_create(&options);
  cu_File_OpenOptions_truncate(&options);

  const char lpath[] = "io_test.txt";
  cu_Slice path = cu_Slice_create((void *)lpath, sizeof(lpath) - 1);
  cu_File_Result res = cu_File_open(path, options);
  ASSERT_TRUE(cu_File_Result_is_ok(&res));
  cu_File file = cu_File_Result_unwrap(&res);

  const char data[] = "hello";
  cu_Slice data_slice = cu_Slice_create((void *)data, sizeof(data) - 1);
  cu_Io_Error_Optional write_err = cu_File_write(&file, data_slice);
  EXPECT_TRUE(cu_Io_Error_Optional_is_none(&write_err));

  cu_File_close(&file);

  cu_File_OpenOptions rd{};
  cu_File_OpenOptions_read(&rd);
  res = cu_File_open(path, rd);
  ASSERT_TRUE(cu_File_Result_is_ok(&res));
  file = cu_File_Result_unwrap(&res);

  char buffer[6] = {0};
  cu_Slice buffer_slice = cu_Slice_create(buffer, sizeof(data) - 1);
  cu_Io_Error_Optional read_err = cu_File_read(&file, buffer_slice);
  EXPECT_TRUE(cu_Io_Error_Optional_is_none(&read_err));
  buffer[sizeof(data) - 1] = '\0';
  EXPECT_STREQ(buffer, data);

  cu_File_close(&file);
}

TEST(File, InvalidOptions) {
  cu_File_OpenOptions options{};
  const char lpath[] = "invalid.txt";
  cu_Slice path = cu_Slice_create((void *)lpath, sizeof(lpath) - 1);
  cu_File_Result res = cu_File_open(path, options);
  EXPECT_FALSE(cu_File_Result_is_ok(&res));
}
