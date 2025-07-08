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
