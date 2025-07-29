#pragma once

/** @file hashmap.h Simple hashmap container. */

#include "hash/hash.h"
#include "macro.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include "object/result.h"
#include "state.h"
#include "utility.h"
#include <stdbool.h>
#include <stddef.h>

/** Bucket used internally by the hashmap. */
typedef struct {
  bool used;     /**< bucket contains a value */
  bool deleted;  /**< bucket has been removed */
  uint64_t hash; /**< cached hash of the key */
  void *key;     /**< pointer to key storage */
  void *value;   /**< pointer to value storage */
} cu_HashMap_Bucket;

typedef uint64_t (*cu_HashMap_HashFn)(const void *key, size_t key_size);
typedef bool (*cu_HashMap_EqualsFn)(
    const void *a, const void *b, size_t key_size);
CU_OPTIONAL_DECL(cu_HashMap_HashFn, cu_HashMap_HashFn)
CU_OPTIONAL_DECL(cu_HashMap_EqualsFn, cu_HashMap_EqualsFn)

/** Simple open addressing hashmap. */
typedef struct {
  cu_HashMap_Bucket *buckets;    /**< bucket array */
  size_t capacity;               /**< number of buckets */
  size_t length;                 /**< number of elements */
  cu_Layout key_layout;          /**< layout of the key */
  cu_Layout value_layout;        /**< layout of the value */
  cu_Allocator allocator;        /**< backing allocator */
  cu_HashMap_HashFn hash_fn;     /**< hashing function */
  cu_HashMap_EqualsFn equals_fn; /**< equality predicate */
  uint32_t seed;                 /**< hash seed to randomize hashes */
} cu_HashMap;

/** Possible error codes returned by hashmap operations. */
typedef enum {
  CU_HASHMAP_ERROR_NONE = 0,
  CU_HASHMAP_ERROR_OOM,
  CU_HASHMAP_ERROR_INVALID_LAYOUT,
  CU_HASHMAP_ERROR_INVALID,
} cu_HashMap_Error;

CU_RESULT_DECL(cu_HashMap, cu_HashMap, cu_HashMap_Error)
CU_OPTIONAL_DECL(cu_HashMap_Error, cu_HashMap_Error)

/**
 * @brief Create a new hashmap.
 *
 * @param allocator allocator used for storage
 * @param key_layout layout describing the key type
 * @param value_layout layout describing the value type
 * @param initial_capacity optional initial bucket count
 * @param hash_fn hashing function, defaults to FNV-1a when none
 * @param equals_fn equality predicate, defaults to bytewise compare
 * @param state randomization source used to seed hashes and mitigate collision
 * attacks
 */
cu_HashMap_Result cu_HashMap_create(cu_Allocator allocator,
    cu_Layout key_layout, cu_Layout value_layout,
    Size_Optional initial_capacity, cu_HashMap_HashFn_Optional hash_fn,
    cu_HashMap_EqualsFn_Optional equals_fn, cu_State state);
/** Release resources held by @p map. */
void cu_HashMap_destroy(cu_HashMap *map);

/**
 * @brief Insert a new key-value pair.
 */
cu_HashMap_Error_Optional cu_HashMap_insert(
    cu_HashMap *map, void *key, void *value);
/**
 * @brief Retrieve the value stored for @p key.
 */
Ptr_Optional cu_HashMap_get(const cu_HashMap *map, const void *key);
/**
 * @brief Iterate over all stored pairs.
 */
bool cu_HashMap_iter(
    const cu_HashMap *map, size_t *index, void **key, void **value);
