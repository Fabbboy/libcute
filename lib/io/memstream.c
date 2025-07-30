#include "io/memstream.h"
#include "macro.h"
#include "nostd.h"

#ifndef CU_FREESTANDING

CU_RESULT_IMPL(cu_MemStream, cu_MemStream, cu_Io_Error)

cu_MemStream_Result cu_MemStream_create(
    size_t capacity, cu_Allocator allocator) {
  cu_Vector_Result vres =
      cu_Vector_create(allocator, cu_Layout_create(sizeof(unsigned char), 1),
          Size_Optional_some(capacity), cu_Destructor_Optional_none());
  if (!cu_Vector_Result_is_ok(&vres)) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_MemStream_Result_error(err);
  }
  cu_MemStream ms;
  ms.buffer = vres.value;
  ms.pos = 0;
  return cu_MemStream_Result_ok(ms);
}

static cu_Io_Error_Optional cu_MemStream_read(void *self, cu_Slice buf) {
  cu_MemStream *ms = (cu_MemStream *)self;
  size_t avail = 0;
  if (ms->buffer.length > ms->pos) {
    avail = ms->buffer.length - ms->pos;
  }
  size_t n = buf.length;
  if (avail < n) {
    n = avail;
  }
  if (n > 0) {
    cu_Slice_Optional src = cu_Vector_subslice(&ms->buffer, ms->pos, n);
    cu_Memory_smemcpy(buf, cu_Slice_Optional_unwrap(&src));
    ms->pos += n;
  }
  if (n < buf.length) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_UNEXPECTED_EOF,
            .errnum = Size_Optional_none()});
  }
  return cu_Io_Error_Optional_none();
}

static cu_Io_Error_Optional cu_MemStream_write(void *self, cu_Slice data) {
  cu_MemStream *ms = (cu_MemStream *)self;
  size_t needed = ms->pos + data.length;
  if (needed > ms->buffer.length) {
    cu_Vector_Error_Optional err = cu_Vector_resize(&ms->buffer, needed);
    if (cu_Vector_Error_Optional_is_some(&err)) {
      return cu_Io_Error_Optional_some(
          (cu_Io_Error){.kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY,
              .errnum = Size_Optional_none()});
    }
  }
  cu_Slice_Optional dest =
      cu_Vector_subslice(&ms->buffer, ms->pos, data.length);
  cu_Memory_smemcpy(cu_Slice_Optional_unwrap(&dest), data);
  ms->pos += data.length;
  return cu_Io_Error_Optional_none();
}

static cu_Io_Error_Optional cu_MemStream_flush_impl(void *ms) {
  CU_UNUSED(ms);
  return cu_Io_Error_Optional_none();
}

static cu_Io_Error_Optional cu_MemStream_seek_impl(
    void *self, cu_File_SeekTo to) {
  cu_MemStream *ms = (cu_MemStream *)self;
  size_t base = 0;
  switch (to.whence) {
  case CU_FILE_SEEK_START:
    base = 0;
    break;
  case CU_FILE_SEEK_CURRENT:
    base = ms->pos;
    break;
  case CU_FILE_SEEK_END:
    base = ms->buffer.length;
    break;
  default:
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }
  size_t offset = 0;
  if (Size_Optional_is_some(&to.offset)) {
    offset = Size_Optional_unwrap(&to.offset);
  }
  size_t new_pos = base + offset;
  if (new_pos > ms->buffer.length) {
    return cu_Io_Error_Optional_some(
        (cu_Io_Error){.kind = CU_IO_ERROR_KIND_INVALID_INPUT,
            .errnum = Size_Optional_none()});
  }
  ms->pos = new_pos;
  return cu_Io_Error_Optional_none();
}

static cu_IoSize_Result cu_MemStream_tell_impl(void *self) {
  cu_MemStream *ms = (cu_MemStream *)self;
  return cu_IoSize_Result_ok(ms->pos);
}

static void cu_MemStream_close_impl(void *self) {
  cu_MemStream *ms = (cu_MemStream *)self;
  cu_Vector_destroy(&ms->buffer);
}

cu_Stream cu_MemStream_stream(cu_MemStream *ms) {
  cu_Stream iface;
  iface.self = ms;
  iface.readFn = cu_MemStream_read;
  iface.writeFn = cu_MemStream_write;
  iface.flushFn =  cu_MemStream_flush_impl;
  iface.closeFn = cu_MemStream_close_impl;
  iface.seekFn = cu_MemStream_seek_impl;
  iface.tellFn = cu_MemStream_tell_impl;
  return iface;
}

cu_Io_Error_Optional cu_MemStream_flush(cu_MemStream *ms) {
  return cu_MemStream_flush_impl(ms);
}

cu_Io_Error_Optional cu_MemStream_seek(cu_MemStream *ms, cu_File_SeekTo to) {
  return cu_MemStream_seek_impl(ms, to);
}

cu_IoSize_Result cu_MemStream_tell(const cu_MemStream *ms) {
  return cu_IoSize_Result_ok(ms->pos);
}

void cu_MemStream_close(cu_MemStream *ms) { cu_MemStream_close_impl(ms); }

cu_Slice_Optional cu_MemStream_slice(const cu_MemStream *ms) {
  return cu_Vector_slice(&ms->buffer);
}

#endif // CU_FREESTANDING
