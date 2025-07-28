#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Non-owning memory view. */
typedef struct cu_Slice {
  void *ptr;     /**< pointer to first element */
  size_t length; /**< number of bytes */
} cu_Slice;

/** Create a slice from a pointer and length without allocating. */
cu_Slice cu_Slice_create(void *ptr, size_t length);

#include <object/optional.h>
#include <object/result.h>

void cu_abort(void);
void cu_panic_handler(const char *format, ...);

void cu_Memory_memmove(void *dest, cu_Slice src);
void cu_Memory_memcpy(void *dest, cu_Slice src);
void cu_Memory_memset(void *dest, int value, size_t size);
bool cu_Memory_memcmp(cu_Slice a, cu_Slice b);

size_t cu_CString_length(const char *cstr);
int cu_CString_cmp(const char *a, const char *b);
int cu_CString_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int cu_CString_snprintf(char *buf, size_t size, const char *fmt, ...);
int cu_CString_vsprintf(char *buf, const char *fmt, va_list args);
int cu_CString_sprintf(char *buf, const char *fmt, ...);
unsigned long cu_CString_strtoul(const char *nptr, char **endptr, int base);

#ifdef __cplusplus
}
#endif
