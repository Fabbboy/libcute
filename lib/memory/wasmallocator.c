#include "memory/wasmallocator.h"
#include "io/error.h"
#include "macro.h"
#include "utility.h"
#include <nostd.h>

#if CU_PLAT_WASM
#include <stdint.h>

struct cu_WasmAllocator_Header {
  size_t offset;
  size_t slot_size;
};

static cu_Slice_Result cu_wasm_alloc(void *self, size_t size, size_t alignment);
static cu_Slice_Result cu_wasm_resize(
    void *self, cu_Slice mem, size_t size, size_t alignment);
static void cu_wasm_free(void *self, cu_Slice mem);

#define CU_WASM_BIGPAGE_SIZE (64 * 1024)
#define CU_WASM_PAGES_PER_BIGPAGE (CU_WASM_BIGPAGE_SIZE / CU_WASM_PAGE_SIZE)
#define CU_WASM_MIN_CLASS ((sizeof(size_t) == 8) ? 4 : 3)
#define CU_WASM_SIZE_CLASS_COUNT (16 - CU_WASM_MIN_CLASS)
#define CU_WASM_BIG_SIZE_CLASS_COUNT ((sizeof(size_t) * 8) - 16)

static size_t next_addrs[CU_WASM_SIZE_CLASS_COUNT] = {0};
static size_t frees[CU_WASM_SIZE_CLASS_COUNT] = {0};
static size_t big_frees[CU_WASM_BIG_SIZE_CLASS_COUNT] = {0};

static inline size_t cu_log2_usize(size_t x) {
#if SIZE_MAX > UINT32_MAX
  return (size_t)(63 - __builtin_clzll(x));
#else
  return (size_t)(31 - __builtin_clz(x));
#endif
}

static inline size_t big_pages_needed(size_t byte_count) {
  return (byte_count + (CU_WASM_BIGPAGE_SIZE + (sizeof(size_t) - 1))) /
         CU_WASM_BIGPAGE_SIZE;
}

static size_t alloc_big_pages(size_t n) {
  size_t pow2_pages = cu_next_pow2(n);
  size_t slot_size_bytes = pow2_pages * CU_WASM_BIGPAGE_SIZE;
  size_t class = cu_log2_usize(pow2_pages);

  size_t top_free = big_frees[class];
  if (top_free != 0) {
    size_t *node = (size_t *)(top_free + slot_size_bytes - sizeof(size_t));
    big_frees[class] = *node;
    return top_free;
  }

  int page_index =
      __builtin_wasm_memory_grow(0, pow2_pages * CU_WASM_PAGES_PER_BIGPAGE);
  if (page_index == -1) {
    return 0;
  }
  return (size_t)page_index * CU_WASM_PAGE_SIZE;
}

static cu_Slice_Result cu_wasm_alloc(
    void *self, size_t size, size_t alignment) {
  CU_UNUSED(self);
  if (size == 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }
  if (alignment == 0) {
    alignment = 1;
  }

  const size_t header_size = sizeof(struct cu_WasmAllocator_Header);
  size_t user_offset = CU_ALIGN_UP(header_size, alignment);
  size_t needed = user_offset + size + sizeof(size_t);
  size_t slot_size = cu_next_pow2(needed);
  size_t class = cu_log2_usize(slot_size) - CU_WASM_MIN_CLASS;

  size_t addr;
  if (class < CU_WASM_SIZE_CLASS_COUNT) {
    size_t top_free = frees[class];
    if (top_free != 0) {
      size_t *node = (size_t *)(top_free + slot_size - sizeof(size_t));
      frees[class] = *node;
      addr = top_free;
    } else {
      size_t next_addr = next_addrs[class];
      if (next_addr % CU_WASM_PAGE_SIZE == 0) {
        addr = alloc_big_pages(1);
        if (addr == 0) {
          cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
              .errnum = Size_Optional_none()};
          return cu_Slice_result_error(err);
        }
        next_addrs[class] = addr + slot_size;
      } else {
        addr = next_addr;
        next_addrs[class] = next_addr + slot_size;
      }
    }
  } else {
    size_t pages = big_pages_needed(needed);
    addr = alloc_big_pages(pages);
    if (addr == 0) {
      cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
          .errnum = Size_Optional_none()};
      return cu_Slice_result_error(err);
    }
  }

  unsigned char *base = (unsigned char *)addr;
  struct cu_WasmAllocator_Header *hdr =
      (struct cu_WasmAllocator_Header *)(base + user_offset - header_size);
  hdr->offset = user_offset - header_size;
  hdr->slot_size = slot_size;
  return cu_Slice_result_ok(cu_Slice_create(base + user_offset, size));
}

static cu_Slice_Result cu_wasm_resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  CU_IF_NULL(mem.ptr) { return cu_wasm_alloc(self, size, alignment); }
  if (size == 0) {
    cu_wasm_free(self, mem);
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }
  if (alignment == 0) {
    alignment = 1;
  }

  struct cu_WasmAllocator_Header *hdr =
      (struct cu_WasmAllocator_Header *)((unsigned char *)mem.ptr -
                                         sizeof(
                                             struct cu_WasmAllocator_Header));
  size_t slot_size = hdr->slot_size;
  size_t offset = hdr->offset + sizeof(struct cu_WasmAllocator_Header);
  if (((uintptr_t)mem.ptr % alignment) == 0 &&
      offset + size + sizeof(size_t) <= slot_size) {
    return cu_Slice_result_ok(cu_Slice_create(mem.ptr, size));
  }

  cu_Slice_Result new_mem = cu_wasm_alloc(self, size, alignment);
  if (!cu_Slice_result_is_ok(&new_mem)) {
    return new_mem;
  }
  cu_Memory_memcpy(new_mem.value.ptr,
      cu_Slice_create(mem.ptr, mem.length < size ? mem.length : size));
  cu_wasm_free(self, mem);
  return new_mem;
}

static void cu_wasm_free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  CU_IF_NULL(mem.ptr) { return; }
  struct cu_WasmAllocator_Header *hdr =
      (struct cu_WasmAllocator_Header *)((unsigned char *)mem.ptr -
                                         sizeof(
                                             struct cu_WasmAllocator_Header));
  size_t slot_size = hdr->slot_size;
  size_t offset = hdr->offset + sizeof(struct cu_WasmAllocator_Header);
  size_t addr = (size_t)((unsigned char *)mem.ptr - offset);
  size_t class = cu_log2_usize(slot_size) - CU_WASM_MIN_CLASS;

  if (class < CU_WASM_SIZE_CLASS_COUNT) {
    size_t *node = (size_t *)(addr + slot_size - sizeof(size_t));
    *node = frees[class];
    frees[class] = addr;
  } else {
    size_t pages = slot_size / CU_WASM_BIGPAGE_SIZE;
    size_t big_class = cu_log2_usize(pages);
    size_t *node = (size_t *)(addr + slot_size - sizeof(size_t));
    *node = big_frees[big_class];
    big_frees[big_class] = addr;
  }
}

cu_Allocator cu_Allocator_WasmAllocator(void) {
  cu_Allocator a = {0};
  a.self = NULL;
  a.allocFn = cu_wasm_alloc;
  a.resizeFn = cu_wasm_resize;
  a.freeFn = cu_wasm_free;
  return a;
}

#endif /* CU_PLAT_WASM */
