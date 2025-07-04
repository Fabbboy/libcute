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
