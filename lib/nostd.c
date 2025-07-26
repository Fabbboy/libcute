#include "nostd.h"
#include "macro.h"

void cu_memmove(void *dest, cu_Slice src) {
  unsigned char *d = (unsigned char *)dest;
  unsigned char *s = (unsigned char *)src.ptr;
  size_t n = src.length;

  CU_IF_NULL(d) return;
  CU_IF_NULL(s) return;

  if (d < s) {
    while (n--) {
      *d++ = *s++;
    }
  } else {
    d += n;
    s += n;
    while (n--) {
      *--d = *--s;
    }
  }
}

void cu_memcpy(void *dest, cu_Slice src) {
  unsigned char *d = (unsigned char *)dest;
  unsigned char *s = (unsigned char *)src.ptr;
  size_t n = src.length;

  CU_IF_NULL(d) return;
  CU_IF_NULL(s) return;

  while (n--) {
    *d++ = *s++;
  }
}

void cu_memset(void *dest, int value, size_t size) {
  unsigned char *d = (unsigned char *)dest;

  CU_IF_NULL(d) return;

  while (size--) {
    *d++ = (unsigned char)value;
  }
}

size_t cu_strlen(const char *str) {
  const char *s = str;

  CU_IF_NULL(s) return 0;

  while (*s) {
    s++;
  }
  return s - str;
}

bool cu_memcmp(cu_Slice a, cu_Slice b) {
  if (a.length != b.length)
    return false;

  unsigned char *p1 = (unsigned char *)a.ptr;
  unsigned char *p2 = (unsigned char *)b.ptr;

  CU_IF_NULL(p1) return false;
  CU_IF_NULL(p2) return false;

  while (a.length--) {
    if (*p1 != *p2) {
      return false;
    }
    p1++;
    p2++;
  }
  return true;
}

cu_Slice cu_Slice_create(void *ptr, size_t length) {
  cu_Slice slice;
  slice.ptr = ptr;
  slice.length = length;
  return slice;
}

CU_OPTIONAL_IMPL(cu_Slice, cu_Slice)
CU_RESULT_IMPL(cu_Slice, cu_Slice, cu_Io_Error)
