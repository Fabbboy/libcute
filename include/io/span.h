#pragma once

#include "io/error.h"
#include "nostd.h"
#include "object/optional.h"
#include "object/result.h"
#include "string/string.h"

#include <stddef.h>

CU_OPTIONAL_DECL(cu_Io_Error, cu_Io_Error)
CU_RESULT_DECL(cu_IoSlice, cu_Slice, cu_Io_Error)
CU_RESULT_DECL(cu_IoString, cu_String, cu_Io_Error)

// TODO: fix circular dependency
// slice defined here
// string depends on allocator which depends on slice which depends on this file
// circular dependency
// forward decl not possible because we alwys use concrete types and no opaque
// types
// which is good no plan on changing that