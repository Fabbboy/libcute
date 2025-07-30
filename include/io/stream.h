#pragma once
#include "io/error.h"
#include "io/file.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @file stream.h Generic buffered stream interface. */

typedef cu_Io_Error_Optional (*cu_Stream_ReadFn)(void *self, cu_Slice buffer);
typedef cu_Io_Error_Optional (*cu_Stream_WriteFn)(void *self, cu_Slice data);
typedef cu_Io_Error_Optional (*cu_Stream_FlushFn)(void *self);
typedef void (*cu_Stream_CloseFn)(void *self);
typedef cu_Io_Error_Optional (*cu_Stream_SeekFn)(void *self, cu_File_SeekTo to);

/** Generic stream interface. */
typedef struct {
  void *self;
  cu_Stream_ReadFn readFn;
  cu_Stream_WriteFn writeFn;
  cu_Stream_FlushFn flushFn;
  cu_Stream_CloseFn closeFn;
  cu_Stream_SeekFn seekFn;
} cu_Stream;

static inline cu_Io_Error_Optional cu_Stream_read(
    cu_Stream *stream, cu_Slice buffer) {
  return stream->readFn(stream->self, buffer);
}

static inline cu_Io_Error_Optional cu_Stream_write(
    cu_Stream *stream, cu_Slice data) {
  return stream->writeFn(stream->self, data);
}

static inline cu_Io_Error_Optional cu_Stream_flush(cu_Stream *stream) {
  return stream->flushFn(stream->self);
}

static inline void cu_Stream_close(cu_Stream *stream) {
  stream->closeFn(stream->self);
}

static inline cu_Io_Error_Optional cu_Stream_seek(
    cu_Stream *stream, cu_File_SeekTo to) {
  return stream->seekFn(stream->self, to);
}

#ifdef __cplusplus
}
#endif
