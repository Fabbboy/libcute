#include "collection/vector.h"
#include "macro.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include "object/result.h"
#include "object/slice.h"
#include "utility.h"
#include <stdalign.h>
#include <stddef.h>

CU_RESULT_IMPL(cu_Vector, cu_Vector, cu_Vector_Error)
CU_OPTIONAL_IMPL(cu_Vector_Error, cu_Vector_Error)

cu_Vector_Result cu_Vector_create(
    cu_Allocator allocator, cu_Layout layout, Size_Optional initial_capacity) {
  CU_LAYOUT_CHECK(layout) {
    return cu_Vector_result_error(CU_VECTOR_ERROR_INVALID_LAYOUT);
  }

  size_t cap = 0;
  cu_Slice_Optional data = cu_Slice_none();
  if (Size_is_some(&initial_capacity) && Size_unwrap(&initial_capacity) > 0) {
    cap = Size_unwrap(&initial_capacity);
    data =
        cu_Allocator_Alloc(allocator, cap * layout.elem_size, layout.alignment);
    if (cu_Slice_is_none(&data)) {
      return cu_Vector_result_error(CU_VECTOR_ERROR_OOM);
    }
  }

  cu_Vector vector;
  vector.data = data;
  vector.length = 0;
  vector.capacity = cap;
  vector.layout = layout;
  vector.allocator = allocator;

  return cu_Vector_result_ok(vector);
}

cu_Vector_Error_Optional cu_Vector_resize(cu_Vector *vector, size_t capacity) {
  CU_IF_NULL(vector) { return cu_Vector_Error_some(CU_VECTOR_ERROR_INVALID); }

  CU_LAYOUT_CHECK(vector->layout) {
    return cu_Vector_Error_some(CU_VECTOR_ERROR_INVALID_LAYOUT);
  }

  if (capacity == vector->capacity) {
    return cu_Vector_Error_none();
  }

  if (capacity == 0) {
    cu_Vector_destroy(vector);
    return cu_Vector_Error_none();
  }

  if (capacity < vector->length) {
    return cu_Vector_Error_some(CU_VECTOR_ERROR_INVALID);
  }

  if (cu_Slice_is_some(&vector->data)) {
    cu_Slice old_data = cu_Slice_unwrap(&vector->data);
    cu_Slice_Optional new_data =
        cu_Allocator_Resize(vector->allocator, old_data,
            capacity * vector->layout.elem_size, vector->layout.alignment);

    if (cu_Slice_is_none(&new_data)) {
      return cu_Vector_Error_some(CU_VECTOR_ERROR_OOM);
    }

    vector->data = new_data;
    vector->capacity = capacity;
    return cu_Vector_Error_none();
  }

  cu_Slice_Optional new_data = cu_Allocator_Alloc(vector->allocator,
      capacity * vector->layout.elem_size, vector->layout.alignment);
  if (cu_Slice_is_none(&new_data)) {
    return cu_Vector_Error_some(CU_VECTOR_ERROR_OOM);
  }

  vector->data = new_data;
  vector->capacity = capacity;
  vector->length = 0;
  return cu_Vector_Error_none();
}

void cu_Vector_destroy(cu_Vector *vector) {
  if (cu_Slice_is_some(&vector->data)) {
    cu_Allocator_Free(vector->allocator, vector->data.value);
    vector->data = cu_Slice_none();
  }
  vector->length = 0;
  vector->capacity = 0;
}