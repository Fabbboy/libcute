#include "http.h"
#include "macro.h"
#include "memory/allocator.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <nostd.h>
#include <stdio.h> // Add for debugging
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>

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
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef CU_FREESTANDING

CU_RESULT_IMPL(cu_HttpServer, cu_HttpServer, cu_Http_Error)

static int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void close_client(cu_HttpServer *server, int fd) {
  close(fd);
  for (size_t i = 0; i < cu_Vector_size(&server->clients); ++i) {
    int *ptr = CU_VECTOR_AT_AS(&server->clients, int, i);
    if (*ptr == fd) {
      size_t last = cu_Vector_size(&server->clients) - 1;
      if (i != last) {
        int *last_ptr = CU_VECTOR_AT_AS(&server->clients, int, last);
        *ptr = *last_ptr;
      }
      int dummy;
      cu_Vector_pop_back(&server->clients, &dummy);
      break;
    }
  }
}

static void send_simple(int fd, int code, const char *msg) {
  char buf[128];
  int len = cu_CString_snprintf(buf, sizeof(buf),
      "HTTP/1.0 %d %s\r\nContent-Length: 0\r\n\r\n", code, msg);
  write(fd, buf, (size_t)len);
}

static void handle_client(cu_HttpServer *server, int fd) {
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

    cu_Slice_Result chunk_res =
        cu_Allocator_Alloc(server->slab_alloc, cu_Layout_create(CHUNK_SIZE, 1));
    if (!cu_Slice_Result_is_ok(&chunk_res)) {
      printf("DEBUG: Failed to allocate chunk\n");
      goto fail;
    }

    char *chunk = (char *)chunk_res.value.ptr;
    ssize_t r = read(fd, chunk, CHUNK_SIZE);
    if (r <= 0) {
      printf("DEBUG: Read error or client closed (r=%zd)\n", r);
      cu_Allocator_Free(server->slab_alloc, chunk_res.value);
      goto fail;
    }

    // Allocate new buffer
    cu_Slice_Result resize_res = cu_Allocator_Alloc(
        server->slab_alloc, cu_Layout_create(total + r + 1, 1));
    if (!cu_Slice_Result_is_ok(&resize_res)) {
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

  int ffd = open(fs_path, O_RDONLY);
  if (ffd < 0) {
    printf("DEBUG: File not found: %s\n", fs_path);
    send_simple(fd, 404, "Not Found");
    goto fail;
  }

  struct stat st;
  if (fstat(ffd, &st) < 0) {
    printf("DEBUG: Failed to stat file\n");
    close(ffd);
    send_simple(fd, 500, "Internal Server Error");
    goto fail;
  }

  char header[256];
  int header_len = cu_CString_snprintf(header, sizeof(header),
      "HTTP/1.0 200 OK\r\nContent-Length: %ld\r\n\r\n", (long)st.st_size);
  printf("DEBUG: Sending header: %s", header);
  write(fd, header, (size_t)header_len);

  cu_Slice_Result buf_res =
      cu_Allocator_Alloc(server->slab_alloc, cu_Layout_create(4096, 1));
  if (!cu_Slice_Result_is_ok(&buf_res)) {
    printf("DEBUG: Failed to allocate file buffer\n");
    close(ffd);
    goto fail;
  }

  char *buf = (char *)buf_res.value.ptr;
  ssize_t n;
  ssize_t total_sent = 0;
  while ((n = read(ffd, buf, buf_res.value.length)) > 0) {
    write(fd, buf, (size_t)n);
    total_sent += n;
  }

  printf("DEBUG: Sent %ld bytes of file content\n", total_sent);
  close(ffd);
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
  cu_Vector_Result vec_res =
      cu_Vector_create(allocator, CU_LAYOUT(int), Size_Optional_some(16));
  if (!cu_Vector_Result_is_ok(&vec_res)) {
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_EPOLL);
  }

  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd < 0) {
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_SOCKET);
  }
  set_nonblocking(sfd);

  int opt = 1;
  setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(sfd);
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_SOCKET);
  }
  if (listen(sfd, 16) < 0) {
    close(sfd);
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_SOCKET);
  }

  int epfd = epoll_create1(0);
  if (epfd < 0) {
    close(sfd);
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_Result_error(CU_HTTP_ERROR_EPOLL);
  }
  struct epoll_event ev = {0};
  ev.events = EPOLLIN;
  ev.data.fd = sfd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);

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
  close(server->listen_fd);
  close(server->epoll_fd);
  for (size_t i = 0; i < cu_Vector_size(&server->clients); ++i) {
    int *fd = CU_VECTOR_AT_AS(&server->clients, int, i);
    close(*fd);
  }
  cu_Vector_destroy(&server->clients);
  cu_SlabAllocator_destroy(&server->slab);
}

void cu_HttpServer_run(cu_HttpServer *server) {
  struct epoll_event events[16];
  printf("DEBUG: Server running, waiting for connections...\n");
  while (true) {
    int n = epoll_wait(server->epoll_fd, events, CU_ARRAY_LEN(events), -1);
    printf("DEBUG: epoll_wait returned %d events\n", n);
    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;
      if (fd == server->listen_fd) {
        printf("DEBUG: New connection on listen socket\n");
        int cfd = accept(server->listen_fd, NULL, NULL);
        if (cfd >= 0) {
          printf("DEBUG: Accepted connection, fd=%d\n", cfd);
          set_nonblocking(cfd);
          struct epoll_event ev = {0};
          ev.events = EPOLLIN;
          ev.data.fd = cfd;
          epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, cfd, &ev);
          cu_Vector_push_back(&server->clients, &cfd);
        } else {
          printf("DEBUG: Failed to accept connection\n");
        }
      } else {
        printf("DEBUG: Data available on client fd=%d\n", fd);
        handle_client(server, fd);
        epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
      }
    }
  }
}

#endif