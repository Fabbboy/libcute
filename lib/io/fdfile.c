#include "io/fdfile.h"
#include "io/error.h"
#include "macro.h"
#include "nostd.h"

#ifndef CU_FREESTANDING

#if CU_PLAT_WINDOWS
#include <io.h>
#include <windows.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

static cu_Io_Error_Optional cu_FdFile_read_impl(cu_FdFile *f, cu_Slice buf) {
  CU_IF_NULL(f) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }
  if (f->handle == CU_INVALID_HANDLE || !buf.ptr) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }
#if CU_PLAT_WINDOWS
  DWORD br;
  if (!ReadFile(f->handle, buf.ptr, (DWORD)buf.length, &br, NULL)) {
    return cu_Io_Error_Optional_some(cu_Io_Error_from_win32(GetLastError()));
  }
#else
  ssize_t br = read(f->handle, buf.ptr, buf.length);
  if (br == -1) {
    return cu_Io_Error_Optional_some(cu_Io_Error_from_errno(errno));
  }
#endif
  return cu_Io_Error_Optional_none();
}

static cu_Io_Error_Optional cu_FdFile_write_impl(cu_FdFile *f, cu_Slice data) {
  CU_IF_NULL(f) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }
  if (f->handle == CU_INVALID_HANDLE || !data.ptr) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }
#if CU_PLAT_WINDOWS
  DWORD bw;
  if (!WriteFile(f->handle, data.ptr, (DWORD)data.length, &bw, NULL)) {
    return cu_Io_Error_Optional_some(cu_Io_Error_from_win32(GetLastError()));
  }
#else
  ssize_t bw = write(f->handle, data.ptr, data.length);
  if (bw == -1) {
    return cu_Io_Error_Optional_some(cu_Io_Error_from_errno(errno));
  }
#endif
  return cu_Io_Error_Optional_none();
}

static cu_Io_Error_Optional cu_FdFile_seek_impl(cu_FdFile *f, cu_File_SeekTo to) {
  CU_IF_NULL(f) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }
  if (f->handle == CU_INVALID_HANDLE) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }
  long offset = 0;
  if (Size_Optional_is_some(&to.offset)) {
    offset = (long)Size_Optional_unwrap(&to.offset);
  }
#if CU_PLAT_WINDOWS
  DWORD method;
  switch (to.whence) {
  case CU_FILE_SEEK_START:
    method = FILE_BEGIN;
    break;
  case CU_FILE_SEEK_CURRENT:
    method = FILE_CURRENT;
    break;
  case CU_FILE_SEEK_END:
    method = FILE_END;
    break;
  default:
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }
  if (SetFilePointer(f->handle, offset, NULL, method) == INVALID_SET_FILE_POINTER) {
    DWORD err = GetLastError();
    if (err != NO_ERROR) {
      return cu_Io_Error_Optional_some(cu_Io_Error_from_win32(err));
    }
  }
#else
  int whence;
  switch (to.whence) {
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
  if (lseek(f->handle, offset, whence) == -1) {
    return cu_Io_Error_Optional_some(cu_Io_Error_from_errno(errno));
  }
#endif
  return cu_Io_Error_Optional_none();
}

static void cu_FdFile_close_impl(cu_FdFile *f) {
  CU_IF_NULL(f) return;
  if (!f->owned || f->handle == CU_INVALID_HANDLE)
    return;
#if CU_PLAT_WINDOWS
  CloseHandle(f->handle);
#else
  close(f->handle);
#endif
  f->handle = CU_INVALID_HANDLE;
}

static cu_Io_Error_Optional cu_stream_read(void *self, cu_Slice buf) {
  return cu_FdFile_read_impl((cu_FdFile *)self, buf);
}

static cu_Io_Error_Optional cu_stream_write(void *self, cu_Slice data) {
  return cu_FdFile_write_impl((cu_FdFile *)self, data);
}

static cu_Io_Error_Optional cu_stream_flush(void *self) {
  CU_UNUSED(self);
  return cu_Io_Error_Optional_none();
}

static void cu_stream_close(void *self) { cu_FdFile_close_impl((cu_FdFile *)self); }

static cu_Io_Error_Optional cu_stream_seek(void *self, cu_File_SeekTo to) {
  return cu_FdFile_seek_impl((cu_FdFile *)self, to);
}

cu_Stream cu_FdFile_stream(cu_FdFile *f) {
  cu_Stream s;
  s.self = f;
  s.readFn = cu_stream_read;
  s.writeFn = cu_stream_write;
  s.flushFn = cu_stream_flush;
  s.closeFn = cu_stream_close;
  s.seekFn = cu_stream_seek;
  return s;
}

cu_Io_Error_Optional cu_FdFile_read(cu_FdFile *f, cu_Slice buf) {
  return cu_FdFile_read_impl(f, buf);
}

cu_Io_Error_Optional cu_FdFile_write(cu_FdFile *f, cu_Slice data) {
  return cu_FdFile_write_impl(f, data);
}

cu_Io_Error_Optional cu_FdFile_seek(cu_FdFile *f, cu_File_SeekTo to) {
  return cu_FdFile_seek_impl(f, to);
}

void cu_FdFile_close(cu_FdFile *f) { cu_FdFile_close_impl(f); }

cu_FdFile cu_FdFile_stdin(void) {
#if CU_PLAT_WINDOWS
  return cu_FdFile_from_handle(GetStdHandle(STD_INPUT_HANDLE), false);
#else
  return cu_FdFile_from_handle(STDIN_FILENO, false);
#endif
}

cu_FdFile cu_FdFile_stdout(void) {
#if CU_PLAT_WINDOWS
  return cu_FdFile_from_handle(GetStdHandle(STD_OUTPUT_HANDLE), false);
#else
  return cu_FdFile_from_handle(STDOUT_FILENO, false);
#endif
}

cu_FdFile cu_FdFile_stderr(void) {
#if CU_PLAT_WINDOWS
  return cu_FdFile_from_handle(GetStdHandle(STD_ERROR_HANDLE), false);
#else
  return cu_FdFile_from_handle(STDERR_FILENO, false);
#endif
}

#endif // CU_FREESTANDING
