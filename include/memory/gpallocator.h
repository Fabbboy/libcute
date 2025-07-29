#pragma once

/** @file gpallocator.h General purpose allocator. */

#include "collection/bitmap.h"
#include "memory/allocator.h"
#include <stdbool.h>
#include <stddef.h>

#define CU_GPA_BUCKET_SIZE 4096     /**< default page size for buckets */
#define CU_GPA_NUM_SMALL_BUCKETS 16 /**< number of size classes */
#define CU_GPA_CANARY 0x9232a6ff85dff10fULL /**< bucket canary value */

/** @cond INTERNAL */
struct cu_GPAllocator_BucketHeader;

/** Object pool used by buckets to track slot usage. */
struct cu_GPAllocator_ObjectPool {
  cu_Bitmap used;      /**< slot usage bitmap */
  unsigned char *data; /**< pointer to slot memory */
  size_t objectSize;   /**< size of each object */
  size_t slotCount;    /**< total slots */
  size_t usedCount;    /**< number of used slots */
};

/** Metadata for a single bucket of small allocations. */
struct cu_GPAllocator_BucketHeader {
  struct cu_GPAllocator_BucketHeader *prev; /**< previous bucket */
  struct cu_GPAllocator_BucketHeader *next; /**< next bucket */
  struct cu_GPAllocator_ObjectPool objects; /**< slot storage information */
  size_t canary;                            /**< header corruption check */
};

/** Metadata for large allocations. */
struct cu_GPAllocator_LargeAlloc {
  struct cu_GPAllocator_LargeAlloc *prev; /**< previous entry */
  struct cu_GPAllocator_LargeAlloc *next; /**< next entry */
  cu_Slice slice;                         /**< allocated memory slice */
};
/** @endcond */

/** Runtime state for the general purpose allocator. */
typedef struct {
  cu_Allocator backingAllocator; /**< allocator used for all bookkeeping */
  struct cu_GPAllocator_BucketHeader *smallBuckets[CU_GPA_NUM_SMALL_BUCKETS];
  struct cu_GPAllocator_BucketHeader
      *smallBucketTails[CU_GPA_NUM_SMALL_BUCKETS];
  struct cu_GPAllocator_LargeAlloc
      *largeAllocs;  /**< linked list of large allocations */
  size_t bucketSize; /**< requested bucket size */
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
