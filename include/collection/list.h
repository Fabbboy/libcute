#pragma once

/** @file list.h Singly linked list container. */

#include "macro.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include "object/result.h"
#include "utility.h"
#include <nostd.h>
#include <stddef.h>

typedef struct cu_List_Node {
  struct cu_List_Node *next;
  unsigned char data[];
} cu_List_Node;

typedef struct {
  cu_List_Node *head;
  size_t length;
  cu_Layout layout;
  cu_Allocator allocator;
} cu_List;

typedef enum {
  CU_LIST_ERROR_NONE = 0,
  CU_LIST_ERROR_OOM,
  CU_LIST_ERROR_INVALID_LAYOUT,
  CU_LIST_ERROR_INVALID,
  CU_LIST_ERROR_EMPTY,
} cu_List_Error;

CU_RESULT_DECL(cu_List, cu_List, cu_List_Error)
CU_OPTIONAL_DECL(cu_List_Error, cu_List_Error)

cu_List_Result cu_List_create(cu_Allocator allocator, cu_Layout layout);
void cu_List_destroy(cu_List *list);

static inline size_t cu_List_size(const cu_List *list) {
  CU_IF_NULL(list) { return 0; }
  return list->length;
}

static inline bool cu_List_is_empty(const cu_List *list) {
  CU_IF_NULL(list) { return true; }
  return list->length == 0;
}

cu_List_Error_Optional cu_List_push_front(cu_List *list, void *elem);
cu_List_Error_Optional cu_List_pop_front(cu_List *list, void *out_elem);
cu_List_Error_Optional cu_List_insert_after(
    cu_List *list, cu_List_Node *node, void *elem);
cu_List_Error_Optional cu_List_insert_before(
    cu_List *list, cu_List_Node *node, void *elem);

bool cu_List_iter(const cu_List *list, cu_List_Node **node, void **out_elem);
