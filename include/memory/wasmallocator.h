#pragma once

/** @file wasmallocator.h WebAssembly page-based allocator. */

#include "memory/allocator.h"

#if defined(CU_PLAT_WASM) 

/** Size of a single WebAssembly memory page. */
#define CU_WASM_PAGE_SIZE 65536

static inline void cu_wasm_memory_grow(size_t pages) {
  __builtin_wasm_memory_grow(0, pages);
}

/** Create a WebAssembly memory allocator. */
cu_Allocator cu_Allocator_WasmAllocator(void);

#endif /* CU_PLAT_WASM */
