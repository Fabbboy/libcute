#include "io/dir.h"
#include "io/error.h"
#include "io/fd.h"
#include "macro.h"
#include "nostd.h"
#include "object/optional.h"
#include "object/result.h"
#if CU_PLAT_POSIX
#include <errno.h>
#include <fcntl.h>
#include <fcntl.h> // for O_RDONLY, O_DIRECTORY, openat, AT_FDCWD
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h> // technically optional, but traditional
#include <unistd.h>
#include <unistd.h> // for close()
#else
#include <windows.h>
#endif

CU_RESULT_IMPL(cu_Dir, cu_Dir, cu_Io_Error)

cu_Dir_Result cu_Dir_open(cu_Slice path, bool create) {
  char lpath[CU_FILE_MAX_PATH_LENGTH] = {0};
  size_t path_len;
  if (path.length < (CU_FILE_MAX_PATH_LENGTH - 1)) {
    path_len = path.length;
  } else {
    path_len = CU_FILE_MAX_PATH_LENGTH - 1;
  }
  cu_Memory_smemcpy(
      cu_Slice_create(lpath, path_len), cu_Slice_create(path.ptr, path_len));
  lpath[path_len] = '\0';

  cu_Handle handle = CU_INVALID_HANDLE;
  cu_File_Stat stat;
  cu_Memory_memset(&stat, 0, sizeof(stat));
#if CU_PLAT_POSIX
  int flags = O_RDONLY;
#ifdef O_DIRECTORY
  flags |= O_DIRECTORY;
#endif
  int mode =
      S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

  if (create) {
    if (mkdir(lpath, mode) != 0 && errno != EEXIST) {
      return cu_Dir_Result_error(cu_Io_Error_from_errno(errno));
    }
  }

  handle = open(lpath, flags);

  if (handle == CU_INVALID_HANDLE) {
    return cu_Dir_Result_error(cu_Io_Error_from_errno(errno));
  }

  stat = cu_File_Stat_from_handle(handle);

#else
  DWORD access = FILE_LIST_DIRECTORY;
  DWORD creation = create ? OPEN_ALWAYS : OPEN_EXISTING;
  DWORD attributes = FILE_FLAG_BACKUP_SEMANTICS;

  if (create) {
    CreateDirectoryA(lpath, NULL);
  }

  handle = CreateFileA(lpath, access, FILE_SHARE_READ | FILE_SHARE_WRITE |
                                       FILE_SHARE_DELETE, NULL, creation,
      attributes, NULL);

  if (handle == INVALID_HANDLE_VALUE) {
    return cu_Dir_Result_error(cu_Io_Error_from_win32(GetLastError()));
  }

  stat = cu_File_Stat_from_handle(handle);
#endif

  cu_Dir dir;
  cu_Memory_memset(&dir, 0, sizeof(dir));
  dir.handle = handle;
  dir.stat = stat;
  return cu_Dir_Result_ok(dir);
}

void cu_Dir_close(cu_Dir *dir) {
  CU_IF_NULL(dir) return;

#if CU_PLAT_POSIX
  close(dir->handle);
#else
  CloseHandle(dir->handle);
#endif
  dir->handle = CU_INVALID_HANDLE;
}
