#include "memory/arenaallocator.h"
#include "io/error.h"
#include <stddef.h>
#include "macro.h"
#include <nostd.h>
#include <stdalign.h>
static void cu_arena_free(void *self, cu_Slice mem);

static struct cu_ArenaAllocator_Chunk *cu_arena_create_chunk(
    cu_ArenaAllocator *arena, size_t size) {
  size_t total = sizeof(struct cu_ArenaAllocator_Chunk) + size;
  cu_IoSlice_Result mem = cu_Allocator_Alloc(
      arena->backingAllocator, cu_Layout_create(total, alignof(cu_max_align_t)));
  if (!cu_IoSlice_Result_is_ok(&mem)) {
    return NULL;
  }
  struct cu_ArenaAllocator_Chunk *chunk =
      (struct cu_ArenaAllocator_Chunk *)mem.value.ptr;
  chunk->prev = NULL;
  chunk->size = size;
  chunk->used = 0;
  return chunk;
}

static cu_IoSlice_Result cu_arena_alloc(void *self, cu_Layout layout) {
  cu_ArenaAllocator *arena = (cu_ArenaAllocator *)self;
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

  const size_t header_size = sizeof(struct cu_ArenaAllocator_Header);
  size_t chunk_size = arena->chunkSize;
  if (chunk_size == 0) {
    chunk_size = CU_ARENA_CHUNK_SIZE;
  }
  size_t needed = alignment - 1 + size + header_size;

  struct cu_ArenaAllocator_Chunk *chunk = arena->current;
  struct cu_ArenaAllocator_Chunk *target = NULL;

  for (struct cu_ArenaAllocator_Chunk *c = chunk; c; c = c->prev) {
    size_t start = CU_ALIGN_UP(c->used + header_size, alignment);
    if (start + size <= c->size) {
      target = c;
      break;
    }
  }

  if (!target) {
    size_t new_size = CU_MAX(needed, chunk_size);
    target = cu_arena_create_chunk(arena, new_size);
    if (!target) {
      cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
          .errnum = Size_Optional_none()};
      return cu_IoSlice_Result_error(err);
    }
    target->prev = arena->current;
    arena->current = target;
  }

  chunk = target;
  size_t start = CU_ALIGN_UP(chunk->used + header_size, alignment);
  size_t header_pos = start - header_size;
  struct cu_ArenaAllocator_Header *hdr =
      (struct cu_ArenaAllocator_Header *)(chunk->data + header_pos);
  hdr->chunk = chunk;
  hdr->prev_offset = chunk->used;
  chunk->used = start + size;
  return cu_IoSlice_Result_ok(cu_Slice_create(chunk->data + start, size));
}


static cu_IoSlice_Result cu_arena_grow(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  if (old_mem.ptr == NULL) {
    return cu_arena_alloc(self, new_layout);
  }

  const size_t header_size = sizeof(struct cu_ArenaAllocator_Header);
  struct cu_ArenaAllocator_Header *hdr =
      (struct cu_ArenaAllocator_Header *)((unsigned char *)old_mem.ptr -
                                          header_size);
  struct cu_ArenaAllocator_Chunk *chunk = hdr->chunk;
  if (!chunk) {
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
        .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }

  unsigned char *mem_end = (unsigned char *)old_mem.ptr + old_mem.length;
  if (mem_end == chunk->data + chunk->used) {
    size_t avail = chunk->size - chunk->used;
    if (new_layout.elem_size <= old_mem.length + avail) {
      size_t start = (unsigned char *)old_mem.ptr - chunk->data;
      chunk->used = start + new_layout.elem_size;
      return cu_IoSlice_Result_ok(
          cu_Slice_create(old_mem.ptr, new_layout.elem_size));
    }
  }

  cu_IoSlice_Result new_mem = cu_arena_alloc(self, new_layout);
  if (!cu_IoSlice_Result_is_ok(&new_mem)) {
    return new_mem;
  }
  if (old_mem.length > 0) {
    cu_Memory_smemcpy(new_mem.value, old_mem);
  }
  cu_arena_free(self, old_mem);
  return new_mem;
}

static cu_IoSlice_Result cu_arena_shrink(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  if (old_mem.ptr == NULL) {
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
        .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }

  const size_t header_size = sizeof(struct cu_ArenaAllocator_Header);
  struct cu_ArenaAllocator_Header *hdr =
      (struct cu_ArenaAllocator_Header *)((unsigned char *)old_mem.ptr -
                                          header_size);
  struct cu_ArenaAllocator_Chunk *chunk = hdr->chunk;
  if (!chunk) {
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
        .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }

  unsigned char *mem_end = (unsigned char *)old_mem.ptr + old_mem.length;
  if (mem_end == chunk->data + chunk->used) {
    size_t start = (unsigned char *)old_mem.ptr - chunk->data;
    chunk->used = start + new_layout.elem_size;
  }

  if (new_layout.elem_size == 0) {
    cu_arena_free(self, old_mem);
    return cu_IoSlice_Result_ok(cu_Slice_create(NULL, 0));
  }

  return cu_IoSlice_Result_ok(
      cu_Slice_create(old_mem.ptr, new_layout.elem_size));
}

static void cu_arena_free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  CU_IF_NULL(mem.ptr) { return; }
  const size_t header_size = sizeof(struct cu_ArenaAllocator_Header);
  struct cu_ArenaAllocator_Header *hdr =
      (struct cu_ArenaAllocator_Header *)((unsigned char *)mem.ptr -
                                          header_size);
  struct cu_ArenaAllocator_Chunk *chunk = hdr->chunk;
  if (!chunk) {
    return;
  }
  if ((unsigned char *)mem.ptr + mem.length != chunk->data + chunk->used) {
    return;
  }
  chunk->used = hdr->prev_offset;
}

static void cu_arena_destroy_chunk(
    cu_ArenaAllocator *arena, struct cu_ArenaAllocator_Chunk *chunk) {
  cu_Allocator_Free(arena->backingAllocator,
      cu_Slice_create(chunk, sizeof(*chunk) + chunk->size));
}

cu_Allocator cu_Allocator_ArenaAllocator(
    cu_ArenaAllocator *arena, cu_ArenaAllocator_Config config) {
  if (cu_Allocator_Optional_is_some(&config.backingAllocator)) {
    arena->backingAllocator = config.backingAllocator.value;
  } else {
#if CU_PLAT_WASM
    arena->backingAllocator = cu_Allocator_WasmAllocator();
#elif !CU_FREESTANDING
    arena->backingAllocator = cu_Allocator_CAllocator();
#else
    arena->backingAllocator = cu_Allocator_NullAllocator();
#endif
  }
  arena->chunkSize = config.chunkSize;
  if (arena->chunkSize == 0) {
    arena->chunkSize = CU_ARENA_CHUNK_SIZE;
  }
  arena->current = NULL;

  cu_Allocator a = {0};
  a.self = arena;
  a.allocFn = cu_arena_alloc;
  a.growFn = cu_arena_grow;
  a.shrinkFn = cu_arena_shrink;
  a.freeFn = cu_arena_free;
  return a;
}

void cu_ArenaAllocator_destroy(cu_ArenaAllocator *arena) {
  struct cu_ArenaAllocator_Chunk *chunk = arena->current;
  while (chunk) {
    struct cu_ArenaAllocator_Chunk *prev = chunk->prev;
    cu_arena_destroy_chunk(arena, chunk);
    chunk = prev;
  }
  arena->current = NULL;
}
