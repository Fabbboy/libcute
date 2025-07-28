#pragma once
#include "object/optional.h"

#ifndef CU_FREESTANDING
#include "io/error.h"
#include "io/fd.h"
#include "nostd.h"
#include "object/result.h"
#include <stdbool.h>
#include <stddef.h>

#if CU_PLAT_POSIX
#define CU_FILE_MAX_PATH_LENGTH 4096
#else
#define CU_FILE_MAX_PATH_LENGTH 260
#endif

typedef struct {
  cu_Handle handle;
} cu_File;

CU_RESULT_DECL(cu_File, cu_File, cu_Io_Error)

typedef struct {
  bool read;
  bool write;
  bool create;
  bool append;
  bool truncate;
} cu_File_OpenOptions;

// Seek operations
typedef enum {
  CU_FILE_SEEK_START,
  CU_FILE_SEEK_CURRENT,
  CU_FILE_SEEK_END
} cu_File_Seek;

typedef struct {
  cu_File_Seek whence;
  Size_Optional offset;
} cu_File_SeekTo;

// Option setters
static inline void cu_File_OpenOptions_read(cu_File_OpenOptions *options) {
  options->read = true;
}

static inline void cu_File_OpenOptions_write(cu_File_OpenOptions *options) {
  options->write = true;
}

static inline void cu_File_OpenOptions_create(cu_File_OpenOptions *options) {
  options->create = true;
}

static inline void cu_File_OpenOptions_append(cu_File_OpenOptions *options) {
  options->append = true;
}

static inline void cu_File_OpenOptions_truncate(cu_File_OpenOptions *options) {
  options->truncate = true;
}

cu_File_Result cu_File_open(cu_Slice path, cu_File_OpenOptions options);
void cu_File_close(cu_File *file);
cu_Io_Error_Optional cu_File_read(cu_File *file, cu_Slice buffer);
cu_Io_Error_Optional cu_File_write(cu_File *file, cu_Slice data);
cu_Io_Error_Optional cu_File_seek(cu_File *file, cu_File_SeekTo seek_to);

#endif // CU_FREESTANDING