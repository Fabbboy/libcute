#include "collection/dlist.h"
#include <nostd.h>
#include "string/nostd.h"
#include <stdalign.h>

CU_RESULT_IMPL(cu_DList, cu_DList, cu_DList_Error)
CU_OPTIONAL_IMPL(cu_DList_Error, cu_DList_Error)

cu_DList_Result cu_DList_create(cu_Allocator allocator, cu_Layout layout) {
  CU_LAYOUT_CHECK(layout) {
    return cu_DList_result_error(CU_DLIST_ERROR_INVALID_LAYOUT);
  }
  cu_DList list;
  list.head = NULL;
  list.tail = NULL;
  list.length = 0;
  list.layout = layout;
  list.allocator = allocator;
  return cu_DList_result_ok(list);
}

void cu_DList_destroy(cu_DList *list) {
  if (!list) {
    return;
  }
  cu_DList_Node *n = list->head;
  while (n) {
    cu_DList_Node *next = n->next;
    cu_Allocator_Free(list->allocator,
        cu_Slice_create(n, sizeof(cu_DList_Node) + list->layout.elem_size));
    n = next;
  }
  list->head = NULL;
  list->tail = NULL;
  list->length = 0;
}

static cu_DList_Error_Optional cu_DList_alloc_node(
    cu_DList *list, void *elem, cu_DList_Node **out_node) {
  size_t size = sizeof(cu_DList_Node) + list->layout.elem_size;
  cu_Slice_Result mem =
      cu_Allocator_Alloc(list->allocator, size, alignof(cu_DList_Node));
  if (!cu_Slice_result_is_ok(&mem)) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_OOM);
  }
  cu_DList_Node *node = (cu_DList_Node *)mem.value.ptr;
  memcpy(node->data, elem, list->layout.elem_size);
  node->prev = NULL;
  node->next = NULL;
  *out_node = node;
  return cu_DList_Error_Optional_none();
}

cu_DList_Error_Optional cu_DList_push_front(cu_DList *list, void *elem) {
  CU_IF_NULL(list) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(list->layout) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID_LAYOUT);
  }
  cu_DList_Node *node = NULL;
  cu_DList_Error_Optional err = cu_DList_alloc_node(list, elem, &node);
  if (cu_DList_Error_Optional_is_some(&err)) {
    return err;
  }
  node->next = list->head;
  if (list->head) {
    list->head->prev = node;
  } else {
    list->tail = node;
  }
  list->head = node;
  list->length++;
  return cu_DList_Error_Optional_none();
}

cu_DList_Error_Optional cu_DList_push_back(cu_DList *list, void *elem) {
  CU_IF_NULL(list) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(list->layout) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID_LAYOUT);
  }
  cu_DList_Node *node = NULL;
  cu_DList_Error_Optional err = cu_DList_alloc_node(list, elem, &node);
  if (cu_DList_Error_Optional_is_some(&err)) {
    return err;
  }
  node->prev = list->tail;
  if (list->tail) {
    list->tail->next = node;
  } else {
    list->head = node;
  }
  list->tail = node;
  list->length++;
  return cu_DList_Error_Optional_none();
}

cu_DList_Error_Optional cu_DList_pop_front(cu_DList *list, void *out_elem) {
  CU_IF_NULL(list) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(list->layout) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID_LAYOUT);
  }
  if (list->length == 0) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_EMPTY);
  }
  cu_DList_Node *node = list->head;
  memcpy(out_elem, node->data, list->layout.elem_size);
  list->head = node->next;
  if (list->head) {
    list->head->prev = NULL;
  } else {
    list->tail = NULL;
  }
  list->length--;
  cu_Allocator_Free(list->allocator,
      cu_Slice_create(node, sizeof(cu_DList_Node) + list->layout.elem_size));
  return cu_DList_Error_Optional_none();
}

cu_DList_Error_Optional cu_DList_pop_back(cu_DList *list, void *out_elem) {
  CU_IF_NULL(list) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(list->layout) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID_LAYOUT);
  }
  if (list->length == 0) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_EMPTY);
  }
  cu_DList_Node *node = list->tail;
  memcpy(out_elem, node->data, list->layout.elem_size);
  list->tail = node->prev;
  if (list->tail) {
    list->tail->next = NULL;
  } else {
    list->head = NULL;
  }
  list->length--;
  cu_Allocator_Free(list->allocator,
      cu_Slice_create(node, sizeof(cu_DList_Node) + list->layout.elem_size));
  return cu_DList_Error_Optional_none();
}

cu_DList_Error_Optional cu_DList_insert_after(
    cu_DList *list, cu_DList_Node *pos, void *elem) {
  CU_IF_NULL(list) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(list->layout) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID_LAYOUT);
  }
  cu_DList_Node *node = NULL;
  cu_DList_Error_Optional err = cu_DList_alloc_node(list, elem, &node);
  if (cu_DList_Error_Optional_is_some(&err)) {
    return err;
  }

  if (!pos) {
    node->next = list->head;
    if (list->head) {
      list->head->prev = node;
    } else {
      list->tail = node;
    }
    list->head = node;
  } else {
    node->next = pos->next;
    node->prev = pos;
    if (pos->next) {
      pos->next->prev = node;
    } else {
      list->tail = node;
    }
    pos->next = node;
  }

  list->length++;
  return cu_DList_Error_Optional_none();
}

cu_DList_Error_Optional cu_DList_insert_before(
    cu_DList *list, cu_DList_Node *pos, void *elem) {
  if (!pos) {
    return cu_DList_insert_after(list, NULL, elem);
  }
  CU_IF_NULL(list) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(list->layout) {
    return cu_DList_Error_Optional_some(CU_DLIST_ERROR_INVALID_LAYOUT);
  }

  cu_DList_Node *node = NULL;
  cu_DList_Error_Optional err = cu_DList_alloc_node(list, elem, &node);
  if (cu_DList_Error_Optional_is_some(&err)) {
    return err;
  }

  node->prev = pos->prev;
  node->next = pos;
  if (pos->prev) {
    pos->prev->next = node;
  } else {
    list->head = node;
  }
  pos->prev = node;
  list->length++;
  return cu_DList_Error_Optional_none();
}

bool cu_DList_iter(
    const cu_DList *list, cu_DList_Node **node, void **out_elem) {
  CU_IF_NULL(list) { return false; }
  CU_IF_NULL(node) { return false; }
  CU_IF_NULL(out_elem) { return false; }
  CU_LAYOUT_CHECK(list->layout) { return false; }

  cu_DList_Node *cur = *node ? (*node)->next : list->head;
  if (!cur) {
    return false;
  }
  *node = cur;
  *out_elem = cur->data;
  return true;
}
