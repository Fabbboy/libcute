#include "state.h"
#ifndef CU_FREESTANDING
#include <stdlib.h>
#include <time.h>
#if defined(__linux__)
#include <sys/random.h>
#endif
#endif

#include "macro.h"

uint32_t cu_random_seed(void) {
#ifndef CU_FREESTANDING
#if CU_PLAT_LINUX
  uint32_t seed;
  if (getrandom(&seed, sizeof(seed), 0) == sizeof(seed)) {
    return seed;
  }
#else
  uint32_t seed;
#endif
  static int seeded = 0;
  if (!seeded) {
    srand((unsigned)time(NULL));
    seeded = 1;
  }
  seed = ((uint32_t)rand() << 16) ^ (uint32_t)rand();
  seed ^= (uint32_t)(uintptr_t)&seed;
  return seed;
#else
  uintptr_t sp = (uintptr_t)&sp;
  uintptr_t fn = (uintptr_t)&cu_random_seed;
#if defined(__GNUC__)
  uintptr_t ret = (uintptr_t)__builtin_return_address(0);
#else
  uintptr_t ret = 0;
#endif
  static uint32_t seed = 1;
  seed = seed * 1664525u + 1013904223u;
  return (uint32_t)(seed ^ sp ^ fn ^ ret);
#endif
}
