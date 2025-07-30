#pragma once
#include "collection/vector.h"
#include "io/error.h"
#include "io/stream.h"
#include "memory/allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @file memstream.h Memory-backed buffered stream. */

typedef struct {
  cu_Vector buffer; /**< underlying storage */
  size_t pos;       /**< current cursor position */
} cu_MemStream;

CU_RESULT_DECL(cu_MemStream, cu_MemStream, cu_Io_Error)

cu_MemStream_Result cu_MemStream_create(
    size_t capacity, cu_Allocator allocator);

cu_Stream cu_MemStream_stream(cu_MemStream *ms);
cu_Io_Error_Optional cu_MemStream_flush(cu_MemStream *ms);
cu_Io_Error_Optional cu_MemStream_seek(cu_MemStream *ms, cu_File_SeekTo to);
void cu_MemStream_close(cu_MemStream *ms);

/** View the entire contents as a slice. */
cu_Slice_Optional cu_MemStream_slice(const cu_MemStream *ms);

#ifdef __cplusplus
}
#endif
