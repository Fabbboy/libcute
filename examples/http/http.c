#include "http.h"
#include "macro.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <nostd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
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
  cu_Slice_Result req_res = cu_Allocator_Alloc(server->slab_alloc, 1025, 1);
  if (!cu_Slice_result_is_ok(&req_res)) {
    close_client(server, fd);
    return;
  }
  char *req = (char *)req_res.value.ptr;
  ssize_t r = read(fd, req, req_res.value.length - 1);
  if (r <= 0) {
    close_client(server, fd);
    cu_Allocator_Free(server->slab_alloc, req_res.value);
    return;
  }
  req[r] = '\0';
  if (strncmp(req, "GET ", 4) != 0) {
    send_simple(fd, 400, "Bad Request");
    close_client(server, fd);
    cu_Allocator_Free(server->slab_alloc, req_res.value);
    return;
  }
  char *path = req + 4;
  char *end = strchr(path, ' ');
  if (!end) {
    send_simple(fd, 400, "Bad Request");
    close_client(server, fd);
    cu_Allocator_Free(server->slab_alloc, req_res.value);
    return;
  }
  *end = '\0';
  char fs_path[512];
  cu_CString_snprintf(fs_path, sizeof(fs_path), ".%s", path);
  int ffd = open(fs_path, O_RDONLY);
  if (ffd < 0) {
    send_simple(fd, 404, "Not Found");
    close_client(server, fd);
    cu_Allocator_Free(server->slab_alloc, req_res.value);
    return;
  }
  struct stat st;
  fstat(ffd, &st);
  char header[256];
  int len = cu_CString_snprintf(header, sizeof(header),
      "HTTP/1.0 200 OK\r\nContent-Length: %lld\r\n\r\n", (long long)st.st_size);
  write(fd, header, (size_t)len);
  cu_Slice_Result buf_res = cu_Allocator_Alloc(server->slab_alloc, 4096, 1);
  if (!cu_Slice_result_is_ok(&buf_res)) {
    close(ffd);
    close_client(server, fd);
    cu_Allocator_Free(server->slab_alloc, req_res.value);
    return;
  }
  char *buf = (char *)buf_res.value.ptr;
  ssize_t n;
  while ((n = read(ffd, buf, buf_res.value.length)) > 0) {
    write(fd, buf, (size_t)n);
  }
  close(ffd);
  close_client(server, fd);
  cu_Allocator_Free(server->slab_alloc, buf_res.value);
  cu_Allocator_Free(server->slab_alloc, req_res.value);
}

cu_HttpServer_Result cu_HttpServer_create(
    cu_Allocator allocator, uint16_t port) {
  cu_Vector_Result vec_res =
      cu_Vector_create(allocator, CU_LAYOUT(int), Size_Optional_some(16));
  if (!cu_Vector_result_is_ok(&vec_res)) {
    return cu_HttpServer_result_error(CU_HTTP_ERROR_EPOLL);
  }

  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd < 0) {
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_result_error(CU_HTTP_ERROR_SOCKET);
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
    return cu_HttpServer_result_error(CU_HTTP_ERROR_SOCKET);
  }
  if (listen(sfd, 16) < 0) {
    close(sfd);
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_result_error(CU_HTTP_ERROR_SOCKET);
  }

  int epfd = epoll_create1(0);
  if (epfd < 0) {
    close(sfd);
    cu_Vector_destroy(&vec_res.value);
    return cu_HttpServer_result_error(CU_HTTP_ERROR_EPOLL);
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
  return cu_HttpServer_result_ok(server);
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
}

#endif
