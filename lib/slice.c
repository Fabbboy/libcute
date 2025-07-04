#include "slice.h"
#include "optional.h"

cu_Slice cu_Slice_create(void *ptr, size_t length) {
  cu_Slice slice;
  slice.ptr = ptr;
  slice.length = length;
  return slice;
}

CU_OPTIONAL_IMPL(Slice, cu_Slice)