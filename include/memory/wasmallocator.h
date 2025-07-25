#pragma once

/** @file wasmallocator.h WebAssembly backed allocator. */

#include "macro.h"
#include "memory/allocator.h"
#include <stddef.h>

#if CU_PLAT_WASM

#define CU_WASM_BIGPAGE_SIZE (64 * 1024)

#if SIZE_MAX > UINT32_MAX
#define CU_WASM_MIN_CLASS 4
#define CU_WASM_BIG_SIZE_CLASS_COUNT 48
#else
#define CU_WASM_MIN_CLASS 3
#define CU_WASM_BIG_SIZE_CLASS_COUNT 16
#endif

#define CU_WASM_SIZE_CLASS_COUNT (16 - CU_WASM_MIN_CLASS)

typedef struct {
  size_t nextAddrs[CU_WASM_SIZE_CLASS_COUNT];
  size_t frees[CU_WASM_SIZE_CLASS_COUNT];
  size_t bigFrees[CU_WASM_BIG_SIZE_CLASS_COUNT];
} cu_WasmAllocator;

/**
 * @brief Create an allocator backed by WebAssembly memory.
 * @param alloc Pointer to the wasm allocator instance.
 * @return A cu_Allocator using the wasm allocator.
 */
cu_Allocator cu_Allocator_WasmAllocator(cu_WasmAllocator *alloc);

#endif /* CU_PLAT_WASM */
