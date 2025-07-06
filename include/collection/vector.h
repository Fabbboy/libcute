#pragma once

#include "macro.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include "object/result.h"
#include "object/slice.h"
#include "utility.h"
#include <stddef.h>

typedef struct {
  cu_Slice_Optional data;
  size_t length;
  size_t capacity;
  cu_Layout layout;
  cu_Allocator allocator;
} cu_Vector;

typedef enum {
  CU_VECTOR_ERROR_NONE = 0,
  CU_VECTOR_ERROR_OOM,
  CU_VECTOR_ERROR_INVALID_LAYOUT,
  CU_VECTOR_ERROR_INVALID,
  CU_VECTOR_ERROR_OOB,
} cu_Vector_Error;

CU_RESULT_DECL(cu_Vector, cu_Vector, cu_Vector_Error)
CU_OPTIONAL_DECL(cu_Vector_Error, cu_Vector_Error)

cu_Vector_Result cu_Vector_create(
    cu_Allocator allocator, cu_Layout layout, Size_Optional initial_capacity);
cu_Vector_Error_Optional cu_Vector_resize(cu_Vector *vector, size_t capacity);
void cu_Vector_destroy(cu_Vector *vector);

static inline size_t cu_Vector_size(const cu_Vector *vector) {
  CU_IF_NULL(vector) { return 0; }
  return vector->length;
}
static inline size_t cu_Vector_capacity(const cu_Vector *vector) {
  CU_IF_NULL(vector) { return 0; }
  return vector->capacity;
}

cu_Vector_Error_Optional cu_Vector_push_back(cu_Vector *vector, void *elem);
cu_Vector_Error_Optional cu_Vector_pop_back(cu_Vector *vector, void *out_elem);