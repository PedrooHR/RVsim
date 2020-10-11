#pragma once

#ifndef _RVSIM_ISA_H_
#define _RVSIM_ISA_H_

#include <stdint.h>
#include <string>

#include "utils.h"

//// TODO: FAZER PRINT DA INSTRUÇÃO (disassembly)

enum class INST_TYPE : int { R = 0, I, S, SB, U, UJ };

enum class MNE : int {
  LB = 0,
  LH,
  LW,
  LBU,
  LHU,
  SB,
  SH,
  SW,
  SLL,
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
  BEQ,
  BNE,
  BLT,
  BGE,
  BLTU,
  BGEU,
  JAL,
  JALR,
  FENCE,
  FENCEI,
  SCALL,
  SBREAK,
  RDCYCLE,
  RDCYCLEH,
  RDTIME,
  RDTIMEH,
  RDINSTRRET,
  RDINSTRETH,
  MUL,
  MULH,
  MULHSU,
  MULHU,
  DIV,
  DIVU,
  REM,
  REMU
};

class instruction_t {
private:
  uint32_t inst;
  unsigned long int address;
  INST_TYPE type;
  uint32_t opcode, rd, rs1, rs2, func3, func7, shammt; 
  int32_t imm;
  std::string mnemonic;
  char disassembly[50];
  MNE op;
  int cycle;

public:
  instruction_t(uint32_t ins, unsigned long int add);

  MNE getOperation();
  INST_TYPE getType();
  std::string getMnemonic();
  uint32_t getOPCode();
  uint32_t getRd();
  uint32_t getRs1();
  uint32_t getRs2();
  uint32_t getFunc3();
  uint32_t getFunc7();
  int32_t getImm();
  uint8_t getImmSize();
  uint32_t getShammt();  
  uint32_t getInsCycle();
  uint32_t getIns();
  unsigned long int getInsAddress();
  std::string PrintInstruction();
};


#endif