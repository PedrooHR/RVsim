#pragma once

#ifndef _RVSIM_PROCESSOR_H
#define _RVSIM_PROCESSOR_H

#include <sstream>
#include <vector>
#include <queue>

#include "isa.h"
#include "memory.h"
#include "registers.h"

//============== GSHARE
class gshare_t {
private:
  int size, btb_size;
  int history, l_history;
  int table_entries;
  std::vector<bool> h_table;
  std::vector<uint32_t> btb;
  std::vector<uint32_t> btb_tag;
  std::queue<std::pair<int, bool>> branch_h; 
  int hits, errors;

public:
  gshare_t(int psize, int pbtb_size);

  uint32_t getBTB(uint32_t pc);
  bool checkBranch();
  void feedback(bool branched, uint32_t pc, uint32_t address);
  
  int getHits();
  int getErrors();
};

//============== PROCESSOR
class processor_t {
private:
  gshare_t *gshare;
  memory_t *memory;
  registers_t registers;
  int cycle;
  int PC;
  bool running;
  int number_i;
  // variaveis relacionadas a branching
  bool branched = false, is_branch;

  // Variaveis de controle da simulação
  bool *used;
  bool *pred;
  uint32_t *ri;
  uint32_t *pc;
  uint32_t *cycles;
  std::string *ds;
  char **d;
  instruction_t *ins;

public:
  processor_t(memory_t *mem, uint32_t entry_point, int n_ins);

  void executeProgram();

  uint32_t Fetch(uint32_t *raw_instruction, uint32_t *pc_address, bool *pred);
  uint32_t Execute(instruction_t ins, char *disassembly);
  std::string doLogLine(instruction_t ins, char *disassembly);
};

#endif