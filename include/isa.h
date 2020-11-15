#pragma once

#ifndef _RVSIM_ISA_H_
#define _RVSIM_ISA_H_

#include <iostream>
#include <stdint.h>
#include <string>

#include "utils.h"

class instruction_t {
private:
  uint32_t inst;
  unsigned long int address;
  INST_TYPE type;
  uint32_t opcode, rd, rs1, rs2, func3, func7, shammt;
  int32_t imm;
  MNE op;
  int cycle;

public:
  instruction_t();
  instruction_t(uint32_t ins, unsigned long int add);

  MNE getOperation();
  INST_TYPE getType();
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
};

#endif