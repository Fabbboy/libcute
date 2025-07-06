#pragma once

#include "memory/allocator.h"
#include "object/bitset.h"

#define CU_BUCKET_SIZE 4096
#define CU_NUM_SMALL_BUCKETS                                                   \
  16 // **< Number of small buckets, must be a power of 2 and cannot be
     // overridden. */
#define CU_NUM_LARGE_BUCKETS                                                   \
  8 // **< Number of large buckets, must be a power of 2 and cannot be
    // overridden. */
#define CU_BUCKET_CANARY                                                       \
  0x9232a6ff85dff10f // **< Canary value for bucket headers to detect
                     // corruption. */

struct cu_GPAllocator_BucketHeader;

CU_BITSET_DECL(cu_GPAllocator_BucketHeader, CU_BUCKET_SIZE)
CU_OPTIONAL_DECL(
    cu_GPAllocator_BucketHeaderPtr, struct cu_GPAllocator_BucketHeader *)

struct cu_GPAllocator_BucketHeader {
  cu_GPAllocator_BucketHeader_BitSet freeSlots;
  cu_GPAllocator_BucketHeaderPtr_Optional prevBucket;
  cu_GPAllocator_BucketHeaderPtr_Optional nextBucket;
  size_t canary;
};

struct cu_GPAllocator_LargeAlloc {};

typedef struct {
  size_t
      bucketSize; /**< Size of each bucket in bytes. Overrides BUCKET_SIZE. */
} cu_GPAllocator_Config;

typedef struct {
  cu_Allocator backingAllocator;
  struct cu_GPAllocator_BucketHeader *smallBuckets[CU_NUM_SMALL_BUCKETS];
} cu_Allocator_GPAllocator;