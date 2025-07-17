#include "6502.h"
#include "macro.h"
#include <stdio.h>
#include <string.h>

#define FLAG_CARRY 0x01
#define FLAG_ZERO 0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL 0x08
#define FLAG_BREAK 0x10
#define FLAG_UNUSED 0x20
#define FLAG_OVERFLOW 0x40
#define FLAG_NEGATIVE 0x80

CU_RESULT_IMPL(cu_6502, cu_6502, cu_6502_Error)

static void update_zn(cu_6502 *cpu, uint8_t value) {
  if (value == 0) {
    cpu->status |= FLAG_ZERO;
  } else {
    cpu->status &= (uint8_t)~FLAG_ZERO;
  }

  if (value & 0x80) {
    cpu->status |= FLAG_NEGATIVE;
  } else {
    cpu->status &= (uint8_t)~FLAG_NEGATIVE;
  }
}

cu_6502_Result cu_6502_create(cu_Allocator allocator) {
  cu_Vector_Result vec_res = cu_Vector_create(
      allocator, CU_LAYOUT(uint8_t), Size_Optional_some(0x10000));
  if (!cu_Vector_result_is_ok(&vec_res)) {
    return cu_6502_result_error(CU_6502_ERROR_OOM);
  }
  cu_Vector_Error_Optional err = cu_Vector_resize(&vec_res.value, 0x10000);
  if (cu_Vector_Error_Optional_is_some(&err)) {
    cu_Vector_destroy(&vec_res.value);
    return cu_6502_result_error(CU_6502_ERROR_OOM);
  }

  cu_6502 cpu = {0};
  cpu.memory = vec_res.value;
  cpu.sp = 0xFD;
  cpu.status = FLAG_UNUSED;
  return cu_6502_result_ok(cpu);
}

void cu_6502_destroy(cu_6502 *cpu) { cu_Vector_destroy(&cpu->memory); }

void cu_6502_load(cu_6502 *cpu, cu_Slice program, uint16_t addr) {
  CU_IF_NULL(cpu) { return; }
  if (addr + program.length > cpu->memory.length) {
    return;
  }
  memcpy((uint8_t *)cpu->memory.data.value.ptr + addr, program.ptr,
      program.length);
  cpu->pc = addr;
}

bool cu_6502_load_file(cu_6502 *cpu, const char *path, uint16_t addr) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    return false;
  }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  rewind(f);
  if (addr + (uint16_t)sz > cpu->memory.length) {
    fclose(f);
    return false;
  }
  fread((uint8_t *)cpu->memory.data.value.ptr + addr, 1, (size_t)sz, f);
  fclose(f);
  cpu->pc = addr;
  return true;
}

void cu_6502_reset(cu_6502 *cpu, uint16_t addr) {
  cpu->a = 0;
  cpu->x = 0;
  cpu->y = 0;
  cpu->sp = 0xFD;
  cpu->status = FLAG_UNUSED;
  cpu->pc = addr;
}

void cu_6502_step(cu_6502 *cpu) {
  uint8_t *mem = (uint8_t *)cpu->memory.data.value.ptr;
  uint8_t opcode = mem[cpu->pc++];
  switch (opcode) {
  case 0xA9: // LDA immediate
    cpu->a = mem[cpu->pc++];
    update_zn(cpu, cpu->a);
    break;
  case 0xA2: // LDX immediate
    cpu->x = mem[cpu->pc++];
    update_zn(cpu, cpu->x);
    break;
  case 0xE8: // INX
    cpu->x += 1;
    update_zn(cpu, cpu->x);
    break;
  case 0x8D: { // STA absolute
    uint16_t addr = mem[cpu->pc++] | ((uint16_t)mem[cpu->pc++] << 8);
    mem[addr] = cpu->a;
    break;
  }
  case 0xAD: { // LDA absolute
    uint16_t addr = mem[cpu->pc++] | ((uint16_t)mem[cpu->pc++] << 8);
    cpu->a = mem[addr];
    update_zn(cpu, cpu->a);
    break;
  }
  case 0xEA: // NOP
    break;
  case 0x00: // BRK
    cpu->status |= FLAG_BREAK;
    break;
  default:
    UNREACHABLE("unknown opcode");
  }
}

void cu_6502_run(cu_6502 *cpu) {
  while ((cpu->status & FLAG_BREAK) == 0) {
    cu_6502_step(cpu);
  }
}
