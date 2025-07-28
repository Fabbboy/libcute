#pragma once

/** @file result.h Generic result helper macros. */

#include <stdbool.h>

#include "macro.h"

/** Build the concrete result type name for a category. */
#define CU_RESULT_NAME(NAME) NAME##_Result
/** Construct a result helper function name. */
#define CU_RESULT_FN(NAME, SUFFIX) CU_CONCAT(CU_RESULT_NAME(NAME), SUFFIX)

/** Declare result helper functions for a given type. */
#define CU_RESULT_HEADER(NAME, T, E)                                           \
  CU_RESULT_NAME(NAME) CU_RESULT_FN(NAME, _ok)(T value);                      \
  CU_RESULT_NAME(NAME) CU_RESULT_FN(NAME, _error)(E error);                   \
  bool CU_RESULT_FN(NAME, _is_ok)(CU_RESULT_NAME(NAME) * result);             \
  T CU_RESULT_FN(NAME, _unwrap)(CU_RESULT_NAME(NAME) * result);               \
  E CU_RESULT_FN(NAME, _unwrap_error)(CU_RESULT_NAME(NAME) * result);

/** Declare the typed result struct and its helpers. */
#define CU_RESULT_DECL(NAME, T, E)                                             \
  typedef struct {                                                             \
    union {                                                                    \
      T value;                                                                 \
      E error;                                                                 \
    };                                                                         \
    bool isOk;                                                                 \
  } CU_RESULT_NAME(NAME);                                                      \
  CU_RESULT_HEADER(NAME, T, E)

/** Implement the typed result helpers. */
#define CU_RESULT_IMPL(NAME, T, E)                                             \
  CU_RESULT_NAME(NAME) CU_RESULT_FN(NAME, _ok)(T value) {                     \
    CU_RESULT_NAME(NAME) result = {0};                                         \
    result.value = value;                                                      \
    result.isOk = true;                                                        \
    return result;                                                             \
  }                                                                            \
                                                                               \
  CU_RESULT_NAME(NAME) CU_RESULT_FN(NAME, _error)(E error) {                  \
    CU_RESULT_NAME(NAME) result = {0};                                         \
    result.error = error;                                                      \
    result.isOk = false;                                                       \
    return result;                                                             \
  }                                                                            \
                                                                               \
  bool CU_RESULT_FN(NAME, _is_ok)(CU_RESULT_NAME(NAME) * result) {             \
    return result->isOk;                                                       \
  }                                                                            \
                                                                               \
  T CU_RESULT_FN(NAME, _unwrap)(CU_RESULT_NAME(NAME) * result) {               \
    if (!result->isOk) {                                                       \
      CU_DIE("Attempted to unwrap an error result");                           \
    }                                                                          \
    return result->value;                                                      \
  }                                                                            \
                                                                               \
  E CU_RESULT_FN(NAME, _unwrap_error)(CU_RESULT_NAME(NAME) * result) {         \
    if (result->isOk) {                                                        \
      CU_DIE("Attempted to unwrap an ok result as error");                     \
    }                                                                          \
    return result->error;                                                      \
  }
