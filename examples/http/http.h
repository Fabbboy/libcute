#pragma once

#include "cute.h"
#include <stdint.h>

#ifndef CU_FREESTANDING

/**\@file http.h Simple epoll-based HTTP 1.0 server using libcute types. */

#if CU_PLAT_WINDOWS
#include <winsock2.h>
typedef SOCKET cu_socket_t;
#else
typedef int cu_socket_t;
#endif

typedef struct {
  cu_socket_t listen_fd;
  cu_socket_t epoll_fd;
  cu_Vector clients;       /**< stores client sockets */
  cu_SlabAllocator slab;   /**< slab allocator for buffers */
  cu_Allocator slab_alloc; /**< allocator derived from slab */
} cu_HttpServer;

typedef enum {
  CU_HTTP_ERROR_NONE = 0,
  CU_HTTP_ERROR_SOCKET,
  CU_HTTP_ERROR_EPOLL,
} cu_Http_Error;

CU_RESULT_DECL(cu_HttpServer, cu_HttpServer, cu_Http_Error)

cu_HttpServer_Result cu_HttpServer_create(
    cu_Allocator allocator, uint16_t port);
void cu_HttpServer_destroy(cu_HttpServer *server);
void cu_HttpServer_run(cu_HttpServer *server);

#endif
