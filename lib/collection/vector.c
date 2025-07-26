#include "collection/vector.h"
#include "macro.h"
#include "memory/allocator.h"
#include "nostd.h"
#include "object/optional.h"
#include "object/result.h"
#include "utility.h"
#include <nostd.h>
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
  cu_Slice_Optional data = cu_Slice_Optional_none();
  if (Size_Optional_is_some(&initial_capacity) &&
      Size_Optional_unwrap(&initial_capacity) > 0) {
    cap = Size_Optional_unwrap(&initial_capacity);
    cu_Slice_Result r =
        cu_Allocator_Alloc(allocator, cap * layout.elem_size, layout.alignment);
    if (!cu_Slice_result_is_ok(&r)) {
      return cu_Vector_result_error(CU_VECTOR_ERROR_OOM);
    }
    data = cu_Slice_Optional_some(r.value);
  }

  cu_Vector vector;
  vector.data = data;
  vector.length = 0;
  vector.capacity = cap;
  vector.layout = layout;
  vector.allocator = allocator;

  return cu_Vector_result_ok(vector);
}

static cu_Vector_Error_Optional cu_Vector_set_capacity(
    cu_Vector *vector, size_t capacity) {
  CU_IF_NULL(vector) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID);
  }

  CU_LAYOUT_CHECK(vector->layout) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID_LAYOUT);
  }

  if (capacity == vector->capacity) {
    return cu_Vector_Error_Optional_none();
  }

  if (capacity == 0) {
    cu_Vector_destroy(vector);
    return cu_Vector_Error_Optional_none();
  }

  if (capacity < vector->length) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID);
  }

  if (cu_Slice_Optional_is_some(&vector->data)) {
    cu_Slice old_data = cu_Slice_Optional_unwrap(&vector->data);
    cu_Slice_Result new_data = cu_Allocator_Resize(vector->allocator, old_data,
        capacity * vector->layout.elem_size, vector->layout.alignment);

    if (!cu_Slice_result_is_ok(&new_data)) {
      return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_OOM);
    }

    vector->data = cu_Slice_Optional_some(new_data.value);
    vector->capacity = capacity;
    return cu_Vector_Error_Optional_none();
  }

  cu_Slice_Result new_data_res = cu_Allocator_Alloc(vector->allocator,
      capacity * vector->layout.elem_size, vector->layout.alignment);
  if (!cu_Slice_result_is_ok(&new_data_res)) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_OOM);
  }
  vector->data = cu_Slice_Optional_some(new_data_res.value);
  vector->capacity = capacity;
  return cu_Vector_Error_Optional_none();
}

cu_Vector_Error_Optional cu_Vector_resize(cu_Vector *vector, size_t size) {
  CU_IF_NULL(vector) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID);
  }

  if (size > vector->capacity) {
    cu_Vector_Error_Optional err = cu_Vector_set_capacity(vector, size);
    if (cu_Vector_Error_Optional_is_some(&err)) {
      return err;
    }
  }

  vector->length = size;
  return cu_Vector_Error_Optional_none();
}

void cu_Vector_destroy(cu_Vector *vector) {
  if (cu_Slice_Optional_is_some(&vector->data)) {
    cu_Allocator_Free(vector->allocator, vector->data.value);
    vector->data = cu_Slice_Optional_none();
  }
  vector->length = 0;
  vector->capacity = 0;
}

cu_Vector_Error_Optional cu_Vector_push_back(cu_Vector *vector, void *elem) {
  CU_IF_NULL(vector) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID);
  }

  CU_LAYOUT_CHECK(vector->layout) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID_LAYOUT);
  }

  if (vector->length >= vector->capacity) {
    size_t new_cap = vector->capacity == 0 ? 1 : vector->capacity * 2;
    cu_Vector_Error_Optional err = cu_Vector_reserve(vector, new_cap);
    if (cu_Vector_Error_Optional_is_some(&err)) {
      return err;
    }
  }

  void *dest = (unsigned char *)vector->data.value.ptr +
               vector->length * vector->layout.elem_size;
  memcpy(dest, elem, vector->layout.elem_size);
  vector->length++;
  return cu_Vector_Error_Optional_none();
}

cu_Vector_Error_Optional cu_Vector_pop_back(cu_Vector *vector, void *out_elem) {
  CU_IF_NULL(vector) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID);
  }

  CU_LAYOUT_CHECK(vector->layout) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID_LAYOUT);
  }

  if (vector->length == 0) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_OOB);
  }

  vector->length--;
  void *src = (unsigned char *)vector->data.value.ptr +
              vector->length * vector->layout.elem_size;
  memcpy(out_elem, src, vector->layout.elem_size);

  if (vector->length == 0) {
    cu_Vector_shrink_to_fit(vector);
  }
  return cu_Vector_Error_Optional_none();
}

cu_Vector_Error_Optional cu_Vector_push_front(cu_Vector *vector, void *elem) {
  CU_IF_NULL(vector) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID);
  }

  CU_LAYOUT_CHECK(vector->layout) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID_LAYOUT);
  }

  if (vector->length >= vector->capacity) {
    size_t new_cap = vector->capacity == 0 ? 1 : vector->capacity * 2;
    cu_Vector_Error_Optional err = cu_Vector_reserve(vector, new_cap);
    if (cu_Vector_Error_Optional_is_some(&err)) {
      return err;
    }
  }

  void *dest =
      (unsigned char *)vector->data.value.ptr + vector->layout.elem_size;
  memmove(
      dest, vector->data.value.ptr, vector->length * vector->layout.elem_size);

  memcpy(vector->data.value.ptr, elem, vector->layout.elem_size);
  vector->length++;
  return cu_Vector_Error_Optional_none();
}

cu_Vector_Error_Optional cu_Vector_pop_front(
    cu_Vector *vector, void *out_elem) {
  CU_IF_NULL(vector) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID);
  }

  CU_LAYOUT_CHECK(vector->layout) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID_LAYOUT);
  }

  if (vector->length == 0) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_OOB);
  }

  void *src = vector->data.value.ptr;
  memcpy(out_elem, src, vector->layout.elem_size);

  void *dest =
      (unsigned char *)vector->data.value.ptr + vector->layout.elem_size;
  memmove(dest, src, (vector->length - 1) * vector->layout.elem_size);

  vector->length--;
  if (vector->length == 0) {
    cu_Vector_shrink_to_fit(vector);
  }
  return cu_Vector_Error_Optional_none();
}

cu_Vector_Result cu_Vector_copy(const cu_Vector *src) {
  CU_IF_NULL(src) { return cu_Vector_result_error(CU_VECTOR_ERROR_INVALID); }

  CU_LAYOUT_CHECK(src->layout) {
    return cu_Vector_result_error(CU_VECTOR_ERROR_INVALID_LAYOUT);
  }

  cu_Vector_Result result = cu_Vector_create(
      src->allocator, src->layout, Size_Optional_some(src->capacity));
  if (!cu_Vector_result_is_ok(&result)) {
    return result;
  }

  if (src->length > 0) {
    size_t size = src->length * src->layout.elem_size;
    memcpy(result.value.data.value.ptr, src->data.value.ptr, size);
    result.value.length = src->length;
  }

  return result;
}

cu_Vector_Error_Optional cu_Vector_reserve(cu_Vector *vector, size_t capacity) {
  CU_IF_NULL(vector) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID);
  }
  if (capacity <= vector->capacity) {
    return cu_Vector_Error_Optional_none();
  }
  return cu_Vector_set_capacity(vector, capacity);
}

cu_Vector_Error_Optional cu_Vector_shrink_to_fit(cu_Vector *vector) {
  CU_IF_NULL(vector) {
    return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_INVALID);
  }
  return cu_Vector_set_capacity(vector, vector->length);
}

void cu_Vector_clear(cu_Vector *vector) {
  CU_IF_NULL(vector) { return; }
  vector->length = 0;
  cu_Vector_shrink_to_fit(vector);
}

void *cu_Vector_at(const cu_Vector *vector, size_t index) {
  CU_IF_NULL(vector) { return NULL; }
  CU_LAYOUT_CHECK(vector->layout) { return NULL; }
  if (index >= vector->length) {
    return NULL;
  }
  return (unsigned char *)vector->data.value.ptr +
         index * vector->layout.elem_size;
}

bool cu_Vector_iter(const cu_Vector *vector, size_t *index, void **out_elem) {
  CU_IF_NULL(vector) { return false; }
  CU_IF_NULL(index) { return false; }
  CU_IF_NULL(out_elem) { return false; }

  CU_LAYOUT_CHECK(vector->layout) { return false; }

  if (*index >= vector->length) {
    return false;
  }

  *out_elem = (unsigned char *)vector->data.value.ptr +
              (*index) * vector->layout.elem_size;
  (*index)++;
  return true;
}
