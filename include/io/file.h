#pragma once

#include "io/error.h" // use io error type for read/writes to Handles
#include "macro.h"
#include "object/optional.h"
#include "object/result.h"
#include "object/slice.h"

#if CU_PLAT_POSIX
#include <unistd.h>
#else
#error "Unsupported platform for file operations."
#endif

typedef struct {
#if CU_PLAT_POSIX
  Int_Optional fd;
#else
#error "Unsupported platform for file descriptor."
#endif
} cu_Handle;

CU_OPTIONAL_DECL(cu_Handle, cu_Handle)

static inline cu_Handle_Optional cu_Handle_FromFd(int fd) {
  if (fd < 0) {
    return cu_Handle_Optional_none();
  }

  cu_Handle handle = {.fd = Int_Optional_some(fd)};
  return cu_Handle_Optional_some(handle);
}

static inline void cu_Handle_Close(cu_Handle handle) {
#if CU_PLAT_POSIX
  if (Int_Optional_is_some(&handle.fd)) {
    close(Int_Optional_unwrap(&handle.fd));
  }
#else
#error "Unsupported platform for closing file descriptor."
#endif
}

typedef enum {
  /** Read access mode. */
  CU_FILE_OMODE_READ = CU_BIT(0),
  /** Write access mode. */
  CU_FILE_OMODE_WRITE = CU_BIT(1),
  /** Read and write access mode. */
  CU_FILE_OMODE_READ_WRITE = CU_FILE_OMODE_READ | CU_FILE_OMODE_WRITE,
} cu_File_OpenMode;

typedef enum {
  CU_FILE_CMODE_CREATE = CU_BIT(0),
  CU_FILE_CMODE_TRUNCATE = CU_BIT(1),
} cu_File_CreateMode;

CU_OPTIONAL_DECL(cu_File_CreateMode, cu_File_CreateMode)

typedef struct {
  cu_Handle handle;
} cu_File;

CU_RESULT_DECL(cu_File, cu_File, cu_Io_Error)

cu_File_Result cu_File_Open(cu_Slice path, cu_File_OpenMode mode,
    cu_File_CreateMode_Optional createMode);
