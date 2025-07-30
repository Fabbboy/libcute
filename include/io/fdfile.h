#pragma once
#include "io/error.h"
#include "io/fd.h"
#include "io/stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @file fdfile.h Generic file descriptor wrapper. */

typedef struct {
  cu_Handle handle; /**< raw file descriptor */
  bool owned;       /**< close handle on destroy */
} cu_FdFile;

/** Initialize from an existing handle. */
static inline cu_FdFile cu_FdFile_from_handle(cu_Handle h, bool owned) {
  cu_FdFile f;
  f.handle = h;
  f.owned = owned;
  return f;
}

void cu_FdFile_close(cu_FdFile *f);
cu_Io_Error_Optional cu_FdFile_read(cu_FdFile *f, cu_Slice buf);
cu_Io_Error_Optional cu_FdFile_write(cu_FdFile *f, cu_Slice data);
cu_Io_Error_Optional cu_FdFile_seek(cu_FdFile *f, cu_File_SeekTo to);
cu_Stream cu_FdFile_stream(cu_FdFile *f);

cu_FdFile cu_FdFile_stdin(void);
cu_FdFile cu_FdFile_stdout(void);
cu_FdFile cu_FdFile_stderr(void);

#ifdef __cplusplus
}
#endif
