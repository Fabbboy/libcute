#include "collection/vector.h"
#include "macro.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include "object/result.h"
#include "utility.h"
#include <nostd.h>
#include <stdalign.h>
#include <stddef.h>

CU_RESULT_IMPL(cu_Vector, cu_Vector, cu_Vector_Error)
CU_OPTIONAL_IMPL(cu_Vector_Error, cu_Vector_Error)

static void cu_Vector_maybe_shrink(cu_Vector *vector);

#define CU_VECTOR_GROW_FACTOR 2
#define CU_VECTOR_SHRINK_DIV 4

cu_Vector_Result cu_Vector_create(cu_Allocator allocator, cu_Layout layout,
    Size_Optional initial_capacity, cu_Destructor_Optional destructor) {
  CU_LAYOUT_CHECK(layout) {
    return cu_Vector_Result_error(CU_VECTOR_ERROR_INVALID_LAYOUT);
  }

  size_t cap = 0;
  cu_Slice_Optional data = cu_Slice_Optional_none();
  if (Size_Optional_is_some(&initial_capacity) &&
      Size_Optional_unwrap(&initial_capacity) > 0) {
    cap = Size_Optional_unwrap(&initial_capacity);
    cu_IoSlice_Result r = cu_Allocator_Alloc(
        allocator, cu_Layout_create(cap * layout.elem_size, layout.alignment));
    if (!cu_IoSlice_Result_is_ok(&r)) {
      return cu_Vector_Result_error(CU_VECTOR_ERROR_OOM);
    }
    data = cu_Slice_Optional_some(r.value);
  }

  cu_Vector vector = {0};
  vector.data = data;
  vector.length = 0;
  vector.capacity = cap;
  vector.layout = layout;
  vector.allocator = allocator;
  vector.destructor = destructor;

  return cu_Vector_Result_ok(vector);
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
    cu_Slice old_slice = cu_Slice_create(
        old_data.ptr, vector->capacity * vector->layout.elem_size);
    cu_Layout new_layout = cu_Layout_create(
        capacity * vector->layout.elem_size, vector->layout.alignment);
    cu_IoSlice_Result new_data;
    if (new_layout.elem_size > old_slice.length) {
      new_data = cu_Allocator_Grow(vector->allocator, old_slice, new_layout);
    } else {
      new_data = cu_Allocator_Shrink(vector->allocator, old_slice, new_layout);
    }

    if (!cu_IoSlice_Result_is_ok(&new_data)) {
      return cu_Vector_Error_Optional_some(CU_VECTOR_ERROR_OOM);
    }

    vector->data = cu_Slice_Optional_some(new_data.value);
    vector->capacity = capacity;
    return cu_Vector_Error_Optional_none();
  }

  cu_IoSlice_Result new_data_res = cu_Allocator_Alloc(
      vector->allocator, cu_Layout_create(capacity * vector->layout.elem_size,
                             vector->layout.alignment));
  if (!cu_IoSlice_Result_is_ok(&new_data_res)) {
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
  if (size < vector->length &&
      cu_Destructor_Optional_is_some(&vector->destructor)) {
    cu_Destructor dtor = cu_Destructor_Optional_unwrap(&vector->destructor);
    for (size_t i = size; i < vector->length; ++i) {
      void *ptr = (unsigned char *)vector->data.value.ptr +
                  i * vector->layout.elem_size;
      dtor(ptr);
    }
  }

  vector->length = size;
  return cu_Vector_Error_Optional_none();
}

void cu_Vector_destroy(cu_Vector *vector) {
  if (cu_Slice_Optional_is_some(&vector->data)) {
    if (cu_Destructor_Optional_is_some(&vector->destructor)) {
      cu_Destructor dtor = cu_Destructor_Optional_unwrap(&vector->destructor);
      for (size_t i = 0; i < vector->length; ++i) {
        void *ptr = (unsigned char *)vector->data.value.ptr +
                    i * vector->layout.elem_size;
        dtor(ptr);
      }
    }
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
    size_t new_cap;
    if (vector->capacity == 0) {
      new_cap = 1;
    } else {
      new_cap = vector->capacity * CU_VECTOR_GROW_FACTOR;
    }
    cu_Vector_Error_Optional err = cu_Vector_reserve(vector, new_cap);
    if (cu_Vector_Error_Optional_is_some(&err)) {
      return err;
    }
  }

  void *dest = (unsigned char *)vector->data.value.ptr +
               vector->length * vector->layout.elem_size;
  cu_Memory_memcpy(
      dest, cu_Slice_create((void *)elem, vector->layout.elem_size));
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
  cu_Memory_memcpy(out_elem, cu_Slice_create(src, vector->layout.elem_size));
  if (cu_Destructor_Optional_is_some(&vector->destructor)) {
    cu_Destructor dtor = cu_Destructor_Optional_unwrap(&vector->destructor);
    dtor(src);
  }

  cu_Vector_maybe_shrink(vector);
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
    size_t new_cap;
    if (vector->capacity == 0) {
      new_cap = 1;
    } else {
      new_cap = vector->capacity * CU_VECTOR_GROW_FACTOR;
    }
    cu_Vector_Error_Optional err = cu_Vector_reserve(vector, new_cap);
    if (cu_Vector_Error_Optional_is_some(&err)) {
      return err;
    }
  }

  void *dest =
      (unsigned char *)vector->data.value.ptr + vector->layout.elem_size;
  cu_Memory_memmove(dest, cu_Slice_create(vector->data.value.ptr,
                              vector->length * vector->layout.elem_size));

  cu_Memory_memcpy(vector->data.value.ptr,
      cu_Slice_create((void *)elem, vector->layout.elem_size));
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
  cu_Memory_memcpy(out_elem, cu_Slice_create(src, vector->layout.elem_size));
  if (cu_Destructor_Optional_is_some(&vector->destructor)) {
    cu_Destructor dtor = cu_Destructor_Optional_unwrap(&vector->destructor);
    dtor(src);
  }

  void *dest =
      (unsigned char *)vector->data.value.ptr + vector->layout.elem_size;
  cu_Memory_memmove(dest,
      cu_Slice_create(src, (vector->length - 1) * vector->layout.elem_size));

  vector->length--;
  cu_Vector_maybe_shrink(vector);
  return cu_Vector_Error_Optional_none();
}

cu_Vector_Result cu_Vector_copy(const cu_Vector *src) {
  CU_IF_NULL(src) { return cu_Vector_Result_error(CU_VECTOR_ERROR_INVALID); }

  CU_LAYOUT_CHECK(src->layout) {
    return cu_Vector_Result_error(CU_VECTOR_ERROR_INVALID_LAYOUT);
  }

  cu_Vector_Result result = cu_Vector_create(src->allocator, src->layout,
      Size_Optional_some(src->capacity), src->destructor);
  if (!cu_Vector_Result_is_ok(&result)) {
    return result;
  }

  if (src->length > 0) {
    size_t size = src->length * src->layout.elem_size;
    cu_Memory_memcpy(result.value.data.value.ptr,
        cu_Slice_create(src->data.value.ptr, size));
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

static void cu_Vector_maybe_shrink(cu_Vector *vector) {
  if (vector == NULL) {
    return;
  }
  if (vector->length == 0) {
    cu_Vector_shrink_to_fit(vector);
    return;
  }
  if (vector->capacity > 1 &&
      vector->length <= vector->capacity / CU_VECTOR_SHRINK_DIV) {
    size_t new_cap = CU_MAX(vector->capacity / 2, 1u);
    cu_Vector_set_capacity(vector, new_cap);
  }
}

void cu_Vector_clear(cu_Vector *vector) {
  CU_IF_NULL(vector) { return; }
  if (cu_Destructor_Optional_is_some(&vector->destructor)) {
    cu_Destructor dtor = cu_Destructor_Optional_unwrap(&vector->destructor);
    for (size_t i = 0; i < vector->length; ++i) {
      void *ptr = (unsigned char *)vector->data.value.ptr +
                  i * vector->layout.elem_size;
      dtor(ptr);
    }
  }
  vector->length = 0;
  cu_Vector_maybe_shrink(vector);
}

Ptr_Optional cu_Vector_at(const cu_Vector *vector, size_t index) {
  CU_IF_NULL(vector) { return Ptr_Optional_none(); }
  CU_LAYOUT_CHECK(vector->layout) { return Ptr_Optional_none(); }
  if (index >= vector->length) {
    return Ptr_Optional_none();
  }
  return Ptr_Optional_some((unsigned char *)vector->data.value.ptr +
                           index * vector->layout.elem_size);
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

cu_Slice_Optional cu_Vector_slice(const cu_Vector *vector) {
  CU_IF_NULL(vector) { return cu_Slice_Optional_none(); }
  if (!cu_Slice_Optional_is_some(&vector->data)) {
    return cu_Slice_Optional_none();
  }
  return cu_Slice_Optional_some(cu_Slice_create(vector->data.value.ptr,
      vector->length * vector->layout.elem_size));
}

cu_Slice_Optional cu_Vector_subslice(const cu_Vector *vector, size_t index,
    size_t count) {
  CU_IF_NULL(vector) { return cu_Slice_Optional_none(); }
  if (!cu_Slice_Optional_is_some(&vector->data) || index > vector->length) {
    return cu_Slice_Optional_none();
  }
  if (index + count > vector->length) {
    count = vector->length - index;
  }
  return cu_Slice_Optional_some(cu_Slice_create(
      (unsigned char *)vector->data.value.ptr +
          index * vector->layout.elem_size,
      count * vector->layout.elem_size));
}
