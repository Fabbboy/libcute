#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "memory/allocator.h"
#include "object/result.h"
#include "object/slice.h"
#include "collection/vector.h"

/**
 * @brief Error codes returned by string operations.
 */
typedef enum {
  CU_STRING_ERROR_NONE = 0, /**< success */
  CU_STRING_ERROR_OOM,      /**< allocation failed */
} cu_String_Error;

/** Result type used by string constructors. */

/**
 * @brief Dynamically allocated UTF-8 string.
 */
typedef struct {
  cu_Vector vec; /**< underlying vector storing the characters */
} cu_String;

/** Get the underlying memory as an optional slice. */
static inline cu_Slice_Optional cu_String_data(const cu_String *str) {
  return cu_Vector_data(&str->vec);
}

static inline size_t cu_String_length(const cu_String *str) {
  return str->vec.length;
}

static inline size_t cu_String_capacity(const cu_String *str) {
  if (str->vec.capacity == 0) {
    return 0;
  }
  return str->vec.capacity - 1;
}
CU_RESULT_DECL(cu_String, cu_String, cu_String_Error)

/** Initialize an empty string using the given allocator. */
cu_String cu_String_init(cu_Allocator allocator);

/** Release resources owned by the string. */
void cu_String_destroy(cu_String *str);

/** Create a new string from a C string. */
cu_String_Result cu_String_from_cstr(cu_Allocator allocator, const char *cstr);

/** Create a new string from a byte slice. */
cu_String_Result cu_String_from_slice(cu_Allocator allocator, cu_Slice slice);

/** Create a copy of another string. */
cu_String_Result cu_String_copy(cu_Allocator allocator, const cu_String *src);

/** Ensure the buffer can hold at least the requested capacity. */
cu_String_Error cu_String_reserve(cu_String *str, size_t capacity);

/** Append the given slice to the string. */
cu_String_Error cu_String_append_slice(cu_String *str, cu_Slice slice);

/** Append a C string to the string. */
cu_String_Error cu_String_append_cstr(cu_String *str, const char *cstr);

/** Append another string. */
cu_String_Error cu_String_append(cu_String *str, const cu_String *other);

/** Get the contents as a byte slice. */
cu_Slice cu_String_as_slice(const cu_String *str);

/** Create a new owning substring from the given range. */
cu_String_Result cu_String_substring(
    const cu_String *str, size_t off, size_t len);

/** Create a non-owning slice from the given range. */
cu_Slice cu_String_subslice(const cu_String *str, size_t off, size_t len);
