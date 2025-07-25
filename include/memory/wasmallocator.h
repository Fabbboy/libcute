#pragma once

/** @file wasmallocator.h WebAssembly backed allocator. */

#include "memory/allocator.h"
#include <stddef.h>

/** Storage for the wasm allocator instance. */
typedef struct {
  int _unused; /**< placeholder field */
} cu_WasmAllocator;

/**
 * @brief Create an allocator backed by WebAssembly memory.
 * @param alloc Pointer to the wasm allocator instance.
 * @return A cu_Allocator using the wasm allocator.
 */
cu_Allocator cu_Allocator_WasmAllocator(cu_WasmAllocator *alloc);

