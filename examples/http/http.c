#include "http.h"
#include "collection/vector.h"
#include "io/file.h"
#include "macro.h"
#include "memory/allocator.h"
#include "object/optional.h"
#include <nostd.h>
#if CU_PLAT_WINDOWS
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <stdio.h> // Add for debugging
#include <string.h>
// Portable memmem implementation for platforms where it's missing
static void *portable_memmem(const void *haystack, size_t haystacklen,
    const void *needle, size_t needlelen) {
  if (needlelen == 0)
    return (void *)haystack;
  if (haystacklen < needlelen)
    return NULL;
  for (size_t i = 0; i <= haystacklen - needlelen; ++i) {
    if (memcmp((const char *)haystack + i, needle, needlelen) == 0)
      return (void *)((const char *)haystack + i);
  }
  return NULL;
}

#ifndef CU_FREESTANDING

CU_RESULT_IMPL(cu_HttpServer, cu_HttpServer, cu_Http_Error)

static int set_nonblocking(cu_socket_t fd) {
#if CU_PLAT_WINDOWS
  u_long mode = 1;
  return ioctlsocket(fd, FIONBIO, &mode);
#else
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

static void close_client(cu_HttpServer *server, cu_socket_t fd) {
#if CU_PLAT_WINDOWS
  closesocket(fd);
#else
  close(fd);
#endif
  for (size_t i = 0; i < cu_Vector_size(&server->clients); ++i) {
    Ptr_Optional ptr = cu_Vector_at(&server->clients, i);
    if (Ptr_Optional_is_some(&ptr) &&
        *CU_AS(Ptr_Optional_unwrap(&ptr), int *) == fd) {
      size_t last = cu_Vector_size(&server->clients) - 1;
      if (i != last) {
        ptr = cu_Vector_at(&server->clients, last);
      }
      int dummy;
      cu_Vector_pop_back(&server->clients, &dummy);
      break;
    }
  }
}

static void send_simple(cu_socket_t fd, int code, const char *msg) {
  char buf[128];
  int len = cu_CString_snprintf(buf, sizeof(buf),
      "HTTP/1.0 %d %s\r\nContent-Length: 0\r\n\r\n", code, msg);
  send(fd, buf, (int)len, 0);
}

static void handle_client(cu_HttpServer *server, cu_socket_t fd) {
  const size_t CHUNK_SIZE = 1024;
  const size_t MAX_REQUEST_SIZE = 8192;

  char *full_req = NULL;
  size_t total = 0;

  while (true) {
    if (total + CHUNK_SIZE > MAX_REQUEST_SIZE) {
      printf("DEBUG: Request too large (%zu bytes)\n", total);
      send_simple(fd, 413, "Request Entity Too Large");
      goto fail;
    }

    cu_IoSlice_Result chunk_res =
        cu_Allocator_Alloc(server->slab_alloc, cu_Layout_create(CHUNK_SIZE, 1));
    if (!cu_IoSlice_Result_is_ok(&chunk_res)) {
      printf("DEBUG: Failed to allocate chunk\n");
      goto fail;
    }

    char *chunk = (char *)chunk_res.value.ptr;
#if CU_PLAT_WINDOWS
    int r = recv(fd, chunk, CHUNK_SIZE, 0);
    if (r <= 0) {
      printf("DEBUG: Read error or client closed (r=%d)\n", r);
      cu_Allocator_Free(server->slab_alloc, chunk_res.value);
      goto fail;
    }
#else
    ssize_t r = read(fd, chunk, CHUNK_SIZE);
    if (r <= 0) {
      printf("DEBUG: Read error or client closed (r=%zd)\n", r);
      cu_Allocator_Free(server->slab_alloc, chunk_res.value);
      goto fail;
    }
#endif

    // Allocate new buffer
    cu_IoSlice_Result resize_res = cu_Allocator_Alloc(
        server->slab_alloc, cu_Layout_create(total + r + 1, 1));
    if (!cu_IoSlice_Result_is_ok(&resize_res)) {
      printf("DEBUG: Realloc fallback failed\n");
      cu_Allocator_Free(server->slab_alloc, chunk_res.value);
      goto fail;
    }

    if (full_req) {
      memcpy(resize_res.value.ptr, full_req, total);
      cu_Allocator_Free(
          server->slab_alloc, (cu_Slice){.ptr = full_req, .length = total});
    }

    memcpy((char *)resize_res.value.ptr + total, chunk, r);
    cu_Allocator_Free(server->slab_alloc, chunk_res.value);
    full_req = (char *)resize_res.value.ptr;
    total += r;
    full_req[total] = '\0';

    // Check if we reached the end of headers
    if (total >= 4 && portable_memmem(full_req, total, "\r\n\r\n", 4)) {
      break;
    }
  }

  printf("DEBUG: Received request (%zu bytes):\n%.*s\n", total, (int)total,
      full_req);

  if (strncmp(full_req, "GET ", 4) != 0) {
    printf("DEBUG: Not a GET request\n");
    send_simple(fd, 400, "Bad Request");
    goto fail;
  }

  char *path = full_req + 4;
  char *end = strchr(path, ' ');
  if (!end) {
    printf("DEBUG: No space found after path\n");
    send_simple(fd, 400, "Bad Request");
    goto fail;
  }

  *end = '\0';
  printf("DEBUG: Requested path: '%s'\n", path);

  char fs_path[512];
  cu_CString_snprintf(fs_path, sizeof(fs_path), ".%s", path);
  printf("DEBUG: File system path: '%s'\n", fs_path);

  cu_File_Options fopts = {0};
  cu_File_Options_read(&fopts);
  cu_File_Result fres =
      cu_File_open(CU_SLICE_CSTR(fs_path), fopts, cu_Allocator_CAllocator());
  if (!cu_File_Result_is_ok(&fres)) {
    printf("DEBUG: File not found: %s\n", fs_path);
    send_simple(fd, 404, "Not Found");
    goto fail;
  }

  cu_File file = fres.value;
  unsigned long long fsize = file.stat.size;

  char header[256];
  int header_len = cu_CString_snprintf(header, sizeof(header),
      "HTTP/1.0 200 OK\r\nContent-Length: %llu\r\n\r\n", fsize);
  printf("DEBUG: Sending header: %s", header);
  send(fd, header, header_len, 0);

  cu_IoSlice_Result buf_res =
      cu_Allocator_Alloc(server->slab_alloc, cu_Layout_create(4096, 1));
  if (!cu_IoSlice_Result_is_ok(&buf_res)) {
    printf("DEBUG: Failed to allocate file buffer\n");
    cu_File_close(&file);
    goto fail;
  }

  char *buf = (char *)buf_res.value.ptr;
  unsigned long long remaining = fsize;
  while (remaining > 0) {
    size_t chunk =
        remaining < buf_res.value.length ? remaining : buf_res.value.length;
    cu_Io_Error_Optional err = cu_File_read(&file, cu_Slice_create(buf, chunk));
    if (cu_Io_Error_Optional_is_some(&err)) {
      printf("DEBUG: Failed to read file\n");
      cu_File_close(&file);
      cu_Allocator_Free(server->slab_alloc, buf_res.value);
      goto fail;
    }
    send(fd, buf, (int)chunk, 0);
    remaining -= chunk;
  }

  printf("DEBUG: Sent %llu bytes of file content\n", fsize);
  cu_File_close(&file);
  cu_Allocator_Free(server->slab_alloc, buf_res.value);
  cu_Allocator_Free(
      server->slab_alloc, (cu_Slice){.ptr = full_req, .length = total});
  close_client(server, fd);
  return;

fail:
  if (full_req) {
    cu_Allocator_Free(
        server->slab_alloc, (cu_Slice){.ptr = full_req, .length = total});
  }
  close_client(server, fd);
}

cu_HttpServer_Result cu_HttpServer_create(
    cu_Allocator allocator, uint16_t port) {
  cu_Vector_Result vec_res = cu_Vector_create(allocator, CU_LAYOUT(int),
      Size_Optional_some(16), cu_Destructor_Optional_none());
  if (!cu_Vector_Result_is_ok(&vec_res)) {
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_EPOLL);
  }

#if CU_PLAT_WINDOWS
  WSADATA ws;
  if (WSAStartup(MAKEWORD(2, 2), &ws) != 0) {
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_SOCKET);
  }
  cu_socket_t sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sfd == INVALID_SOCKET) {
    cu_Vector_destroy(&vec_res.value);
    WSACleanup();
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_SOCKET);
  }
#else
  cu_socket_t sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd < 0) {
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_SOCKET);
  }
#endif

  set_nonblocking(sfd);

  int opt = 1;
  setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
#if CU_PLAT_WINDOWS
    closesocket(sfd);
    WSACleanup();
#else
    close(sfd);
#endif
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_SOCKET);
  }
  if (listen(sfd, 16) < 0) {
#if CU_PLAT_WINDOWS
    closesocket(sfd);
    WSACleanup();
#else
    close(sfd);
#endif
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_SOCKET);
  }

#if CU_PLAT_WINDOWS
  cu_socket_t epfd = 0;
#else
  cu_socket_t epfd = epoll_create1(0);
  if (epfd < 0) {
    close(sfd);
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_EPOLL);
  }
  struct epoll_event ev = {0};
  ev.events = EPOLLIN;
  ev.data.fd = sfd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);
#endif

  cu_HttpServer server = {0};
  server.listen_fd = sfd;
  server.epoll_fd = epfd;
  server.clients = vec_res.value;
  cu_SlabAllocator_Config scfg = {0};
  scfg.slabSize = 4096;
  scfg.backingAllocator = cu_Allocator_Optional_some(allocator);
  server.slab_alloc = cu_Allocator_SlabAllocator(&server.slab, scfg);

  printf("DEBUG: HTTP server created on port %d\n", port);
  return cu_HttpServer_Result_ok(server);
}

void cu_HttpServer_destroy(cu_HttpServer *server) {
#if CU_PLAT_WINDOWS
  closesocket(server->listen_fd);
#else
  close(server->listen_fd);
  close(server->epoll_fd);
#endif
  for (size_t i = 0; i < cu_Vector_size(&server->clients); ++i) {
    Ptr_Optional ptr = cu_Vector_at(&server->clients, i);
    if (Ptr_Optional_is_some(&ptr)) {
      cu_socket_t *fd = CU_AS(Ptr_Optional_unwrap(&ptr), cu_socket_t *);
      printf("DEBUG: Closing client fd=%ld\n", (long)*fd);
#if CU_PLAT_WINDOWS
      closesocket(*fd);
#else
      close(*fd);
#endif
    }
  }
  cu_Vector_destroy(&server->clients);
  cu_SlabAllocator_destroy(&server->slab);
#if CU_PLAT_WINDOWS
  WSACleanup();
#endif
}

void cu_HttpServer_run(cu_HttpServer *server) {
#if CU_PLAT_WINDOWS
  printf("DEBUG: Server running, waiting for connections...\n");
  while (true) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server->listen_fd, &readfds);
    cu_socket_t maxfd = server->listen_fd;
    for (size_t i = 0; i < cu_Vector_size(&server->clients); ++i) {
      Ptr_Optional ptr = cu_Vector_at(&server->clients, i);
      if (Ptr_Optional_is_some(&ptr)) {
        cu_socket_t *fd = CU_AS(Ptr_Optional_unwrap(&ptr), cu_socket_t *);
        FD_SET(*fd, &readfds);
        if (*fd > maxfd) {
          maxfd = *fd;
        }
      }
    }

    int n = select((int)(maxfd + 1), &readfds, NULL, NULL, NULL);
    if (n <= 0) {
      continue;
    }

    if (FD_ISSET(server->listen_fd, &readfds)) {
      cu_socket_t cfd = accept(server->listen_fd, NULL, NULL);
      if (cfd != INVALID_SOCKET) {
        set_nonblocking(cfd);
        cu_Vector_push_back(&server->clients, &cfd);
      }
    }

    for (size_t i = 0; i < cu_Vector_size(&server->clients); ++i) {
      Ptr_Optional ptr = cu_Vector_at(&server->clients, i);
      if (Ptr_Optional_is_some(&ptr)) {
        cu_socket_t fd = *CU_AS(Ptr_Optional_unwrap(&ptr), cu_socket_t *);
        if (FD_ISSET(fd, &readfds)) {
          handle_client(server, fd);
          int dummy;
          cu_Vector_pop_back(&server->clients, &dummy);
        }
      }
    }
  }
#else
  struct epoll_event events[16];
  printf("DEBUG: Server running, waiting for connections...\n");
  while (true) {
    int n = epoll_wait(server->epoll_fd, events, CU_ARRAY_LEN(events), -1);
    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;
      if (fd == server->listen_fd) {
        int cfd = accept(server->listen_fd, NULL, NULL);
        if (cfd >= 0) {
          set_nonblocking(cfd);
          struct epoll_event ev = {0};
          ev.events = EPOLLIN;
          ev.data.fd = cfd;
          epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, cfd, &ev);
          cu_Vector_push_back(&server->clients, &cfd);
        }
      } else {
        handle_client(server, fd);
        epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
      }
    }
  }
#endif
}

#endif