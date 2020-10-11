#pragma once

#ifndef _RVSIM_PROCESSOR_H
#define _RVSIM_PROCESSOR_H

#include "isa.h"
#include "memory.h"
#include "registers.h"

class processor_t {
private:
  memory_t * memory;
  registers_t registers;
  int cycle;
  int PC;
  bool running;
  char disassembly[50];
  
public:
  processor_t(memory_t * mem, uint32_t entry_point);

  void executeProgram();

  instruction_t Fetch();
  void Execute(instruction_t ins);
};

#endif