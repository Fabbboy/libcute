#define _GNU_SOURCE
#include "cute.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

typedef struct {
  cu_Allocator allocator;
} server_ctx;

static bool build_response(
    server_ctx *ctx, const char *path, cu_String *response) {
  FILE *f = fopen(path, "rb");
  cu_StrBuilder b = cu_StrBuilder_init(ctx->allocator);
  if (!f) {
    if (cu_StrBuilder_append_cstr(&b, "HTTP/1.0 404 Not Found\r\n\r\n") !=
        CU_STRING_ERROR_NONE) {
      cu_StrBuilder_destroy(&b);
      return false;
    }
  } else {
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    cu_Slice_Optional file_mem =
        cu_Allocator_Alloc(ctx->allocator, (size_t)sz, 1);
    if (cu_Slice_Optional_is_none(&file_mem)) {
      fclose(f);
      cu_StrBuilder_destroy(&b);
      return false;
    }
    fread(file_mem.value.ptr, 1, (size_t)sz, f);
    fclose(f);
    if (cu_StrBuilder_append_cstr(&b, "HTTP/1.0 200 OK\r\n") !=
            CU_STRING_ERROR_NONE ||
        cu_StrBuilder_appendf(&b, "Content-Length: %ld\r\n\r\n", sz) !=
            CU_STRING_ERROR_NONE ||
        cu_StrBuilder_append_slice(
            &b, cu_Slice_create(file_mem.value.ptr, (size_t)sz)) !=
            CU_STRING_ERROR_NONE) {
      cu_Allocator_Free(
          ctx->allocator, cu_Slice_create(file_mem.value.ptr, (size_t)sz));
      cu_StrBuilder_destroy(&b);
      return false;
    }
    cu_Allocator_Free(
        ctx->allocator, cu_Slice_create(file_mem.value.ptr, (size_t)sz));
  }
  cu_String_Result res = cu_StrBuilder_finalize(&b);
  cu_StrBuilder_destroy(&b);
  if (!cu_String_result_is_ok(&res)) {
    return false;
  }
  *response = res.value;
  return true;
}

typedef struct {
  uv_tcp_t handle;
  uv_write_t write_req;
  cu_String response;
  server_ctx *ctx;
} client_t;

static void alloc_buffer(
    uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  server_ctx *ctx = handle->loop->data;
  cu_Slice_Optional mem = cu_Allocator_Alloc(ctx->allocator, suggested_size, 1);
  if (cu_Slice_Optional_is_none(&mem)) {
    buf->base = NULL;
    buf->len = 0;
    return;
  }
  buf->base = mem.value.ptr;
  buf->len = mem.value.length;
}

static void on_close(uv_handle_t *handle) {
  client_t *client = handle->data;
  cu_String_destroy(&client->response);
  cu_Allocator_Free(
      client->ctx->allocator, cu_Slice_create(client, sizeof(*client)));
}

static void after_write(uv_write_t *req, int status) {
  (void)status;
  uv_close((uv_handle_t *)req->handle, on_close);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  client_t *client = stream->data;
  if (nread > 0) {
    char path[1024] = {0};
    if (sscanf(buf->base, "GET %1023s", path) != 1) {
      strcpy(path, "index.html");
    }
    if (strcmp(path, "/") == 0) {
      strcpy(path, "index.html");
    } else if (path[0] == '/') {
      memmove(path, path + 1, strlen(path));
    }
    if (!build_response(client->ctx, path, &client->response)) {
      uv_close((uv_handle_t *)stream, on_close);
    } else {
      uv_buf_t resbuf = uv_buf_init(
          (char *)client->response.data, (unsigned int)client->response.length);
      uv_write(&client->write_req, stream, &resbuf, 1, after_write);
    }
  } else {
    uv_close((uv_handle_t *)stream, on_close);
  }
  if (buf->base) {
    cu_Allocator_Free(
        client->ctx->allocator, cu_Slice_create(buf->base, buf->len));
  }
}

static void on_new_connection(uv_stream_t *server, int status) {
  if (status < 0) {
    fprintf(stderr, "new connection error: %s\n", uv_strerror(status));
    return;
  }
  server_ctx *ctx = server->loop->data;
  cu_Slice_Optional mem =
      cu_Allocator_Alloc(ctx->allocator, sizeof(client_t), alignof(client_t));
  if (cu_Slice_Optional_is_none(&mem)) {
    return;
  }
  client_t *client = mem.value.ptr;
  client->ctx = ctx;
  uv_tcp_init(server->loop, &client->handle);
  client->handle.data = client;
  if (uv_accept(server, (uv_stream_t *)&client->handle) == 0) {
    uv_read_start((uv_stream_t *)&client->handle, alloc_buffer, on_read);
  } else {
    uv_close((uv_handle_t *)&client->handle, on_close);
  }
}

int main(void) {
  uv_loop_t *loop = uv_default_loop();
  server_ctx ctx = {0};
  ctx.allocator = cu_Allocator_CAllocator();
  loop->data = &ctx;
  uv_tcp_t server;
  uv_tcp_init(loop, &server);
  struct sockaddr_in addr;
  uv_ip4_addr("0.0.0.0", 8000, &addr);
  uv_tcp_bind(&server, (const struct sockaddr *)&addr, 0);
  int r = uv_listen((uv_stream_t *)&server, 128, on_new_connection);
  if (r) {
    fprintf(stderr, "listen error: %s\n", uv_strerror(r));
    return 1;
  }
  printf("Listening on http://localhost:8000\n");
  return uv_run(loop, UV_RUN_DEFAULT);
}
