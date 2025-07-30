#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifndef CU_FREESTANDING
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if CU_PLAT_WINDOWS
// Microsoft doesn't define max_align_t in C, so we fake it
typedef struct {
    long long ll;
    long double ld;
} cu_max_align_t;
#else
#include <stddef.h>
typedef max_align_t cu_max_align_t;
#endif

/** Non-owning memory view. */
typedef struct cu_Slice {
  void *ptr;     /**< pointer to first element */
  size_t length; /**< number of bytes */
} cu_Slice;

/** Create a slice from a pointer and length without allocating. */
cu_Slice cu_Slice_create(void *ptr, size_t length);
#define CU_SLICE_CSTR(s) cu_Slice_create((void *)(s), cu_CString_length(s))

void cu_abort(void);
#ifndef CU_FREESTANDING
#define cu_panic_handler(...)                                                  \
  do {                                                                         \
    fprintf(stderr, "Panic: ");                                                \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
    cu_abort();                                                                \
  } while (0);
// intentionally NO else case if we are building for freestanding the user has
// to define their own panic handler like rusts #[panic_handler]
#endif

void cu_Memory_memmove(void *dest, cu_Slice src);
void cu_Memory_smemmove(cu_Slice dest, cu_Slice src);
void cu_Memory_memcpy(void *dest, cu_Slice src);
void cu_Memory_smemcpy(cu_Slice dest, cu_Slice src);
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
