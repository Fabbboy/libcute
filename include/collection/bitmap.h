#pragma once

#include "memory/allocator.h"
#include "object/optional.h"
#include <stddef.h>

typedef struct {
  cu_Allocator backingAllocator; /**< allocator used for all bookkeeping */
  size_t *bits;
  size_t bitCount; /**< number of bits in the bitmap */
} cu_Bitmap;

CU_OPTIONAL_DECL(cu_Bitmap, cu_Bitmap)

cu_Bitmap_Optional cu_Bitmap_create(
    cu_Allocator backingAllocator, size_t bitCount);
void cu_Bitmap_destroy(cu_Bitmap *bitmap);
bool cu_Bitmap_get(const cu_Bitmap *bitmap, size_t index);
void cu_Bitmap_set(cu_Bitmap *bitmap, size_t index);
void cu_Bitmap_clear(cu_Bitmap *bitmap, size_t index);
void cu_Bitmap_clear_all(cu_Bitmap *bitmap);
static inline size_t cu_Bitmap_size(const cu_Bitmap *bitmap) {
  return bitmap->bitCount;
}
