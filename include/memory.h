#pragma once

#ifndef _RVSIM_MEMORY_H
#define _RVSIM_MEMORY_H

#include <cstdint>

#include <map>
#include <vector>

typedef struct d4_cache_struct d4cache;

enum class ACCESS_TYPE { INSTRUCTION, DATA, LOAD};

class memory_t {
private:
  std::map<std::string, d4cache *> caches;
  int li_misses = 0;
  int ld_misses = 0;
  int l2_misses = 0;
  int l3_misses = 0;

public:
  std::vector<uint8_t> memory;
  memory_t(uint32_t size);

  uint32_t writeMem(uint32_t address, uint8_t value, ACCESS_TYPE type);
  uint32_t readMem(uint32_t address, uint8_t *value, ACCESS_TYPE type);

  uint32_t getTotalSize();
  void printCacheMisses();
  void printd4log();
};

#endif