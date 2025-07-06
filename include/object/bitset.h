#pragma once

#include "object/optional.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CU_BITSET_HEADER(NAME)                                                 \
  NAME##_BitSet NAME##_create(void);                                           \
  void NAME##_set(NAME##_BitSet *set, size_t index);                           \
  void NAME##_clear(NAME##_BitSet *set, size_t index);                         \
  Int_Optional NAME##_get(NAME##_BitSet *set, size_t index);                   \
  bool NAME##_is_set(NAME##_BitSet *set, size_t index);                        \
  void NAME##_clear_all(NAME##_BitSet *set);                                   \
  size_t NAME##_size(void);

#define CU_BITSET_DECL(NAME, SIZE)                                             \
  typedef struct {                                                             \
    uint8_t bits[(SIZE + 7) / 8];                                              \
    enum { NAME##_FixedSize = (SIZE) };                                        \
  } NAME##_BitSet;                                                             \
  CU_BITSET_HEADER(NAME)

#define CU_BITSET_IMPL(NAME, SIZE)                                             \
  NAME##_BitSet NAME##_create(void) {                                          \
    NAME##_BitSet set = {0};                                                   \
    return set;                                                                \
  }                                                                            \
                                                                               \
  void NAME##_set(NAME##_BitSet *set, size_t index) {                          \
    if (index < SIZE) {                                                        \
      set->bits[index / 8] |= (1 << (index % 8));                              \
    }                                                                          \
  }                                                                            \
                                                                               \
  void NAME##_clear(NAME##_BitSet *set, size_t index) {                        \
    if (index < SIZE) {                                                        \
      set->bits[index / 8] &= ~(1 << (index % 8));                             \
    }                                                                          \
  }                                                                            \
                                                                               \
  Int_Optional NAME##_get(NAME##_BitSet *set, size_t index) {                  \
    if (index < SIZE && (set->bits[index / 8] & (1 << (index % 8)))) {         \
      return Int_some(index);                                                  \
    } else {                                                                   \
      return Int_none();                                                       \
    }                                                                          \
  }                                                                            \
                                                                               \
  bool NAME##_is_set(NAME##_BitSet *set, size_t index) {                       \
    return index < SIZE && (set->bits[index / 8] & (1 << (index % 8)));        \
  }                                                                            \
                                                                               \
  void NAME##_clear_all(NAME##_BitSet *set) {                                  \
    for (size_t i = 0; i < sizeof(set->bits); ++i) {                           \
      set->bits[i] = 0;                                                        \
    }                                                                          \
  }                                                                            \
                                                                               \
  size_t NAME##_size(void) { return SIZE; }
\
