#include "io/error.h"
#include "macro.h"
#include "object/optional.h"

#ifndef CU_FREESTANDING

#if CU_PLAT_WINDOWS
#include <windows.h>
#else
#include <errno.h>
#endif

CU_OPTIONAL_IMPL(cu_Io_Error, cu_Io_Error)
CU_RESULT_IMPL(cu_IoSlice, cu_Slice, cu_Io_Error)

cu_Io_Error cu_Io_Error_from_errno(int error_code) {
  cu_Io_ErrorKind kind;

  switch (error_code) {
#if CU_PLAT_POSIX
  case ENOENT:
    kind = CU_IO_ERROR_KIND_NOT_FOUND;
    break;
  case EACCES:
  case EPERM:
    kind = CU_IO_ERROR_KIND_PERMISSION_DENIED;
    break;
  case ECONNREFUSED:
    kind = CU_IO_ERROR_KIND_CONNECTION_REFUSED;
    break;
  case ECONNRESET:
    kind = CU_IO_ERROR_KIND_CONNECTION_RESET;
    break;
  case ECONNABORTED:
    kind = CU_IO_ERROR_KIND_CONNECTION_ABORTED;
    break;
  case ENOTCONN:
    kind = CU_IO_ERROR_KIND_NOT_CONNECTED;
    break;
  case EADDRINUSE:
    kind = CU_IO_ERROR_KIND_ADDR_IN_USE;
    break;
  case EADDRNOTAVAIL:
    kind = CU_IO_ERROR_KIND_ADDR_NOT_AVAILABLE;
    break;
  case EPIPE:
    kind = CU_IO_ERROR_KIND_BROKEN_PIPE;
    break;
  case EAGAIN:
#if EAGAIN != EWOULDBLOCK
  case EWOULDBLOCK:
#endif
    kind = CU_IO_ERROR_KIND_WOULD_BLOCK;
    break;
  case EINVAL:
    kind = CU_IO_ERROR_KIND_INVALID_INPUT;
    break;
  case ETIMEDOUT:
    kind = CU_IO_ERROR_KIND_TIMED_OUT;
    break;
  case EINTR:
    kind = CU_IO_ERROR_KIND_INTERRUPTED;
    break;
  case ENOMEM:
    kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY;
    break;
#endif
  default:
    kind = CU_IO_ERROR_KIND_OTHER;
    break;
  }

  return (cu_Io_Error){
      .kind = kind, .errnum = Size_Optional_some((size_t)error_code)};
}

#if CU_PLAT_WINDOWS
cu_Io_Error cu_Io_Error_from_win32(unsigned long error_code) {
  cu_Io_ErrorKind kind;

  switch (error_code) {
  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
    kind = CU_IO_ERROR_KIND_NOT_FOUND;
    break;
  case ERROR_ACCESS_DENIED:
    kind = CU_IO_ERROR_KIND_PERMISSION_DENIED;
    break;
  case ERROR_INVALID_PARAMETER:
  case ERROR_INVALID_HANDLE:
    kind = CU_IO_ERROR_KIND_INVALID_INPUT;
    break;
  case ERROR_NOT_ENOUGH_MEMORY:
  case ERROR_OUTOFMEMORY:
    kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY;
    break;
  case ERROR_BROKEN_PIPE:
    kind = CU_IO_ERROR_KIND_BROKEN_PIPE;
    break;
  case ERROR_OPERATION_ABORTED:
    kind = CU_IO_ERROR_KIND_INTERRUPTED;
    break;
  default:
    kind = CU_IO_ERROR_KIND_OTHER;
    break;
  }

  return (cu_Io_Error){
      .kind = kind, .errnum = Size_Optional_some((size_t)error_code)};
}
#endif

#endif
