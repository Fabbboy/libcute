#pragma once

#include "io/error.h"
#include "io/fd.h"

typedef struct {
  cu_Handle handle;
  cu_File_Stat stat;
} cu_Dir;

CU_RESULT_DECL(cu_Dir, cu_Dir, cu_Io_Error)

cu_Dir_Result cu_Dir_open(cu_Slice path, bool create);
void cu_Dir_close(cu_Dir *dir);
