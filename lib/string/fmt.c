#include "string/fmt.h"
#include <stdarg.h>
#include <nostd.h>

cu_StrBuilder cu_StrBuilder_init(cu_Allocator allocator) {
  cu_StrBuilder builder;
  builder.string = cu_String_init(allocator);
  return builder;
}

void cu_StrBuilder_destroy(cu_StrBuilder *builder) {
  cu_String_destroy(&builder->string);
}

void cu_StrBuilder_clear(cu_StrBuilder *builder) {
  cu_String_clear(&builder->string);
}

cu_String_Error cu_StrBuilder_appendf(
    cu_StrBuilder *builder, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char tmp[128];
  int needed = cu_CString_vsnprintf(tmp, sizeof(tmp), fmt, args);
  va_end(args);
  if (needed < 0) {
    return CU_STRING_ERROR_OOM;
  }
  size_t new_len = builder->string.length + (size_t)needed;
  cu_String_Error err = cu_String_reserve(&builder->string, new_len);
  if (err != CU_STRING_ERROR_NONE) {
    return err;
  }
  va_start(args, fmt);
  cu_CString_vsnprintf(
      builder->string.data + builder->string.length, needed + 1, fmt, args);
  va_end(args);
  builder->string.length = new_len;
  return CU_STRING_ERROR_NONE;
}

cu_String_Error cu_StrBuilder_append_slice(
    cu_StrBuilder *builder, cu_Slice slice) {
  return cu_String_append_slice(&builder->string, slice);
}

cu_String_Error cu_StrBuilder_append_cstr(
    cu_StrBuilder *builder, const char *cstr) {
  return cu_String_append_cstr(&builder->string, cstr);
}

cu_String_Error cu_StrBuilder_append(
    cu_StrBuilder *builder, const cu_String *str) {
  return cu_String_append(&builder->string, str);
}

cu_Slice cu_StrBuilder_as_slice(const cu_StrBuilder *builder) {
  return cu_String_as_slice(&builder->string);
}

cu_String_Result cu_StrBuilder_finalize(const cu_StrBuilder *builder) {
  return cu_String_copy(builder->string.allocator, &builder->string);
}
