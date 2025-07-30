#include "io/dir.h"
#include "io/error.h"
#include "io/fd.h"
#include "macro.h"
#include "nostd.h"
#include "object/result.h"
#include "string/fmt.h"
#include "string/string.h"
#include <stdlib.h>
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

#ifndef CU_FREESTANDING

CU_RESULT_IMPL(cu_Dir, cu_Dir, cu_Io_Error)

cu_Dir_Result cu_Dir_open(
    cu_Slice path, bool create, cu_Allocator allocator) {
  char lpath[CU_FILE_MAX_PATH_LENGTH] = {0};
  cu_Slice lpath_slice = cu_Slice_create(lpath, CU_FILE_MAX_PATH_LENGTH);
  cu_Memory_smemcpy(lpath_slice, path);

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
  cu_String_Result pres = cu_String_from_cstr(allocator, lpath);
  if (!cu_String_Result_is_ok(&pres)) {
    close(handle);
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
        .errnum = Size_Optional_none()};
    return cu_Dir_Result_error(err);
  }
  stat.path = pres.value;

#else
  DWORD access = FILE_LIST_DIRECTORY;
  DWORD creation = OPEN_EXISTING;
  if (create) {
    creation = OPEN_ALWAYS;
  }
  DWORD attributes = FILE_FLAG_BACKUP_SEMANTICS;

  if (create) {
    CreateDirectoryA(lpath, NULL);
  }

  handle = CreateFileA(lpath, access,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, creation,
      attributes, NULL);

  if (handle == INVALID_HANDLE_VALUE) {
    return cu_Dir_Result_error(cu_Io_Error_from_win32(GetLastError()));
  }

  stat = cu_File_Stat_from_handle(handle);
  pres = cu_String_from_cstr(allocator, lpath);
  if (!cu_String_Result_is_ok(&pres)) {
    CloseHandle(handle);
    cu_Io_Error err = {.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
        .errnum = Size_Optional_none()};
    return cu_Dir_Result_error(err);
  }
  stat.path = pres.value;
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
  cu_File_Stat_destroy(&dir->stat);
}

cu_String_Optional cu_Dir_Home(cu_Allocator allocator) {
#if CU_PLAT_WINDOWS
  const char *home = getenv("USERPROFILE");
  if (!home) {
    const char *drive = getenv("HOMEDRIVE");
    const char *path = getenv("HOMEPATH");
    if (drive && path) {
      cu_StrBuilder sb = cu_StrBuilder_init(allocator);
      if (cu_StrBuilder_append_cstr(&sb, drive) != CU_STRING_ERROR_NONE ||
          cu_StrBuilder_append_cstr(&sb, path) != CU_STRING_ERROR_NONE) {
        cu_StrBuilder_destroy(&sb);
        return cu_String_Optional_none();
      }
      cu_String_Result res = cu_StrBuilder_finalize(&sb);
      cu_StrBuilder_destroy(&sb);
      return cu_String_Optional_some(res.value);
    }
    return cu_String_Optional_none();
  }
#else
  const char *home = getenv("HOME");
  if (!home) {
    return cu_String_Optional_none();
  }
#endif
  cu_String_Result res = cu_String_from_cstr(allocator, home);
  if (!cu_String_Result_is_ok(&res)) {
    return cu_String_Optional_none();
  }
  return cu_String_Optional_some(res.value);
}

cu_String_Optional cu_Dir_Tmp(cu_Allocator allocator) {
#if CU_PLAT_WINDOWS
  const char *tmp = getenv("TEMP");
  if (!tmp)
    tmp = getenv("TMP");
  if (!tmp)
    return cu_String_Optional_none();
#else
  const char *tmp = getenv("TMPDIR");
  if (!tmp)
    tmp = "/tmp";
#endif
  cu_String_Result res = cu_String_from_cstr(allocator, tmp);
  if (!cu_String_Result_is_ok(&res)) {
    return cu_String_Optional_none();
  }
  return cu_String_Optional_some(res.value);
}

cu_String_Optional cu_Dir_Config(cu_Allocator allocator) {
#if CU_PLAT_WINDOWS
  const char *cfg = getenv("APPDATA");
  if (!cfg)
    cfg = getenv("LOCALAPPDATA");
  if (!cfg)
    return cu_String_Optional_none();
  cu_String_Result res = cu_String_from_cstr(allocator, cfg);
  if (!cu_String_Result_is_ok(&res)) {
    return cu_String_Optional_none();
  }
  return cu_String_Optional_some(res.value);
#else
  const char *xdg = getenv("XDG_CONFIG_HOME");
  if (xdg) {
    cu_String_Result res = cu_String_from_cstr(allocator, xdg);
    if (!cu_String_Result_is_ok(&res)) {
      return cu_String_Optional_none();
    }
    return cu_String_Optional_some(res.value);
  }
  const char *home = getenv("HOME");
  if (!home) {
    return cu_String_Optional_none();
  }
  cu_StrBuilder sb = cu_StrBuilder_init(allocator);
  if (cu_StrBuilder_append_cstr(&sb, home) != CU_STRING_ERROR_NONE ||
      cu_StrBuilder_append_cstr(&sb, "/.config") != CU_STRING_ERROR_NONE) {
    cu_StrBuilder_destroy(&sb);
    return cu_String_Optional_none();
  }
  cu_String_Result res = cu_StrBuilder_finalize(&sb);
  cu_StrBuilder_destroy(&sb);
  if (!cu_String_Result_is_ok(&res)) {
    return cu_String_Optional_none();
  }
  return cu_String_Optional_some(res.value);
#endif
}

#endif
