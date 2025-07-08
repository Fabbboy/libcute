#pragma once

#include "string.h"
#include <stdarg.h>

/** Simple formatting buffer built on top of cu_String. */
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
