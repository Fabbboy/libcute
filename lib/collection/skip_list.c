#include "collection/skip_list.h"
#include "memory/allocator.h"
#include "state.h"
#include "utility.h"
#include <nostd.h>
#include <stdalign.h>

CU_RESULT_IMPL(cu_SkipList, cu_SkipList, cu_SkipList_Error)
CU_OPTIONAL_IMPL(cu_SkipList_Error, cu_SkipList_Error)
CU_OPTIONAL_IMPL(cu_SkipList_CmpFn, cu_SkipList_CmpFn)

static int cu_SkipList_default_cmp(const void *a, const void *b) {
  const int *ia = (const int *)a;
  const int *ib = (const int *)b;
  if (*ia < *ib)
    return -1;
  if (*ia > *ib)
    return 1;
  return 0;
}

static size_t cu_SkipList_random_level(cu_SkipList *list) {
  size_t lvl = 1;
  while ((cu_State_next(&list->state) & 0xFFFF) < 0x8000 &&
         lvl < list->max_level) {
    lvl++;
  }
  return lvl;
}

static cu_SkipList_Error_Optional cu_SkipList_alloc_node(cu_SkipList *list,
    size_t level, void *key, void *value, struct cu_SkipList_Node **out) {
  size_t fwd_sz = level * sizeof(struct cu_SkipList_Node *);
  size_t node_sz = sizeof(struct cu_SkipList_Node) + fwd_sz;
  cu_IoSlice_Result mem = cu_Allocator_Alloc(list->allocator,
      cu_Layout_create(node_sz, alignof(struct cu_SkipList_Node)));
  if (!cu_IoSlice_Result_is_ok(&mem)) {
    return cu_SkipList_Error_Optional_some(CU_SKIPLIST_ERROR_OOM);
  }
  struct cu_SkipList_Node *node = (struct cu_SkipList_Node *)mem.value.ptr;
  node->forward =
      (struct cu_SkipList_Node **)((unsigned char *)node + sizeof(*node));
  node->level = level;
  for (size_t i = 0; i < level; ++i) {
    node->forward[i] = NULL;
  }

  cu_IoSlice_Result k = cu_Allocator_Alloc(list->allocator,
      cu_Layout_create(list->key_layout.elem_size, list->key_layout.alignment));
  if (!cu_IoSlice_Result_is_ok(&k)) {
    cu_Allocator_Free(list->allocator, mem.value);
    return cu_SkipList_Error_Optional_some(CU_SKIPLIST_ERROR_OOM);
  }
  cu_Memory_memcpy(
      k.value.ptr, cu_Slice_create(key, list->key_layout.elem_size));
  node->key = k.value.ptr;

  cu_IoSlice_Result v = cu_Allocator_Alloc(
      list->allocator, cu_Layout_create(list->value_layout.elem_size,
                           list->value_layout.alignment));
  if (!cu_IoSlice_Result_is_ok(&v)) {
    cu_Allocator_Free(list->allocator, mem.value);
    cu_Allocator_Free(list->allocator, k.value);
    return cu_SkipList_Error_Optional_some(CU_SKIPLIST_ERROR_OOM);
  }
  cu_Memory_memcpy(
      v.value.ptr, cu_Slice_create(value, list->value_layout.elem_size));
  node->value = v.value.ptr;

  *out = node;
  return cu_SkipList_Error_Optional_none();
}

cu_SkipList_Result cu_SkipList_create(cu_Allocator allocator,
    cu_Layout key_layout, cu_Layout value_layout, size_t max_level,
    cu_SkipList_CmpFn_Optional cmp, cu_Destructor_Optional key_destructor,
    cu_Destructor_Optional value_destructor, cu_State state) {
  CU_LAYOUT_CHECK(key_layout) {
    return cu_SkipList_Result_error(CU_SKIPLIST_ERROR_INVALID_LAYOUT);
  }
  CU_LAYOUT_CHECK(value_layout) {
    return cu_SkipList_Result_error(CU_SKIPLIST_ERROR_INVALID_LAYOUT);
  }
  if (max_level == 0) {
    return cu_SkipList_Result_error(CU_SKIPLIST_ERROR_INVALID);
  }

  size_t head_size = sizeof(struct cu_SkipList_Node) +
                     max_level * sizeof(struct cu_SkipList_Node *);
  cu_IoSlice_Result mem = cu_Allocator_Alloc(
      allocator, cu_Layout_create(head_size, alignof(struct cu_SkipList_Node)));
  if (!cu_IoSlice_Result_is_ok(&mem)) {
    return cu_SkipList_Result_error(CU_SKIPLIST_ERROR_OOM);
  }
  struct cu_SkipList_Node *head = (struct cu_SkipList_Node *)mem.value.ptr;
  head->forward =
      (struct cu_SkipList_Node **)((unsigned char *)head + sizeof(*head));
  head->level = max_level;
  for (size_t i = 0; i < max_level; ++i) {
    head->forward[i] = NULL;
  }
  head->key = NULL;
  head->value = NULL;

  cu_SkipList list = {0};
  list.head = head;
  list.level = 1;
  list.max_level = max_level;
  list.cmp = cu_SkipList_default_cmp;
  if (cu_SkipList_CmpFn_Optional_is_some(&cmp)) {
    list.cmp = cu_SkipList_CmpFn_Optional_unwrap(&cmp);
  }
  list.key_layout = key_layout;
  list.value_layout = value_layout;
  list.allocator = allocator;
  list.key_destructor = key_destructor;
  list.value_destructor = value_destructor;
  list.state = state;
  return cu_SkipList_Result_ok(list);
}

void cu_SkipList_destroy(cu_SkipList *list) {
  if (!list)
    return;
  struct cu_SkipList_Node *node = list->head->forward[0];
  while (node) {
    struct cu_SkipList_Node *next = node->forward[0];
    if (cu_Destructor_Optional_is_some(&list->key_destructor)) {
      cu_Destructor kd = cu_Destructor_Optional_unwrap(&list->key_destructor);
      kd(node->key);
    }
    if (cu_Destructor_Optional_is_some(&list->value_destructor)) {
      cu_Destructor vd = cu_Destructor_Optional_unwrap(&list->value_destructor);
      vd(node->value);
    }
    cu_Allocator_Free(list->allocator,
        cu_Slice_create(node->key, list->key_layout.elem_size));
    cu_Allocator_Free(list->allocator,
        cu_Slice_create(node->value, list->value_layout.elem_size));
    cu_Allocator_Free(list->allocator,
        cu_Slice_create(
            node, sizeof(struct cu_SkipList_Node) +
                      node->level * sizeof(struct cu_SkipList_Node *)));
    node = next;
  }
  cu_Allocator_Free(list->allocator,
      cu_Slice_create(
          list->head, sizeof(struct cu_SkipList_Node) +
                          list->max_level * sizeof(struct cu_SkipList_Node *)));
  list->head = NULL;
  list->level = 0;
  list->max_level = 0;
}

cu_SkipList_Error_Optional cu_SkipList_insert(
    cu_SkipList *list, void *key, void *value) {
  CU_IF_NULL(list) {
    return cu_SkipList_Error_Optional_some(CU_SKIPLIST_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(list->key_layout) {
    return cu_SkipList_Error_Optional_some(CU_SKIPLIST_ERROR_INVALID_LAYOUT);
  }
  CU_LAYOUT_CHECK(list->value_layout) {
    return cu_SkipList_Error_Optional_some(CU_SKIPLIST_ERROR_INVALID_LAYOUT);
  }
  struct cu_SkipList_Node *update[list->max_level];
  struct cu_SkipList_Node *x = list->head;
  for (size_t i = list->level; i-- > 0;) {
    while (x->forward[i] && list->cmp(x->forward[i]->key, key) < 0) {
      x = x->forward[i];
    }
    update[i] = x;
  }
  x = x->forward[0];
  if (x && list->cmp(x->key, key) == 0) {
    if (cu_Destructor_Optional_is_some(&list->value_destructor)) {
      cu_Destructor vd = cu_Destructor_Optional_unwrap(&list->value_destructor);
      vd(x->value);
    }
    cu_Memory_memcpy(
        x->value, cu_Slice_create(value, list->value_layout.elem_size));
    return cu_SkipList_Error_Optional_none();
  }
  size_t lvl = cu_SkipList_random_level(list);
  if (lvl > list->level) {
    for (size_t i = list->level; i < lvl; ++i) {
      update[i] = list->head;
    }
    list->level = lvl;
  }
  struct cu_SkipList_Node *node = NULL;
  cu_SkipList_Error_Optional err =
      cu_SkipList_alloc_node(list, lvl, key, value, &node);
  if (cu_SkipList_Error_Optional_is_some(&err)) {
    return err;
  }
  for (size_t i = 0; i < lvl; ++i) {
    node->forward[i] = update[i]->forward[i];
    update[i]->forward[i] = node;
  }
  return cu_SkipList_Error_Optional_none();
}

Ptr_Optional cu_SkipList_find(const cu_SkipList *list, const void *key) {
  CU_IF_NULL(list) { return Ptr_Optional_none(); }
  struct cu_SkipList_Node *x = list->head;
  for (size_t i = list->level; i-- > 0;) {
    while (x->forward[i] && list->cmp(x->forward[i]->key, key) < 0) {
      x = x->forward[i];
    }
  }
  x = x->forward[0];
  if (x && list->cmp(x->key, key) == 0) {
    return Ptr_Optional_some(x->value);
  }
  return Ptr_Optional_none();
}

cu_SkipList_Error_Optional cu_SkipList_remove(
    cu_SkipList *list, const void *key) {
  CU_IF_NULL(list) {
    return cu_SkipList_Error_Optional_some(CU_SKIPLIST_ERROR_INVALID);
  }
  struct cu_SkipList_Node *update[list->max_level];
  struct cu_SkipList_Node *x = list->head;
  for (size_t i = list->level; i-- > 0;) {
    while (x->forward[i] && list->cmp(x->forward[i]->key, key) < 0) {
      x = x->forward[i];
    }
    update[i] = x;
  }
  x = x->forward[0];
  if (!x || list->cmp(x->key, key) != 0) {
    return cu_SkipList_Error_Optional_some(CU_SKIPLIST_ERROR_INVALID);
  }
  for (size_t i = 0; i < list->level; ++i) {
    if (update[i]->forward[i] != x) {
      break;
    }
    update[i]->forward[i] = x->forward[i];
  }
  while (list->level > 1 && list->head->forward[list->level - 1] == NULL) {
    list->level--;
  }
  if (cu_Destructor_Optional_is_some(&list->key_destructor)) {
    cu_Destructor kd = cu_Destructor_Optional_unwrap(&list->key_destructor);
    kd(x->key);
  }
  if (cu_Destructor_Optional_is_some(&list->value_destructor)) {
    cu_Destructor vd = cu_Destructor_Optional_unwrap(&list->value_destructor);
    vd(x->value);
  }
  cu_Allocator_Free(
      list->allocator, cu_Slice_create(x->key, list->key_layout.elem_size));
  cu_Allocator_Free(
      list->allocator, cu_Slice_create(x->value, list->value_layout.elem_size));
  cu_Allocator_Free(list->allocator,
      cu_Slice_create(x, sizeof(struct cu_SkipList_Node) +
                             x->level * sizeof(struct cu_SkipList_Node *)));
  return cu_SkipList_Error_Optional_none();
}

bool cu_SkipList_iter(const cu_SkipList *list, struct cu_SkipList_Node **node,
    void **key, void **value) {
  CU_IF_NULL(list) { return false; }
  CU_IF_NULL(node) { return false; }
  CU_IF_NULL(key) { return false; }
  CU_IF_NULL(value) { return false; }

  struct cu_SkipList_Node *cur;
  if (*node) {
    cur = (*node)->forward[0];
  } else {
    cur = list->head->forward[0];
  }
  if (!cur) {
    return false;
  }
  *node = cur;
  *key = cur->key;
  *value = cur->value;
  return true;
}
