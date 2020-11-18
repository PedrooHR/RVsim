#include "utils.h"

#include <cstdio>

#include <fstream>

std::string register_name[32] = {
    "Zero", // x0 -  Always zero
    "ra",   // x1   -  Return addres Caller
    "sp",   // x2   -  Stack pointer Callee
    "gp",   // x3   -  Global pointer
    "tp",   // x4   -  Thread pointer
    "t0",   // x5   -  Temporary / alternate return address Caller
    "t1",   // x6   -  Temporary Caller
    "t2",   // x7   -  Temporary Caller
    "s0",   // x8   -  Saved register / frame pointer Callee (Também chamado de
            // "fp")
    "s1",   // x9   -  Saved register Callee
    "a0",   // x10  -  Function argument / return value Caller
    "a1",   // x11  -  Function argument / return value Caller
    "a2",   // x12  -  Function argument Caller
    "a3",   // x13  -  Function argument Caller
    "a4",   // x14  -  Function argument Caller
    "a5",   // x15  -  Function argument Caller
    "a6",   // x16  -  Function argument Caller
    "a7",   // x17  -  Function argument Caller
    "s2",   // x18  -  Saved register Callee
    "s3",   // x19  -  Saved register Callee
    "s4",   // x20  -  Saved register Callee
    "s5",   // x21  -  Saved register Callee
    "s6",   // x22  -  Saved register Callee
    "s7",   // x23  -  Saved register Callee
    "s8",   // x24  -  Saved register Callee
    "s9",   // x25  -  Saved register Callee
    "s10",  // x26 -  Saved register Callee
    "s11",  // x27 -  Saved register Callee
    "t3",   // x28  -  Temporary Caller
    "t4",   // x29  -  Temporary Caller
    "t5",   // x30  -  Temporary Caller
    "t6"    // x31  -  Temporary Caller
};

std::string status_register_name[6] = {"RDCYCLE", "RDCYCLEH",   "RDTIME",
                                       "RDTIMEH", "RDINSTRRET", "RDINSTRETH"};

uint32_t sign_extend(uint32_t value, uint8_t bits) {
  // gera uma mascara pro sinal
  uint32_t sign_mask = 1 << (bits - 1);  
  // zera os bits a esquerda do sinal
  value = value & ((1 << bits) - 1);
  // extende o sinal
  value = (value ^ sign_mask) - sign_mask; 
}

uint32_t get_elf_entry(std::vector<uint8_t> mem, uint32_t address, uint32_t size) {
  uint32_t entry = 0;
  for (int i = 0; i < size; i++) {
    entry += mem[address + i] << 8 * i;
  }
  // printf("Entry: 0x%lX\n", entry);
  return entry;
}

std::vector<uint8_t> elf_by_byte(const char * elf_file) {
  std::vector<uint8_t> mem;
  if(std::ifstream is{elf_file, std::ios::binary | std::ios::ate}) {
    auto elf_size = is.tellg();
    std::string str(elf_size, '\0'); // construct string to stream size
    is.seekg(0);
    if(is.read(&str[0], elf_size)) {
      mem.resize(elf_size);
      for (int i = 0; i < elf_size; i++) {
        mem[i] = (uint8_t) str[i];
      }
    } 
  }
  return mem;
}