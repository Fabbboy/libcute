#pragma once

#include <stddef.h>
#include "macro.h"

#if CU_PLAT_POSIX == 1
typedef size_t cu_Handle;
#else 
#include <windows.h>
typedef HANDLE* cu_Handle;
#endif