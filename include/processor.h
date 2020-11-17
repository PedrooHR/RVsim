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
#define BASE_FETCH_DURATION     4
#define BASE_DECODE_DURATION    1
#define BASE_ALLOC_DURATION     1
#define BASE_ISSUE_DURATION     3
#define BASE_EXECUTE_DURATION   2
#define BASE_COMMIT_DURATION    1

#define BRANCH_PREDICTION_PEN   8

#define NUMBER_OF_ALU           3
#define NUMBER_OF_AGU           1
#define NUMBER_OF_BRU           1
#define CICLES_ALU              1
#define CICLES_AGU              1
#define CICLES_BRU              1

class processor_t {
private:
  gshare_t *gshare;
  memory_t *memory;
  registers_t registers;
  std::vector<uint32_t> ALU;
  std::vector<uint32_t> AGU;
  std::vector<uint32_t> BRU;
  uint32_t memory_avail;
  int cycle;
  int PC;
  bool running;
  int number_i;
  // variaveis relacionadas a branching
  bool branched = false, is_branch;
  bool wrote;

public:
  processor_t(memory_t *mem, uint32_t entry_point, int n_ins);

  void executeProgram();

  uint32_t Fetch(uint32_t *raw_instruction, uint32_t *pc_address, bool *pred);
  uint32_t Execute(instruction_t ins, char *disassembly);
  std::string doLogLine(instruction_t ins, char *disassembly);
  uint32_t getNextALU(uint32_t time);
  uint32_t getNextAGU(uint32_t time);
  uint32_t getNextBRU(uint32_t time);
  uint32_t getNextMEM(uint32_t time, uint32_t time_used);
};

#endif