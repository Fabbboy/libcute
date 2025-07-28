#pragma once

/** @file rc.h Reference counted pointer utilities. */

#include "memory/allocator.h"
#include "object/result.h"
#include "utility.h"
#include <stdalign.h>

#define CU_RC_NAME(NAME) NAME##_Rc
/** Construct a helper function name for the rc type. */
#define CU_RC_FN(NAME, SUFFIX) CU_CONCAT(CU_RC_NAME(NAME), SUFFIX)

/** Container holding the value and reference count. */
#define CU_RC_DESTRUCTOR_NAME(NAME) CU_RC_FN(NAME, Destructor)
#define CU_RC_CONTAINER(NAME, T)                                               \
  typedef void (*CU_RC_DESTRUCTOR_NAME(NAME))(T *);                            \
  struct CU_RC_FN(NAME, Container) {                                           \
    T item;                                                                    \
    size_t ref_count;                                                          \
    CU_RC_DESTRUCTOR_NAME(NAME) destructor;                                    \
  };

/** Result type for RC creation using existing result system */
#define CU_RC_RESULT_NAME(NAME) NAME##_Rc_Result

/** Declare functions for a reference counted type. */
#define CU_RC_HEADER(NAME, T)                                                  \
  CU_RC_RESULT_NAME(NAME)                                                      \
  CU_CONCAT(cu_, CU_RC_FN(NAME, _create))(                                     \
      cu_Allocator alloc, T value, CU_RC_DESTRUCTOR_NAME(NAME) destructor);    \
  CU_RC_NAME(NAME) CU_CONCAT(cu_, CU_RC_FN(NAME, _clone))(const CU_RC_NAME(NAME) * rc); \
  void CU_CONCAT(cu_, CU_RC_FN(NAME, _destroy))(CU_RC_NAME(NAME) * rc);        \
  T *CU_CONCAT(cu_, CU_RC_FN(NAME, _get))(const CU_RC_NAME(NAME) * rc);

/** Declare the rc struct and its helpers. */
#define CU_RC_DECL(NAME, T)                                                    \
  CU_RC_CONTAINER(NAME, T)                                                     \
  typedef struct {                                                             \
    struct CU_RC_FN(NAME, Container) * inner;                                  \
    cu_Allocator alloc;                                                        \
  } CU_RC_NAME(NAME);                                                          \
  CU_RESULT_DECL(CU_RC_FN(NAME, Result), CU_RC_NAME(NAME), cu_IoError)        \
  CU_RC_HEADER(NAME, T)

/** Define the implementation for the rc helpers. */
#define CU_RC_IMPL(NAME, T)                                                    \
  CU_RESULT_IMPL(CU_RC_FN(NAME, Result), CU_RC_NAME(NAME), cu_IoError)        \
  CU_RC_RESULT_NAME(NAME)                                                      \
  CU_RC_FN(NAME, _create)(                                                     \
      cu_Allocator alloc, T value, CU_RC_DESTRUCTOR_NAME(NAME) destructor) {   \
    cu_Slice_Result mem =                                                      \
        cu_Allocator_Alloc(alloc, CU_LAYOUT(struct CU_RC_FN(NAME, Container))); \
    if (!cu_Slice_Result_is_ok(&mem)) {                                        \
      return CU_RC_FN(NAME, Result_error)(cu_Slice_Result_unwrap_error(&mem)); \
    }                                                                          \
    struct CU_RC_FN(NAME, Container) *cont =                                   \
        (struct CU_RC_FN(NAME, Container) *)cu_Slice_Result_unwrap(&mem).ptr;  \
    cont->item = value;                                                        \
    cont->ref_count = 1;                                                       \
    cont->destructor = destructor;                                             \
    CU_RC_NAME(NAME) rc = {cont, alloc};                                       \
    return CU_RC_FN(NAME, Result_ok)(rc);                                      \
  }                                                                            \
  CU_RC_NAME(NAME) CU_RC_FN(NAME, _clone)(const CU_RC_NAME(NAME) * rc) {       \
    if (!rc || !rc->inner) {                                                   \
      CU_RC_NAME(NAME) tmp = {0};                                              \
      return tmp;                                                              \
    }                                                                          \
    rc->inner->ref_count++;                                                    \
    CU_RC_NAME(NAME) clone = {rc->inner, rc->alloc};                           \
    return clone;                                                              \
  }                                                                            \
  void CU_RC_FN(NAME, _destroy)(CU_RC_NAME(NAME) * rc) {                       \
    if (!rc || !rc->inner) {                                                   \
      return;                                                                  \
    }                                                                          \
    if (--rc->inner->ref_count == 0) {                                         \
      if (rc->inner->destructor) {                                             \
        rc->inner->destructor(&rc->inner->item);                               \
      }                                                                        \
      cu_Allocator_Free(                                                       \
          rc->alloc, cu_Slice_create(rc->inner, sizeof(*rc->inner)));          \
    }                                                                          \
    rc->inner = NULL;                                                          \
  }                                                                            \
  T *CU_RC_FN(NAME, _get)(const CU_RC_NAME(NAME) * rc) {                       \
    CU_IF_NULL(rc) { return NULL; }                                            \
    CU_IF_NULL(rc->inner) { return NULL; }                                     \
    return &rc->inner->item;                                                   \
  }