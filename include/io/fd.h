#pragma once
#include <stddef.h>
#include "macro.h"

#if CU_PLAT_POSIX
typedef int cu_Handle;
#define CU_INVALID_HANDLE -1
#else
#include <windows.h>
typedef HANDLE cu_Handle;
#define CU_INVALID_HANDLE INVALID_HANDLE_VALUE
#endif