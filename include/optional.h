#pragma once

#include <stdbool.h>

#include "macro.h"

#define CU_OPTIONAL_HEADER(NAME, T)                                            \
  NAME##_Optional NAME##_some(T value);                                        \
  NAME##_Optional NAME##_none(void);                                           \
  bool NAME##_is_some(NAME##_Optional *opt);                                   \
  bool NAME##_is_none(NAME##_Optional *opt);                                   \
  T NAME##_unwrap(NAME##_Optional *opt);

#define CU_OPTIONAL_DECL(NAME, T)                                              \
  typedef struct {                                                             \
    T value;                                                                   \
    bool isSome;                                                               \
  } NAME##_Optional;                                                           \
  CU_OPTIONAL_HEADER(NAME, T)

#define CU_OPTIONAL_IMPL(NAME, T)                                              \
  NAME##_Optional NAME##_some(T value) {                                       \
    NAME##_Optional opt;                                                       \
    opt.value = value;                                                         \
    opt.isSome = true;                                                         \
    return opt;                                                                \
  }                                                                            \
                                                                               \
  NAME##_Optional NAME##_none(void) {                                          \
    NAME##_Optional opt;                                                       \
    opt.isSome = false;                                                        \
    return opt;                                                                \
  }                                                                            \
                                                                               \
  bool NAME##_is_some(NAME##_Optional *opt) { return opt->isSome; }            \
                                                                               \
  bool NAME##_is_none(NAME##_Optional *opt) { return !opt->isSome; }           \
                                                                               \
  T NAME##_unwrap(NAME##_Optional *opt) {                                      \
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
