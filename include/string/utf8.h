#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
