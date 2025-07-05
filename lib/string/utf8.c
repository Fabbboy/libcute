#include "string/utf8.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/** \brief Helper to read a 32-bit value from an unaligned pointer. */
static inline uint32_t cu_utf8_read_uint32(const unsigned char *p) {
  uint32_t v = 0;
  memcpy(&v, p, sizeof(uint32_t));
  return v;
}

/** Decode one UTF-8 codepoint from a buffer. */
size_t cu_utf8_decode(const unsigned char *s, size_t len, uint32_t *codepoint) {
  if (len == 0) {
    return 0;
  }

  unsigned char c0 = s[0];
  if (c0 < 0x80) {
    if (codepoint) {
      *codepoint = c0;
    }
    return 1;
  } else if ((c0 & 0xe0) == 0xc0) {
    if (len < 2)
      return 0;
    if ((s[1] & 0xc0) != 0x80)
      return 0;
    if (codepoint) {
      *codepoint = ((c0 & 0x1f) << 6) | (s[1] & 0x3f);
    }
    return 2;
  } else if ((c0 & 0xf0) == 0xe0) {
    if (len < 3)
      return 0;
    if ((s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80)
      return 0;
    if (codepoint) {
      *codepoint = ((c0 & 0x0f) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
    }
    return 3;
  } else if ((c0 & 0xf8) == 0xf0) {
    if (len < 4)
      return 0;
    if ((s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 || (s[3] & 0xc0) != 0x80)
      return 0;
    if (codepoint) {
      *codepoint = ((c0 & 0x07) << 18) | ((s[1] & 0x3f) << 12) |
                   ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
    }
    return 4;
  }

  return 0;
}

/** Validate a UTF-8 byte sequence. */
bool cu_utf8_validate(const unsigned char *s, size_t len) {
  size_t i = 0;
  while (i < len) {
    size_t step = cu_utf8_decode(s + i, len - i, NULL);
    if (step == 0) {
      return false;
    }
    i += step;
  }
  return true;
}

/** Count the codepoints in a UTF-8 string. */
size_t cu_utf8_codepoint_count(const unsigned char *s, size_t len) {
  size_t i = 0;
  size_t count = 0;
  while (i < len) {
    size_t step = cu_utf8_decode(s + i, len - i, NULL);
    if (step == 0) {
      return 0;
    }
    i += step;
    count++;
  }
  return count;
}
