#pragma once
#include "macro.h"
#include <stdbool.h>
#include <stddef.h>

#if CU_PLAT_POSIX
typedef int cu_Handle;
#define CU_INVALID_HANDLE -1
#else
#include <windows.h>
typedef HANDLE cu_Handle;
#define CU_INVALID_HANDLE INVALID_HANDLE_VALUE
#endif

#if CU_PLAT_POSIX
#define CU_FILE_MAX_PATH_LENGTH 4096
#else
#define CU_FILE_MAX_PATH_LENGTH 260
#endif

typedef struct {
  size_t length;
  bool is_dir;
  size_t mtime;
  size_t ctime;
  size_t atime;
} cu_File_Stat;

#if CU_PLAT_POSIX
#include <sys/stat.h>
cu_File_Stat cu_File_Stat_from(struct stat *st) {
  return (cu_File_Stat){
      .length = st->st_size,
      .is_dir = S_ISDIR(st->st_mode),
      .mtime = st->st_mtime,
      .ctime = st->st_ctime,
      .atime = st->st_atime,
  };
}
#else
#error "Windows implementation not yet supported"
#endif
