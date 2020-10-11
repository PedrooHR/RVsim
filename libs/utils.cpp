#include "utils.h"

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