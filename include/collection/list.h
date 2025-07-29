#pragma once

/** @file list.h Singly linked list container. */

#include "macro.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include "object/result.h"
#include "utility.h"
#include <nostd.h>
#include <stddef.h>

/**
 * @brief Node within a singly linked list.
 */
typedef struct cu_List_Node {
  struct cu_List_Node *next; /**< pointer to the next element */
  unsigned char data[];      /**< inlined element storage */
} cu_List_Node;

/**
 * @brief Singly linked list container.
 */
typedef struct {
  cu_List_Node *head; /**< first node in the list */
  size_t length;      /**< number of elements */
  cu_Layout layout;   /**< layout of each element */
  cu_Allocator allocator; /**< allocator used for storage */
} cu_List;

/**
 * @brief Error codes returned by list operations.
 */
typedef enum {
  CU_LIST_ERROR_NONE = 0,       /**< success */
  CU_LIST_ERROR_OOM,            /**< out of memory */
  CU_LIST_ERROR_INVALID_LAYOUT, /**< invalid element layout */
  CU_LIST_ERROR_INVALID,        /**< invalid argument */
  CU_LIST_ERROR_EMPTY,          /**< list has no elements */
} cu_List_Error;

CU_RESULT_DECL(cu_List, cu_List, cu_List_Error)
CU_OPTIONAL_DECL(cu_List_Error, cu_List_Error)

/**
 * @brief Create an empty list.
 *
 * @param allocator allocator used for node storage
 * @param layout layout describing each element
 * @return Result containing the created list on success
 */
cu_List_Result cu_List_create(cu_Allocator allocator, cu_Layout layout);

/**
 * @brief Destroy a list and free all nodes.
 */
void cu_List_destroy(cu_List *list);

static inline size_t cu_List_size(const cu_List *list) {
  CU_IF_NULL(list) { return 0; }
  return list->length;
}

static inline bool cu_List_is_empty(const cu_List *list) {
  CU_IF_NULL(list) { return true; }
  return list->length == 0;
}

/** Add an element to the front of the list. */
cu_List_Error_Optional cu_List_push_front(cu_List *list, void *elem);
/** Remove the first element and copy it into @p out_elem. */
cu_List_Error_Optional cu_List_pop_front(cu_List *list, void *out_elem);
/** Insert a new element after @p node. */
cu_List_Error_Optional cu_List_insert_after(
    cu_List *list, cu_List_Node *node, void *elem);
/** Insert a new element before @p node. */
cu_List_Error_Optional cu_List_insert_before(
    cu_List *list, cu_List_Node *node, void *elem);

/**
 * @brief Iterate over the list.
 *
 * The iteration state is held in @p node. Pass NULL for the first call.
 * @param list list to iterate
 * @param node current node, updated on success
 * @param out_elem pointer receiving the element data
 * @return true when another element was produced
 */
bool cu_List_iter(const cu_List *list, cu_List_Node **node, void **out_elem);
