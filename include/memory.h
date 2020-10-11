#pragma once

#ifndef _RVSIM_MEMORY_H
#define _RVSIM_MEMORY_H

#include <vector>
#include <stdint.h>

class memory_t {
private:
  std::vector<uint8_t> memory;

public:
  memory_t(uint32_t size);

  void writeMem(uint32_t address, uint8_t value);
  uint8_t readMem(uint32_t address);

  uint32_t getTotalSize();
};

#endif