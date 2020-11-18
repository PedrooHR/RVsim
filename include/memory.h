#pragma once

#ifndef _RVSIM_MEMORY_H
#define _RVSIM_MEMORY_H

#include <cstdint>

#include <map>

// Tamanho da memória 
#define MEM_SIZE          UINT32_MAX

#define L1_MISS_PENALTY   10
#define L2_MISS_PENALTY   50
#define L3_MISS_PENALTY   100

// foward declaration pro dinero
typedef struct d4_cache_struct d4cache;

enum class ACCESS_TYPE { INSTRUCTION, DATA, LOAD};

class memory_t {
private:
  uint32_t add_offset;
  std::map<uint32_t, uint8_t> memory_map;
  std::map<std::string, d4cache *> caches;
  int li_misses = 0;
  int ld_misses = 0;
  int l2_misses = 0;
  int l3_misses = 0;

public:
  memory_t();

  uint32_t writeMem(uint32_t address, uint8_t value, ACCESS_TYPE type);
  uint32_t readMem(uint32_t address, uint8_t *value, ACCESS_TYPE type);

  uint32_t getTotalSize();
  void printCacheMisses();
  void printd4log();
};

#endif