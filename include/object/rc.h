#pragma once

/** @file rc.h Reference counted pointer utilities. */

#include "memory/allocator.h"
#include "object/optional.h"
#include "object/result.h"
#include "utility.h"
#include <stdalign.h>

#define CU_RC_NAME(NAME) NAME##_Rc
/** Construct a helper function name for the rc type. */
#define CU_RC_FN(NAME, SUFFIX) CU_CONCAT(CU_RC_NAME(NAME), SUFFIX)
/** Optional type helper names for rc. */
#define CU_RC_OPTIONAL_NAME(NAME) CU_CONCAT(CU_RC_NAME(NAME), _Optional)
#define CU_RC_OPTIONAL_FN(NAME, SUFFIX)                                        \
  CU_CONCAT(CU_RC_OPTIONAL_NAME(NAME), SUFFIX)
/** Destructor optional helper names. */
#define CU_RC_DESTRUCTOR_OPTIONAL_NAME(NAME)                                   \
  CU_CONCAT(CU_RC_DESTRUCTOR_NAME(NAME), _Optional)
#define CU_RC_DESTRUCTOR_OPTIONAL_FN(NAME, SUFFIX)                             \
  CU_OPTIONAL_FN(CU_RC_DESTRUCTOR_NAME(NAME), SUFFIX)

/** Container holding the value and reference count. */
#define CU_RC_DESTRUCTOR_NAME(NAME) CU_RC_FN(NAME, _Destructor)
#define CU_RC_CONTAINER(NAME, T)                                               \
  typedef void (*CU_RC_DESTRUCTOR_NAME(NAME))(T *);                            \
  CU_OPTIONAL_DECL(CU_RC_DESTRUCTOR_NAME(NAME), CU_RC_DESTRUCTOR_NAME(NAME))   \
  struct CU_RC_FN(NAME, _Container) {                                          \
    T item;                                                                    \
    size_t ref_count;                                                          \
    CU_RC_DESTRUCTOR_OPTIONAL_NAME(NAME) destructor;                           \
  };

/** Declare functions for a reference counted type. */
#define CU_RC_RESULT_NAME(NAME) NAME##_Rc_Result

#define CU_RC_HEADER(NAME, T)                                                  \
  CU_RC_RESULT_NAME(NAME)                                                      \
  CU_RC_FN(NAME, _create)(cu_Allocator alloc, T value,                         \
      CU_RC_DESTRUCTOR_OPTIONAL_NAME(NAME) destructor);                        \
  CU_RC_OPTIONAL_NAME(NAME)                                                    \
  CU_RC_FN(NAME, _clone)(const CU_RC_NAME(NAME) * rc);                         \
  void CU_RC_FN(NAME, _destroy)(CU_RC_NAME(NAME) * rc);                        \
  T *CU_RC_FN(NAME, _get)(const CU_RC_NAME(NAME) * rc);

/** Declare the rc struct and its helpers. */
#define CU_RC_DECL(NAME, T)                                                    \
  CU_RC_CONTAINER(NAME, T)                                                     \
  typedef struct {                                                             \
    struct CU_RC_FN(NAME, _Container) * inner;                                 \
    cu_Allocator alloc;                                                        \
  } CU_RC_NAME(NAME);                                                          \
  CU_OPTIONAL_DECL(CU_RC_NAME(NAME), CU_RC_NAME(NAME))                         \
  CU_RESULT_DECL(CU_RC_NAME(NAME), CU_RC_NAME(NAME), cu_Io_Error)              \
  CU_RC_HEADER(NAME, T)

/** Define the implementation for the rc helpers. */
#define CU_RC_IMPL(NAME, T)                                                    \
  CU_OPTIONAL_IMPL(CU_RC_NAME(NAME), CU_RC_NAME(NAME))                         \
  CU_OPTIONAL_IMPL(CU_RC_DESTRUCTOR_NAME(NAME), CU_RC_DESTRUCTOR_NAME(NAME))   \
  CU_RESULT_IMPL(CU_RC_NAME(NAME), CU_RC_NAME(NAME), cu_Io_Error)              \
  CU_RC_RESULT_NAME(NAME)                                                      \
  CU_RC_FN(NAME, _create)(cu_Allocator alloc, T value,                         \
      CU_RC_DESTRUCTOR_OPTIONAL_NAME(NAME) destructor) {                       \
    cu_IoSlice_Result mem = cu_Allocator_Alloc(                                \
        alloc, CU_LAYOUT(struct CU_RC_FN(NAME, _Container)));                  \
    if (!cu_IoSlice_Result_is_ok(&mem)) {                                      \
      return CU_RESULT_FN(CU_RC_NAME(NAME), _error)(                           \
          cu_IoSlice_Result_unwrap_error(&mem));                               \
    }                                                                          \
    struct CU_RC_FN(NAME, _Container) *cont =                                  \
        (struct CU_RC_FN(NAME, _Container) *)cu_IoSlice_Result_unwrap(&mem)    \
            .ptr;                                                              \
    cont->item = value;                                                        \
    cont->ref_count = 1;                                                       \
    cont->destructor = destructor;                                             \
    CU_RC_NAME(NAME) rc = {cont, alloc};                                       \
    return CU_RESULT_FN(CU_RC_NAME(NAME), _ok)(rc);                            \
  }                                                                            \
  CU_RC_OPTIONAL_NAME(NAME)                                                    \
  CU_RC_FN(NAME, _clone)(const CU_RC_NAME(NAME) * rc) {                        \
    CU_IF_NULL(rc) { return CU_RC_OPTIONAL_FN(NAME, _none)(); }                \
    CU_IF_NULL(rc->inner) { return CU_RC_OPTIONAL_FN(NAME, _none)(); }         \
    rc->inner->ref_count++;                                                    \
    CU_RC_NAME(NAME) clone = {rc->inner, rc->alloc};                           \
    return CU_RC_OPTIONAL_FN(NAME, _some)(clone);                              \
  }                                                                            \
  void CU_RC_FN(NAME, _destroy)(CU_RC_NAME(NAME) * rc) {                       \
    CU_IF_NULL(rc) { return; }                                                 \
    if (--rc->inner->ref_count == 0) {                                         \
      if (CU_RC_DESTRUCTOR_OPTIONAL_FN(NAME, _is_some)(                        \
              &rc->inner->destructor)) {                                       \
        CU_RC_DESTRUCTOR_NAME(NAME)                                            \
        dtor = CU_RC_DESTRUCTOR_OPTIONAL_FN(NAME, _unwrap)(                    \
            &rc->inner->destructor);                                           \
        dtor(&rc->inner->item);                                                \
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
