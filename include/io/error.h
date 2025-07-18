#pragma once

#include "object/optional.h"

typedef enum {
  CU_IO_ERROR_NONE = 0,    // No error / success case
  CU_IO_ERROR_EOF,         // End of stream / file
  CU_IO_ERROR_UNSUPPORTED, // Operation not supported
  CU_IO_ERROR_INVALID,     // Invalid argument
  CU_IO_ERROR_IO,          // Generic I/O failure
  CU_IO_ERROR_PERMISSION,  // Permission denied
  CU_IO_ERROR_CLOSED,      // Operation on closed stream
  CU_IO_ERROR_WOULDBLOCK,  // Non-blocking op would block
} cu_Io_Error;

CU_OPTIONAL_DECL(cu_Io_Error, cu_Io_Error)
