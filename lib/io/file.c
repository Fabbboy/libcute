#include "io/file.h"
#include "io/error.h"
#include "macro.h"
#include "nostd.h"
#include "object/optional.h"
#include "object/result.h"
#include <errno.h>

#ifndef CU_FREESTANDING

CU_RESULT_IMPL(cu_File, cu_File, cu_Io_Error)
#if CU_PLAT_POSIX
#include <fcntl.h>
#include <unistd.h>
Int_Optional cu_File_OpenOptions_construct(const cu_File_OpenOptions *options) {
  CU_IF_NULL(options) { return Int_Optional_none(); }

  int flags = 0;
  if (options->read) {
    flags |= O_RDONLY;
  }

  if (options->write) {
    flags |= O_WRONLY;
  }

  if (options->create) {
    flags |= O_CREAT;
  }

  if (options->append) {
    flags |= O_APPEND;
  }

  if (options->truncate) {
    flags |= O_TRUNC;
  }

  if (flags == 0) {
    return Int_Optional_none();
  }

  return Int_Optional_some(flags);
}
#endif

cu_File_Result cu_File_open(cu_Slice path, cu_File_OpenOptions options) {
  Int_Optional flags = cu_File_OpenOptions_construct(&options);
  if (Int_Optional_is_none(&flags)) {
    cu_Io_Error error = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT,
        .errnum = Size_Optional_none(),
    };
    return cu_File_result_error(error);
  }

  char lpath[MAX_PATH_LENGTH] = {0};
  cu_Memory_smemcpy(cu_Slice_create(lpath, MAX_PATH_LENGTH), path);
  lpath[MAX_PATH_LENGTH - 1] = '\0';

  cu_Handle handle = -1;
#if CU_PLAT_POSIX
  handle = open(lpath, Int_Optional_unwrap(&flags));
  if (handle < 0) {
    cu_Io_Error error = {
        .kind = CU_IO_ERROR_KIND_OTHER,
        .errnum = Size_Optional_some(errno),
    };
    return cu_File_result_error(error);
  }
#else
#error "Windows file opening not implemented"
#endif

  cu_File file = {.handle = handle};
  return cu_File_result_ok(file);
}
void cu_File_close(cu_File *file) {
  CU_IF_NULL(file) return;

#if CU_PLAT_POSIX
  close(file->handle);
#else
#error "Windows file closing not implemented"
#endif
  file->handle = -1; // Invalidate handle
}
#endif