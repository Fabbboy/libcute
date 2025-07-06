#pragma once

#include "macro.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include "object/result.h"
#include "object/slice.h"
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
  CU_VECTOR_ERROR_NONE = 0,    /**< success */
  CU_VECTOR_ERROR_OOM,         /**< out of memory */
  CU_VECTOR_ERROR_INVALID_LAYOUT, /**< invalid element layout */
  CU_VECTOR_ERROR_INVALID,     /**< invalid argument */
  CU_VECTOR_ERROR_OOB,         /**< out of bounds access */
} cu_Vector_Error;

CU_RESULT_DECL(cu_Vector, cu_Vector, cu_Vector_Error)
CU_OPTIONAL_DECL(cu_Vector_Error, cu_Vector_Error)

/** Create a new vector. */
cu_Vector_Result cu_Vector_create(
    cu_Allocator allocator, cu_Layout layout, Size_Optional initial_capacity);
/** Adjust the capacity of the vector. */
cu_Vector_Error_Optional cu_Vector_resize(cu_Vector *vector, size_t capacity);
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

/** Append a new element to the end of the vector. */
cu_Vector_Error_Optional cu_Vector_push_back(cu_Vector *vector, void *elem);
/** Remove the last element and copy it into @p out_elem. */
cu_Vector_Error_Optional cu_Vector_pop_back(cu_Vector *vector, void *out_elem);
