#pragma once

#include "string.h"
#include <stdarg.h>

/** String builder for efficient string construction and formatting. */
typedef struct {
  cu_String string;
} cu_StrBuilder;

/** Initialize a string builder using the given allocator. */
cu_StrBuilder cu_StrBuilder_init(cu_Allocator allocator);

/** Release resources owned by the string builder. */
void cu_StrBuilder_destroy(cu_StrBuilder *builder);

/** Clear the string builder contents without releasing memory. */
void cu_StrBuilder_clear(cu_StrBuilder *builder);

/**
 * @brief Append formatted data to the string builder.
 *
 * Accepts printf-style format specifiers.
 */
cu_String_Error cu_StrBuilder_appendf(
    cu_StrBuilder *builder, const char *fmt, ...);

/** Append a slice to the string builder. */
cu_String_Error cu_StrBuilder_append_slice(
    cu_StrBuilder *builder, cu_Slice slice);

/** Append a C string to the string builder. */
cu_String_Error cu_StrBuilder_append_cstr(
    cu_StrBuilder *builder, const char *cstr);

/** Append another string to the string builder. */
cu_String_Error cu_StrBuilder_append(
    cu_StrBuilder *builder, const cu_String *str);

/** View the string builder contents as a slice. */
cu_Slice cu_StrBuilder_as_slice(const cu_StrBuilder *builder);

/** Finalize the string builder and create a new string with the contents. */
cu_String_Result cu_StrBuilder_finalize(const cu_StrBuilder *builder);
