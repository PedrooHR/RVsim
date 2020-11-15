#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include "utils.h"
#include "memory.h"
#include "processor.h"

using namespace std;

int main(int argc, char** argv) {
  if ( argc != 2 ) {
      std::cout << "Uso: rvsim <elf_program>" << std::endl;
      return 1;
  }

  // Le arquivo ELF e retorna vetor com os bytes do arquivo
  std::vector<uint8_t> elf_bytes = elf_by_byte(argv[1]);
  // Pega informações relevantes pra execução
  uint32_t entry_point = get_elf_entry(elf_bytes, 0x18, 4);
  uint32_t shes = get_elf_entry(elf_bytes, 0x2E, 2);
  uint32_t shen = get_elf_entry(elf_bytes, 0x30, 2);
  uint32_t entry_names = get_elf_entry(elf_bytes, 0x32, 2);
  uint32_t shos = elf_bytes.size() - (shen * shes);

  // Constroi a memoria do programa 
  memory_t memory(256);
  for (int i = 0; i < shen; i++) {
    uint32_t s_addr = get_elf_entry(elf_bytes, shos + (i * shes) + 0x0C, 4);
    uint32_t s_offset = get_elf_entry(elf_bytes, shos + (i * shes) + 0x10, 4);
    uint32_t s_size = get_elf_entry(elf_bytes, shos + (i * shes) + 0x14, 4);
    for (int j = 0; j < s_size; j += 4) {
      int32_t base_address = s_addr + j;
      if (base_address >= 0) {
        memory.writeMem(base_address, elf_bytes[s_offset + j], ACCESS_TYPE::LOAD);
        memory.writeMem(base_address + 1, elf_bytes[s_offset + j + 1], ACCESS_TYPE::LOAD);
        memory.writeMem(base_address + 2, elf_bytes[s_offset + j + 2], ACCESS_TYPE::LOAD);
        memory.writeMem(base_address + 3, elf_bytes[s_offset + j + 3], ACCESS_TYPE::LOAD);
      }
    }
  }

  // Executa o simulador
  processor_t processor(&memory, entry_point, 4);
  processor.executeProgram();

  return 0;
}