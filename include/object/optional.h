#pragma once

/** @file optional.h Generic optional helper macros. */

#include <stdbool.h>
#include <stdint.h>

#include "macro.h"
#include <nostd.h>

/** Build the concrete optional type name for a category. */
#define CU_OPTIONAL_NAME(NAME) NAME##_Optional
/** Construct an optional helper function name. */
#define CU_OPTIONAL_FN(NAME, SUFFIX) CU_CONCAT(CU_OPTIONAL_NAME(NAME), SUFFIX)

/** Declare optional helper functions for a given type. */
#define CU_OPTIONAL_HEADER(NAME, T)                                            \
  CU_OPTIONAL_NAME(NAME) CU_OPTIONAL_FN(NAME, _some)(T value);                \
  CU_OPTIONAL_NAME(NAME) CU_OPTIONAL_FN(NAME, _none)(void);                   \
  bool CU_OPTIONAL_FN(NAME, _is_some)(const CU_OPTIONAL_NAME(NAME) * opt);    \
  bool CU_OPTIONAL_FN(NAME, _is_none)(const CU_OPTIONAL_NAME(NAME) * opt);    \
  T CU_OPTIONAL_FN(NAME, _unwrap)(CU_OPTIONAL_NAME(NAME) * opt);

/** Declare the optional struct and associated functions. */
#define CU_OPTIONAL_DECL(NAME, T)                                              \
  typedef struct {                                                             \
    T value;                                                                   \
    bool isSome;                                                               \
  } CU_OPTIONAL_NAME(NAME);                                                    \
  CU_OPTIONAL_HEADER(NAME, T)

/** Define the implementation of the optional helpers. */
#define CU_OPTIONAL_IMPL(NAME, T)                                              \
  CU_OPTIONAL_NAME(NAME) CU_OPTIONAL_FN(NAME, _some)(T value) {               \
    CU_OPTIONAL_NAME(NAME) opt;                                                \
    cu_Memory_memset(&opt, 0, sizeof(opt));                                    \
    opt.value = value;                                                         \
    opt.isSome = true;                                                         \
    return opt;                                                                \
  }                                                                            \
                                                                               \
  CU_OPTIONAL_NAME(NAME) CU_OPTIONAL_FN(NAME, _none)(void) {                  \
    CU_OPTIONAL_NAME(NAME) opt;                                                \
    cu_Memory_memset(&opt, 0, sizeof(opt));                                    \
    opt.isSome = false;                                                        \
    return opt;                                                                \
  }                                                                            \
                                                                               \
  bool CU_OPTIONAL_FN(NAME, _is_some)(const CU_OPTIONAL_NAME(NAME) * opt) {    \
    return opt->isSome;                                                        \
  }                                                                            \
                                                                               \
  bool CU_OPTIONAL_FN(NAME, _is_none)(const CU_OPTIONAL_NAME(NAME) * opt) {    \
    return !opt->isSome;                                                       \
  }                                                                            \
                                                                               \
  T CU_OPTIONAL_FN(NAME, _unwrap)(CU_OPTIONAL_NAME(NAME) * opt) {              \
    if (!opt->isSome) {                                                        \
      CU_DIE("Attempted to unwrap a None optional");                           \
    }                                                                          \
    return opt->value;                                                         \
  }

CU_OPTIONAL_DECL(Ptr, void *)
CU_OPTIONAL_DECL(Int, int)
CU_OPTIONAL_DECL(Float, float)
CU_OPTIONAL_DECL(Double, double)
CU_OPTIONAL_DECL(Char, char)
CU_OPTIONAL_DECL(Bool, bool)

CU_OPTIONAL_DECL(U8, uint8_t)
CU_OPTIONAL_DECL(U16, uint16_t)
CU_OPTIONAL_DECL(U32, uint32_t)
CU_OPTIONAL_DECL(U64, uint64_t)
CU_OPTIONAL_DECL(I8, int8_t)
CU_OPTIONAL_DECL(I16, int16_t)
CU_OPTIONAL_DECL(I32, int32_t)
CU_OPTIONAL_DECL(I64, int64_t)
CU_OPTIONAL_DECL(Size, size_t)
CU_OPTIONAL_DECL(cu_Slice, cu_Slice)
