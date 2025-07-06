#pragma once

/** @file gpallocator.h General purpose allocator. */

#include "memory/allocator.h"
#include "object/bitset.h"
#include <stdbool.h>
#include <stddef.h>

#define CU_GPA_BUCKET_SIZE 4096     /**< default page size for buckets */
#define CU_GPA_NUM_SMALL_BUCKETS 16 /**< number of size classes */
#define CU_GPA_CANARY 0x9232a6ff85dff10fULL /**< bucket canary value */

CU_BITSET_DECL(cu_GPAllocator_UsedBits, CU_GPA_BUCKET_SIZE)

typedef struct cu_GPAllocator_BucketHeader cu_GPAllocator_BucketHeader;

/** Object pool used by buckets to track slot usage. */
typedef struct {
  cu_GPAllocator_UsedBits_BitSet used; /**< slot usage bitset */
  unsigned char *data;                 /**< pointer to slot memory */
  size_t objectSize;                   /**< size of each object */
  size_t slotCount;                    /**< total slots */
  size_t usedCount;                    /**< number of used slots */
} cu_GPAllocator_ObjectPool;

/** Metadata for a single bucket of small allocations. */
struct cu_GPAllocator_BucketHeader {
  cu_GPAllocator_BucketHeader *prev; /**< previous bucket */
  cu_GPAllocator_BucketHeader *next; /**< next bucket */
  cu_GPAllocator_ObjectPool objects; /**< slot storage information */
  size_t canary;                     /**< header corruption check */
};

/** Metadata for large allocations. */
typedef struct cu_GPALargeAlloc {
  struct cu_GPALargeAlloc *prev; /**< previous entry */
  struct cu_GPALargeAlloc *next; /**< next entry */
  cu_Slice slice;                /**< allocated memory slice */
} cu_GPALargeAlloc;

/** Runtime state for the general purpose allocator. */
typedef struct {
  cu_Allocator backingAllocator; /**< allocator used for all bookkeeping */
  cu_GPAllocator_BucketHeader *smallBuckets[CU_GPA_NUM_SMALL_BUCKETS];
  cu_GPALargeAlloc *largeAllocs; /**< linked list of large allocations */
  size_t bucketSize;             /**< requested bucket size */
} cu_GPAllocator;

typedef struct {
  size_t bucketSize;                      /**< desired bucket size */
  cu_Allocator_Optional backingAllocator; /**< custom backing allocator */
} cu_GPAllocator_Config;

/** Create a GP allocator using the given configuration. */
cu_Allocator cu_Allocator_GPAllocator(
    cu_GPAllocator *alloc, cu_GPAllocator_Config config);

/** Release all buckets and large allocations. */
void cu_GPAllocator_destroy(cu_GPAllocator *alloc);
