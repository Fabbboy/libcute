#include "nostd.h"
#include "macro.h"
#include "object/optional.h"
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

void cu_Memory_memmove(void *dest, cu_Slice src) {
  unsigned char *d = (unsigned char *)dest;
  unsigned char *s = (unsigned char *)src.ptr;
  size_t n = src.length;

  CU_IF_NULL(d) return;
  CU_IF_NULL(s) return;

  if (d < s) {
    // Forward copy when dest is before src
    while (n--) {
      *d++ = *s++;
    }
  } else {
    // Backward copy when dest overlaps with src
    d += n;
    s += n;
    while (n--) {
      *--d = *--s;
    }
  }
}

void cu_Memory_smemmove(cu_Slice dest, cu_Slice src) {
  unsigned char *d = (unsigned char *)dest.ptr;
  unsigned char *s = (unsigned char *)src.ptr;
  size_t n = src.length;

  CU_IF_NULL(d) return;
  CU_IF_NULL(s) return;

  if (d < s) {
    // Forward copy when dest is before src
    while (n--) {
      *d++ = *s++;
    }
  } else {
    // Backward copy when dest overlaps with src
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

void cu_Memory_smemcpy(cu_Slice dest, cu_Slice src) {
  unsigned char *d = (unsigned char *)dest.ptr;
  unsigned char *s = (unsigned char *)src.ptr;
  
  CU_IF_NULL(d) return;
  CU_IF_NULL(s) return;
  
  size_t n = CU_MIN(dest.length, src.length);
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
  return (size_t)(s - cstr);
}

int cu_CString_cmp(const char *a, const char *b) {
  const unsigned char *p1 = (const unsigned char *)a;
  const unsigned char *p2 = (const unsigned char *)b;

  CU_IF_NULL(p1) {
    if (p2) {
      return -1;
    }
    return 0;
  }
  CU_IF_NULL(p2) {
    if (p1) {
      return 1;
    }
    return 0;
  }

  while (*p1 && *p1 == *p2) {
    p1++;
    p2++;
  }
  return (int)(*p1) - (int)(*p2);
}

bool cu_Memory_memcmp(cu_Slice a, cu_Slice b) {
  if (a.length != b.length)
    return false;

  unsigned char *p1 = (unsigned char *)a.ptr;
  unsigned char *p2 = (unsigned char *)b.ptr;

  CU_IF_NULL(p1) return b.ptr == NULL; // Both NULL should be equal
  CU_IF_NULL(p2) return false;         // Only one NULL

  for (size_t i = 0; i < a.length; i++) {
    if (p1[i] != p2[i]) {
      return false;
    }
  }
  return true;
}

// Helper function to convert number to string
static int format_number(char *buf, size_t bufsize, unsigned long long num,
    bool is_signed, bool is_negative, int base, bool uppercase) {
  if (bufsize == 0)
    return 0;

  const char *digits;
  if (uppercase) {
    digits = "0123456789ABCDEF";
  } else {
    digits = "0123456789abcdef";
  }
  char temp[64]; // Enough for 64-bit number in any base
  int temp_pos = 0;
  int result_len = 0;

  // Handle zero specially
  if (num == 0) {
    temp[temp_pos++] = '0';
  } else {
    // Convert to string (reversed)
    while (num > 0) {
      temp[temp_pos++] = digits[num % base];
      num /= base;
    }
  }

  // Add sign if needed
  if (is_signed && is_negative) {
    temp[temp_pos++] = '-';
  }

  // Copy to buffer (reversing back to correct order)
  for (int i = temp_pos - 1; i >= 0 && result_len < (int)bufsize - 1; i--) {
    buf[result_len++] = temp[i];
  }

  buf[result_len] = '\0';
  return temp_pos; // Return total length needed (for truncation detection)
}

int cu_CString_vsnprintf(
    char *dst, size_t size, const char *fmt, va_list args) {
  bool measuring = dst == NULL;
  if (measuring) {
    dst = NULL;
    size = 0;
  } else if (size == 0) {
    return 0;
  }

  size_t pos = 0;
  size_t total = 0;
  const char *p = fmt;

  while (*p) {
    if (*p == '%') {
      ++p;

      // Parse format specifier
      bool is_long = false;
      bool is_long_long = false;
      bool uppercase = false;

      // Handle modifiers
      if (*p == 'l') {
        is_long = true;
        ++p;
        if (*p == 'l') {
          is_long_long = true;
          ++p;
        }
      }

      if (*p == 'd' || *p == 'i') {
        // Signed decimal integer
        unsigned long long num;
        bool negative = false;

        if (is_long_long) {
          long long val = va_arg(args, long long);
          if (val < 0) {
            negative = true;
            num = (unsigned long long)(-val);
          } else {
            num = (unsigned long long)val;
          }
        } else if (is_long) {
          long val = va_arg(args, long);
          if (val < 0) {
            negative = true;
            num = (unsigned long)(-val);
          } else {
            num = (unsigned long)val;
          }
        } else {
          int val = va_arg(args, int);
          if (val < 0) {
            negative = true;
            num = (unsigned int)(-val);
          } else {
            num = (unsigned int)val;
          }
        }

        char numbuf[64];
        int len = format_number(
            numbuf, sizeof(numbuf), num, true, negative, 10, false);

        for (int i = 0; numbuf[i]; i++) {
          if (!measuring && pos < size - 1)
            dst[pos] = numbuf[i];
          pos++;
        }
        total += len;

      } else if (*p == 'u') {
        // Unsigned decimal integer
        unsigned long long num;

        if (is_long_long) {
          num = va_arg(args, unsigned long long);
        } else if (is_long) {
          num = va_arg(args, unsigned long);
        } else {
          num = va_arg(args, unsigned int);
        }

        char numbuf[64];
        int len =
            format_number(numbuf, sizeof(numbuf), num, false, false, 10, false);

        for (int i = 0; numbuf[i]; i++) {
          if (!measuring && pos < size - 1)
            dst[pos] = numbuf[i];
          pos++;
        }
        total += len;

      } else if (*p == 'x' || *p == 'X') {
        // Hexadecimal integer
        uppercase = (*p == 'X');
        unsigned long long num;

        if (is_long_long) {
          num = va_arg(args, unsigned long long);
        } else if (is_long) {
          num = va_arg(args, unsigned long);
        } else {
          num = va_arg(args, unsigned int);
        }

        char numbuf[64];
        int len = format_number(
            numbuf, sizeof(numbuf), num, false, false, 16, uppercase);

        for (int i = 0; numbuf[i]; i++) {
          if (!measuring && pos < size - 1)
            dst[pos] = numbuf[i];
          pos++;
        }
        total += len;

      } else if (*p == 'c') {
        // Character
        int c = va_arg(args, int);
        if (!measuring && pos < size - 1)
          dst[pos] = (char)c;
        pos++;
        total++;

      } else if (*p == 's') {
        // String
        const char *s = va_arg(args, const char *);
        if (s == NULL) {
          s = "(null)";
        }

        while (*s) {
          if (!measuring && pos < size - 1)
            dst[pos] = *s;
          pos++;
          s++;
          total++;
        }

      } else if (*p == 'p') {
        // Pointer
        void *ptr = va_arg(args, void *);
        uintptr_t addr = (uintptr_t)ptr;

        // Add "0x" prefix
        if (!measuring && pos < size - 1)
          dst[pos] = '0';
        pos++;
        if (!measuring && pos < size - 1)
          dst[pos] = 'x';
        pos++;
        total += 2;

        char numbuf[64];
        int len = format_number(numbuf, sizeof(numbuf),
            (unsigned long long)addr, false, false, 16, false);

        for (int i = 0; numbuf[i]; i++) {
          if (!measuring && pos < size - 1)
            dst[pos] = numbuf[i];
          pos++;
        }
        total += len;

      } else if (*p == '%') {
        // Literal percent
        if (!measuring && pos < size - 1)
          dst[pos] = '%';
        pos++;
        total++;

      } else {
        // Unknown format specifier - treat literally
        if (!measuring && pos < size - 1)
          dst[pos] = '%';
        pos++;
        if (*p) {
          if (!measuring && pos < size - 1)
            dst[pos] = *p;
          pos++;
        }
        total += 2;
      }

      if (*p)
        ++p;

    } else {
      // Regular character
      if (!measuring && pos < size - 1)
        dst[pos] = *p;
      pos++;
      ++p;
      total++;
    }
  }

  if (!measuring && size > 0) {
    size_t idx;
    if (pos < size) {
      idx = pos;
    } else {
      idx = size - 1;
    }
    dst[idx] = '\0';
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

unsigned long cu_CString_strtoul(const char *nptr, char **endptr, int base) {
  const char *s = nptr;

  CU_IF_NULL(s) {
    if (endptr)
      *endptr = (char *)nptr;
    return 0;
  }

  // Skip whitespace
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f' ||
         *s == '\v') {
    s++;
  }

  bool neg = false;
  if (*s == '+') {
    s++;
  } else if (*s == '-') {
    neg = true;
    s++;
  }

  // Determine base
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

  // Validate base
  if (base < 2 || base > 36) {
    if (endptr)
      *endptr = (char *)nptr;
    return 0;
  }

  unsigned long result = 0;
  const char *start = s;
  const unsigned long cutoff = ULONG_MAX / (unsigned long)base;
  const int cutlim = (int)(ULONG_MAX % (unsigned long)base);

  while (*s) {
    char c = *s;
    int digit;

    if (c >= '0' && c <= '9') {
      digit = c - '0';
    } else if (c >= 'a' && c <= 'z') {
      digit = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'Z') {
      digit = c - 'A' + 10;
    } else {
      break;
    }

    if (digit >= base) {
      break;
    }

    // Check for overflow
    if (result > cutoff || (result == cutoff && digit > cutlim)) {
      result = ULONG_MAX;
      // Continue parsing to find the end
      s++;
      while (*s) {
        c = *s;
        if ((c >= '0' && c <= '9' && (c - '0') < base) ||
            (c >= 'a' && c <= 'z' && (c - 'a' + 10) < base) ||
            (c >= 'A' && c <= 'Z' && (c - 'A' + 10) < base)) {
          s++;
        } else {
          break;
        }
      }
      break;
    }

    result = result * (unsigned long)base + (unsigned long)digit;
    s++;
  }

  // If no digits were processed, return 0
  if (s == start) {
    if (endptr)
      *endptr = (char *)nptr;
    return 0;
  }

  if (endptr) {
    *endptr = (char *)s;
  }

  if (neg) {
    return (unsigned long)(-(long)result);
  }
  return result;
}

cu_Slice cu_Slice_create(void *ptr, size_t length) {
  cu_Slice slice;
  slice.ptr = ptr;
  slice.length = length;
  return slice;
}

CU_OPTIONAL_IMPL(cu_Slice, cu_Slice)
