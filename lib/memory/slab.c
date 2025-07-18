#include "memory/slab.h"
#include "collection/bitmap.h"
#include "macro.h"
#include "object/slice.h"
#include <stdalign.h>
#include <string.h>

#define CU_DIV_CEIL(x, y) (((x) + (y) - 1) / (y))

struct cu_SlabAllocator_Header {
  struct cu_SlabAllocator_Slab *slab;
  size_t index;
  size_t count;
};

/**
 * @brief Memory block managed by the slab allocator.
 *
 * Each block maintains its own bitmap sized to `slabCount` bits that
 * records which slab slots are currently in use.
 */
struct cu_SlabAllocator_Slab {
  struct cu_SlabAllocator_Slab *next; /**< next slab in the list */
  cu_Bitmap used;                     /**< allocation bitmap */
  size_t slabCount;                   /**< total slab slots */
  size_t freeCount;                   /**< remaining free slots */
  unsigned char data[];               /**< backing storage */
};

static cu_Slice_Optional cu_slab_alloc(
    void *self, size_t size, size_t alignment);
static cu_Slice_Optional cu_slab_resize(
    void *self, cu_Slice mem, size_t size, size_t alignment);
static void cu_slab_free(void *self, cu_Slice mem);

static unsigned char *cu_slab_data(struct cu_SlabAllocator_Slab *slab) {
  return slab->data;
}

static size_t cu_find_run(struct cu_SlabAllocator_Slab *slab, size_t need) {
  size_t run = 0;
  for (size_t i = 0; i < slab->slabCount; ++i) {
    if (!cu_Bitmap_get(&slab->used, i)) {
      run++;
    } else {
      run = 0;
    }
    if (run == need) {
      return i + 1 - need;
    }
  }
  return (size_t)-1;
}

static struct cu_SlabAllocator_Slab *cu_create_slab(
    cu_SlabAllocator *alloc, size_t count) {
  size_t total = sizeof(struct cu_SlabAllocator_Slab) + count * alloc->slabSize;
  cu_Slice_Optional mem =
      cu_Allocator_Alloc(alloc->backingAllocator, total, alignof(max_align_t));
  if (cu_Slice_Optional_is_none(&mem)) {
    return NULL;
  }
  cu_Bitmap_Optional bits = cu_Bitmap_create(alloc->backingAllocator, count);
  if (cu_Bitmap_Optional_is_none(&bits)) {
    cu_Allocator_Free(alloc->backingAllocator, mem.value);
    return NULL;
  }
  struct cu_SlabAllocator_Slab *slab =
      (struct cu_SlabAllocator_Slab *)mem.value.ptr;
  slab->next = NULL;
  slab->used = bits.value;
  slab->slabCount = count;
  slab->freeCount = count;
  return slab;
}

static cu_Slice_Optional cu_slab_alloc(
    void *self, size_t size, size_t alignment) {
  cu_SlabAllocator *alloc = (cu_SlabAllocator *)self;
  if (size == 0) {
    return cu_Slice_Optional_none();
  }
  if (alignment == 0) {
    alignment = 1;
  }
  if (alignment > alloc->slabSize) {
    return cu_Slice_Optional_none();
  }

  size_t total = size + sizeof(struct cu_SlabAllocator_Header);
  size_t need = CU_DIV_CEIL(total, alloc->slabSize);

  struct cu_SlabAllocator_Slab *slab = alloc->slabs;
  size_t index = (size_t)-1;
  while (slab) {
    if (slab->freeCount >= need) {
      size_t pos = cu_find_run(slab, need);
      if (pos != (size_t)-1) {
        index = pos;
        break;
      }
    }
    slab = slab->next;
  }

  if (!slab) {
    size_t def = CU_SLAB_DEFAULT_SIZE / alloc->slabSize;
    if (def == 0) {
      def = 1;
    }
    size_t count = need > def ? need : def;
    slab = cu_create_slab(alloc, count);
    if (!slab) {
      return cu_Slice_Optional_none();
    }
    slab->next = alloc->slabs;
    alloc->slabs = slab;
    index = 0;
  }

  for (size_t i = 0; i < need; ++i) {
    cu_Bitmap_set(&slab->used, index + i);
  }
  slab->freeCount -= need;

  unsigned char *ptr = cu_slab_data(slab) + index * alloc->slabSize;
  struct cu_SlabAllocator_Header *hdr = (struct cu_SlabAllocator_Header *)ptr;
  hdr->slab = slab;
  hdr->index = index;
  hdr->count = need;

  return cu_Slice_Optional_some(
      cu_Slice_create(ptr + sizeof(struct cu_SlabAllocator_Header), size));
}

static cu_Slice_Optional cu_slab_resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  cu_SlabAllocator *alloc = (cu_SlabAllocator *)self;
  if (mem.ptr == NULL) {
    return cu_slab_alloc(self, size, alignment);
  }
  if (size == 0) {
    cu_slab_free(self, mem);
    return cu_Slice_Optional_none();
  }

  struct cu_SlabAllocator_Header *hdr =
      (struct cu_SlabAllocator_Header *)((unsigned char *)mem.ptr -
                                         sizeof(
                                             struct cu_SlabAllocator_Header));
  size_t current =
      hdr->count * alloc->slabSize - sizeof(struct cu_SlabAllocator_Header);
  if (size <= current && alignment <= alloc->slabSize) {
    return cu_Slice_Optional_some(cu_Slice_create(mem.ptr, size));
  }

  cu_Slice_Optional new_mem = cu_slab_alloc(self, size, alignment);
  if (cu_Slice_Optional_is_none(&new_mem)) {
    return cu_Slice_Optional_none();
  }
  memcpy(new_mem.value.ptr, mem.ptr, mem.length < size ? mem.length : size);
  cu_slab_free(self, mem);
  return new_mem;
}

static void cu_slab_free(void *self, cu_Slice mem) {
  cu_SlabAllocator *alloc = (cu_SlabAllocator *)self;
  if (mem.ptr == NULL) {
    return;
  }
  struct cu_SlabAllocator_Header *hdr =
      (struct cu_SlabAllocator_Header *)((unsigned char *)mem.ptr -
                                         sizeof(
                                             struct cu_SlabAllocator_Header));
  struct cu_SlabAllocator_Slab *slab = hdr->slab;
  for (size_t i = 0; i < hdr->count; ++i) {
    cu_Bitmap_clear(&slab->used, hdr->index + i);
  }
  slab->freeCount += hdr->count;
  CU_UNUSED(alloc);
}

cu_Allocator cu_Allocator_SlabAllocator(
    cu_SlabAllocator *alloc, cu_SlabAllocator_Config cfg) {
  if (cu_Allocator_Optional_is_some(&cfg.backingAllocator)) {
    alloc->backingAllocator = cfg.backingAllocator.value;
  } else {
    alloc->backingAllocator = cu_Allocator_CAllocator();
  }
  alloc->slabs = NULL;
  alloc->current = NULL;
  alloc->slabSize = cfg.slabSize ? cfg.slabSize : CU_SLAB_DEFAULT_SIZE;

  cu_Allocator a;
  a.self = alloc;
  a.allocFn = cu_slab_alloc;
  a.resizeFn = cu_slab_resize;
  a.freeFn = cu_slab_free;
  return a;
}

void cu_SlabAllocator_destroy(cu_SlabAllocator *alloc) {
  struct cu_SlabAllocator_Slab *slab = alloc->slabs;
  while (slab) {
    struct cu_SlabAllocator_Slab *next = slab->next;
    size_t total = sizeof(*slab) + slab->slabCount * alloc->slabSize;
    cu_Bitmap_destroy(&slab->used);
    cu_Allocator_Free(alloc->backingAllocator, cu_Slice_create(slab, total));
    slab = next;
  }
  alloc->slabs = NULL;
  alloc->current = NULL;
}
