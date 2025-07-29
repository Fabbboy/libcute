#include "utility.h"

#if CU_PLAT_WINDOWS
long long cu_Time_filetime_to_unix(FILETIME ft) {
  ULARGE_INTEGER uli;
  uli.LowPart = ft.dwLowDateTime;
  uli.HighPart = ft.dwHighDateTime;
  return (long long)((uli.QuadPart - CU_TIME_WINDOWS_EPOCH_DIFF) *
                     CU_TIME_WINDOWS_TICK_NS);
}

FILETIME cu_Time_unix_to_filetime(long long ns) {
  ULARGE_INTEGER uli;
  uli.QuadPart = (unsigned long long)(ns / CU_TIME_WINDOWS_TICK_NS) +
                 CU_TIME_WINDOWS_EPOCH_DIFF;
  FILETIME ft;
  ft.dwLowDateTime = uli.LowPart;
  ft.dwHighDateTime = uli.HighPart;
  return ft;
}
#endif
