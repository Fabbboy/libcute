#pragma once

/** @file dlist.h Doubly linked list container. */

#include "macro.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include "object/result.h"
#include "utility.h"
#include <nostd.h>
#include <stddef.h>

typedef struct cu_DList_Node {
  struct cu_DList_Node *prev;
  struct cu_DList_Node *next;
  unsigned char data[];
} cu_DList_Node;

typedef struct {
  cu_DList_Node *head;
  cu_DList_Node *tail;
  size_t length;
  cu_Layout layout;
  cu_Allocator allocator;
} cu_DList;

typedef enum {
  CU_DLIST_ERROR_NONE = 0,
  CU_DLIST_ERROR_OOM,
  CU_DLIST_ERROR_INVALID_LAYOUT,
  CU_DLIST_ERROR_INVALID,
  CU_DLIST_ERROR_EMPTY,
} cu_DList_Error;

CU_RESULT_DECL(cu_DList, cu_DList, cu_DList_Error)
CU_OPTIONAL_DECL(cu_DList_Error, cu_DList_Error)

cu_DList_Result cu_DList_create(cu_Allocator allocator, cu_Layout layout);
void cu_DList_destroy(cu_DList *list);

static inline size_t cu_DList_size(const cu_DList *list) {
  CU_IF_NULL(list) { return 0; }
  return list->length;
}

static inline bool cu_DList_is_empty(const cu_DList *list) {
  CU_IF_NULL(list) { return true; }
  return list->length == 0;
}

cu_DList_Error_Optional cu_DList_push_front(cu_DList *list, void *elem);
cu_DList_Error_Optional cu_DList_push_back(cu_DList *list, void *elem);
cu_DList_Error_Optional cu_DList_pop_front(cu_DList *list, void *out_elem);
cu_DList_Error_Optional cu_DList_pop_back(cu_DList *list, void *out_elem);
cu_DList_Error_Optional cu_DList_insert_after(
    cu_DList *list, cu_DList_Node *pos, void *elem);
cu_DList_Error_Optional cu_DList_insert_before(
    cu_DList *list, cu_DList_Node *pos, void *elem);

bool cu_DList_iter(const cu_DList *list, cu_DList_Node **node, void **out_elem);
