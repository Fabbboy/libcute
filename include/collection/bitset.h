#pragma once

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @file bitset.h Fixed-size local bitset utilities. */
/** Declare helper functions for a fixed-size bitset type. */

#define CU_BITSET_HEADER(NAME)                                                 \
  NAME##_BitSet NAME##_BitSet_create(void);                                           \
  void NAME##_BitSet_set(NAME##_BitSet *set, size_t index);                           \
  void NAME##_BitSet_clear(NAME##_BitSet *set, size_t index);                         \
  bool NAME##_BitSet_get(const NAME##_BitSet *set, size_t index);                     \
  void NAME##_BitSet_clear_all(NAME##_BitSet *set);                                   \
  size_t NAME##_BitSet_size(void);

/** Declare the bitset struct and its helpers. */

#define CU_BITSET_DECL(NAME, SIZE)                                             \
  typedef struct {                                                             \
    alignas(sizeof(size_t)) uint8_t bits[((SIZE) + 7) / 8];                    \
  } NAME##_BitSet;                                                             \
  CU_BITSET_HEADER(NAME)

/** Implement the helpers declared by \ref CU_BITSET_DECL. */

#define CU_BITSET_IMPL(NAME, SIZE)                                             \
  NAME##_BitSet NAME##_BitSet_create(void) {                                          \
    NAME##_BitSet set = {0};                                                   \
    return set;                                                                \
  }                                                                            \
                                                                               \
  void NAME##_BitSet_set(NAME##_BitSet *set, size_t index) {                          \
    if (index < SIZE) {                                                        \
      set->bits[index / 8] |= (1u << (index % 8));                             \
    }                                                                          \
  }                                                                            \
                                                                               \
  void NAME##_BitSet_clear(NAME##_BitSet *set, size_t index) {                        \
    if (index < SIZE) {                                                        \
      set->bits[index / 8] &= (uint8_t)~(1u << (index % 8));                   \
    }                                                                          \
  }                                                                            \
                                                                               \
  bool NAME##_BitSet_get(const NAME##_BitSet *set, size_t index) {                    \
    return index < SIZE && (set->bits[index / 8] & (1u << (index % 8)));       \
  }                                                                            \
                                                                               \
  void NAME##_BitSet_clear_all(NAME##_BitSet *set) {                                  \
    for (size_t i = 0; i < sizeof(set->bits); ++i) {                           \
      set->bits[i] = 0;                                                        \
    }                                                                          \
  }                                                                            \
                                                                               \
  size_t NAME##_BitSet_size(void) { return SIZE; }
