#include "collection/hashmap.h"
#include "hash/hash.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include "object/result.h"
#include "utility.h"
#include <nostd.h>
#include <stdalign.h>
#include <stddef.h>

CU_RESULT_IMPL(cu_HashMap, cu_HashMap, cu_HashMap_Error)
CU_OPTIONAL_IMPL(cu_HashMap_Error, cu_HashMap_Error)
CU_OPTIONAL_IMPL(cu_HashMap_HashFn, cu_HashMap_HashFn)
CU_OPTIONAL_IMPL(cu_HashMap_EqualsFn, cu_HashMap_EqualsFn)

static uint64_t cu_HashMap_default_hash(const void *key, size_t key_size) {
  return cu_Hash_FNV1a64(key, key_size);
}

static bool cu_HashMap_default_equals(
    const void *a, const void *b, size_t key_size) {
  return cu_Memory_memcmp(
             cu_Slice_create((void *)a, key_size),
             cu_Slice_create((void *)b, key_size)) == true;
}

static cu_HashMap_Bucket *cu_HashMap_lookup_bucket(
    const cu_HashMap *map, const void *key, uint64_t hash) {
  size_t mask = map->capacity - 1;
  size_t idx = hash & mask;
  for (;;) {
    cu_HashMap_Bucket *b = &map->buckets[idx];
    if (!b->used) {
      return NULL;
    }
    if (!b->deleted && b->hash == hash &&
        map->equals_fn(b->key, key, map->key_layout.elem_size)) {
      return b;
    }
    idx = (idx + 1) & mask;
  }
}

static cu_HashMap_Bucket *cu_HashMap_find_slot(
    cu_HashMap *map, const void *key, uint64_t hash) {
  size_t mask = map->capacity - 1;
  size_t idx = hash & mask;
  cu_HashMap_Bucket *first_del = NULL;
  for (;;) {
    cu_HashMap_Bucket *b = &map->buckets[idx];
    if (!b->used) {
      return first_del ? first_del : b;
    }
    if (b->deleted) {
      if (!first_del) {
        first_del = b;
      }
    } else if (b->hash == hash &&
               map->equals_fn(b->key, key, map->key_layout.elem_size)) {
      return b;
    }
    idx = (idx + 1) & mask;
  }
}

static cu_HashMap_Error_Optional cu_HashMap_rehash(
    cu_HashMap *map, size_t new_cap) {
  if (new_cap < 16) {
    new_cap = 16;
  }
  new_cap = cu_next_pow2(new_cap);
  cu_Slice_Result mem = cu_Allocator_Alloc(
      map->allocator,
      cu_Layout_create(new_cap * sizeof(cu_HashMap_Bucket),
          alignof(cu_HashMap_Bucket)));
  if (!cu_Slice_result_is_ok(&mem)) {
    return cu_HashMap_Error_Optional_some(CU_HASHMAP_ERROR_OOM);
  }
  cu_HashMap_Bucket *new_buckets = (cu_HashMap_Bucket *)mem.value.ptr;
  cu_Memory_memset(new_buckets, 0, new_cap * sizeof(cu_HashMap_Bucket));

  size_t old_cap = map->capacity;
  cu_HashMap_Bucket *old_buckets = map->buckets;
  map->buckets = new_buckets;
  map->capacity = new_cap;
  map->length = 0;

  for (size_t i = 0; i < old_cap; ++i) {
    cu_HashMap_Bucket *b = &old_buckets[i];
    if (!b->used || b->deleted) {
      continue;
    }
    cu_HashMap_Bucket *slot = cu_HashMap_find_slot(map, b->key, b->hash);
    slot->used = true;
    slot->deleted = false;
    slot->hash = b->hash;
    slot->key = b->key;
    slot->value = b->value;
    map->length++;
  }
  if (old_buckets) {
    cu_Allocator_Free(map->allocator,
        cu_Slice_create(old_buckets, old_cap * sizeof(cu_HashMap_Bucket)));
  }
  return cu_HashMap_Error_Optional_none();
}

cu_HashMap_Result cu_HashMap_create(cu_Allocator allocator,
    cu_Layout key_layout, cu_Layout value_layout,
    Size_Optional initial_capacity, cu_HashMap_HashFn_Optional hash_fn,
    cu_HashMap_EqualsFn_Optional equals_fn) {
  CU_LAYOUT_CHECK(key_layout) {
    return cu_HashMap_result_error(CU_HASHMAP_ERROR_INVALID_LAYOUT);
  }
  CU_LAYOUT_CHECK(value_layout) {
    return cu_HashMap_result_error(CU_HASHMAP_ERROR_INVALID_LAYOUT);
  }
  size_t cap = 16;
  if (Size_Optional_is_some(&initial_capacity)) {
    cap = Size_Optional_unwrap(&initial_capacity);
  }
  cap = cu_next_pow2(cap);
  cu_Slice_Result mem = cu_Allocator_Alloc(
      allocator,
      cu_Layout_create(cap * sizeof(cu_HashMap_Bucket),
          alignof(cu_HashMap_Bucket)));
  if (!cu_Slice_result_is_ok(&mem)) {
    return cu_HashMap_result_error(CU_HASHMAP_ERROR_OOM);
  }
  cu_HashMap_Bucket *buckets = (cu_HashMap_Bucket *)mem.value.ptr;
  cu_Memory_memset(buckets, 0, cap * sizeof(cu_HashMap_Bucket));

  cu_HashMap map = {0};
  map.buckets = buckets;
  map.capacity = cap;
  map.length = 0;
  map.key_layout = key_layout;
  map.value_layout = value_layout;
  map.allocator = allocator;
  map.hash_fn = cu_HashMap_default_hash;
  if (cu_HashMap_HashFn_Optional_is_some(&hash_fn)) {
    map.hash_fn = cu_HashMap_HashFn_Optional_unwrap(&hash_fn);
  }

  map.equals_fn = cu_HashMap_default_equals;
  if (cu_HashMap_EqualsFn_Optional_is_some(&equals_fn)) {
    map.equals_fn = cu_HashMap_EqualsFn_Optional_unwrap(&equals_fn);
  }
  return cu_HashMap_result_ok(map);
}

void cu_HashMap_destroy(cu_HashMap *map) {
  if (!map) {
    return;
  }
  for (size_t i = 0; i < map->capacity; ++i) {
    cu_HashMap_Bucket *b = &map->buckets[i];
    if (b->used && !b->deleted) {
      cu_Allocator_Free(
          map->allocator, cu_Slice_create(b->key, map->key_layout.elem_size));
      cu_Allocator_Free(map->allocator,
          cu_Slice_create(b->value, map->value_layout.elem_size));
    }
  }
  cu_Allocator_Free(map->allocator,
      cu_Slice_create(map->buckets, map->capacity * sizeof(cu_HashMap_Bucket)));
  map->buckets = NULL;
  map->capacity = 0;
  map->length = 0;
}

cu_HashMap_Error_Optional cu_HashMap_insert(
    cu_HashMap *map, void *key, void *value) {
  CU_IF_NULL(map) {
    return cu_HashMap_Error_Optional_some(CU_HASHMAP_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(map->key_layout) {
    return cu_HashMap_Error_Optional_some(CU_HASHMAP_ERROR_INVALID_LAYOUT);
  }
  CU_LAYOUT_CHECK(map->value_layout) {
    return cu_HashMap_Error_Optional_some(CU_HASHMAP_ERROR_INVALID_LAYOUT);
  }
  if ((map->length + 1) * 2 >= map->capacity) {
    cu_HashMap_Error_Optional err = cu_HashMap_rehash(map, map->capacity * 2);
    if (cu_HashMap_Error_Optional_is_some(&err)) {
      return err;
    }
  }
  uint64_t hash = map->hash_fn(key, map->key_layout.elem_size);
  cu_HashMap_Bucket *slot = cu_HashMap_find_slot(map, key, hash);
  if (slot->used && !slot->deleted) {
    cu_Memory_memcpy(slot->value,
        cu_Slice_create((void *)value, map->value_layout.elem_size));
    return cu_HashMap_Error_Optional_none();
  }

  slot->used = true;
  slot->deleted = false;
  slot->hash = hash;

  cu_Slice_Result key_mem = cu_Allocator_Alloc(
      map->allocator,
      cu_Layout_create(
          map->key_layout.elem_size, map->key_layout.alignment));
  if (!cu_Slice_result_is_ok(&key_mem)) {
    return cu_HashMap_Error_Optional_some(CU_HASHMAP_ERROR_OOM);
  }
  cu_Slice_Result val_mem = cu_Allocator_Alloc(
      map->allocator,
      cu_Layout_create(
          map->value_layout.elem_size, map->value_layout.alignment));
  if (!cu_Slice_result_is_ok(&val_mem)) {
    cu_Allocator_Free(map->allocator, key_mem.value);
    return cu_HashMap_Error_Optional_some(CU_HASHMAP_ERROR_OOM);
  }
  cu_Memory_memcpy(key_mem.value.ptr,
      cu_Slice_create((void *)key, map->key_layout.elem_size));
  cu_Memory_memcpy(val_mem.value.ptr,
      cu_Slice_create((void *)value, map->value_layout.elem_size));
  slot->key = key_mem.value.ptr;
  slot->value = val_mem.value.ptr;
  map->length++;
  return cu_HashMap_Error_Optional_none();
}

Ptr_Optional cu_HashMap_get(const cu_HashMap *map, const void *key) {
  CU_IF_NULL(map) { return Ptr_Optional_none(); }
  CU_LAYOUT_CHECK(map->key_layout) { return Ptr_Optional_none(); }
  if (map->capacity == 0) {
    return Ptr_Optional_none();
  }
  uint64_t hash = map->hash_fn(key, map->key_layout.elem_size);
  cu_HashMap_Bucket *b = cu_HashMap_lookup_bucket(map, key, hash);
  return b ? Ptr_Optional_some(b->value) : Ptr_Optional_none();
}

bool cu_HashMap_iter(
    const cu_HashMap *map, size_t *index, void **key, void **value) {
  CU_IF_NULL(map) { return false; }
  CU_IF_NULL(index) { return false; }
  CU_IF_NULL(key) { return false; }
  CU_IF_NULL(value) { return false; }

  while (*index < map->capacity) {
    cu_HashMap_Bucket *b = &map->buckets[*index];
    (*index)++;
    if (b->used && !b->deleted) {
      *key = b->key;
      *value = b->value;
      return true;
    }
  }
  return false;
}
