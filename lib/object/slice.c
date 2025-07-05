#include "object/slice.h"
#include "object/optional.h"

cu_Slice cu_Slice_create(void *ptr, size_t length) {
  cu_Slice slice;
  slice.ptr = ptr;
  slice.length = length;
  return slice;
}

CU_OPTIONAL_IMPL(cu_Slice, cu_Slice)
