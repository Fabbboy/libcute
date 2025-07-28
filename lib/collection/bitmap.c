#include "collection/bitmap.h"

CU_OPTIONAL_IMPL(cu_Bitmap, cu_Bitmap)

cu_Bitmap_Optional cu_Bitmap_create(
    cu_Allocator backingAllocator, size_t bitCount) {
  if (bitCount == 0) {
    return cu_Bitmap_Optional_none();
  }

  size_t size = (bitCount + sizeof(size_t) * 8 - 1) / (sizeof(size_t) * 8);
  cu_Slice_Result mem = cu_Allocator_Alloc(
      backingAllocator,
      cu_Layout_create(size * sizeof(size_t), sizeof(size_t)));
  if (!cu_Slice_result_is_ok(&mem)) {
    return cu_Bitmap_Optional_none();
  }

  cu_Bitmap bitmap;
  bitmap.backingAllocator = backingAllocator;
  bitmap.bits = (size_t *)mem.value.ptr;
  bitmap.bitCount = bitCount;
  for (size_t i = 0; i < size; ++i) {
    bitmap.bits[i] = 0;
  }
  return cu_Bitmap_Optional_some(bitmap);
}

void cu_Bitmap_destroy(cu_Bitmap *bitmap) {
  if (bitmap->bits) {
    size_t size =
        (bitmap->bitCount + sizeof(size_t) * 8 - 1) / (sizeof(size_t) * 8);
    cu_Allocator_Free(bitmap->backingAllocator,
        cu_Slice_create(bitmap->bits, size * sizeof(size_t)));
    bitmap->bits = NULL;
    bitmap->bitCount = 0;
  }
}

bool cu_Bitmap_get(const cu_Bitmap *bitmap, size_t index) {
  if (index >= bitmap->bitCount) {
    return false;
  }
  size_t bitIndex = index / (sizeof(size_t) * 8);
  size_t bitOffset = index % (sizeof(size_t) * 8);
  return (bitmap->bits[bitIndex] & ((size_t)1 << bitOffset)) != 0;
}

void cu_Bitmap_set(cu_Bitmap *bitmap, size_t index) {
  if (index >= bitmap->bitCount) {
    return;
  }
  size_t bitIndex = index / (sizeof(size_t) * 8);
  size_t bitOffset = index % (sizeof(size_t) * 8);
  bitmap->bits[bitIndex] |= ((size_t)1 << bitOffset);
}

void cu_Bitmap_clear(cu_Bitmap *bitmap, size_t index) {
  if (index >= bitmap->bitCount) {
    return;
  }
  size_t bitIndex = index / (sizeof(size_t) * 8);
  size_t bitOffset = index % (sizeof(size_t) * 8);
  bitmap->bits[bitIndex] &= ~((size_t)1 << bitOffset);
}

void cu_Bitmap_clear_all(cu_Bitmap *bitmap) {
  size_t size =
      (bitmap->bitCount + sizeof(size_t) * 8 - 1) / (sizeof(size_t) * 8);
  for (size_t i = 0; i < size; ++i) {
    bitmap->bits[i] = 0;
  }
}
