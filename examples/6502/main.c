#include "6502.h"
#include <nostd.h>
#include <stdio.h>

int main(int argc, char **argv) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  cu_6502_Result res = cu_6502_create(alloc);
  if (!cu_6502_Result_is_ok(&res)) {
    return 1;
  }
  cu_6502 cpu = res.value;

  uint16_t addr = 0x0600;
  if (argc > 2) {
    addr = (uint16_t)cu_CString_strtoul(argv[2], NULL, 0);
  }

  if (argc > 1) {
    if (!cu_6502_load_file(&cpu, argv[1], addr)) {
      fprintf(stderr, "failed to load %s\n", argv[1]);
      cu_6502_destroy(&cpu);
      return 1;
    }
  } else {
    uint8_t prog[] = {0xA9, 0x01, 0xA2, 0x05, 0xE8, 0x00};
    cu_6502_load(&cpu, cu_Slice_create(prog, sizeof(prog)), addr);
  }

  cu_6502_reset(&cpu, addr);
  cu_6502_run(&cpu);

  printf("A=%u X=%u\n", cpu.a, cpu.x);

  cu_6502_destroy(&cpu);
  return 0;
}
