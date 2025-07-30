#include "io/fstream.h"
#include "macro.h"
#include "nostd.h"

#ifndef CU_FREESTANDING

CU_RESULT_IMPL(cu_FStream, cu_FStream, cu_Io_Error)

static cu_Io_Error_Optional cu_FStream_flush_buffer(cu_FStream *fs) {
  size_t len = cu_RingBuffer_size(&fs->buffer);
  if (len == 0) {
    return cu_Io_Error_Optional_none();
  }

  cu_IoSlice_Result mem =
      cu_Allocator_Alloc(fs->buffer.allocator, cu_Layout_create(len, 1));
  if (!cu_IoSlice_Result_is_ok(&mem)) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
            .errnum = Size_Optional_none()});
  }

  unsigned char *tmp = mem.value.ptr;
  for (size_t i = 0; i < len; ++i) {
    cu_RingBuffer_pop(&fs->buffer, tmp + i);
  }

  cu_Io_Error_Optional err =
      cu_File_write(&fs->file, cu_Slice_create(tmp, len));
  cu_Allocator_Free(fs->buffer.allocator, cu_Slice_create(tmp, len));
  return err;
}

static cu_Io_Error_Optional cu_FStream_flush_impl(cu_FStream *fs) {
  if (!fs->reading) {
    return cu_FStream_flush_buffer(fs);
  }
  return cu_Io_Error_Optional_none();
}

static cu_Io_Error_Optional cu_FStream_fill_buffer(cu_FStream *fs) {
  cu_IoSlice_Result mem = cu_Allocator_Alloc(
      fs->buffer.allocator, cu_Layout_create(fs->buffer.capacity, 1));
  if (!cu_IoSlice_Result_is_ok(&mem)) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
            .errnum = Size_Optional_none()});
  }

  unsigned char *tmp = mem.value.ptr;
  cu_Io_Error_Optional err =
      cu_File_read(&fs->file, cu_Slice_create(tmp, fs->buffer.capacity));
  if (cu_Io_Error_Optional_is_some(&err)) {
    cu_Allocator_Free(
        fs->buffer.allocator, cu_Slice_create(tmp, fs->buffer.capacity));
    return err;
  }

  for (size_t j = 0; j < fs->buffer.capacity; ++j) {
    cu_RingBuffer_push(&fs->buffer, tmp + j);
  }
  cu_Allocator_Free(
      fs->buffer.allocator, cu_Slice_create(tmp, fs->buffer.capacity));
  return cu_Io_Error_Optional_none();
}

static cu_Io_Error_Optional cu_FStream_read(void *self, cu_Slice buf) {
  cu_FStream *fs = (cu_FStream *)self;
  if (!fs->reading) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }

  for (size_t i = 0; i < buf.length; ++i) {
    if (cu_RingBuffer_is_empty(&fs->buffer)) {
      cu_Io_Error_Optional err = cu_FStream_fill_buffer(fs);
      if (cu_Io_Error_Optional_is_some(&err)) {
        return err;
      }
    }

    cu_RingBuffer_pop(&fs->buffer, (unsigned char *)buf.ptr + i);
  }

  return cu_Io_Error_Optional_none();
}

static cu_Io_Error_Optional cu_FStream_write(void *self, cu_Slice data) {
  cu_FStream *fs = (cu_FStream *)self;
  if (fs->reading) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }

  for (size_t i = 0; i < data.length; ++i) {
    if (cu_RingBuffer_is_full(&fs->buffer)) {
      cu_Io_Error_Optional err = cu_FStream_flush_buffer(fs);
      if (cu_Io_Error_Optional_is_some(&err)) {
        return err;
      }
    }

    cu_RingBuffer_push(&fs->buffer, (unsigned char *)data.ptr + i);
  }

  return cu_Io_Error_Optional_none();
}

static cu_Io_Error_Optional cu_FStream_flush_iface(void *self) {
  return cu_FStream_flush_impl((cu_FStream *)self);
}

static cu_Io_Error_Optional cu_FStream_seek_impl(
    void *self, cu_File_SeekTo to) {
  cu_FStream *fs = (cu_FStream *)self;
  cu_Io_Error_Optional err = cu_FStream_flush_impl(fs);
  if (cu_Io_Error_Optional_is_some(&err)) {
    return err;
  }
  cu_RingBuffer_clear(&fs->buffer);
  return cu_File_seek(&fs->file, to);
}

static void cu_FStream_close_impl(void *self) {
  cu_FStream *fs = (cu_FStream *)self;
  (void)cu_FStream_flush_impl(fs);
  cu_File_close(&fs->file);
  cu_RingBuffer_destroy(&fs->buffer);
}

cu_FStream_Result cu_FStream_open(cu_Slice path, cu_File_Options options,
    size_t buffer_size, cu_Allocator allocator) {
  cu_File_Result fres = cu_File_open(path, options, allocator);
  if (!cu_File_Result_is_ok(&fres)) {
    return cu_FStream_Result_error(cu_File_Result_unwrap_error(&fres));
  }
  cu_File file = cu_File_Result_unwrap(&fres);

  cu_RingBuffer_Result rbres = cu_RingBuffer_create(allocator,
      cu_Layout_create(sizeof(unsigned char), 1), buffer_size,
      cu_Destructor_Optional_none());
  if (!cu_RingBuffer_Result_is_ok(&rbres)) {
    cu_File_close(&file);
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_FStream_Result_error(err);
  }

  cu_FStream fs;
  fs.file = file;
  fs.buffer = rbres.value;
  fs.reading = options.read && !options.write;

  return cu_FStream_Result_ok(fs);
}

cu_Stream cu_FStream_stream(cu_FStream *fs) {
  cu_Stream iface;
  iface.self = fs;
  iface.readFn = cu_FStream_read;
  iface.writeFn = cu_FStream_write;
  iface.flushFn = cu_FStream_flush_iface;
  iface.closeFn = cu_FStream_close_impl;
  iface.seekFn = cu_FStream_seek_impl;
  return iface;
}

cu_Io_Error_Optional cu_FStream_flush(cu_FStream *stream) {
  return cu_FStream_flush_impl(stream);
}

cu_Io_Error_Optional cu_FStream_seek(cu_FStream *stream, cu_File_SeekTo to) {
  return cu_FStream_seek_impl(stream, to);
}

void cu_FStream_close(cu_FStream *stream) { cu_FStream_close_impl(stream); }

#endif // CU_FREESTANDING
