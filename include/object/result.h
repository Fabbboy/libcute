#pragma once

/** @file result.h Generic result helper macros. */

#include <stdbool.h>

#include "macro.h"

/** Declare result helper functions for a given type. */
#define CU_RESULT_HEADER(NAME, T, E)                                           \
  NAME##_Result NAME##_result_ok(T value);                                     \
  NAME##_Result NAME##_result_error(E error);                                  \
  bool NAME##_result_is_ok(NAME##_Result *result);                             \
  T NAME##_result_unwrap(NAME##_Result *result);                               \
  E NAME##_result_unwrap_error(NAME##_Result *result);

/** Declare the typed result struct and its helpers. */
#define CU_RESULT_DECL(NAME, T, E)                                             \
  typedef struct {                                                             \
    union {                                                                    \
      T value;                                                                 \
      E error;                                                                 \
    };                                                                         \
    bool isOk;                                                                 \
  } NAME##_Result;                                                             \
  CU_RESULT_HEADER(NAME, T, E)

/** Implement the typed result helpers. */
#define CU_RESULT_IMPL(NAME, T, E)                                             \
  NAME##_Result NAME##_result_ok(T value) {                                    \
    NAME##_Result result = {0};                                                \
    result.value = value;                                                      \
    result.isOk = true;                                                        \
    return result;                                                             \
  }                                                                            \
                                                                               \
  NAME##_Result NAME##_result_error(E error) {                                 \
    NAME##_Result result = {0};                                                \
    result.error = error;                                                      \
    result.isOk = false;                                                       \
    return result;                                                             \
  }                                                                            \
                                                                               \
  bool NAME##_result_is_ok(NAME##_Result *result) { return result->isOk; }     \
                                                                               \
  T NAME##_result_unwrap(NAME##_Result *result) {                              \
    if (!result->isOk) {                                                       \
      CU_DIE("Attempted to unwrap an error result");                           \
    }                                                                          \
    return result->value;                                                      \
  }                                                                            \
                                                                               \
  E NAME##_result_unwrap_error(NAME##_Result *result) {                        \
    if (result->isOk) {                                                        \
      CU_DIE("Attempted to unwrap an ok result as error");                     \
    }                                                                          \
    return result->error;                                                      \
  }
