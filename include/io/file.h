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
#define MAX_PATH_LENGTH 4096
#else
#define MAX_PATH_LENGTH 260
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

#if CU_PLAT_POSIX
Int_Optional cu_File_OpenOptions_construct(const cu_File_OpenOptions *options);
#endif

cu_File_Result cu_File_open(cu_Slice path, cu_File_OpenOptions options);
void cu_File_close(cu_File *file);

#endif