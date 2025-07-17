#include "memory/slab.h"
#include "macro.h"
#include "object/slice.h"
#include <string.h>

struct cu_SlabAllocator_Slab {
  struct cu_SlabAllocator_Slab *next;
  size_t offset;
  size_t capacity;
  unsigned char data[];
};

static cu_Slice_Optional cu_slab_alloc(
    void *self, size_t size, size_t alignment);
static cu_Slice_Optional cu_slab_resize(
    void *self, cu_Slice mem, size_t size, size_t alignment);
static void cu_slab_free(void *self, cu_Slice mem);

static struct cu_SlabAllocator_Slab *cu_create_slab(
    cu_SlabAllocator *alloc, size_t capacity) {
  size_t total = sizeof(struct cu_SlabAllocator_Slab) + capacity;
  cu_Slice_Optional mem =
      cu_Allocator_Alloc(alloc->backingAllocator, total, sizeof(void *));
  if (cu_Slice_Optional_is_none(&mem)) {
    return NULL;
  }
  struct cu_SlabAllocator_Slab *slab =
      (struct cu_SlabAllocator_Slab *)mem.value.ptr;
  slab->next = NULL;
  slab->offset = 0;
  slab->capacity = capacity;
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

  struct cu_SlabAllocator_Slab *slab = alloc->current;
  size_t aligned = 0;
  if (slab) {
    aligned = CU_ALIGN_UP(slab->offset, alignment);
  }
  if (!slab || aligned + size > slab->capacity) {
    size_t cap = size > alloc->slabSize ? size : alloc->slabSize;
    slab = cu_create_slab(alloc, cap);
    if (!slab) {
      return cu_Slice_Optional_none();
    }
    if (alloc->current) {
      alloc->current->next = slab;
    } else {
      alloc->slabs = slab;
    }
    alloc->current = slab;
    aligned = CU_ALIGN_UP(slab->offset, alignment);
  }

  void *ptr = slab->data + aligned;
  slab->offset = aligned + size;
  return cu_Slice_Optional_some(cu_Slice_create(ptr, size));
}

static cu_Slice_Optional cu_slab_resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  if (mem.ptr == NULL) {
    return cu_slab_alloc(self, size, alignment);
  }
  if (size == 0) {
    cu_slab_free(self, mem);
    return cu_Slice_Optional_none();
  }
  if (size <= mem.length) {
    return cu_Slice_Optional_some(cu_Slice_create(mem.ptr, size));
  }
  cu_Slice_Optional new_mem = cu_slab_alloc(self, size, alignment);
  if (cu_Slice_Optional_is_none(&new_mem)) {
    return cu_Slice_Optional_none();
  }
  memcpy(new_mem.value.ptr, mem.ptr, mem.length < size ? mem.length : size);
  return new_mem;
}

static void cu_slab_free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  CU_UNUSED(mem);
  /* individual frees are ignored */
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
    cu_Allocator_Free(alloc->backingAllocator,
        cu_Slice_create(slab, sizeof(*slab) + slab->capacity));
    slab = next;
  }
  alloc->slabs = NULL;
  alloc->current = NULL;
}
