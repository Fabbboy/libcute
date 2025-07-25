#pragma once

/** @file macro.h Common utility macros. */

#include <stdio.h>
#include <stdlib.h>

/** Execute the following block if @p expr is NULL. */
#define CU_IF_NULL(expr) if ((expr) == NULL)
/** Execute the following block if @p expr is not NULL. */
#define CU_IF_NOT_NULL(expr) if ((expr) != NULL)
/** Abort the program with an error message. */
#define CU_DIE(msg)                                                            \
  do {                                                                         \
    fprintf(stderr, "Fatal error (%s:%d): %s\n", __FILE__, __LINE__, msg);     \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

/** Round @p x up to the nearest multiple of @p align. */
#define CU_ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
/** Silence unused variable warnings. */
#define CU_UNUSED(expr) (void)(expr)
/** Array element count. */
#define CU_ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
/** Create a bit mask with bit @p x set. */
#define CU_BIT(x) (1u << (x))
/** Platform detection macros */
#if defined(_WIN32) || defined(_WIN64)
#define CU_PLAT_WINDOWS 1
#else
#define CU_PLAT_WINDOWS 0
#endif

#if defined(__APPLE__)
#define CU_PLAT_MACOS 1
#else
#define CU_PLAT_MACOS 0
#endif

#if defined(__linux__)
#define CU_PLAT_LINUX 1
#else
#define CU_PLAT_LINUX 0
#endif

/* WebAssembly platform detection */
#if defined(__EMSCRIPTEN__) || defined(__wasm__) || defined(__wasm32__)
#define CU_PLAT_WASM 1
#else
#define CU_PLAT_WASM 0
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define CU_PLAT_BSD 1
#else
#define CU_PLAT_BSD 0
#endif

#if CU_PLAT_MACOS || CU_PLAT_LINUX || CU_PLAT_BSD
#define CU_PLAT_POSIX 1
#else
#define CU_PLAT_POSIX 0
#endif

#define UNREACHABLE(msg)                                                       \
  do {                                                                         \
    fprintf(stderr, "Unreachable code reached (%s:%d): %s\n", __FILE__,        \
        __LINE__, msg);                                                        \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define TODO(msg)                                                              \
  do {                                                                         \
    fprintf(stderr, "TODO (%s:%d): %s\n", __FILE__, __LINE__, msg);            \
    exit(EXIT_FAILURE);                                                        \
  } while (0)
