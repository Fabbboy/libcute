#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** @file cute.h Umbrella header including all of libcute. */

#include "memory/allocator.h"
#include "memory/arenaallocator.h"
#include "memory/fixedallocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
#include "memory/slab.h"

#include "collection/bitmap.h"
#include "collection/bitset.h"
#include "collection/dlist.h"
#include "collection/hashmap.h"
#include "collection/list.h"
#include "collection/ring_buffer.h"
#include "collection/vector.h"

#include "hash/hash.h"
#include "macro.h"
#include "nostd.h"
#include "object/optional.h"
#include "object/result.h"
#include "string/fmt.h"
#include "string/nostd.h"

#include "io/error.h"
#include "utility.h"
#ifdef __cplusplus
}
#endif
