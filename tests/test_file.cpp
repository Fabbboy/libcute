#include <gtest/gtest.h>
#include <nostd.h>
extern "C" {
#include "cute.h"
}

TEST(File, OpenAndClose) {
  cu_File_OpenOptions options = {0};
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
