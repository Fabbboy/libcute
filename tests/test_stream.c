#include "cute.h"
#include "unity.h"
#include <nostd.h>

static void Stream_WriteRead(void) {
  cu_File_Options opt = {0};
  cu_File_Options_write(&opt);
  cu_File_Options_create(&opt);
  cu_File_Options_truncate(&opt);

  const char path_c[] = "stream_test.txt";
  cu_Slice path = CU_SLICE_CSTR(path_c);

  cu_FStream_Result res =
      cu_FStream_open(path, opt, 16, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_FStream_Result_is_ok(&res));
  cu_FStream fs = cu_FStream_Result_unwrap(&res);
  cu_Stream s = cu_FStream_stream(&fs);

  const char data[] = "hello stream";
  cu_Io_Error_Optional err = cu_Stream_write(&s, CU_SLICE_CSTR(data));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  cu_Stream_close(&s);

  cu_File_Options rd = {0};
  cu_File_Options_read(&rd);
  res = cu_FStream_open(path, rd, 8, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_FStream_Result_is_ok(&res));
  fs = cu_FStream_Result_unwrap(&res);
  s = cu_FStream_stream(&fs);

  char buf[32] = {0};
  err = cu_Stream_read(&s, cu_Slice_create(buf, sizeof(data) - 1));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  buf[sizeof(data) - 1] = '\0';
  TEST_ASSERT_EQUAL_STRING(buf, data);
  cu_Stream_close(&s);
}

static void Stream_Seek(void) {
  cu_File_Options opt = {0};
  cu_File_Options_write(&opt);
  cu_File_Options_create(&opt);
  cu_File_Options_truncate(&opt);

  const char path_c[] = "stream_seek.txt";
  cu_Slice path = CU_SLICE_CSTR(path_c);

  cu_FStream_Result res =
      cu_FStream_open(path, opt, 8, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_FStream_Result_is_ok(&res));
  cu_FStream fs = cu_FStream_Result_unwrap(&res);
  cu_Stream s = cu_FStream_stream(&fs);

  const char data[] = "abcdefgh";
  cu_Io_Error_Optional err = cu_Stream_write(&s, CU_SLICE_CSTR(data));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  err = cu_Stream_seek(&s, (cu_File_SeekTo){.whence = CU_FILE_SEEK_START,
                               .offset = Size_Optional_some(4)});
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  const char rep[] = "XY";
  err = cu_Stream_write(&s, CU_SLICE_CSTR(rep));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  cu_Stream_close(&s);

  cu_File_Options rd = {0};
  cu_File_Options_read(&rd);
  res = cu_FStream_open(path, rd, 8, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_FStream_Result_is_ok(&res));
  fs = cu_FStream_Result_unwrap(&res);
  s = cu_FStream_stream(&fs);

  char buf[10] = {0};
  err = cu_Stream_read(&s, cu_Slice_create(buf, 8));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  buf[8] = '\0';
  TEST_ASSERT_EQUAL_STRING(buf, "abcdXYgh");
  cu_Stream_close(&s);
}

static void MemStream_Roundtrip(void) {
  cu_MemStream_Result res = cu_MemStream_create(8, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_MemStream_Result_is_ok(&res));
  cu_MemStream ms = cu_MemStream_Result_unwrap(&res);
  cu_Stream s = cu_MemStream_stream(&ms);

  const char msg[] = "memdata";
  cu_Io_Error_Optional err = cu_Stream_write(&s, CU_SLICE_CSTR(msg));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));

  err = cu_Stream_seek(&s, (cu_File_SeekTo){.whence = CU_FILE_SEEK_START,
                               .offset = Size_Optional_none()});
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));

  char buf[8] = {0};
  err = cu_Stream_read(&s, cu_Slice_create(buf, 7));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  buf[7] = '\0';
  TEST_ASSERT_EQUAL_STRING(buf, msg);
  cu_Stream_close(&s);
}

static void MemStream_Grow(void) {
  cu_MemStream_Result res = cu_MemStream_create(2, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_MemStream_Result_is_ok(&res));
  cu_MemStream ms = cu_MemStream_Result_unwrap(&res);
  cu_Stream s = cu_MemStream_stream(&ms);

  const char msg[] = "01234567";
  cu_Io_Error_Optional err = cu_Stream_write(&s, CU_SLICE_CSTR(msg));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));

  err = cu_Stream_seek(&s, (cu_File_SeekTo){.whence = CU_FILE_SEEK_START,
                               .offset = Size_Optional_none()});
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));

  char buf[9] = {0};
  err = cu_Stream_read(&s, cu_Slice_create(buf, 8));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  buf[8] = '\0';
  TEST_ASSERT_EQUAL_STRING(buf, msg);
  cu_Stream_close(&s);
}

static void MemStream_Tell(void) {
  cu_MemStream_Result res = cu_MemStream_create(4, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_MemStream_Result_is_ok(&res));
  cu_MemStream ms = cu_MemStream_Result_unwrap(&res);
  cu_Stream s = cu_MemStream_stream(&ms);

  const char msg[] = "abc";
  cu_Io_Error_Optional err = cu_Stream_write(&s, CU_SLICE_CSTR(msg));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));

  cu_IoSize_Result posres = cu_Stream_tell(&s);
  TEST_ASSERT_TRUE(cu_IoSize_Result_is_ok(&posres));
  size_t pos = cu_IoSize_Result_unwrap(&posres);
  TEST_ASSERT_EQUAL_INT64(3, pos);

  err = cu_Stream_seek(&s, (cu_File_SeekTo){.whence = CU_FILE_SEEK_START,
                               .offset = Size_Optional_none()});
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  posres = cu_Stream_tell(&s);
  TEST_ASSERT_TRUE(cu_IoSize_Result_is_ok(&posres));
  pos = cu_IoSize_Result_unwrap(&posres);
  TEST_ASSERT_EQUAL_INT64(0, pos);
  cu_Stream_close(&s);
}

static void FStream_OpenAt(void) {
  cu_Dir_Result dres =
      cu_Dir_open(CU_SLICE_CSTR("fsdir"), true, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_Dir_Result_is_ok(&dres));
  cu_Dir dir = cu_Dir_Result_unwrap(&dres);

  cu_File_Options opt = {0};
  cu_File_Options_write(&opt);
  cu_File_Options_create(&opt);
  cu_File_Options_truncate(&opt);

  const char name[] = "note.txt";
  cu_FStream_Result fres = cu_FStream_openat(
      &dir, CU_SLICE_CSTR(name), opt, 8, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_FStream_Result_is_ok(&fres));
  cu_FStream fs = cu_FStream_Result_unwrap(&fres);
  cu_Stream s = cu_FStream_stream(&fs);

  const char msg[] = "ok";
  cu_Io_Error_Optional err = cu_Stream_write(&s, CU_SLICE_CSTR(msg));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  cu_Stream_close(&s);

  cu_File_Options rd = {0};
  cu_File_Options_read(&rd);
  fres = cu_FStream_openat(
      &dir, CU_SLICE_CSTR(name), rd, 4, cu_Allocator_CAllocator());
  TEST_ASSERT_TRUE(cu_FStream_Result_is_ok(&fres));
  fs = cu_FStream_Result_unwrap(&fres);
  s = cu_FStream_stream(&fs);

  char buf[3] = {0};
  err = cu_Stream_read(&s, cu_Slice_create(buf, 2));
  TEST_ASSERT_TRUE(cu_Io_Error_Optional_is_none(&err));
  buf[2] = '\0';
  TEST_ASSERT_EQUAL_STRING(buf, msg);
  cu_Stream_close(&s);

  cu_Dir_close(&dir);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(Stream_WriteRead);
  RUN_TEST(Stream_Seek);
  RUN_TEST(MemStream_Roundtrip);
  RUN_TEST(MemStream_Grow);
  RUN_TEST(MemStream_Tell);
  RUN_TEST(FStream_OpenAt);
  return UNITY_END();
}
