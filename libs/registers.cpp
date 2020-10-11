#include "registers.h"

#include <stdio.h>

static int debug = 0;

registers_t::registers_t() {
  regs.clear();
  regs.resize(32, 0);
}

void registers_t::writeReg(unsigned int reg, uint32_t value) {
  if (reg != 0) {
    regs[reg] = value;
  } else {
    if (debug)
      printf("Can't write on register 0\n");
  }
}

uint32_t registers_t::readReg(unsigned int reg) { return regs[reg]; }