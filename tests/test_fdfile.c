#include "cute.h"
#include "unity.h"
#include <nostd.h>

static void FdFile_Wrap(void) {
  cu_File_Options opt = {0};
  cu_File_Options_write(&opt);
  cu_File_Options_create(&opt);
  cu_File_Options_truncate(&opt);

  cu_File_Result res =
      cu_File_open(CU_SLICE_CSTR("fdwrap.txt"), opt, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_File_Result_is_ok(&res));
  cu_File file = cu_File_Result_unwrap(&res);

  cu_FdFile fd = cu_FdFile_from_handle(file.handle, false);
  cu_Stream s = cu_FdFile_stream(&fd);
  const char data[] = "fddata";
  cu_Io_Error_Optional err = cu_Stream_write(&s, CU_SLICE_CSTR(data));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));

  cu_Stream_close(&s); /* does not close handle */
  cu_File_close(&file);
}

static void FdFile_Std(void) {
  cu_FdFile out = cu_FdFile_stdout();
  TEST_ASSERT(out.handle != CU_INVALID_HANDLE);
  cu_FdFile errf = cu_FdFile_stderr();
  TEST_ASSERT(errf.handle != CU_INVALID_HANDLE);
  cu_FdFile in = cu_FdFile_stdin();
  TEST_ASSERT(in.handle != CU_INVALID_HANDLE);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(FdFile_Wrap);
  RUN_TEST(FdFile_Std);
  return UNITY_END();
}
