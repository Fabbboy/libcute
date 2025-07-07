#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** @file cute.h Umbrella header including all of libcute. */

#include "memory/allocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"

#include "collection/bitmap.h"
#include "collection/bitset.h"
#include "collection/vector.h"
#include "collection/hashmap.h"

#include "hash/hash.h"
#include "macro.h"
#include "object/optional.h"
#include "object/result.h"
#include "object/slice.h"
#include "string/string.h"

#include "utility.h"
#ifdef __cplusplus
}
#endif