#include "string/fmt.h"
#include <stdarg.h>
#include <stdio.h>

cu_FmtBuffer cu_FmtBuffer_init(cu_Allocator allocator) {
  cu_FmtBuffer buf;
  buf.string = cu_String_init(allocator);
  return buf;
}

void cu_FmtBuffer_destroy(cu_FmtBuffer *buf) {
  cu_String_destroy(&buf->string);
}

void cu_FmtBuffer_clear(cu_FmtBuffer *buf) { cu_String_clear(&buf->string); }

cu_String_Error cu_FmtBuffer_appendf(cu_FmtBuffer *buf, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int needed = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  if (needed < 0) {
    return CU_STRING_ERROR_OOM;
  }
  size_t new_len = buf->string.length + (size_t)needed;
  cu_String_Error err = cu_String_reserve(&buf->string, new_len);
  if (err != CU_STRING_ERROR_NONE) {
    return err;
  }
  va_start(args, fmt);
  vsnprintf(buf->string.data + buf->string.length, needed + 1, fmt, args);
  va_end(args);
  buf->string.length = new_len;
  return CU_STRING_ERROR_NONE;
}

cu_String_Error cu_FmtBuffer_append_slice(cu_FmtBuffer *buf, cu_Slice slice) {
  return cu_String_append_slice(&buf->string, slice);
}

cu_String_Error cu_FmtBuffer_append_cstr(cu_FmtBuffer *buf, const char *cstr) {
  return cu_String_append_cstr(&buf->string, cstr);
}

cu_String_Error cu_FmtBuffer_append(cu_FmtBuffer *buf, const cu_String *str) {
  return cu_String_append(&buf->string, str);
}

cu_Slice cu_FmtBuffer_as_slice(const cu_FmtBuffer *buf) {
  return cu_String_as_slice(&buf->string);
}

const char *cu_FmtBuffer_cstr(const cu_FmtBuffer *buf) { return buf->string.data; }

cu_String cu_FmtBuffer_into_string(cu_FmtBuffer *buf) {
  cu_String out = buf->string;
  buf->string = cu_String_init(out.allocator);
  return out;
}
