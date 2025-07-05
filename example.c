#include "memory/allocator.h"
#include "string/string.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  cu_Allocator alloc = cu_Allocator_CAllocator();

  cu_String_Result res = cu_String_from_cstr(alloc, "hello");
  if (!cu_String_result_is_ok(&res)) {
    fprintf(stderr, "string allocation failed\n");
    return EXIT_FAILURE;
  }
  cu_String hello = res.value;

  cu_String_append_cstr(&hello, ", world");
  printf("%s\n", hello.data);

  cu_String_destroy(&hello);
  return EXIT_SUCCESS;
}
