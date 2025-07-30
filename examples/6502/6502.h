#pragma once

#include "cute.h"
#include <stdint.h>
#ifndef CU_FREESTANDING

/**\@file 6502.h Simple 6502 CPU emulator using libcute types. */

typedef struct {
  uint8_t a;
  uint8_t x;
  uint8_t y;
  uint8_t sp;
  uint16_t pc;
  uint8_t status;
  cu_Vector memory;
} cu_6502;

typedef enum {
  CU_6502_ERROR_NONE = 0,
  CU_6502_ERROR_OOM,
} cu_6502_Error;

CU_RESULT_DECL(cu_6502, cu_6502, cu_6502_Error)

cu_6502_Result cu_6502_create(cu_Allocator allocator);
void cu_6502_destroy(cu_6502 *cpu);
void cu_6502_load(cu_6502 *cpu, cu_Slice program, uint16_t addr);
bool cu_6502_load_file(cu_6502 *cpu, const char *path, uint16_t addr, cu_Allocator allocator);
void cu_6502_reset(cu_6502 *cpu, uint16_t addr);
void cu_6502_step(cu_6502 *cpu);
void cu_6502_run(cu_6502 *cpu);

#endif
