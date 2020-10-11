#pragma once

#ifndef _RVSIM_REGISTERS_H
#define _RVSIM_REGISTERS_H

#include <stdint.h>
#include <vector>

class registers_t {
private:
  std::vector<uint32_t> regs;
public:
  registers_t();

  void writeReg(unsigned int reg, uint32_t value);
  uint32_t readReg(unsigned int reg);
};

#endif