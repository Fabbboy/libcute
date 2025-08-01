#include "memory/slab.h"
#include "collection/bitmap.h"
#include "io/error.h"
#include "macro.h" 
#include <nostd.h>
#include <stdalign.h>

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

static cu_IoSlice_Result cu_slab_alloc(void *self, cu_Layout layout);
static cu_IoSlice_Result cu_slab_grow(
    void *self, cu_Slice old_mem, cu_Layout new_layout);
static cu_IoSlice_Result cu_slab_shrink(
    void *self, cu_Slice old_mem, cu_Layout new_layout);
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
  cu_IoSlice_Result mem = cu_Allocator_Alloc(
      alloc->backingAllocator, cu_Layout_create(total, alignof(cu_max_align_t)));
  if (!cu_IoSlice_Result_is_ok(&mem)) {
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

static cu_IoSlice_Result cu_slab_alloc(void *self, cu_Layout layout) {
  cu_SlabAllocator *alloc = (cu_SlabAllocator *)self;
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
  if (alignment > alloc->slabSize) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }

  size_t needed = alignment - 1 + size + sizeof(struct cu_SlabAllocator_Header);
  size_t need = CU_DIV_CEIL(needed, alloc->slabSize);

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
    size_t count = CU_MAX(need, def);
    slab = cu_create_slab(alloc, count);
    if (!slab) {
      cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
          .errnum = Size_Optional_none()};
      return cu_IoSlice_Result_error(err);
    }
    slab->next = alloc->slabs;
    alloc->slabs = slab;
    index = 0;
  }

  for (size_t i = 0; i < need; ++i) {
    cu_Bitmap_set(&slab->used, index + i);
  }
  slab->freeCount -= need;

  unsigned char *data = cu_slab_data(slab);
  size_t start = index * alloc->slabSize;
  size_t user_pos =
      CU_ALIGN_UP(
          (size_t)(data + start + sizeof(struct cu_SlabAllocator_Header)),
          alignment) -
      (size_t)data;
  size_t header_pos = user_pos - sizeof(struct cu_SlabAllocator_Header);
  struct cu_SlabAllocator_Header *hdr =
      (struct cu_SlabAllocator_Header *)(data + header_pos);
  hdr->slab = slab;
  hdr->index = index;
  hdr->count = need;
  return cu_IoSlice_Result_ok(cu_Slice_create(data + user_pos, size));
}

static cu_IoSlice_Result cu_slab_grow(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  cu_SlabAllocator *alloc = (cu_SlabAllocator *)self;
  CU_IF_NULL(old_mem.ptr) {
    return cu_slab_alloc(self, new_layout);
  }

  struct cu_SlabAllocator_Header *hdr =
      (struct cu_SlabAllocator_Header *)((unsigned char *)old_mem.ptr -
                                         sizeof(struct cu_SlabAllocator_Header));
  unsigned char *base = cu_slab_data(hdr->slab) + hdr->index * alloc->slabSize;
  size_t prefix = (unsigned char *)old_mem.ptr - base;
  size_t current = hdr->count * alloc->slabSize - prefix;
  if (new_layout.elem_size <= current) {
    return cu_IoSlice_Result_ok(
        cu_Slice_create(old_mem.ptr, new_layout.elem_size));
  }

  cu_IoSlice_Result new_mem = cu_slab_alloc(self, new_layout);
  if (!cu_IoSlice_Result_is_ok(&new_mem)) {
    return new_mem;
  }
  size_t copy = CU_MIN(old_mem.length, new_layout.elem_size);
  cu_Memory_smemcpy(cu_Slice_create(new_mem.value.ptr, copy),
      cu_Slice_create(old_mem.ptr, copy));
  cu_slab_free(self, old_mem);
  return new_mem;
}

static cu_IoSlice_Result cu_slab_shrink(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  cu_SlabAllocator *alloc = (cu_SlabAllocator *)self;
  CU_IF_NULL(old_mem.ptr) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }

  struct cu_SlabAllocator_Header *hdr =
      (struct cu_SlabAllocator_Header *)((unsigned char *)old_mem.ptr -
                                         sizeof(struct cu_SlabAllocator_Header));
  unsigned char *base = cu_slab_data(hdr->slab) + hdr->index * alloc->slabSize;
  size_t prefix = (unsigned char *)old_mem.ptr - base;
  size_t current = hdr->count * alloc->slabSize - prefix;
  if (new_layout.elem_size <= current) {
    size_t need = CU_DIV_CEIL(prefix + new_layout.elem_size, alloc->slabSize);
    if (need < hdr->count) {
      for (size_t i = hdr->index + need; i < hdr->index + hdr->count; ++i) {
        cu_Bitmap_clear(&hdr->slab->used, i);
      }
      hdr->slab->freeCount += hdr->count - need;
      hdr->count = need;
    }
    return cu_IoSlice_Result_ok(
        cu_Slice_create(old_mem.ptr, new_layout.elem_size));
  }

  cu_IoSlice_Result new_mem = cu_slab_alloc(self, new_layout);
  if (!cu_IoSlice_Result_is_ok(&new_mem)) {
    return new_mem;
  }
  size_t copy = CU_MIN(new_layout.elem_size, old_mem.length);
  cu_Memory_smemcpy(cu_Slice_create(new_mem.value.ptr, copy),
      cu_Slice_create(old_mem.ptr, copy));
  cu_slab_free(self, old_mem);
  return new_mem;
}

static void cu_slab_free(void *self, cu_Slice mem) {
  cu_SlabAllocator *alloc = (cu_SlabAllocator *)self;
  CU_IF_NULL(mem.ptr) { return; }
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
#if CU_PLAT_WASM
    alloc->backingAllocator = cu_Allocator_WasmAllocator();
#elif !CU_FREESTANDING
    alloc->backingAllocator = cu_Allocator_CAllocator();
#else
    alloc->backingAllocator = cu_Allocator_NullAllocator();
#endif
  }
  alloc->slabs = NULL;
  alloc->current = NULL;
  alloc->slabSize = cfg.slabSize;
  if (alloc->slabSize == 0) {
    alloc->slabSize = CU_SLAB_DEFAULT_SIZE;
  }

  cu_Allocator a = {0};
  a.self = alloc;
  a.allocFn = cu_slab_alloc;
  a.growFn = cu_slab_grow;
  a.shrinkFn = cu_slab_shrink;
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
