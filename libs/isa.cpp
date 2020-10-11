#include "isa.h"

#include <stdio.h>

#include "utils.h"

static int debug = 1;


instruction_t::instruction_t(uint32_t ins, unsigned long int add) {
  address = add;
  inst = ins;
  opcode = ins & 127; // 7 bits de opcode
  rd = -1;
  rd = -1;
  rs1 = -1;
  rs2 = -1;
  func3 = -1;
  func7 = -1;
  imm = -1;
  shammt = -1;
  cycle = 1; // Atualmente todas instruções levam 1 ciclo
  
  // printf("INSTRUCAO: 0x%X OPCODE: 0x%x\n", ins, opcode);

  // INSTRUCTION DECODE
  // Atualmente no IM temos 11 OPCODES
  switch (opcode) {
  case 0b0110111: {
    type = INST_TYPE::U;
    rd = (ins >> 7) & 31;    // 5 bits de reg
    imm = (ins >> 12) << 12; // 20 bits imm
    mnemonic = "LUI";
    op = MNE::LUI;
    break; // LUI
  }
  case 0b0010111: {
    type = INST_TYPE::U;
    rd = (ins >> 7) & 31;    // 5 bits de reg
    imm = (ins >> 12) << 12; // 20 bits imm
    // imm = ins >> 12;
    // printf("AUIPC imm %x\n", imm);
    mnemonic = "AUIPC";
    op = MNE::AUIPC;
    break; // AUIPC
  }
  case 0b1101111: {
    type = INST_TYPE::UJ;
    rd = (ins >> 7) & 31;                        // 5 bits de reg
    int temp_imlow = ((ins >> 21) & 1023) << 1;  // 10 bits de imm low
    int temp_im11 = ((ins >> 20) & 1) << 11;     // 1 bit
    int temp_imhigh = ((ins >> 12) & 255) << 12; // 8 bits imm high
    int temp_im20 = ((ins >> 31) & 1) << 20;     // 1 bit
    imm = temp_imlow + temp_im11 + temp_imhigh + temp_im20; // 21 bits de imm
    mnemonic = "JAL";
    op = MNE::JAL;
    break; // JAL
  }
  case 0b1100111: {
    type = INST_TYPE::I;
    rd = (ins >> 7) & 31;     // 5 bits de reg
    func3 = (ins >> 12) & 7;  // 3 bits func3
    rs1 = (ins >> 15) & 31;   // 5 bits de reg
    imm = (ins >> 20) & 4095; // 12 bits imm
    mnemonic = "JALR";
    op = MNE::JALR;
    break; // JALR
  }
  case 0b1100011: {
    type = INST_TYPE::SB;
    int temp_imlow = ((ins >> 8) & 15) << 1;                // 4 bits de imm low
    int temp_im11 = ((ins >> 7) & 1) << 11;                 // 1 bit
    func3 = (ins >> 12) & 7;                                // 3 bits func3
    rs1 = (ins >> 15) & 31;                                 // 5 bits de reg
    rs2 = (ins >> 20) & 31;                                 // 5 bits de reg
    int temp_imhigh = ((ins >> 25) & 63) << 5;              // 6 bits imm high
    int temp_im12 = ((ins >> 31) & 1) << 12;                // 1 bit
    imm = temp_imlow + temp_im11 + temp_imhigh + temp_im12; // 13 bits de imm

    switch (func3) {
    case 0: // 000
      mnemonic = "BEQ";
      op = MNE::BEQ;
      break;
    case 1: // 001
      mnemonic = "BNE";
      op = MNE::BNE;
      break;
    case 4: // 100
      mnemonic = "BLT";
      op = MNE::BLT;
      break;
    case 5: // 101
      mnemonic = "BGE";
      op = MNE::BGE;
      break;
    case 6: // 110
      mnemonic = "BLTU";
      op = MNE::BLTU;
      break;
    case 7: // 111
      mnemonic = "BGEU";
      op = MNE::BGEU;
      break;
    }
    break; // Gen B type (Branches)
  }
  case 0b0000011: {
    type = INST_TYPE::I;
    rd = (ins >> 7) & 31;     // 5 bits de reg
    func3 = (ins >> 12) & 7;  // 3 bits func3
    rs1 = (ins >> 15) & 31;   // 5 bits de reg
    imm = (ins >> 20) & 4095; // 12 bits imm

    switch (func3) {
    case 0: // 000
      mnemonic = "LB";
      op = MNE::LB;
      break;
    case 1: // 001
      mnemonic = "LH";
      op = MNE::LH;
      break;
    case 2: // 010
      mnemonic = "LW";
      op = MNE::LW;
      break;
    case 4: // 100
      mnemonic = "LBU";
      op = MNE::LBU;
      break;
    case 5: // 101
      mnemonic = "LHU";
      op = MNE::LHU;
      break;
    }
    break; // Load instructions
  }
  case 0b0100011: {
    type = INST_TYPE::S;
    int temp_imlow = (ins >> 7) & 31;      // 5 bits de imm low
    func3 = (ins >> 12) & 7;               // 3 bits func3
    rs1 = (ins >> 15) & 31;                // 5 bits de reg
    rs2 = (ins >> 20) & 31;                // 5 bits de reg
    int temp_imhigh = (ins >> 25) & 127;   // 7 bits imm high
    imm = temp_imlow + (temp_imhigh << 5); // 12 bits de imm

    switch (func3) {
    case 0: // 000
      mnemonic = "SB";
      op = MNE::SB;
      break;
    case 1: // 001
      mnemonic = "SH";
      op = MNE::SH;
      break;
    case 2: // 010
      mnemonic = "SW";
      op = MNE::SW;
      break;
    }
    break; // Gen S type
  }
  case 0b0010011: {
    type = INST_TYPE::I;
    rd = (ins >> 7) & 31;     // 5 bits de reg
    func3 = (ins >> 12) & 7;  // 3 bits func3
    rs1 = (ins >> 15) & 31;   // 5 bits de reg
    imm = (ins >> 20) & 4095; // 12 bits imm
    switch (func3) {
    case 0: // 000
      mnemonic = "ADDI";
      op = MNE::ADDI;
      // printf("ADDI imm: %d\n", sign_extend(imm, 12));
      break;
    case 2: // 010
      mnemonic = "SLTI";
      op = MNE::SLTI;
      break;
    case 3: // 011
      mnemonic = "SLTIU";
      op = MNE::SLTIU;
      break;
    case 4: // 100
      mnemonic = "XORI";
      op = MNE::XORI;
      break;
    case 6: // 110
      mnemonic = "ORI";
      op = MNE::ORI;
      break;
    case 7: // 111
      mnemonic = "ANDI";
      op = MNE::ANDI;
      break;
    case 1: // 001
      mnemonic = "SLLI";
      op = MNE::SLLI;
      shammt = (ins >> 20) & 31; // 5 bits de shift
      break;
    case 5:                          // 111
      shammt = (ins >> 20) & 31;     // 5 bits de shift
      int func7 = (ins >> 25) & 127; // 7 bits func7
      if (func7 == 0) {
        mnemonic = "SRLI";
        op = MNE::SRLI;
      } else if (func7 == 32) {
        mnemonic = "SRAI";
        op = MNE::SRAI;
      }
      break;
    }
    break; // Gen I type
  }
  case 0b0110011: {
    type = INST_TYPE::R;
    rd = (ins >> 7) & 31;      // 5 bits de reg
    func3 = (ins >> 12) & 7;   // 3 bits func3
    rs1 = (ins >> 15) & 31;    // 5 bits de reg
    rs2 = (ins >> 20) & 31;    // 5 bits de reg
    func7 = (ins >> 25) & 127; // 7 bits func7

    if (func7 == 1) { // Multiplicação e divisão
      switch (func3) {
      case 0: // 000
        mnemonic = "MUL";
        op = MNE::MUL;
        break;
      case 1: // 001
        mnemonic = "MULH";
        op = MNE::MULH;
        break;
      case 2: // 010
        mnemonic = "MULHSU";
        op = MNE::MULHSU;
        break;
      case 3: // 011
        mnemonic = "MULHU";
        op = MNE::MULHU;
        break;
      case 4: // 100
        mnemonic = "DIV";
        op = MNE::DIV;
        break;
      case 5: // 101
        mnemonic = "DIVU";
        op = MNE::DIVU;
        break;
      case 6: // 110
        mnemonic = "REM";
        op = MNE::REM;
        break;
      case 7: // 111
        mnemonic = "REMU";
        op = MNE::REMU;
        break;
      }
    } else { // Outras
      switch (func3) {
      case 0: // 000
        if (func7 == 0) {
          mnemonic = "ADD";
          op = MNE::ADD;
        } else if (func7 == 32) {
          mnemonic = "SUB";
          op = MNE::SUB;
        }
        break;
      case 1: // 001
        mnemonic = "SLL";
        op = MNE::SLL;
        break;
      case 2: // 010
        mnemonic = "SLT";
        op = MNE::SLT;
        break;
      case 3: // 011
        mnemonic = "SLTU";
        op = MNE::SLTU;
        break;
      case 4: // 100
        mnemonic = "XOR";
        op = MNE::XOR;
        break;
      case 5: // 101
        if (func7 == 0) {
          mnemonic = "SRL";
          op = MNE::SRL;
        } else if (func7 == 32) {
          mnemonic = "SRA";
          op = MNE::SRA;
        }
        break;
      case 6: // 110
        mnemonic = "OR";
        op = MNE::OR;
        break;
      case 7: // 111
        mnemonic = "AND";
        op = MNE::AND;
        break;
      }
    }
    break; // Gen R type
  }
  case 0b0001111: {
    type = INST_TYPE::I;
    rd = (ins >> 7) & 31;     // 5 bits de reg
    func3 = (ins >> 12) & 7;  // 3 bits func3
    rs1 = (ins >> 15) & 31;   // 5 bits de reg
    imm = (ins >> 20) & 4095; // 12 bits imm

    if (func3 == 0) {
      mnemonic = "FENCE";
      op = MNE::FENCE;
    } else if (func3 == 1) {
      mnemonic = "FENCE.I";
      op = MNE::FENCEI;
    }
    break; // Sync
  }
  case 0b1110011: {
    type = INST_TYPE::I;
    rd = (ins >> 7) & 31;     // 5 bits de reg
    func3 = (ins >> 12) & 7;  // 3 bits func3
    rs1 = (ins >> 15) & 31;   // 5 bits de reg
    imm = (ins >> 20) & 4095; // 12 bits imm

    switch (func3) {
    case 0: // 000
      if (imm == 0) {
        mnemonic = "SCALL";
        op = MNE::SCALL;
      } else if (imm == 1) {
        mnemonic = "SBREAK";
        op = MNE::SBREAK;
      }
      break;
    case 1: // 001
      mnemonic = "RDCYCLE";
      op = MNE::RDCYCLE;
      break;
    case 2: // 010
      mnemonic = "RDCYCLEH";
      op = MNE::RDCYCLEH;
      break;
    case 3: // 011
      mnemonic = "RDTIME";
      op = MNE::RDTIME;
      break;
    case 5: // 101
      mnemonic = "RDTIMEH";
      op = MNE::RDTIMEH;
      break;
    case 6: // 110
      mnemonic = "RDINSTRRET";
      op = MNE::RDINSTRRET;
      break;
    case 7: // 111
      mnemonic = "RDINSTRETH";
      op = MNE::RDINSTRETH;
      break;
    }
    break; // System/Counters
  }
  default:
    if (debug)
      printf("Invalid Opcode\n");
  }
}

MNE instruction_t::getOperation() { return op; }

INST_TYPE instruction_t::getType() { return type; }

std::string instruction_t::getMnemonic() { return mnemonic; }

uint32_t instruction_t::getOPCode() { return opcode; }

uint32_t instruction_t::getRd() { return rd; }

uint32_t instruction_t::getRs1() { return rs1; }

uint32_t instruction_t::getRs2() { return rs2; }

uint32_t instruction_t::getFunc3() { return func3; }

uint32_t instruction_t::getFunc7() { return func7; }

int32_t instruction_t::getImm() { return imm; }

uint8_t instruction_t::getImmSize() {
  switch (type) {
  case INST_TYPE::R:
    return 0;
    break;
  case INST_TYPE::I:
    return 12;
    break;
  case INST_TYPE::S:
    return 12;
    break;
  case INST_TYPE::SB:
    return 13;
    break;
  case INST_TYPE::U:
    return 31;
    break;
  case INST_TYPE::UJ:
    return 21;
    break;
  }
}

uint32_t instruction_t::getShammt() { return shammt; }

uint32_t instruction_t::getInsCycle() { return cycle; }

unsigned long int instruction_t::getInsAddress() { return address; }

std::string instruction_t::PrintInstruction() {
  return std::string(disassembly);
}

uint32_t instruction_t::getIns() { return inst; }