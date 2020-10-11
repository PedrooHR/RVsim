#include "memory.h"

#include <stdio.h>

#define INT_MAX (2147483647)

static int debug = 1;

memory_t::memory_t(uint32_t size) { // size in MiBytes
  memory.clear();
  memory.resize(size * 1024, 0);
}

void memory_t::writeMem(uint32_t address, uint8_t value) {
  // if(address >= 0x119b8 && address < 0x119bc) {
  //   printf("ESCREVENDO 0x%x NO ENDERECO 0x%x\n", value, address);
  // }
  memory[address] = value;
}

uint8_t memory_t::readMem(uint32_t address) {
//   if(address >= 0x119b8 && address < 0x119bc) {
//     printf("LENDO 0x%x DO ENDERECO 0x%x\n", memory[address], address);
//   }
  return memory[address];
}

uint32_t memory_t::getTotalSize() {
  return memory.size();
}
