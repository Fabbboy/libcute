#include "memory/gpallocator.h"
#include "io/error.h"
#include "macro.h"
#include "memory/wasmallocator.h"
#include "utility.h"
#include <nostd.h>

/* Helper forward declarations */
static cu_IoSlice_Result cu_gpa_alloc(void *self, cu_Layout layout);
static cu_IoSlice_Result cu_gpa_grow(
    void *self, cu_Slice old_mem, cu_Layout new_layout);
static cu_IoSlice_Result cu_gpa_shrink(
    void *self, cu_Slice old_mem, cu_Layout new_layout);
static void cu_gpa_free(void *self, cu_Slice mem);
static void cu_gpa_destroy_bucket(
    cu_GPAllocator *gpa, struct cu_GPAllocator_BucketHeader *bucket);

/* -------------------------------------------------------------------------- */
/* Utility functions                                                          */
/* -------------------------------------------------------------------------- */

static size_t cu_gpa_calc_slot_count(cu_GPAllocator *gpa, size_t obj_size) {
  size_t max_data = gpa->bucketSize;
  if (max_data == 0) {
    max_data = CU_GPA_BUCKET_SIZE;
  }
  size_t count = max_data / obj_size;
  if (count == 0) {
    count = 1;
  }
  return count;
}

static struct cu_GPAllocator_BucketHeader *cu_gpa_create_bucket(
    cu_GPAllocator *gpa, size_t obj_size) {
  size_t slot_count = cu_gpa_calc_slot_count(gpa, obj_size);
  size_t total =
      sizeof(struct cu_GPAllocator_BucketHeader) + slot_count * obj_size;
  cu_IoSlice_Result mem = cu_Allocator_Alloc(
      gpa->backingAllocator, cu_Layout_create(total, obj_size));
  if (!cu_IoSlice_Result_is_ok(&mem)) {
    return NULL;
  }

  struct cu_GPAllocator_BucketHeader *bucket =
      (struct cu_GPAllocator_BucketHeader *)mem.value.ptr;
  bucket->prev = NULL;
  bucket->next = NULL;
  cu_Bitmap_Optional bits = cu_Bitmap_create(gpa->backingAllocator, slot_count);
  if (cu_Bitmap_Optional_is_none(&bits)) {
    cu_Allocator_Free(
        gpa->backingAllocator, cu_Slice_create(mem.value.ptr, total));
    return NULL;
  }
  bucket->objects.used = bits.value;
  bucket->objects.data = (unsigned char *)(bucket + 1);
  bucket->objects.objectSize = obj_size;
  bucket->objects.slotCount = slot_count;
  bucket->objects.usedCount = 0;
  bucket->canary = CU_GPA_CANARY;
  return bucket;
}

static int cu_gpa_index_from_size(size_t size) {
  int idx = 0;
  size_t s = 1;
  while (s < size && idx < CU_GPA_NUM_SMALL_BUCKETS) {
    s <<= 1;
    idx++;
  }
  if (idx < CU_GPA_NUM_SMALL_BUCKETS) {
    return idx;
  }
  return -1;
}

static struct cu_GPAllocator_BucketHeader *cu_gpa_find_bucket(
    cu_GPAllocator *gpa, void *ptr, size_t *slot_out) {
  for (int i = 0; i < CU_GPA_NUM_SMALL_BUCKETS; ++i) {
    struct cu_GPAllocator_BucketHeader *b = gpa->smallBuckets[i];
    while (b) {
      uintptr_t start = (uintptr_t)b->objects.data;
      uintptr_t end = start + b->objects.objectSize * b->objects.slotCount;
      if ((uintptr_t)ptr >= start && (uintptr_t)ptr < end) {
        if (slot_out) {
          *slot_out = ((uintptr_t)ptr - start) / b->objects.objectSize;
        }
        return b;
      }
      b = b->next;
    }
  }
  return NULL;
}

static struct cu_GPAllocator_LargeAlloc *cu_gpa_find_large(
    cu_GPAllocator *gpa, void *ptr) {
  struct cu_GPAllocator_LargeAlloc *cur = gpa->largeAllocs;
  while (cur) {
    if (cur->slice.ptr == ptr) {
      return cur;
    }
    cur = cur->next;
  }
  return NULL;
}

/* -------------------------------------------------------------------------- */
/* Allocation paths */
/* -------------------------------------------------------------------------- */

static cu_IoSlice_Result cu_gpa_alloc_small(cu_GPAllocator *gpa, size_t size,
    size_t alignment, size_t obj_size, int idx) {
  struct cu_GPAllocator_BucketHeader *bucket = gpa->smallBuckets[idx];
  if (!bucket || bucket->objects.usedCount == bucket->objects.slotCount) {
    bucket = cu_gpa_create_bucket(gpa, obj_size);
    if (!bucket) {
      cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
          .errnum = Size_Optional_none()};
      return cu_IoSlice_Result_error(err);
    }
    bucket->prev = gpa->smallBucketTails[idx];
    bucket->next = NULL;
    if (bucket->prev) {
      bucket->prev->next = bucket;
    } else {
      gpa->smallBuckets[idx] = bucket;
    }
    gpa->smallBucketTails[idx] = bucket;
  }

  size_t slot = 0;
  for (; slot < bucket->objects.slotCount; ++slot) {
    if (!cu_Bitmap_get(&bucket->objects.used, slot)) {
      break;
    }
  }
  if (slot == bucket->objects.slotCount) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }
  cu_Bitmap_set(&bucket->objects.used, slot);
  bucket->objects.usedCount++;
  void *ptr = bucket->objects.data + slot * bucket->objects.objectSize;
  CU_UNUSED(alignment);
  return cu_IoSlice_Result_ok(cu_Slice_create(ptr, size));
}

static cu_IoSlice_Result cu_gpa_alloc_large(
    cu_GPAllocator *gpa, size_t size, size_t alignment) {
  cu_IoSlice_Result mem = cu_Allocator_Alloc(
      gpa->backingAllocator, cu_Layout_create(size, alignment));
  if (!cu_IoSlice_Result_is_ok(&mem)) {
    return mem;
  }
  cu_IoSlice_Result meta_mem = cu_Allocator_Alloc(gpa->backingAllocator,
      cu_Layout_create(
          sizeof(struct cu_GPAllocator_LargeAlloc), sizeof(void *)));
  if (!cu_IoSlice_Result_is_ok(&meta_mem)) {
    cu_Allocator_Free(gpa->backingAllocator, mem.value);
    return meta_mem;
  }
  struct cu_GPAllocator_LargeAlloc *meta =
      (struct cu_GPAllocator_LargeAlloc *)meta_mem.value.ptr;
  meta->slice = mem.value;
  meta->prev = NULL;
  meta->next = gpa->largeAllocs;
  if (meta->next) {
    meta->next->prev = meta;
  }
  gpa->largeAllocs = meta;
  return cu_IoSlice_Result_ok(meta->slice);
}

static cu_IoSlice_Result cu_gpa_alloc(void *self, cu_Layout layout) {
  cu_GPAllocator *gpa = (cu_GPAllocator *)self;
  if (layout.elem_size == 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }
  size_t size = layout.elem_size;
  size_t alignment = layout.alignment;
  if (alignment == 0) {
    alignment = 1;
  }

  size_t need = CU_MAX(size, alignment);
  size_t obj_size = cu_next_pow2(need);

  int idx = cu_gpa_index_from_size(obj_size);
  if (idx < 0 || obj_size > gpa->bucketSize) {
    return cu_gpa_alloc_large(gpa, size, alignment);
  }
  return cu_gpa_alloc_small(gpa, size, alignment, obj_size, idx);
}

/* -------------------------------------------------------------------------- */
/* Resize and free */
/* -------------------------------------------------------------------------- */

static cu_IoSlice_Result cu_gpa_grow(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  cu_GPAllocator *gpa = (cu_GPAllocator *)self;
  CU_IF_NULL(old_mem.ptr) {
    return cu_gpa_alloc(self, new_layout);
  }

  size_t slot;
  struct cu_GPAllocator_BucketHeader *bucket =
      cu_gpa_find_bucket(gpa, old_mem.ptr, &slot);
  if (!bucket) {
    struct cu_GPAllocator_LargeAlloc *meta = cu_gpa_find_large(gpa, old_mem.ptr);
    if (!meta) {
      cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
          .errnum = Size_Optional_none()};
      return cu_IoSlice_Result_error(err);
    }
    cu_IoSlice_Result resized = cu_Allocator_Grow(
        gpa->backingAllocator, meta->slice,
        cu_Layout_create(new_layout.elem_size, new_layout.alignment));
    if (!cu_IoSlice_Result_is_ok(&resized)) {
      return resized;
    }
    meta->slice = resized.value;
    return resized;
  }

  if (new_layout.elem_size <= bucket->objects.objectSize) {
    return cu_IoSlice_Result_ok(
        cu_Slice_create(old_mem.ptr, new_layout.elem_size));
  }

  cu_IoSlice_Result new_mem = cu_gpa_alloc(self, new_layout);
  if (!cu_IoSlice_Result_is_ok(&new_mem)) {
    return new_mem;
  }
  cu_Memory_smemcpy(new_mem.value, old_mem);
  cu_gpa_free(self, old_mem);
  return new_mem;
}

static cu_IoSlice_Result cu_gpa_shrink(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  cu_GPAllocator *gpa = (cu_GPAllocator *)self;
  CU_IF_NULL(old_mem.ptr) {
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
        .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }

  size_t slot;
  struct cu_GPAllocator_BucketHeader *bucket =
      cu_gpa_find_bucket(gpa, old_mem.ptr, &slot);
  if (!bucket) {
    struct cu_GPAllocator_LargeAlloc *meta = cu_gpa_find_large(gpa, old_mem.ptr);
    if (!meta) {
      cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
          .errnum = Size_Optional_none()};
      return cu_IoSlice_Result_error(err);
    }
    cu_IoSlice_Result resized = cu_Allocator_Shrink(
        gpa->backingAllocator, meta->slice,
        cu_Layout_create(new_layout.elem_size, new_layout.alignment));
    if (!cu_IoSlice_Result_is_ok(&resized)) {
      return resized;
    }
    meta->slice = resized.value;
    return resized;
  }

  return cu_IoSlice_Result_ok(
      cu_Slice_create(old_mem.ptr, new_layout.elem_size));
}

static void cu_gpa_free_small(cu_GPAllocator *gpa,
    struct cu_GPAllocator_BucketHeader *bucket, size_t slot) {
  cu_Bitmap_clear(&bucket->objects.used, slot);
  if (bucket->objects.usedCount > 0) {
    bucket->objects.usedCount--;
  }
  if (bucket->objects.usedCount == 0) {
    int idx = cu_gpa_index_from_size(bucket->objects.objectSize);
    if (gpa->smallBuckets[idx] != bucket || bucket->next != NULL) {
      if (bucket->prev) {
        bucket->prev->next = bucket->next;
      } else {
        gpa->smallBuckets[idx] = bucket->next;
      }
      if (bucket->next) {
        bucket->next->prev = bucket->prev;
      } else {
        gpa->smallBucketTails[idx] = bucket->prev;
      }
      cu_gpa_destroy_bucket(gpa, bucket);
    }
  }
}

static void cu_gpa_free_large(
    cu_GPAllocator *gpa, struct cu_GPAllocator_LargeAlloc *meta) {
  if (meta->prev) {
    meta->prev->next = meta->next;
  } else {
    gpa->largeAllocs = meta->next;
  }
  if (meta->next) {
    meta->next->prev = meta->prev;
  }
  cu_Allocator_Free(gpa->backingAllocator, meta->slice);
  cu_Allocator_Free(gpa->backingAllocator,
      cu_Slice_create(meta, sizeof(struct cu_GPAllocator_LargeAlloc)));
}

static void cu_gpa_free(void *self, cu_Slice mem) {
  cu_GPAllocator *gpa = (cu_GPAllocator *)self;
  CU_IF_NULL(mem.ptr) { return; }
  size_t slot;
  struct cu_GPAllocator_BucketHeader *bucket =
      cu_gpa_find_bucket(gpa, mem.ptr, &slot);
  if (bucket) {
    cu_gpa_free_small(gpa, bucket, slot);
    return;
  }
  struct cu_GPAllocator_LargeAlloc *meta = cu_gpa_find_large(gpa, mem.ptr);
  if (meta) {
    cu_gpa_free_large(gpa, meta);
  }
}

/* -------------------------------------------------------------------------- */
/* Public API */
/* -------------------------------------------------------------------------- */

cu_Allocator cu_Allocator_GPAllocator(
    cu_GPAllocator *alloc, cu_GPAllocator_Config config) {
  if (cu_Allocator_Optional_is_some(&config.backingAllocator)) {
    alloc->backingAllocator = config.backingAllocator.value;
  } else {
#if CU_PLAT_WASM
    alloc->backingAllocator = cu_Allocator_WasmAllocator();
#elif !CU_FREESTANDING
    alloc->backingAllocator = cu_Allocator_CAllocator();
#else
    alloc->backingAllocator = cu_Allocator_NullAllocator();
#endif
  }
  alloc->bucketSize = config.bucketSize;
  if (alloc->bucketSize == 0) {
    alloc->bucketSize = CU_GPA_BUCKET_SIZE;
  }
  for (int i = 0; i < CU_GPA_NUM_SMALL_BUCKETS; ++i) {
    alloc->smallBuckets[i] = NULL;
    alloc->smallBucketTails[i] = NULL;
  }
  alloc->largeAllocs = NULL;

  cu_Allocator a = {0};
  a.self = alloc;
  a.allocFn = cu_gpa_alloc;
  a.growFn = cu_gpa_grow;
  a.shrinkFn = cu_gpa_shrink;
  a.freeFn = cu_gpa_free;
  return a;
}

static void cu_gpa_destroy_bucket(
    cu_GPAllocator *gpa, struct cu_GPAllocator_BucketHeader *bucket) {
  cu_Bitmap_destroy(&bucket->objects.used);
  cu_Allocator_Free(gpa->backingAllocator,
      cu_Slice_create(bucket, sizeof(*bucket) + bucket->objects.objectSize *
                                                    bucket->objects.slotCount));
}

void cu_GPAllocator_destroy(cu_GPAllocator *alloc) {
  for (int i = 0; i < CU_GPA_NUM_SMALL_BUCKETS; ++i) {
    struct cu_GPAllocator_BucketHeader *b = alloc->smallBuckets[i];
    while (b) {
      struct cu_GPAllocator_BucketHeader *next = b->next;
      cu_gpa_destroy_bucket(alloc, b);
      b = next;
    }
    alloc->smallBuckets[i] = NULL;
    alloc->smallBucketTails[i] = NULL;
  }
  struct cu_GPAllocator_LargeAlloc *la = alloc->largeAllocs;
  while (la) {
    struct cu_GPAllocator_LargeAlloc *next = la->next;
    cu_gpa_free_large(alloc, la);
    la = next;
  }
  alloc->largeAllocs = NULL;
}
