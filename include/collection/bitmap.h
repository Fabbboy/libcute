#pragma once

#include "memory/allocator.h"
#include "object/optional.h"
#include <stddef.h>

/**
 * @brief Dynamically allocated bitmap.
 *
 * The bitmap stores its bits on the heap using the provided allocator. Bits
 * are referenced by index starting at zero. Out‑of‑range accesses are ignored
 * to keep the API simple. The number of bits is specified during creation and
 * can be queried with ::cu_Bitmap_size.
 */
typedef struct {
  cu_Allocator backingAllocator; /**< allocator used for bookkeeping */
  size_t *bits;                  /**< bit storage */
  size_t bitCount;               /**< total bits contained */
} cu_Bitmap;

CU_OPTIONAL_DECL(cu_Bitmap, cu_Bitmap)

/** Create a bitmap with the given number of bits. */
cu_Bitmap_Optional cu_Bitmap_create(
    cu_Allocator backingAllocator, size_t bitCount);
/** Release all memory owned by @p bitmap. */
void cu_Bitmap_destroy(cu_Bitmap *bitmap);
/** Query a single bit. */
bool cu_Bitmap_get(const cu_Bitmap *bitmap, size_t index);
/** Set a single bit. */
void cu_Bitmap_set(cu_Bitmap *bitmap, size_t index);
/** Clear a single bit. */
void cu_Bitmap_clear(cu_Bitmap *bitmap, size_t index);
/** Clear all bits in the bitmap. */
void cu_Bitmap_clear_all(cu_Bitmap *bitmap);
/** Number of bits held by the bitmap. */
static inline size_t cu_Bitmap_size(const cu_Bitmap *bitmap) {
  return bitmap->bitCount;
}
