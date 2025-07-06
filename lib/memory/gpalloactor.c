#include "memory/gpallocator.h"
#include "object/bitset.h"

CU_BITSET_IMPL(
    cu_GPAllocator_BucketHeader, NUM_SMALL_BUCKETS + NUM_LARGE_BUCKETS);