/** @file vector.h Growable vector container. */
#pragma once

#include "macro.h"
#include "memory/allocator.h"
#include "nostd.h"
#include "object/optional.h"
#include "object/result.h"
#include "utility.h"
#include <stddef.h>

/**
 * @brief Growable array of elements.
 *
 * The vector manages a contiguous memory block whose layout is described by
 * ::cu_Layout. It automatically resizes when elements are inserted.
 */
typedef struct {
  cu_Slice_Optional data; /**< memory backing the vector */
  size_t length;          /**< number of valid elements */
  size_t capacity;        /**< allocated element capacity */
  cu_Layout layout;       /**< layout of each element */
  cu_Allocator allocator; /**< allocator used for storage */
} cu_Vector;

/**
 * @brief Error codes returned by vector operations.
 */
typedef enum {
  CU_VECTOR_ERROR_NONE = 0,       /**< success */
  CU_VECTOR_ERROR_OOM,            /**< out of memory */
  CU_VECTOR_ERROR_INVALID_LAYOUT, /**< invalid element layout */
  CU_VECTOR_ERROR_INVALID,        /**< invalid argument */
  CU_VECTOR_ERROR_OOB,            /**< out of bounds access */
} cu_Vector_Error;

CU_RESULT_DECL(cu_Vector, cu_Vector, cu_Vector_Error)
CU_OPTIONAL_DECL(cu_Vector_Error, cu_Vector_Error)

/** Create a new vector. */
cu_Vector_Result cu_Vector_create(
    cu_Allocator allocator, cu_Layout layout, Size_Optional initial_capacity);
/**
 * @brief Change the number of stored elements.
 *
 * The vector grows to at least @p size elements, reserving additional
 * capacity when required. When shrinking, elements are discarded but the
 * underlying buffer is retained.
 */
cu_Vector_Error_Optional cu_Vector_resize(cu_Vector *vector, size_t size);
/** Release resources owned by @p vector. */
void cu_Vector_destroy(cu_Vector *vector);

/** Current number of elements held by the vector. */
static inline size_t cu_Vector_size(const cu_Vector *vector) {
  CU_IF_NULL(vector) { return 0; }
  return vector->length;
}
/** Allocated capacity of the vector. */
static inline size_t cu_Vector_capacity(const cu_Vector *vector) {
  CU_IF_NULL(vector) { return 0; }
  return vector->capacity;
}

static inline cu_Slice_Optional cu_Vector_data(const cu_Vector *vector) {
  CU_IF_NULL(vector) { return cu_Slice_Optional_none(); }
  return vector->data;
}

/** Append a new element to the end of the vector. */
cu_Vector_Error_Optional cu_Vector_push_back(cu_Vector *vector, void *elem);
/** Remove the last element and copy it into @p out_elem. */
cu_Vector_Error_Optional cu_Vector_pop_back(cu_Vector *vector, void *out_elem);

cu_Vector_Error_Optional cu_Vector_push_front(cu_Vector *vector, void *elem);
cu_Vector_Error_Optional cu_Vector_pop_front(cu_Vector *vector, void *out_elem);

cu_Vector_Result cu_Vector_copy(const cu_Vector *src);

static inline bool cu_Vector_is_empty(const cu_Vector *vector) {
  CU_IF_NULL(vector) { return true; }
  return vector->length == 0;
}

/** Ensure at least @p capacity slots are allocated. */
cu_Vector_Error_Optional cu_Vector_reserve(cu_Vector *vector, size_t capacity);
/** Reduce capacity to the current size. */
cu_Vector_Error_Optional cu_Vector_shrink_to_fit(cu_Vector *vector);
/** Reset the vector. */
void cu_Vector_clear(cu_Vector *vector);

Ptr_Optional cu_Vector_at(const cu_Vector *vector, size_t index);

bool cu_Vector_iter(const cu_Vector *vector, size_t *index, void **out_elem);
