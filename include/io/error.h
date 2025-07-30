#pragma once
#include "object/optional.h"
#include "object/result.h"
#include <stddef.h>

typedef enum {
  CU_IO_ERROR_KIND_NOT_FOUND,
  CU_IO_ERROR_KIND_PERMISSION_DENIED,
  CU_IO_ERROR_KIND_CONNECTION_REFUSED,
  CU_IO_ERROR_KIND_CONNECTION_RESET,
  CU_IO_ERROR_KIND_CONNECTION_ABORTED,
  CU_IO_ERROR_KIND_NOT_CONNECTED,
  CU_IO_ERROR_KIND_ADDR_IN_USE,
  CU_IO_ERROR_KIND_ADDR_NOT_AVAILABLE,
  CU_IO_ERROR_KIND_BROKEN_PIPE,
  CU_IO_ERROR_KIND_WOULD_BLOCK,
  CU_IO_ERROR_KIND_INVALID_INPUT,
  CU_IO_ERROR_KIND_INVALID_DATA,
  CU_IO_ERROR_KIND_TIMED_OUT,
  CU_IO_ERROR_KIND_INTERRUPTED,
  CU_IO_ERROR_KIND_UNEXPECTED_EOF,
  CU_IO_ERROR_KIND_UNSUPPORTED,
  CU_IO_ERROR_KIND_OUT_OF_MEMORY,
  CU_IO_ERROR_KIND_OTHER
} cu_Io_ErrorKind;

typedef struct {
  cu_Io_ErrorKind kind;
  Size_Optional errnum;
} cu_Io_Error;

CU_OPTIONAL_DECL(cu_Io_Error, cu_Io_Error)
CU_RESULT_DECL(cu_IoSlice, cu_Slice, cu_Io_Error)
CU_RESULT_DECL(cu_IoSize, size_t, cu_Io_Error)

#ifndef CU_FREESTANDING
cu_Io_Error cu_Io_Error_from_errno(int error_code);

#if CU_PLAT_WINDOWS
cu_Io_Error cu_Io_Error_from_win32(unsigned long error_code);
#endif
#endif
