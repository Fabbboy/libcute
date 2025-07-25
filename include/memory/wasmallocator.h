#pragma once

/** @file wasmallocator.h WebAssembly page-based allocator. */

#include "memory/allocator.h"
#include "wasm.h"

#if CU_PLAT_WASM

/** Storage for the WebAssembly allocator. */
typedef struct {
  int _unused; /**< placeholder */
} cu_WasmAllocator;

/** Create a WebAssembly memory allocator. */
cu_Allocator cu_Allocator_WasmAllocator(cu_WasmAllocator *allocator);

#endif /* CU_PLAT_WASM */
