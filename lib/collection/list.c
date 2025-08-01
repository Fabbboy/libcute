#include "collection/list.h"
#include <nostd.h>
#include <stdalign.h>

CU_RESULT_IMPL(cu_List, cu_List, cu_List_Error)
CU_OPTIONAL_IMPL(cu_List_Error, cu_List_Error)

cu_List_Result cu_List_create(cu_Allocator allocator, cu_Layout layout,
    cu_Destructor_Optional destructor) {
  CU_LAYOUT_CHECK(layout) {
    return cu_List_Result_error(CU_LIST_ERROR_INVALID_LAYOUT);
  }

  cu_List list = {0};
  list.head = NULL;
  list.length = 0;
  list.layout = layout;
  list.allocator = allocator;
  list.destructor = destructor;
  return cu_List_Result_ok(list);
}

void cu_List_destroy(cu_List *list) {
  if (!list) {
    return;
  }
  struct cu_List_Node *n = list->head;
  while (n) {
    struct cu_List_Node *next = n->next;
    if (cu_Destructor_Optional_is_some(&list->destructor)) {
      cu_Destructor dtor = cu_Destructor_Optional_unwrap(&list->destructor);
      dtor(n->data);
    }
    cu_Allocator_Free(list->allocator,
        cu_Slice_create(
            n, sizeof(struct cu_List_Node) + list->layout.elem_size));
    n = next;
  }
  list->head = NULL;
  list->length = 0;
}

cu_List_Error_Optional cu_List_push_front(cu_List *list, void *elem) {
  CU_IF_NULL(list) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(list->layout) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_INVALID_LAYOUT);
  }
  size_t size = sizeof(struct cu_List_Node) + list->layout.elem_size;
  cu_IoSlice_Result mem = cu_Allocator_Alloc(
      list->allocator, cu_Layout_create(size, alignof(struct cu_List_Node)));
  if (!cu_IoSlice_Result_is_ok(&mem)) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_OOM);
  }
  struct cu_List_Node *node = (struct cu_List_Node *)mem.value.ptr;
  node->next = list->head;
  cu_Memory_memcpy(
      node->data, cu_Slice_create((void *)elem, list->layout.elem_size));
  list->head = node;
  list->length++;
  return cu_List_Error_Optional_none();
}

cu_List_Error_Optional cu_List_pop_front(cu_List *list, void *out_elem) {
  CU_IF_NULL(list) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(list->layout) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_INVALID_LAYOUT);
  }
  if (list->length == 0) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_EMPTY);
  }
  struct cu_List_Node *node = list->head;
  cu_Memory_memcpy(
      out_elem, cu_Slice_create(node->data, list->layout.elem_size));
  list->head = node->next;
  list->length--;
  if (cu_Destructor_Optional_is_some(&list->destructor)) {
    cu_Destructor dtor = cu_Destructor_Optional_unwrap(&list->destructor);
    dtor(node->data);
  }
  cu_Allocator_Free(list->allocator,
      cu_Slice_create(
          node, sizeof(struct cu_List_Node) + list->layout.elem_size));
  return cu_List_Error_Optional_none();
}

cu_List_Error_Optional cu_List_insert_after(
    cu_List *list, struct cu_List_Node *pos, void *elem) {
  CU_IF_NULL(list) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(list->layout) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_INVALID_LAYOUT);
  }

  size_t size = sizeof(struct cu_List_Node) + list->layout.elem_size;
  cu_IoSlice_Result mem = cu_Allocator_Alloc(
      list->allocator, cu_Layout_create(size, alignof(struct cu_List_Node)));
  if (!cu_IoSlice_Result_is_ok(&mem)) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_OOM);
  }

  struct cu_List_Node *node = (struct cu_List_Node *)mem.value.ptr;
  cu_Memory_memcpy(
      node->data, cu_Slice_create((void *)elem, list->layout.elem_size));

  if (!pos) {
    node->next = list->head;
    list->head = node;
  } else {
    node->next = pos->next;
    pos->next = node;
  }

  list->length++;
  return cu_List_Error_Optional_none();
}

cu_List_Error_Optional cu_List_insert_before(
    cu_List *list, struct cu_List_Node *pos, void *elem) {
  if (!pos) {
    return cu_List_insert_after(list, NULL, elem);
  }
  CU_IF_NULL(list) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(list->layout) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_INVALID_LAYOUT);
  }

  if (pos == list->head) {
    return cu_List_insert_after(list, NULL, elem);
  }

  struct cu_List_Node *prev = list->head;
  while (prev && prev->next != pos) {
    prev = prev->next;
  }
  if (!prev) {
    return cu_List_Error_Optional_some(CU_LIST_ERROR_INVALID);
  }
  return cu_List_insert_after(list, prev, elem);
}

bool cu_List_iter(
    const cu_List *list, struct cu_List_Node **node, void **out_elem) {
  CU_IF_NULL(list) { return false; }
  CU_IF_NULL(node) { return false; }
  CU_IF_NULL(out_elem) { return false; }
  CU_LAYOUT_CHECK(list->layout) { return false; }

  struct cu_List_Node *cur;
  if (*node) {
    cur = (*node)->next;
  } else {
    cur = list->head;
  }
  if (!cur) {
    return false;
  }
  *node = cur;
  *out_elem = cur->data;
  return true;
}
