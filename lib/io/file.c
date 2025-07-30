#include "io/file.h"
#include "io/error.h"
#include "io/fd.h"
#include "string/string.h"
#include "macro.h"
#include "nostd.h"
#include "object/optional.h"
#include "object/result.h"

#ifndef CU_FREESTANDING

#if CU_PLAT_WINDOWS
#include <io.h>
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

CU_RESULT_IMPL(cu_File, cu_File, cu_Io_Error)

#if CU_PLAT_POSIX
static int cu_File_Options_to_posix_flags(const cu_File_Options *options) {
  int flags = 0;

  if (options->read && options->write) {
    flags |= O_RDWR;
  } else if (options->write) {
    flags |= O_WRONLY;
  } else if (options->read) {
    flags |= O_RDONLY;
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

  return flags;
}
#endif

#if CU_PLAT_WINDOWS
static DWORD cu_File_Options_to_win32(
    const cu_File_OpenOptions *options) {
  DWORD access = 0;

  if (options->read) {
    access |= GENERIC_READ;
  }

  if (options->write) {
    access |= GENERIC_WRITE;
  }

  return access;
}

static DWORD cu_File_OpenOptions_to_win32_creation(
    const cu_File_OpenOptions *options) {
  if (options->create && options->truncate) {
    return CREATE_ALWAYS;
  } else if (options->create) {
    return OPEN_ALWAYS;
  } else if (options->truncate) {
    return TRUNCATE_EXISTING;
  } else {
    return OPEN_EXISTING;
  }
}
#endif

cu_File_Result cu_File_open(
    cu_Slice path, cu_File_Options options, cu_Allocator allocator) {
  if (!options.read && !options.write) {
    cu_Io_Error error = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT,
        .errnum = Size_Optional_none(),
    };
    return cu_File_Result_error(error);
  }

  char lpath[CU_FILE_MAX_PATH_LENGTH] = {0};
  cu_Slice lpath_slice = cu_Slice_create(lpath, CU_FILE_MAX_PATH_LENGTH);
  cu_Memory_smemcpy(lpath_slice, path);

  cu_Handle handle = CU_INVALID_HANDLE;
  cu_File_Stat stat;
  cu_Memory_memset(&stat, 0, sizeof(stat));

#if CU_PLAT_POSIX
  int flags = cu_File_Options_to_posix_flags(&options);
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 644

  handle = open(lpath, flags, mode);
  if (handle == -1) {
    return cu_File_Result_error(cu_Io_Error_from_errno(errno));
  }

  stat = cu_File_Stat_from_handle(handle);
  cu_String_Result pres = cu_String_from_cstr(allocator, lpath);
  if (!cu_String_Result_is_ok(&pres)) {
    close(handle);
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
        .errnum = Size_Optional_none()};
    return cu_File_Result_error(err);
  }
  stat.path = pres.value;

#else
  DWORD access = cu_File_Options_to_win32(&options);
  DWORD creation = cu_File_OpenOptions_to_win32_creation(&options);
  DWORD attributes = FILE_ATTRIBUTE_NORMAL;

  handle = CreateFileA(lpath, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
      creation, attributes, NULL);

  if (handle == INVALID_HANDLE_VALUE) {
    return cu_File_Result_error(cu_Io_Error_from_win32(GetLastError()));
  }

  stat = cu_File_Stat_from_handle(handle);
  pres = cu_String_from_cstr(allocator, lpath);
  if (!cu_String_Result_is_ok(&pres)) {
    CloseHandle(handle);
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
        .errnum = Size_Optional_none()};
    return cu_File_Result_error(err);
  }
  stat.path = pres.value;
#endif

  cu_File file;
  cu_Memory_memset(&file, 0, sizeof(file));
  file.handle = handle;
  file.stat = stat;
  return cu_File_Result_ok(file);
}

void cu_File_close(cu_File *file) {
  CU_IF_NULL(file) return;
  if (file->handle == CU_INVALID_HANDLE)
    return;

#if CU_PLAT_POSIX
  close(file->handle);
#else
  CloseHandle(file->handle);
#endif

  file->handle = CU_INVALID_HANDLE;
  cu_File_Stat_destroy(&file->stat);
}

cu_Io_Error_Optional cu_File_read(cu_File *file, cu_Slice buffer) {
  CU_IF_NULL(file) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }

  if (file->handle == CU_INVALID_HANDLE || !buffer.ptr) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }

#if CU_PLAT_POSIX
  ssize_t bytes_read = read(file->handle, buffer.ptr, buffer.length);
  if (bytes_read == -1) {
    return cu_Io_Error_Optional_some(cu_Io_Error_from_errno(errno));
  }
#else
  DWORD bytes_read;
  if (!ReadFile(
          file->handle, buffer.ptr, (DWORD)buffer.length, &bytes_read, NULL)) {
    return cu_Io_Error_Optional_some(cu_Io_Error_from_win32(GetLastError()));
  }
#endif

  return cu_Io_Error_Optional_none();
}

cu_Io_Error_Optional cu_File_write(cu_File *file, cu_Slice data) {
  CU_IF_NULL(file) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }

  if (file->handle == CU_INVALID_HANDLE || !data.ptr) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }

#if CU_PLAT_POSIX
  ssize_t bytes_written = write(file->handle, data.ptr, data.length);
  if (bytes_written == -1) {
    return cu_Io_Error_Optional_some(cu_Io_Error_from_errno(errno));
  }
#else
  DWORD bytes_written;
  if (!WriteFile(
          file->handle, data.ptr, (DWORD)data.length, &bytes_written, NULL)) {
    return cu_Io_Error_Optional_some(cu_Io_Error_from_win32(GetLastError()));
  }
#endif

  return cu_Io_Error_Optional_none();
}

cu_Io_Error_Optional cu_File_seek(cu_File *file, cu_File_SeekTo seek_to) {
  CU_IF_NULL(file) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }

  if (file->handle == CU_INVALID_HANDLE) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }

  long offset = 0;
  if (Size_Optional_is_some(&seek_to.offset)) {
    offset = (long)Size_Optional_unwrap(&seek_to.offset);
  }

#if CU_PLAT_POSIX
  int whence;
  switch (seek_to.whence) {
  case CU_FILE_SEEK_START:
    whence = SEEK_SET;
    break;
  case CU_FILE_SEEK_CURRENT:
    whence = SEEK_CUR;
    break;
  case CU_FILE_SEEK_END:
    whence = SEEK_END;
    break;
  default:
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }

  if (lseek(file->handle, offset, whence) == -1) {
    return cu_Io_Error_Optional_some(cu_Io_Error_from_errno(errno));
  }
#else
  DWORD move_method;
  switch (seek_to.whence) {
  case CU_FILE_SEEK_START:
    move_method = FILE_BEGIN;
    break;
  case CU_FILE_SEEK_CURRENT:
    move_method = FILE_CURRENT;
    break;
  case CU_FILE_SEEK_END:
    move_method = FILE_END;
    break;
  default:
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }

  if (SetFilePointer(file->handle, offset, NULL, move_method) ==
      INVALID_SET_FILE_POINTER) {
    DWORD error = GetLastError();
    if (error != NO_ERROR) {
      return cu_Io_Error_Optional_some(cu_Io_Error_from_win32(error));
    }
  }
#endif

  return cu_Io_Error_Optional_none();
}

cu_File_Result cu_Dir_openat(
    cu_Dir *dir, cu_Slice path, cu_File_Options options) {
  CU_IF_NULL(dir) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT,
        .errnum = Size_Optional_none(),
    };
    return cu_File_Result_error(err);
  }

  if (dir->handle == CU_INVALID_HANDLE) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT,
        .errnum = Size_Optional_none(),
    };
    return cu_File_Result_error(err);
  }

  if (!options.read && !options.write) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT,
        .errnum = Size_Optional_none(),
    };
    return cu_File_Result_error(err);
  }

  char lpath[CU_FILE_MAX_PATH_LENGTH] = {0};
  cu_Slice lpath_slice = cu_Slice_create(lpath, CU_FILE_MAX_PATH_LENGTH);
  cu_Memory_smemcpy(lpath_slice, path);

  cu_Handle handle = CU_INVALID_HANDLE;
  cu_File_Stat stat;
  cu_Memory_memset(&stat, 0, sizeof(stat));

  char fullpath[CU_FILE_MAX_PATH_LENGTH] = {0};
  cu_Slice fullpath_slice = cu_Slice_create(fullpath, CU_FILE_MAX_PATH_LENGTH);

  cu_Memory_smemcpy(fullpath_slice, cu_String_as_slice(&dir->stat.path));
  fullpath_slice.length += dir->stat.path.length;
  cu_Memory_smemcpy(fullpath_slice, lpath_slice);

#if CU_PLAT_POSIX
  int flags = cu_File_Options_to_posix_flags(&options);
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  handle = open(fullpath, flags, mode);
  if (handle == -1) {
    return cu_File_Result_error(cu_Io_Error_from_errno(errno));
  }

  stat = cu_File_Stat_from_handle(handle);
  cu_String_Result pres =
      cu_String_from_cstr(dir->stat.path.allocator, lpath);
  if (!cu_String_Result_is_ok(&pres)) {
    close(handle);
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
        .errnum = Size_Optional_none()};
    return cu_File_Result_error(err);
  }
  stat.path = pres.value;
#else
  DWORD access = cu_File_Options_to_win32(&options);
  DWORD creation = cu_File_OpenOptions_to_win32_creation(&options);
  DWORD attributes = FILE_ATTRIBUTE_NORMAL;

  handle = CreateFileA(fullpath, access,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, creation, attributes, NULL);

  if (handle == INVALID_HANDLE_VALUE) {
    return cu_File_Result_error(cu_Io_Error_from_win32(GetLastError()));
  }

  stat = cu_File_Stat_from_handle(handle);
  cu_String_Result pres =
      cu_String_from_cstr(dir->stat.path.allocator, lpath);
  if (!cu_String_Result_is_ok(&pres)) {
    CloseHandle(handle);
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
        .errnum = Size_Optional_none()};
    return cu_File_Result_error(err);
  }
  stat.path = pres.value;
#endif

  cu_File file;
  cu_Memory_memset(&file, 0, sizeof(file));
  file.handle = handle;
  file.stat = stat;
  return cu_File_Result_ok(file);
}

#endif // CU_FREESTANDING
