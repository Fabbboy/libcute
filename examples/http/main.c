#include "http.h"
#include <nostd.h>

int main(int argc, char **argv) {
  uint16_t port = 8080;
  if (argc > 1) {
    port = (uint16_t)cu_CString_strtoul(argv[1], NULL, 10);
  }
  cu_Allocator alloc = cu_Allocator_CAllocator();
  cu_HttpServer_Result res = cu_HttpServer_create(alloc, port);
  if (!cu_HttpServer_result_is_ok(&res)) {
    return 1;
  }
  cu_HttpServer server = res.value;
  cu_HttpServer_run(&server);
  cu_HttpServer_destroy(&server);
  return 0;
}
