#include "registers.h"

#include <stdio.h>

static int debug = 0;

registers_t::registers_t() {
  regs.clear();
  regs.resize(32, 0);
  under_write.clear();
  under_write.resize(32, 0);
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

void registers_t::setUW(unsigned int reg, uint32_t time) {
  // O registrado 0 tem que estar sempre disponível
  if (reg != 0)
    under_write[reg] = time;
}

uint32_t registers_t::checkUW(unsigned int reg) { 
  return under_write[reg]; 
}