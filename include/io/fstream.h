#pragma once
#include "collection/ring_buffer.h"
#include "io/dir.h"
#include "io/error.h"
#include "io/file.h"
#include "io/stream.h"
#include "memory/allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @file fstream.h Buffered stream backed by files. */

typedef struct {
  cu_File file;         /**< underlying file */
  cu_RingBuffer buffer; /**< internal ring buffer */
  bool reading;         /**< true when opened in read mode */
} cu_FStream;

CU_RESULT_DECL(cu_FStream, cu_FStream, cu_Io_Error)

cu_FStream_Result cu_FStream_open(cu_Slice path, cu_File_Options options,
    size_t buffer_size, cu_Allocator allocator);
cu_FStream_Result cu_FStream_openat(cu_Dir *dir, cu_Slice path,
    cu_File_Options options, size_t buffer_size, cu_Allocator allocator);
/** View the fstream as a generic stream interface. */
cu_Stream cu_FStream_stream(cu_FStream *fs);
cu_Io_Error_Optional cu_FStream_flush(cu_FStream *stream);
cu_Io_Error_Optional cu_FStream_seek(cu_FStream *stream, cu_File_SeekTo to);
cu_IoSize_Result cu_FStream_tell(cu_FStream *stream);
/** Release stream resources and close the file. */
void cu_FStream_close(cu_FStream *stream);

#ifdef __cplusplus
}
#endif
