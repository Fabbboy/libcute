#pragma once

#include "object/bitset.h"
#define BUCKET_SIZE 4096
#define NUM_SMALL_BUCKETS 16
#define NUM_LARGE_BUCKETS 8

struct cu_GPAllocator_BucketHeader;

CU_BITSET_DECL(
    cu_GPAllocator_BucketHeader, NUM_SMALL_BUCKETS + NUM_LARGE_BUCKETS)
CU_OPTIONAL_DECL(
    cu_GPAllocator_BucketHeaderPtr, struct cu_GPAllocator_BucketHeader *)

struct cu_GPAllocator_BucketHeader {
  cu_GPAllocator_BucketHeader_BitSet freeSlots;
  cu_GPAllocator_BucketHeaderPtr_Optional prevBucket;
  cu_GPAllocator_BucketHeaderPtr_Optional nextBucket;
};

typedef struct {

} cu_Allocator_GPAllocator;