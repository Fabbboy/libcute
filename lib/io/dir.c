#include "io/dir.h"
#include "io/error.h"
#include "io/fd.h"
#include "macro.h"
#include "nostd.h"
#include <errno.h>
#include <fcntl.h>
#include <fcntl.h> // for O_RDONLY, O_DIRECTORY, openat, AT_FDCWD
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h> // technically optional, but traditional
#include <unistd.h>
#include <unistd.h> // for close()

// POSIX IMPL ONLY
//  TODO: ADD WINDOWS IMPL

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
#if CU_PLAT_POSIX
  int flags = O_RDONLY;
  if (create) {
    flags |= O_CREAT;
  }

  int mode =
      S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

  handle = open(lpath, flags, mode);

  if (handle == CU_INVALID_HANDLE) {
    return cu_Dir_Result_error(cu_Io_Error_from_errno(errno));
  }

  struct stat st;
  if (fstat(handle, &st) == -1) {
    close(handle);
    return cu_Dir_Result_error(cu_Io_Error_from_errno(errno));
  }

  if (!S_ISDIR(st.st_mode)) {
    close(handle);
    return cu_Dir_Result_error(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }

#else
#error "Windows implementation not yet supported"
#endif

  cu_Dir dir = {.handle = handle};
  return cu_Dir_Result_ok(dir);
}

void cu_Dir_close(cu_Dir *dir) {
  CU_IF_NULL(dir) return;

#if CU_PLAT_POSIX
  close(dir->handle);
#else
#error "Windows implementation not yet supported"
#endif
}
