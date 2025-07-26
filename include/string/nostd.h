#pragma once

#include <stddef.h>
#include "nostd.h"

#ifdef CU_NO_STD
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *dest, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
size_t strlen(const char *s);
#else
#include <string.h>
#endif
