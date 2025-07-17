#pragma once

#include "string.h"
#include <stdarg.h>

/** Simple formatting buffer built on top of ::cu_String. */
typedef struct {
  cu_String string;
} cu_FmtBuffer;

/** Initialize a formatting buffer using the given allocator. */
cu_FmtBuffer cu_FmtBuffer_init(cu_Allocator allocator);

/** Release resources owned by the buffer. */
void cu_FmtBuffer_destroy(cu_FmtBuffer *buf);

/** Clear the buffer contents without releasing memory. */
void cu_FmtBuffer_clear(cu_FmtBuffer *buf);

/**
 * @brief Append formatted data to the buffer.
 *
 * Accepts printf-style format specifiers.
 */
cu_String_Error cu_FmtBuffer_appendf(cu_FmtBuffer *buf, const char *fmt, ...);

/** Append a slice to the buffer. */
cu_String_Error cu_FmtBuffer_append_slice(cu_FmtBuffer *buf, cu_Slice slice);

/** Append a C string to the buffer. */
cu_String_Error cu_FmtBuffer_append_cstr(cu_FmtBuffer *buf, const char *cstr);

/** Append another string. */
cu_String_Error cu_FmtBuffer_append(cu_FmtBuffer *buf, const cu_String *str);

/** View the buffer contents as a slice. */
cu_Slice cu_FmtBuffer_as_slice(const cu_FmtBuffer *buf);

/** View the buffer contents as a C string. */
const char *cu_FmtBuffer_cstr(const cu_FmtBuffer *buf);

/** Finalize the buffer and move out the underlying string. */
cu_String cu_FmtBuffer_into_string(cu_FmtBuffer *buf);
