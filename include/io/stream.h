#pragma once

#include "io/error.h"
#include "object/slice.h"
#include <stddef.h>

typedef enum {
  CU_STREAM_WHENCE_SET = SEEK_SET,
  CU_STREAM_WHENCE_CURRENT = SEEK_CUR,
  CU_STREAM_WHENCE_END = SEEK_END,
} cu_Stream_Whence;

typedef cu_Io_Error_Optional (*cu_Stream_ReadFunc)(
    void *self, cu_Slice out); // read present data or from seeker
typedef cu_Io_Error_Optional (*cu_Stream_WriteFunc)(void *self,
    const cu_Slice *in); // write to the stream (if supported at seeker)
typedef void (*cu_Stream_CloseFunc)(
    void *self); // close the stream and all related resources

typedef cu_Io_Error_Optional (*cu_Stream_SeekFunc)(void *self, size_t offset,
    cu_Stream_Whence whence); // seek in the stream, if supported

typedef struct {
  void *self;

  cu_Stream_ReadFunc readFn;
  cu_Stream_WriteFunc writeFn;
  cu_Stream_CloseFunc closeFn;

  cu_Stream_SeekFunc seekFn;
} cu_Stream;

static inline cu_Io_Error_Optional cu_Stream_Read(
    cu_Stream stream, cu_Slice out) {
  return stream.readFn(stream.self, out);
}

static inline cu_Io_Error_Optional cu_Stream_Write(
    cu_Stream stream, const cu_Slice *in) {
  return stream.writeFn(stream.self, in);
}

static inline void cu_Stream_Close(cu_Stream stream) {
  stream.closeFn(stream.self);
}

static inline cu_Io_Error_Optional cu_Stream_Seek(
    cu_Stream stream, size_t offset, cu_Stream_Whence whence) {
  return stream.seekFn(stream.self, offset, whence);
}
