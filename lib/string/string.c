#include "macro.h"
#include "string/string.h"
#include <nostd.h>

CU_RESULT_IMPL(cu_String, cu_String, cu_String_Error)

cu_String cu_String_init(cu_Allocator allocator) {
  cu_String str;
  str.allocator = allocator;
  str.data = NULL;
  str.length = 0;
  str.capacity = 0;
  return str;
}

static cu_String_Error cu_string_alloc(cu_String *str, size_t cap) {
  cu_Slice_Result mem;
  if (str->data == NULL) {
    mem = cu_Allocator_Alloc(str->allocator, cap + 1, 1);
  } else {
    mem = cu_Allocator_Resize(str->allocator,
        cu_Slice_create(str->data, str->capacity + 1), cap + 1, 1);
  }
  if (!cu_Slice_result_is_ok(&mem)) {
    return CU_STRING_ERROR_OOM;
  }
  str->data = mem.value.ptr;
  str->capacity = cap;
  return CU_STRING_ERROR_NONE;
}

void cu_String_destroy(cu_String *str) {
  if (str->data != NULL) {
    cu_Allocator_Free(
        str->allocator, cu_Slice_create(str->data, str->capacity + 1));
  }
  str->data = NULL;
  str->length = 0;
  str->capacity = 0;
}

void cu_String_clear(cu_String *str) {
  str->length = 0;
  if (str->data != NULL) {
    str->data[0] = '\0';
  }
}

cu_String_Result cu_String_from_cstr(cu_Allocator allocator, const char *cstr) {
  cu_String str = cu_String_init(allocator);
  size_t len = cstr ? cu_CString_length(cstr) : 0;
  if (cu_string_alloc(&str, len) != CU_STRING_ERROR_NONE) {
    return cu_String_result_error(CU_STRING_ERROR_OOM);
  }
  if (len > 0) {
    cu_Memory_memcpy(str.data, cu_Slice_create((void *)cstr, len));
  }
  str.data[len] = '\0';
  str.length = len;
  return cu_String_result_ok(str);
}

cu_String_Result cu_String_from_slice(cu_Allocator allocator, cu_Slice slice) {
  cu_String str = cu_String_init(allocator);
  if (cu_string_alloc(&str, slice.length) != CU_STRING_ERROR_NONE) {
    return cu_String_result_error(CU_STRING_ERROR_OOM);
  }
  if (slice.length > 0) {
    cu_Memory_memcpy(str.data, cu_Slice_create(slice.ptr, slice.length));
  }
  str.data[slice.length] = '\0';
  str.length = slice.length;
  return cu_String_result_ok(str);
}

cu_String_Result cu_String_copy(cu_Allocator allocator, const cu_String *src) {
  return cu_String_from_slice(allocator, cu_String_as_slice(src));
}

cu_String_Error cu_String_reserve(cu_String *str, size_t capacity) {
  if (capacity <= str->capacity) {
    return CU_STRING_ERROR_NONE;
  }
  return cu_string_alloc(str, capacity);
}

cu_String_Error cu_String_append_slice(cu_String *str, cu_Slice slice) {
  size_t new_len = str->length + slice.length;
  if (new_len > str->capacity) {
    size_t new_cap = str->capacity ? str->capacity * 2 : slice.length;
    if (new_cap < new_len) {
      new_cap = new_len;
    }
    cu_String_Error err = cu_string_alloc(str, new_cap);
    if (err != CU_STRING_ERROR_NONE) {
      return err;
    }
  }
  if (slice.length > 0) {
    cu_Memory_memcpy(str->data + str->length,
        cu_Slice_create(slice.ptr, slice.length));
  }
  str->length = new_len;
  str->data[new_len] = '\0';
  return CU_STRING_ERROR_NONE;
}

cu_String_Error cu_String_append_cstr(cu_String *str, const char *cstr) {
  size_t len = cstr ? cu_CString_length(cstr) : 0;
  return cu_String_append_slice(str, cu_Slice_create((void *)cstr, len));
}

cu_String_Error cu_String_append(cu_String *str, const cu_String *other) {
  return cu_String_append_slice(str, cu_String_as_slice(other));
}

cu_Slice cu_String_as_slice(const cu_String *str) {
  return cu_Slice_create(str->data, str->length);
}

cu_String_Result cu_String_substring(
    const cu_String *str, size_t off, size_t len) {
  if (off > str->length) {
    return cu_String_from_slice(str->allocator, cu_Slice_create(NULL, 0));
  }
  if (off + len > str->length) {
    len = str->length - off;
  }
  return cu_String_from_slice(
      str->allocator, cu_Slice_create(str->data + off, len));
}

cu_Slice cu_String_subslice(const cu_String *str, size_t off, size_t len) {
  if (off > str->length) {
    return cu_Slice_create(NULL, 0);
  }
  if (off + len > str->length) {
    len = str->length - off;
  }
  return cu_Slice_create(str->data + off, len);
}
