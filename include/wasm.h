#pragma once

/** @file wasm.h WebAssembly specific helpers. */

#include "macro.h"

#if CU_PLAT_WASM
#include <stddef.h>

static inline void cu_wasm_memory_grow(size_t pages) {
  __builtin_wasm_memory_grow(0, pages);
}
#endif
