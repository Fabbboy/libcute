#pragma once

/** @file state.h Interface for pseudo-random state. */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Callback type to fetch the next random value. */
typedef uint32_t (*cu_State_NextFn)(void *ctx);

/** Generic state interface. */
typedef struct {
  void *ctx;           /**< user supplied state object */
  cu_State_NextFn next; /**< function generating next value */
} cu_State;

/** Simple linear congruential generator implementation. */
typedef struct {
  uint32_t value;
} cu_RandomState;

static inline uint32_t cu_RandomState_next(void *ctx) {
  cu_RandomState *st = (cu_RandomState *)ctx;
  st->value = st->value * 1664525u + 1013904223u;
  return st->value;
}

static inline cu_State cu_RandomState_init(cu_RandomState *impl, uint32_t seed) {
  impl->value = seed;
  cu_State st = {impl, cu_RandomState_next};
  return st;
}

static inline uint32_t cu_State_next(cu_State *st) { return st->next(st->ctx); }

#ifdef __cplusplus
}
#endif
