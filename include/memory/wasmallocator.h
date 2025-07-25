#pragma once

/** @file wasmallocator.h WebAssembly page-based allocator. */

#include "memory/allocator.h"
#include "wasm.h"

#if CU_PLAT_WASM

/** Create a WebAssembly memory allocator. */
cu_Allocator cu_Allocator_WasmAllocator(void);

#endif /* CU_PLAT_WASM */
