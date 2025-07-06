#include "string/string.h"
#include "macro.h"
#include <string.h>

CU_RESULT_IMPL(cu_String, cu_String, cu_String_Error)

cu_String cu_String_init(cu_Allocator allocator) {
  cu_String str;
  cu_Vector_Result res =
      cu_Vector_create(allocator, CU_LAYOUT(char), Size_some(0));
  str.vec = cu_Vector_result_unwrap(&res);
  return str;
}

static cu_String_Error cu_string_alloc(cu_String *str, size_t cap) {
  cu_Vector_Error_Optional err = cu_Vector_resize(&str->vec, cap + 1);
  if (cu_Vector_Error_is_some(&err)) {
    return CU_STRING_ERROR_OOM;
  }
  return CU_STRING_ERROR_NONE;
}

void cu_String_destroy(cu_String *str) { cu_Vector_destroy(&str->vec); }

cu_String_Result cu_String_from_cstr(cu_Allocator allocator, const char *cstr) {
  cu_String str = cu_String_init(allocator);
  size_t len = cstr ? strlen(cstr) : 0;
  if (cu_string_alloc(&str, len) != CU_STRING_ERROR_NONE) {
    return cu_String_result_error(CU_STRING_ERROR_OOM);
  }
  if (len > 0) {
    cu_Slice dst = cu_Slice_unwrap(&str.vec.data);
    memcpy(dst.ptr, cstr, len);
    ((char *)dst.ptr)[len] = '\0';
  } else {
    cu_Slice dst = cu_Slice_unwrap(&str.vec.data);
    ((char *)dst.ptr)[0] = '\0';
  }
  str.vec.length = len;
  return cu_String_result_ok(str);
}

cu_String_Result cu_String_from_slice(cu_Allocator allocator, cu_Slice slice) {
  cu_String str = cu_String_init(allocator);
  if (cu_string_alloc(&str, slice.length) != CU_STRING_ERROR_NONE) {
    return cu_String_result_error(CU_STRING_ERROR_OOM);
  }
  cu_Slice dst = cu_Slice_unwrap(&str.vec.data);
  if (slice.length > 0) {
    memcpy(dst.ptr, slice.ptr, slice.length);
  }
  ((char *)dst.ptr)[slice.length] = '\0';
  str.vec.length = slice.length;
  return cu_String_result_ok(str);
}

cu_String_Result cu_String_copy(cu_Allocator allocator, const cu_String *src) {
  return cu_String_from_slice(allocator, cu_String_as_slice(src));
}

cu_String_Error cu_String_reserve(cu_String *str, size_t capacity) {
  if (capacity <= cu_String_capacity(str)) {
    return CU_STRING_ERROR_NONE;
  }
  return cu_string_alloc(str, capacity);
}

cu_String_Error cu_String_append_slice(cu_String *str, cu_Slice slice) {
  size_t new_len = cu_String_length(str) + slice.length;
  if (new_len > cu_String_capacity(str)) {
    size_t new_cap = cu_String_capacity(str)
                        ? cu_String_capacity(str) * 2
                        : slice.length;
    if (new_cap < new_len) {
      new_cap = new_len;
    }
    cu_String_Error err = cu_string_alloc(str, new_cap);
    if (err != CU_STRING_ERROR_NONE) {
      return err;
    }
  }
  if (slice.length > 0) {
    cu_Slice dst = cu_Slice_unwrap(&str->vec.data);
    memcpy((char *)dst.ptr + cu_String_length(str), slice.ptr, slice.length);
    ((char *)dst.ptr)[new_len] = '\0';
  } else {
    cu_Slice dst = cu_Slice_unwrap(&str->vec.data);
    ((char *)dst.ptr)[new_len] = '\0';
  }
  str->vec.length = new_len;
  return CU_STRING_ERROR_NONE;
}

cu_String_Error cu_String_append_cstr(cu_String *str, const char *cstr) {
  size_t len = cstr ? strlen(cstr) : 0;
  return cu_String_append_slice(str, cu_Slice_create((void *)cstr, len));
}

cu_String_Error cu_String_append(cu_String *str, const cu_String *other) {
  return cu_String_append_slice(str, cu_String_as_slice(other));
}

cu_Slice cu_String_as_slice(const cu_String *str) {
  cu_Slice data = cu_Slice_unwrap(&str->vec.data);
  return cu_Slice_create(data.ptr, cu_String_length(str));
}

cu_String_Result cu_String_substring(
    const cu_String *str, size_t off, size_t len) {
  if (off > cu_String_length(str)) {
    return cu_String_from_slice(str->vec.allocator, cu_Slice_create(NULL, 0));
  }
  if (off + len > cu_String_length(str)) {
    len = cu_String_length(str) - off;
  }
  cu_Slice data = cu_Slice_unwrap(&str->vec.data);
  return cu_String_from_slice(
      str->vec.allocator, cu_Slice_create((char *)data.ptr + off, len));
}

cu_Slice cu_String_subslice(const cu_String *str, size_t off, size_t len) {
  if (off > cu_String_length(str)) {
    return cu_Slice_create(NULL, 0);
  }
  if (off + len > cu_String_length(str)) {
    len = cu_String_length(str) - off;
  }
  cu_Slice data = cu_Slice_unwrap(&str->vec.data);
  return cu_Slice_create((char *)data.ptr + off, len);
}
