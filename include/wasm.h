#pragma once

/** @file wasm.h WebAssembly specific helpers. */

#include "macro.h"

#if CU_PLAT_WASM
#include <stddef.h>

/** Size of a single WebAssembly memory page. */
#define CU_WASM_PAGE_SIZE 65536

static inline void cu_wasm_memory_grow(size_t pages) {
  __builtin_wasm_memory_grow(0, pages);
}
#endif
