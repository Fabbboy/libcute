#pragma once

/** @file destructor.h Optional destructor helpers. */

#include "object/optional.h"

/** Function pointer type for element destructors. */
typedef void (*cu_Destructor)(void *);

CU_OPTIONAL_DECL(cu_Destructor, cu_Destructor)
