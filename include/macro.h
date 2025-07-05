#pragma once

#include <stdio.h>
#include <stdlib.h>

#define CU_IF_NULL(expr) if ((expr) == NULL)
#define CU_IF_NOT_NULL(expr) if ((expr) != NULL)
#define CU_DIE(msg)                                                            \
  do {                                                                         \
    fprintf(stderr, "Fatal error (%s:%d): %s\n", __FILE__, __LINE__, msg);     \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define CU_ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define CU_UNUSED(expr) (void)(expr)
#define CU_ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
#define CU_BIT(x) (1u << (x))

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
