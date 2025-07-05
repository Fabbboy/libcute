#include "memory/allocator.h"
#include "memory/page.h"
#include "slice.h"
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

  exit(EXIT_SUCCESS);
}
