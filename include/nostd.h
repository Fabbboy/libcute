#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <object/result.h>
#include <object/optional.h>
#include <io/error.h>
#ifdef CU_NO_STD
void cu_panic_handler(const char *format, ...);
#else
#define cu_panic_handler(format, ...)                                          \
  do {                                                                         \
    fprintf(stderr, "Panic: " format "\n", __VA_ARGS__);                       \
    exit(EXIT_FAILURE);                                                        \
  } while (0)
#endif

/** Non-owning memory view. */
typedef struct cu_Slice {
  void *ptr;     /**< pointer to first element */
  size_t length; /**< number of bytes */
} cu_Slice;

/** Create a slice from a pointer and length without allocating. */
cu_Slice cu_Slice_create(void *ptr, size_t length);

CU_OPTIONAL_DECL(cu_Slice, cu_Slice)
CU_RESULT_DECL(cu_Slice, cu_Slice, cu_Io_Error)

// NOTSTD is a header that works across all platforms it provides a very minimal
// replacement for nostd.h
// it is activly mixing libcute primitives like cu_Slice but also provides
// normal apis but we prefer libcute primitives
// we assume we only have access to stdbool.h, stddef.h and stdint.h

void cu_memmove(void *dest, cu_Slice src);
void cu_memcpy(void *dest, cu_Slice src);
void cu_memset(void *dest, int value, size_t size);
bool cu_memcmp(cu_Slice a, cu_Slice b);

size_t cu_strlen(const char *str);
