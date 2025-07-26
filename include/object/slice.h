#pragma once

/** @file slice.h Non-owning memory slices. */

#include "io/error.h"
#include "object/optional.h"
#include "object/result.h"
#include <stdbool.h>
#include <stddef.h>

/** Non-owning memory view. */
typedef struct {
  void *ptr;     /**< pointer to first element */
  size_t length; /**< number of bytes */
} cu_Slice;

/** Create a slice from a pointer and length without allocating. */
cu_Slice cu_Slice_create(void *ptr, size_t length);

CU_OPTIONAL_DECL(cu_Slice, cu_Slice)
CU_RESULT_DECL(cu_Slice, cu_Slice, cu_Io_Error)
