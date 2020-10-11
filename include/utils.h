#pragma once

#ifndef _RVSIM_UTILS_H_
#define _RVSIM_UTILS_H_

#include <stdint.h>
#include <vector>
#include <fstream>

uint32_t sign_extend(uint32_t value, uint8_t bits);

uint32_t get_elf_entry(std::vector<uint8_t> mem, uint32_t address, uint32_t size);

std::vector<uint8_t> elf_by_byte(const char * elf_file);

#endif