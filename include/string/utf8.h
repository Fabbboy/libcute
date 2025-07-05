#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** \brief True if the codepoint represents an ASCII digit. */
#define CU_UTF8_IS_DIGIT(cp) ((cp) >= '0' && (cp) <= '9')
/** \brief True if the codepoint is an ASCII uppercase letter. */
#define CU_UTF8_IS_UPPER(cp) ((cp) >= 'A' && (cp) <= 'Z')
/** \brief True if the codepoint is an ASCII lowercase letter. */
#define CU_UTF8_IS_LOWER(cp) ((cp) >= 'a' && (cp) <= 'z')
/** \brief True if the codepoint is an ASCII alphabetic character. */
#define CU_UTF8_IS_ALPHA(cp) (CU_UTF8_IS_UPPER(cp) || CU_UTF8_IS_LOWER(cp))
/** \brief True if the codepoint is an ASCII alphanumeric character. */
#define CU_UTF8_IS_ALNUM(cp) (CU_UTF8_IS_ALPHA(cp) || CU_UTF8_IS_DIGIT(cp))
/** \brief True if the codepoint is an ASCII whitespace character. */
#define CU_UTF8_IS_SPACE(cp)                                                   \
  ((cp) == ' ' || (cp) == '\t' || (cp) == '\n' || (cp) == '\r' ||              \
      (cp) == '\f' || (cp) == '\v')

/** \brief Classification result for codepoints with no special category. */
#define CU_UTF8_CASE_OTHER 0x00u
/** \brief Classification result for ASCII digits. */
#define CU_UTF8_CASE_DIGIT 0x8fu
/** \brief Classification result for ASCII uppercase letters. */
#define CU_UTF8_CASE_UPPER 0x90u
/** \brief Classification result for ASCII lowercase letters. */
#define CU_UTF8_CASE_LOWER 0x91u
/** \brief Classification result for ASCII whitespace. */
#define CU_UTF8_CASE_SPACE 0x92u

/**
 * \brief Decode the next UTF-8 codepoint.
 *
 * Returns the number of bytes consumed or zero on failure.
 */
size_t cu_utf8_decode(const unsigned char *s, size_t len, uint32_t *codepoint);
/** \brief Validate a UTF-8 byte sequence. */
bool cu_utf8_validate(const unsigned char *s, size_t len);
/** \brief Count UTF-8 codepoints in a buffer. */
size_t cu_utf8_codepoint_count(const unsigned char *s, size_t len);
/** \brief Classify a codepoint into switch-friendly categories. */
uint8_t cu_utf8_case(uint32_t codepoint);
