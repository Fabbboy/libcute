#include "memory/arenaallocator.h"
#include "macro.h"
#include "nostd.h"
#include <stdalign.h>
#include <string.h>
static void cu_arena_free(void *self, cu_Slice mem);

static struct cu_ArenaAllocator_Chunk *cu_arena_create_chunk(
    cu_ArenaAllocator *arena, size_t size) {
  size_t total = sizeof(struct cu_ArenaAllocator_Chunk) + size;
  cu_Slice_Result mem =
      cu_Allocator_Alloc(arena->backingAllocator, total, alignof(max_align_t));
  if (!cu_Slice_result_is_ok(&mem)) {
    return NULL;
  }
  struct cu_ArenaAllocator_Chunk *chunk =
      (struct cu_ArenaAllocator_Chunk *)mem.value.ptr;
  chunk->prev = NULL;
  chunk->size = size;
  chunk->used = 0;
  return chunk;
}

static cu_Slice_Result cu_arena_alloc(
    void *self, size_t size, size_t alignment) {
  cu_ArenaAllocator *arena = (cu_ArenaAllocator *)self;
  if (size == 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }
  if (alignment == 0) {
    alignment = 1;
  }

  const size_t header_size = sizeof(struct cu_ArenaAllocator_Header);
  size_t chunk_size = arena->chunkSize ? arena->chunkSize : CU_ARENA_CHUNK_SIZE;
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
    size_t new_size = (needed > chunk_size) ? needed : chunk_size;
    target = cu_arena_create_chunk(arena, new_size);
    if (!target) {
      cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
          .errnum = Size_Optional_none()};
      return cu_Slice_result_error(err);
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
  return cu_Slice_result_ok(cu_Slice_create(chunk->data + start, size));
}

static cu_Slice_Result cu_arena_resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  if (mem.ptr == NULL) {
    return cu_arena_alloc(self, size, alignment);
  }
  if (size == 0) {
    cu_arena_free(self, mem);
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }
  const size_t header_size = sizeof(struct cu_ArenaAllocator_Header);
  struct cu_ArenaAllocator_Header *hdr =
      (struct cu_ArenaAllocator_Header *)((unsigned char *)mem.ptr -
                                          header_size);
  struct cu_ArenaAllocator_Chunk *chunk = hdr->chunk;
  if (!chunk) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }
  if ((unsigned char *)mem.ptr + mem.length == chunk->data + chunk->used) {
    size_t avail = chunk->size - (chunk->used - mem.length);
    if (size <= mem.length + avail) {
      chunk->used = (chunk->used - mem.length) + size;
      return cu_Slice_result_ok(cu_Slice_create(mem.ptr, size));
    }
    // not enough space, fall through
  }

  cu_Slice_Result new_mem = cu_arena_alloc(self, size, alignment);
  if (!cu_Slice_result_is_ok(&new_mem)) {
    return new_mem;
  }
  size_t copy = mem.length < size ? mem.length : size;
  memmove(new_mem.value.ptr, mem.ptr, copy);
  cu_arena_free(self, mem);
  return new_mem;
}

static void cu_arena_free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  if (mem.ptr == NULL) {
    return;
  }
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
    arena->backingAllocator = cu_Allocator_CAllocator();
  }
  arena->chunkSize = config.chunkSize ? config.chunkSize : CU_ARENA_CHUNK_SIZE;
  arena->current = NULL;

  cu_Allocator a;
  a.self = arena;
  a.allocFn = cu_arena_alloc;
  a.resizeFn = cu_arena_resize;
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
