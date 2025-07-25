#include "memory/wasmallocator.h"
#include "macro.h"
#include "object/slice.h"
#include "utility.h"
#include <string.h>
#include <stdint.h>

#if !defined(__wasm__)
cu_Allocator cu_Allocator_WasmAllocator(cu_WasmAllocator *alloc) {
  CU_UNUSED(alloc);
  CU_DIE("Wasm allocator only available on wasm targets");
  cu_Allocator a = {0};
  return a;
}
#else

#define CU_WASM_BIGPAGE_SIZE (64 * 1024)

#if SIZE_MAX > UINT32_MAX
#define CU_WASM_MIN_CLASS 4
#define CU_WASM_BIG_SIZE_CLASS_COUNT 48
#else
#define CU_WASM_MIN_CLASS 3
#define CU_WASM_BIG_SIZE_CLASS_COUNT 16
#endif

#define CU_WASM_SIZE_CLASS_COUNT (16 - CU_WASM_MIN_CLASS)

static size_t next_addrs[CU_WASM_SIZE_CLASS_COUNT] = {0};
static size_t frees[CU_WASM_SIZE_CLASS_COUNT] = {0};
static size_t big_frees[CU_WASM_BIG_SIZE_CLASS_COUNT] = {0};

static inline size_t cu_wasm_log2(size_t x) {
#if SIZE_MAX > UINT32_MAX
  return (sizeof(size_t) * 8 - 1) - __builtin_clzl(x);
#else
  return 31 - __builtin_clz(x);
#endif
}

static inline size_t cu_wasm_big_pages_needed(size_t byte_count) {
  return (byte_count + (CU_WASM_BIGPAGE_SIZE + (sizeof(size_t) - 1))) /
         CU_WASM_BIGPAGE_SIZE;
}

static size_t cu_wasm_alloc_big_pages(size_t n) {
  size_t pow2_pages = cu_next_pow2(n);
  size_t slot_size = pow2_pages * CU_WASM_BIGPAGE_SIZE;
  size_t class = cu_wasm_log2(pow2_pages);
  size_t top = big_frees[class];
  if (top != 0) {
    size_t *node = (size_t *)(top + slot_size - sizeof(size_t));
    big_frees[class] = *node;
    return top;
  }
  size_t prev = __builtin_wasm_memory_grow(0, pow2_pages);
  if (prev == (size_t)-1) {
    return 0;
  }
  return prev * CU_WASM_BIGPAGE_SIZE;
}

static cu_Slice_Result cu_wasm_alloc(void *self, size_t len, size_t alignment) {
  CU_UNUSED(self);
  if (len == 0) {
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
                       .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }
  if (alignment == 0) {
    alignment = 1;
  }
  size_t actual_len = len + sizeof(size_t);
  if (actual_len < alignment) {
    actual_len = alignment;
  }
  size_t slot_size = cu_next_pow2(actual_len);
  size_t class = cu_wasm_log2(slot_size) - CU_WASM_MIN_CLASS;
  size_t addr;
  if (class < CU_WASM_SIZE_CLASS_COUNT) {
    size_t top = frees[class];
    if (top != 0) {
      size_t *node = (size_t *)(top + slot_size - sizeof(size_t));
      frees[class] = *node;
      addr = top;
    } else {
      size_t next = next_addrs[class];
      if (next % CU_WASM_BIGPAGE_SIZE == 0) {
        addr = cu_wasm_alloc_big_pages(1);
        if (addr == 0) {
          cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
                             .errnum = Size_Optional_none()};
          return cu_Slice_result_error(err);
        }
        next_addrs[class] = addr + slot_size;
      } else {
        addr = next;
        next_addrs[class] = next + slot_size;
      }
    }
  } else {
    size_t need = cu_wasm_big_pages_needed(actual_len);
    addr = cu_wasm_alloc_big_pages(need);
    if (addr == 0) {
      cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
                         .errnum = Size_Optional_none()};
      return cu_Slice_result_error(err);
    }
  }
  return cu_Slice_result_ok(cu_Slice_create((void *)addr, len));
}

static cu_Slice_Result cu_wasm_resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  if (mem.ptr == NULL) {
    return cu_wasm_alloc(self, size, alignment);
  }
  if (size == 0) {
    cu_wasm_free(self, mem);
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
                       .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }
  if (alignment == 0) {
    alignment = 1;
  }
  size_t old_actual = mem.length + sizeof(size_t);
  if (old_actual < alignment) {
    old_actual = alignment;
  }
  size_t new_actual = size + sizeof(size_t);
  if (new_actual < alignment) {
    new_actual = alignment;
  }
  size_t old_slot = cu_next_pow2(old_actual);
  size_t old_class = cu_wasm_log2(old_slot) - CU_WASM_MIN_CLASS;
  if (old_class < CU_WASM_SIZE_CLASS_COUNT) {
    size_t new_slot = cu_next_pow2(new_actual);
    if (old_slot == new_slot) {
      return cu_Slice_result_ok(cu_Slice_create(mem.ptr, size));
    }
  } else {
    size_t old_need = cu_wasm_big_pages_needed(old_actual);
    size_t old_slot_pages = cu_next_pow2(old_need);
    size_t new_need = cu_wasm_big_pages_needed(new_actual);
    size_t new_slot_pages = cu_next_pow2(new_need);
    if (old_slot_pages == new_slot_pages) {
      return cu_Slice_result_ok(cu_Slice_create(mem.ptr, size));
    }
  }
  cu_Slice_Result new_mem = cu_wasm_alloc(self, size, alignment);
  if (!cu_Slice_result_is_ok(&new_mem)) {
    return new_mem;
  }
  memcpy(new_mem.value.ptr, mem.ptr,
         mem.length < size ? mem.length : size);
  cu_wasm_free(self, mem);
  return new_mem;
}

static void cu_wasm_free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  if (mem.ptr == NULL) {
    return;
  }
  size_t actual_len = mem.length + sizeof(size_t);
  size_t slot_size = cu_next_pow2(actual_len);
  size_t class = cu_wasm_log2(slot_size) - CU_WASM_MIN_CLASS;
  size_t addr = (size_t)mem.ptr;
  if (class < CU_WASM_SIZE_CLASS_COUNT) {
    size_t *node = (size_t *)(addr + slot_size - sizeof(size_t));
    *node = frees[class];
    frees[class] = addr;
  } else {
    size_t need = cu_wasm_big_pages_needed(actual_len);
    size_t pow2_pages = cu_next_pow2(need);
    size_t big_slot = pow2_pages * CU_WASM_BIGPAGE_SIZE;
    size_t *node = (size_t *)(addr + big_slot - sizeof(size_t));
    size_t big_class = cu_wasm_log2(pow2_pages);
    *node = big_frees[big_class];
    big_frees[big_class] = addr;
  }
}

cu_Allocator cu_Allocator_WasmAllocator(cu_WasmAllocator *alloc) {
  CU_UNUSED(alloc);
  cu_Allocator a;
  a.self = alloc;
  a.allocFn = cu_wasm_alloc;
  a.resizeFn = cu_wasm_resize;
  a.freeFn = cu_wasm_free;
  return a;
}

#endif /* __wasm__ */

