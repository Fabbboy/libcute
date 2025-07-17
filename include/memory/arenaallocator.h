#pragma once

/** @file arenaallocator.h Arena allocator. */

#include "memory/allocator.h"
#include <stddef.h>

/** default size for new chunks */
#define CU_ARENA_CHUNK_SIZE 4096

/** Metadata for each arena chunk. */
struct cu_ArenaAllocator_Chunk {
  struct cu_ArenaAllocator_Chunk *prev; /**< previous chunk */
  size_t size;                          /**< number of usable bytes */
  size_t used;                          /**< used bytes */
  unsigned char data[];                 /**< flexible array for storage */
};

/** Runtime state for the arena allocator. */
typedef struct {
  cu_Allocator backingAllocator;           /**< chunk backing allocator */
  struct cu_ArenaAllocator_Chunk *current; /**< current active chunk */
  size_t chunkSize;                        /**< requested chunk size */
} cu_ArenaAllocator;

/** Configuration when creating an arena allocator. */
typedef struct {
  size_t chunkSize;                       /**< desired chunk size */
  cu_Allocator_Optional backingAllocator; /**< optional custom allocator */
} cu_ArenaAllocator_Config;

/** Create an arena allocator using the given configuration. */
cu_Allocator cu_Allocator_ArenaAllocator(
    cu_ArenaAllocator *arena, cu_ArenaAllocator_Config config);

/** Release all chunks allocated by the arena. */
void cu_ArenaAllocator_destroy(cu_ArenaAllocator *arena);
