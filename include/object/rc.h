#pragma once

/** @file rc.h Reference counted pointer utilities. */

#include "utility.h"
#include <stdalign.h>

#define CU_RC_NAME(NAME) NAME##_Rc
/** Build the optional type name for a given rc. */
#define CU_RC_OPTIONAL_NAME(NAME) NAME##_Rc_Optional
#define CU_RC_OPTIONAL_FN(NAME, SUFFIX)                                        \
  CU_CONCAT(CU_RC_OPTIONAL_NAME(NAME), SUFFIX)
/** Construct a helper function name for the rc type. */
#define CU_RC_FN(NAME, SUFFIX) CU_CONCAT(CU_RC_NAME(NAME), SUFFIX)

/** Container holding the value and reference count. */
#define CU_RC_DESTRUCTOR_NAME(NAME) CU_RC_FN(NAME, _destructor)
#define CU_RC_CONTAINER(NAME, T)                                               \
  typedef void (*CU_RC_DESTRUCTOR_NAME(NAME))(T *);                            \
  typedef struct {                                                             \
    T item;                                                                    \
    size_t ref_count;                                                          \
    CU_RC_DESTRUCTOR_NAME(NAME) destructor;                                    \
  } CU_RC_FN(NAME, Container);

#define CU_RC_OPTIONAL_DECL(NAME)                                              \
  typedef struct {                                                             \
    CU_RC_NAME(NAME) value;                                                    \
    bool isSome;                                                               \
  } CU_RC_OPTIONAL_NAME(NAME);                                                 \
  CU_RC_OPTIONAL_NAME(NAME)                                                    \
  CU_RC_OPTIONAL_FN(NAME, _some)(CU_RC_NAME(NAME) value);                      \
  CU_RC_OPTIONAL_NAME(NAME) CU_RC_OPTIONAL_FN(NAME, _none)(void);              \
  bool CU_RC_OPTIONAL_FN(NAME, _is_some)(                                      \
      const CU_RC_OPTIONAL_NAME(NAME) * opt);                                  \
  bool CU_RC_OPTIONAL_FN(NAME, _is_none)(                                      \
      const CU_RC_OPTIONAL_NAME(NAME) * opt);                                  \
  CU_RC_NAME(NAME)                                                             \
  CU_RC_OPTIONAL_FN(NAME, _unwrap)(CU_RC_OPTIONAL_NAME(NAME) * opt);

/** Declare functions for a reference counted type. */
#define CU_RC_HEADER(NAME, T)                                                  \
  CU_RC_OPTIONAL_DECL(NAME)                                                    \
  CU_RC_OPTIONAL_NAME(NAME)                                                    \
  CU_RC_FN(NAME, _create)(                                                     \
      cu_Allocator alloc, T value, CU_RC_DESTRUCTOR_NAME(NAME) destructor);    \
  CU_RC_NAME(NAME) CU_RC_FN(NAME, _clone)(const CU_RC_NAME(NAME) * rc);        \
  void CU_RC_FN(NAME, _drop)(CU_RC_NAME(NAME) * rc);                           \
  T *CU_RC_FN(NAME, _get)(const CU_RC_NAME(NAME) * rc);

/** Declare the rc struct and its helpers. */
#define CU_RC_DECL(NAME, T)                                                    \
  CU_RC_CONTAINER(NAME, T)                                                     \
  typedef struct {                                                             \
    CU_RC_FN(NAME, Container) * inner;                                         \
    cu_Allocator alloc;                                                        \
  } CU_RC_NAME(NAME);                                                          \
  CU_RC_HEADER(NAME, T)

/** Define the implementation for the rc helpers. */
#define CU_RC_IMPL(NAME, T)                                                    \
  CU_RC_OPTIONAL_NAME(NAME)                                                    \
  CU_RC_FN(NAME, _create)(                                                     \
      cu_Allocator alloc, T value, CU_RC_DESTRUCTOR_NAME(NAME) destructor) {   \
    cu_Slice_Result mem =                                                      \
        cu_Allocator_Alloc(alloc, CU_LAYOUT(CU_RC_FN(NAME, Container)));       \
    if (!cu_Slice_Result_is_ok(&mem)) {                                        \
      return CU_RC_OPTIONAL_FN(NAME, _none)();                                 \
    }                                                                          \
    CU_RC_FN(NAME, Container) *cont =                                          \
        (CU_RC_FN(NAME, Container) *)mem.value.ptr;                            \
    cont->item = value;                                                        \
    cont->ref_count = 1;                                                       \
    cont->destructor = destructor;                                             \
    CU_RC_NAME(NAME) rc = {cont, alloc};                                       \
    return CU_RC_OPTIONAL_FN(NAME, _some)(rc);                                 \
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
  void CU_RC_FN(NAME, _drop)(CU_RC_NAME(NAME) * rc) {                          \
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
  }                                                                            \
  CU_RC_OPTIONAL_NAME(NAME)                                                    \
  CU_RC_OPTIONAL_FN(NAME, _some)(CU_RC_NAME(NAME) value) {                     \
    CU_RC_OPTIONAL_NAME(NAME) opt = {value, true};                             \
    return opt;                                                                \
  }                                                                            \
  CU_RC_OPTIONAL_NAME(NAME) CU_RC_OPTIONAL_FN(NAME, _none)(void) {             \
    CU_RC_OPTIONAL_NAME(NAME) opt = {{0}, false};                              \
    return opt;                                                                \
  }                                                                            \
  bool CU_RC_OPTIONAL_FN(NAME, _is_some)(                                      \
      const CU_RC_OPTIONAL_NAME(NAME) * opt) {                                 \
    return opt->isSome;                                                        \
  }                                                                            \
  bool CU_RC_OPTIONAL_FN(NAME, _is_none)(                                      \
      const CU_RC_OPTIONAL_NAME(NAME) * opt) {                                 \
    return !opt->isSome;                                                       \
  }                                                                            \
  CU_RC_NAME(NAME)                                                             \
  CU_RC_OPTIONAL_FN(NAME, _unwrap)(CU_RC_OPTIONAL_NAME(NAME) * opt) {          \
    if (!opt->isSome) {                                                        \
      CU_DIE("Attempted to unwrap a None optional");                           \
    }                                                                          \
    return opt->value;                                                         \
  }
