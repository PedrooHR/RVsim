#pragma once

#ifndef _RVSIM_UTILS_H_
#define _RVSIM_UTILS_H_

#include <stdint.h>
#include <vector>
#include <string>

enum class INST_TYPE : int { R = 0, I, S, SB, U, UJ };

enum class MNE : int {
  // Instruções que usam a AGU
  LB = 100,
  LH,
  LW,
  LBU,
  LHU,
  SB,
  SH,
  SW,
  // Instruções que usam a ALU
  SLL = 200,
  SLLI,
  SRL,
  SRLI,
  SRA,
  SRAI,
  ADD,
  ADDI,
  SUB,
  LUI,
  AUIPC,
  XOR,
  XORI,
  OR,
  ORI,
  AND,
  ANDI,
  SLT,
  SLTI,
  SLTU,
  SLTIU, 
  MUL,
  MULH,
  MULHSU,
  MULHU,
  DIV,
  DIVU,
  REM,
  REMU,
  // Instruções que usam a BRU
  BEQ = 300,
  BNE,
  BLT,
  BGE,
  BLTU,
  BGEU,
  JAL,
  JALR,
  // Instruções que atualmente não fazem nada
  FENCE = 400,
  FENCEI,
  SCALL,
  SBREAK,
  RDCYCLE,
  RDCYCLEH,
  RDTIME,
  RDTIMEH,
  RDINSTRRET,
  RDINSTRETH
};

uint32_t sign_extend(uint32_t value, uint8_t bits);

uint32_t get_elf_entry(std::vector<uint8_t> mem, uint32_t address, uint32_t size);

std::vector<uint8_t> elf_by_byte(const char * elf_file);

extern std::string register_name[32];
extern std::string status_register_name[6];

#endif