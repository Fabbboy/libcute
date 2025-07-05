#include "hash/hash.h"
#include "memory/allocator.h"
#include "memory/page.h"
#include "object/slice.h"
#include "string/utf8.h"
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  cu_PageAllocator allocator = {0};
  cu_Allocator page_allocator = cu_Allocator_PageAllocator(&allocator);

  Slice_Optional allocation =
      cu_Allocator_Alloc(page_allocator, 400, alignof(int));
  if (Slice_is_none(&allocation)) {
    fprintf(stderr, "allocation failed!\n");
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "allocation successful!\n");
  cu_Allocator_Free(page_allocator, allocation.value);

  const char *text = "hello👍";
  bool valid = cu_utf8_validate((const unsigned char *)text, 5);
  uint64_t hash = cu_Hash_FNV1a64(text, 5);
  uint32_t cp = 0;
  cu_utf8_decode((const unsigned char *)text, 5, &cp);
  fprintf(stderr, "utf8 valid: %s, hash: %llu\n", valid ? "yes" : "no",
      (unsigned long long)hash);

  exit(EXIT_SUCCESS);
}
