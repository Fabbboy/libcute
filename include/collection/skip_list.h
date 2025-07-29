#pragma once

/** @file skip_list.h Simple skip list map. */

#include "macro.h"
#include "memory/allocator.h"
#include "object/destructor.h"
#include "object/optional.h"
#include "object/result.h"
#include "state.h"
#include "utility.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*cu_SkipList_CmpFn)(const void *a, const void *b);
CU_OPTIONAL_DECL(cu_SkipList_CmpFn, cu_SkipList_CmpFn)

/** @cond INTERNAL */
struct cu_SkipList_Node {
  struct cu_SkipList_Node **forward;
  size_t level;
  void *key;
  void *value;
};
/** @endcond */

typedef struct {
  struct cu_SkipList_Node *head;
  size_t level;
  size_t max_level;
  cu_SkipList_CmpFn cmp;
  cu_Layout key_layout;
  cu_Layout value_layout;
  cu_Allocator allocator;
  cu_Destructor_Optional key_destructor;
  cu_Destructor_Optional value_destructor;
  cu_State state; /**< random source for levels */
} cu_SkipList;

typedef enum {
  CU_SKIPLIST_ERROR_NONE = 0,
  CU_SKIPLIST_ERROR_OOM,
  CU_SKIPLIST_ERROR_INVALID_LAYOUT,
  CU_SKIPLIST_ERROR_INVALID,
} cu_SkipList_Error;

CU_RESULT_DECL(cu_SkipList, cu_SkipList, cu_SkipList_Error)
CU_OPTIONAL_DECL(cu_SkipList_Error, cu_SkipList_Error)

/** Create a new skip list using @p state for random level generation. */
cu_SkipList_Result cu_SkipList_create(cu_Allocator allocator,
    cu_Layout key_layout, cu_Layout value_layout, size_t max_level,
    cu_SkipList_CmpFn_Optional cmp, cu_Destructor_Optional key_destructor,
    cu_Destructor_Optional value_destructor, cu_State state);

void cu_SkipList_destroy(cu_SkipList *list);

cu_SkipList_Error_Optional cu_SkipList_insert(
    cu_SkipList *list, void *key, void *value);

Ptr_Optional cu_SkipList_find(const cu_SkipList *list, const void *key);

cu_SkipList_Error_Optional cu_SkipList_remove(
    cu_SkipList *list, const void *key);

bool cu_SkipList_iter(const cu_SkipList *list, struct cu_SkipList_Node **node,
    void **key, void **value);

#ifdef __cplusplus
}
#endif
