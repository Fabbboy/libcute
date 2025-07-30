#pragma once
#include "macro.h"
#include "utility.h"
#include <nostd.h>
#include <stdbool.h>
#include <stddef.h>
#include "string/string.h"

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

#if CU_PLAT_WINDOWS
#define CU_PATH_SEPARATOR '\\'
#else
#define CU_PATH_SEPARATOR '/'
#endif

typedef enum {
  CU_FILE_TYPE_UNKNOWN,
  CU_FILE_TYPE_FILE,
  CU_FILE_TYPE_DIRECTORY,
  CU_FILE_TYPE_SYMLINK,
  CU_FILE_TYPE_NAMED_PIPE,
  CU_FILE_TYPE_BLOCK_DEVICE,
  CU_FILE_TYPE_CHAR_DEVICE,
  CU_FILE_TYPE_SOCKET,
  CU_FILE_TYPE_DOOR,
  CU_FILE_TYPE_EVENT_PORT,
} cu_File_Type;

typedef struct {
  cu_String path;
  unsigned long long size;
  unsigned int mode;
  cu_File_Type kind;
  long long atime;
  long long mtime;
  long long ctime;
} cu_File_Stat;

static inline void cu_File_Stat_destroy(cu_File_Stat *stat) {
  cu_String_destroy(&stat->path);
  cu_Memory_memset(stat, 0, sizeof(*stat));
}

#if CU_PLAT_POSIX
#include <sys/stat.h>
#include <sys/types.h>
static inline cu_File_Stat cu_File_Stat_from_handle(cu_Handle handle) {
  struct stat st;
  cu_Memory_memset(&st, 0, sizeof(st));
  cu_File_Stat out;
  cu_Memory_memset(&out, 0, sizeof(out));
  if (fstat(handle, &st) != 0) {
    return out;
  }

  out.size = (unsigned long long)st.st_size;
  out.mode = (unsigned int)st.st_mode;

  if (S_ISREG(st.st_mode)) {
    out.kind = CU_FILE_TYPE_FILE;
  } else if (S_ISDIR(st.st_mode)) {
    out.kind = CU_FILE_TYPE_DIRECTORY;
  } else if (S_ISCHR(st.st_mode)) {
    out.kind = CU_FILE_TYPE_CHAR_DEVICE;
  } else if (S_ISBLK(st.st_mode)) {
    out.kind = CU_FILE_TYPE_BLOCK_DEVICE;
  } else if (S_ISFIFO(st.st_mode)) {
    out.kind = CU_FILE_TYPE_NAMED_PIPE;
  } else if (S_ISLNK(st.st_mode)) {
    out.kind = CU_FILE_TYPE_SYMLINK;
#ifdef S_ISSOCK
  } else if (S_ISSOCK(st.st_mode)) {
    out.kind = CU_FILE_TYPE_SOCKET;
#endif
#ifdef S_IFDOOR
  } else if ((st.st_mode & S_IFMT) == S_IFDOOR) {
    out.kind = CU_FILE_TYPE_DOOR;
#endif
#ifdef S_IFPORT
  } else if ((st.st_mode & S_IFMT) == S_IFPORT) {
    out.kind = CU_FILE_TYPE_EVENT_PORT;
#endif
  } else {
    out.kind = CU_FILE_TYPE_UNKNOWN;
  }

#ifdef __APPLE__
  out.atime = (long long)st.st_atimespec.tv_sec * CU_TIME_NS_PER_SEC +
              st.st_atimespec.tv_nsec;
  out.mtime = (long long)st.st_mtimespec.tv_sec * CU_TIME_NS_PER_SEC +
              st.st_mtimespec.tv_nsec;
  out.ctime = (long long)st.st_ctimespec.tv_sec * CU_TIME_NS_PER_SEC +
              st.st_ctimespec.tv_nsec;
#elif defined(_STATBUF_ST_NSEC) && defined(st_atim)
  out.atime =
      (long long)st.st_atim.tv_sec * CU_TIME_NS_PER_SEC + st.st_atim.tv_nsec;
  out.mtime =
      (long long)st.st_mtim.tv_sec * CU_TIME_NS_PER_SEC + st.st_mtim.tv_nsec;
  out.ctime =
      (long long)st.st_ctim.tv_sec * CU_TIME_NS_PER_SEC + st.st_ctim.tv_nsec;
#elif defined(_STATBUF_ST_NSEC)
  out.atime = (long long)st.st_atime * CU_TIME_NS_PER_SEC + st.st_atimensec;
  out.mtime = (long long)st.st_mtime * CU_TIME_NS_PER_SEC + st.st_mtimensec;
  out.ctime = (long long)st.st_ctime * CU_TIME_NS_PER_SEC + st.st_ctimensec;
#else
  out.atime = (long long)st.st_atime * CU_TIME_NS_PER_SEC;
  out.mtime = (long long)st.st_mtime * CU_TIME_NS_PER_SEC;
  out.ctime = (long long)st.st_ctime * CU_TIME_NS_PER_SEC;
#endif

  return out;
}
#else
#include <windows.h>
static inline cu_File_Stat cu_File_Stat_from_handle(cu_Handle handle) {
  BY_HANDLE_FILE_INFORMATION info;
  cu_File_Stat out;
  cu_Memory_memset(&out, 0, sizeof(out));
  if (!GetFileInformationByHandle(handle, &info)) {
    return out;
  }
  ULARGE_INTEGER size;
  size.HighPart = info.nFileSizeHigh;
  size.LowPart = info.nFileSizeLow;

  out.size = (unsigned long long)size.QuadPart;
  out.mode = info.dwFileAttributes;
  if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    out.kind = CU_FILE_TYPE_DIRECTORY;
  } else if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
    out.kind = CU_FILE_TYPE_SYMLINK;
  } else {
    out.kind = CU_FILE_TYPE_FILE;
  }

  out.mtime = cu_Time_filetime_to_unix(info.ftLastWriteTime);
  out.ctime = cu_Time_filetime_to_unix(info.ftCreationTime);
  out.atime = cu_Time_filetime_to_unix(info.ftLastAccessTime);

  return out;
}
#endif
