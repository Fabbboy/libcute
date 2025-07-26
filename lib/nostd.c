#include "nostd.h"
#include "io/error.h"
#include "macro.h"
#include <stdarg.h>

void cu_Memory_memmove(void *dest, cu_Slice src) {
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

void cu_Memory_memcpy(void *dest, cu_Slice src) {
  unsigned char *d = (unsigned char *)dest;
  unsigned char *s = (unsigned char *)src.ptr;
  size_t n = src.length;

  CU_IF_NULL(d) return;
  CU_IF_NULL(s) return;

  while (n--) {
    *d++ = *s++;
  }
}

void cu_Memory_memset(void *dest, int value, size_t size) {
  unsigned char *d = (unsigned char *)dest;

  CU_IF_NULL(d) return;

  while (size--) {
    *d++ = (unsigned char)value;
  }
}

size_t cu_CString_length(const char *cstr) {
  const char *s = cstr;

  CU_IF_NULL(s) return 0;

  while (*s) {
    s++;
  }
  return s - cstr;
}

bool cu_Memory_memcmp(cu_Slice a, cu_Slice b) {
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

int cu_CString_vsnprintf(
    char *dst, size_t size, const char *fmt, va_list args) {
  size_t pos = 0;
  size_t total = 0;
  const char *p = fmt;
  while (*p) {
    if (*p == '%') {
      ++p;
      if (*p == 'd') {
        int val = va_arg(args, int);
        char numbuf[32];
        int blen = 0;
        bool neg = false;
        unsigned int u = (unsigned int)val;
        if (val < 0) {
          neg = true;
          u = (unsigned int)(-val);
        }
        char tmp[32];
        int t = 0;
        do {
          tmp[t++] = (char)('0' + (u % 10));
          u /= 10;
        } while (u > 0);
        if (neg)
          numbuf[blen++] = '-';
        while (t-- > 0)
          numbuf[blen++] = tmp[t];
        for (int i = 0; i < blen; ++i) {
          if (dst && pos + 1 < size)
            dst[pos] = numbuf[i];
          if (pos + 1 < SIZE_MAX)
            pos++;
          total++;
        }
      } else if (*p == 's') {
        const char *s = va_arg(args, const char *);
        size_t len = cu_CString_length(s);
        for (size_t i = 0; i < len; ++i) {
          if (dst && pos + 1 < size)
            dst[pos] = s[i];
          if (pos + 1 < SIZE_MAX)
            pos++;
          total++;
        }
      } else if (*p == '%') {
        if (dst && pos + 1 < size)
          dst[pos] = '%';
        if (pos + 1 < SIZE_MAX)
          pos++;
        total++;
      }
      ++p;
    } else {
      if (dst && pos + 1 < size)
        dst[pos] = *p;
      if (pos + 1 < SIZE_MAX)
        pos++;
      ++p;
      total++;
    }
  }
  if (size > 0) {
    if (pos >= size)
      pos = size - 1;
    if (dst)
      dst[pos] = '\0';
  }
  return (int)total;
}

int cu_CString_snprintf(char *dst, size_t size, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int r = cu_CString_vsnprintf(dst, size, fmt, args);
  va_end(args);
  return r;
}

int cu_CString_vsprintf(char *dst, const char *fmt, va_list args) {
  return cu_CString_vsnprintf(dst, SIZE_MAX, fmt, args);
}

int cu_CString_sprintf(char *dst, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int r = cu_CString_vsnprintf(dst, SIZE_MAX, fmt, args);
  va_end(args);
  return r;
}

void cu_abort(void) { __builtin_trap(); }

void cu_panic_handler(const char *format, ...) {
  (void)format;
  cu_abort();
}

unsigned long cu_CString_strtoul(const char *nptr, char **endptr, int base) {
  const char *s = nptr;
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f' ||
         *s == '\v')
    s++;

  bool neg = false;
  if (*s == '+') {
    s++;
  } else if (*s == '-') {
    neg = true;
    s++;
  }

  if (base == 0) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
      base = 16;
      s += 2;
    } else if (s[0] == '0') {
      base = 8;
      s += 1;
    } else {
      base = 10;
    }
  } else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
    s += 2;
  }

  unsigned long result = 0;
  while (*s) {
    char c = *s;
    int digit;
    if (c >= '0' && c <= '9') {
      digit = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      digit = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      digit = c - 'A' + 10;
    } else {
      break;
    }
    if (digit >= base)
      break;
    result = result * (unsigned long)base + (unsigned long)digit;
    s++;
  }

  if (endptr != NULL)
    *endptr = (char *)s;

  if (neg)
    return (unsigned long)(-(long)result);
  return result;
}

cu_Slice cu_Slice_create(void *ptr, size_t length) {
  cu_Slice slice;
  slice.ptr = ptr;
  slice.length = length;
  return slice;
}

CU_OPTIONAL_IMPL(cu_Slice, cu_Slice)
CU_RESULT_IMPL(cu_Slice, cu_Slice, cu_Io_Error)
