#include "processor.h"

#include <iomanip>
#include <iostream>

#include "utils.h"

//// IMPORTANTE: SEGUNDO A ESPECIFICAÇÃO, TODO IMEDIATO É SIGN-EXTENDED

processor_t::processor_t(memory_t *mem, uint32_t entry_point) {
  memory = mem;
  PC = entry_point;
  registers.writeReg(2, memory->getTotalSize() - 4); // ajusta stack pointer
  cycle = 0;
}

void processor_t::executeProgram() {
  running = true;
  while (running) {
    instruction_t ins = Fetch();
    // std::cout << "PC[" << std::hex << ins.getInsAddress() << "] - (0x"
    //           << std::hex << std::setw(8) << std::setfill('0') << ins.getIns()
    //           << ") = ";
    Execute(ins);
    cycle += ins.getInsCycle();
    std::cout << doLogLine(ins).str() << std::endl;
  }
  std::cout << "Total number of cycles: " << cycle << std::endl;
}

instruction_t processor_t::Fetch() {
  uint32_t pc_value = memory->readMem(PC);
  pc_value += memory->readMem(PC + 1) << 8;
  pc_value += memory->readMem(PC + 2) << 16;
  pc_value += memory->readMem(PC + 3) << 24;
  instruction_t ins(pc_value, PC);
  PC = PC + 4;
  return ins;
}

void processor_t::Execute(instruction_t ins) {
  switch (ins.getOperation()) {
  case MNE::LUI: { // Coloca o imediato no rd (preenche os 12 lower bits com 0)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    registers.writeReg(ins.getRd(), imm_se);
    sprintf(disassembly, "%-8s %s, %d", "LUI",
            register_name[ins.getRd()].c_str(), imm_se);
    break;
  }
  case MNE::AUIPC: { // Adiciona o imediato ao address da instrução e coloca no
                     // rd (preenche os 12 lower bits com 0)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    registers.writeReg(ins.getRd(), imm_se + ins.getInsAddress());
    sprintf(disassembly, "%-8s %s, %d", "AUIPC",
            register_name[ins.getRd()].c_str(), imm_se);
    break;
  }
  case MNE::LB: { // Carrega 1 byte do address (rs1 + imm) em rd (sign extend)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = memory->readMem(registers.readReg(ins.getRs1()) + imm_se);
    value = sign_extend(value, 8);
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "LB",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::LH: { // Carrega 2 bytes do address (rs1 + imm) em rd (sign extend)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = memory->readMem(registers.readReg(ins.getRs1()) + imm_se);
    value =
        value + memory->readMem(registers.readReg(ins.getRs1()) + imm_se + 1)
        << 8;
    value = sign_extend(value, 16);
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "LH",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::LW: { // Carrega 4 byte do address (rs1 + imm) em rd
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = memory->readMem(registers.readReg(ins.getRs1()) + imm_se);
    value += memory->readMem(registers.readReg(ins.getRs1()) + imm_se + 1)
        << 8;
    value += memory->readMem(registers.readReg(ins.getRs1()) + imm_se + 2)
        << 16;
    value += memory->readMem(registers.readReg(ins.getRs1()) + imm_se + 3)
        << 24;
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "LW",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::LBU: { // Carrega 1 byte do address (rs1 + imm) em rd (zero extend)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = memory->readMem(registers.readReg(ins.getRs1()) + imm_se);
    value = value << 24;
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "LBU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::LHU: { // Carrega 2 bytes do address (rs1 + imm) em rd (zero extend)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = memory->readMem(registers.readReg(ins.getRs1()) + imm_se);
    value =
        value + memory->readMem(registers.readReg(ins.getRs1()) + imm_se + 1)
        << 8;
    value = value << 16;
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "LHU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::SB: { // store byte na memoria
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = registers.readReg(ins.getRs2()) & 255; // 1 byte
    memory->writeMem(registers.readReg(ins.getRs1()) + imm_se, value);
    sprintf(disassembly, "%-8s %s, %s, %d", "SB",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::SH: { // store half word na memoria
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = registers.readReg(ins.getRs2()) & 4095; // 2 byte
    memory->writeMem(registers.readReg(ins.getRs1()) + imm_se, value);
    memory->writeMem(registers.readReg(ins.getRs1()) + imm_se + 1, value >> 8);
    sprintf(disassembly, "%-8s %s, %s, %d", "SH",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::SW: { // store word na memoria
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = registers.readReg(ins.getRs2()); // 4 bytes
    memory->writeMem(registers.readReg(ins.getRs1()) + imm_se, value);
    memory->writeMem(registers.readReg(ins.getRs1()) + imm_se + 1, value >> 8);
    memory->writeMem(registers.readReg(ins.getRs1()) + imm_se + 2, value >> 16);
    memory->writeMem(registers.readReg(ins.getRs1()) + imm_se + 3, value >> 24);
    sprintf(disassembly, "%-8s %s, %s, %d", "SW",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::SLL: { // shift rs1 left (lower 5 bits de rs2)
    uint32_t shammt = registers.readReg(ins.getRs2()) & 31;
    uint32_t value = registers.readReg(ins.getRs1()) << shammt;
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "SLL",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::SLLI: { // shift rs1 left shammt e guarda e rd
    uint32_t value = registers.readReg(ins.getRs1()) << ins.getShammt();
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "SLLI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), ins.getShammt());
    break;
  }
  case MNE::SRL: { // shift rs1 right (lower 5 bits de rs2)
    uint32_t shammt = registers.readReg(ins.getRs2()) & 31;
    uint32_t value = registers.readReg(ins.getRs1()) >> shammt;
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "SRL",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::SRLI: { // shift rs1 right shammt e guarda e rd
    uint32_t value = registers.readReg(ins.getRs1()) >> ins.getShammt();
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "SRLI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), ins.getShammt());
    break;
  }
  case MNE::SRA: { // shift rs1 right aritimetico (lower 5 bits de rs2)
    uint32_t shammt = registers.readReg(ins.getRs2()) & 31;
    uint32_t value = registers.readReg(ins.getRs1()) >> shammt;
    value = sign_extend(value, 32 - shammt);
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "SRA",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::SRAI: { // shift rs1 right aritimetico e guarda e rd
    int32_t value = registers.readReg(ins.getRs1()) >> ins.getShammt();
    value = sign_extend(value, 32 - ins.getShammt());
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "SRAI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), ins.getShammt());
    break;
  }
  case MNE::ADD: { // soma rs1 e rs2 e guarda em rd
    int32_t value = (int32_t)registers.readReg(ins.getRs1()) +
                    (int32_t)registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "ADD",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::ADDI: { // soma rs1 e immediato (sign extended)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    int32_t value = (int32_t)registers.readReg(ins.getRs1()) + imm_se;
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "ADDI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::SUB: { // subtrai rs2 de rs1 e guarda em rd
    int32_t value = (int32_t)registers.readReg(ins.getRs1()) -
                    (int32_t)registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "SUB",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::XOR: { // bitwise XOR entre rs1 e rs2 e guarda em rd
    uint32_t value =
        registers.readReg(ins.getRs1()) ^ registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "XOR",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::XORI: {
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = registers.readReg(ins.getRs1()) ^ imm_se;
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "XORI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::OR: { // bitwise or entre rs1 e rs2 e guarda em rd
    uint32_t value =
        registers.readReg(ins.getRs1()) | registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "OR",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::ORI: {
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = registers.readReg(ins.getRs1()) | imm_se;
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "ORI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::AND: { // bitwise and entre rs1 e rs2 e guarda em rd
    uint32_t value =
        registers.readReg(ins.getRs1()) & registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "AND",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::ANDI: { // bitwise and entre rs1 e imm e guarda em rd
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = registers.readReg(ins.getRs1()) & imm_se;
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %d", "ANDI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::SLT: { // rs1 < rs2 ? (com sinal)
    bool lessthan = (int32_t)registers.readReg(ins.getRs1()) <
                    (int32_t)registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), lessthan == true ? 1 : 0);
    sprintf(disassembly, "%-8s %s, %s, %s", "SLT",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::SLTI: { // rs1 < imm ? (com sinal)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    bool lessthan = (int32_t)registers.readReg(ins.getRs1()) < imm_se;
    registers.writeReg(ins.getRd(), lessthan == true ? 1 : 0);
    sprintf(disassembly, "%-8s %s, %s, %d", "SLTI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::SLTU: { // rs1 < rs2 ? (sem sinal)
    bool lessthan =
        registers.readReg(ins.getRs1()) < registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), lessthan == true ? 1 : 0);
    sprintf(disassembly, "%-8s %s, %s, %s", "SLTU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::SLTIU: { // rs1 < imm ? (sem sinal)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    bool lessthan = registers.readReg(ins.getRs1()) < imm_se;
    registers.writeReg(ins.getRd(), lessthan == true ? 1 : 0);
    sprintf(disassembly, "%-8s %s, %s, %d", "SLTIU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::BEQ: { // salta se igual (atual + imm sig)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if (registers.readReg(ins.getRs1()) == registers.readReg(ins.getRs2()))
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
    sprintf(disassembly, "%-8s %s, %s, %d", "BEQ",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::BNE: { // salta se diferente (atual + imm sig)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if (registers.readReg(ins.getRs1()) != registers.readReg(ins.getRs2()))
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
    sprintf(disassembly, "%-8s %s, %s, %d", "BNE",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::BLT: { // salta se rs1 < rs2 (atual + imm sig) - sig compare
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if ((int32_t)(registers.readReg(ins.getRs1())) <
        (int32_t)(registers.readReg(ins.getRs2())))
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
    sprintf(disassembly, "%-8s %s, %s, %d", "BLT",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::BGE: { // salta se rs1 >= rs2 (atual + imm sig) - sig compare
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if ((int32_t)(registers.readReg(ins.getRs1())) >=
        (int32_t)(registers.readReg(ins.getRs2())))
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
    sprintf(disassembly, "%-8s %s, %s, %d", "BGE",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::BLTU: { // salta se rs1 < rs2 (atual + imm sig) - unsig compare
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if (registers.readReg(ins.getRs1()) < registers.readReg(ins.getRs2()))
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
    sprintf(disassembly, "%-8s %s, %s, %d", "BLTU",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::BGEU: { // salta se rs1 >= rs2 (atual + imm sig) - unsig compare
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if (registers.readReg(ins.getRs1()) >= registers.readReg(ins.getRs2()))
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
    sprintf(disassembly, "%-8s %s, %s, %d", "BGEU",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::JAL: { // jump (atual + imm) and link pc+4
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    registers.writeReg(ins.getRd(), ins.getInsAddress() + 4);
    PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
    sprintf(disassembly, "%-8s %s, %d", "JAL",
            register_name[ins.getRd()].c_str(), imm_se);
    break;
  }
  case MNE::JALR: { // jump (rs1 + imm) and link pc+4
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    registers.writeReg(ins.getRd(), ins.getInsAddress() + 4);
    PC = (uint32_t)((int32_t)registers.readReg(ins.getRs1()) + imm_se);
    sprintf(disassembly, "%-8s %s, %s, %d", "JALR",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::FENCE:
    sprintf(disassembly, "%-8s", "FENCE");
    break;
  case MNE::FENCEI: // sync instruction and data streams
    sprintf(disassembly, "%-8s", "FENCE.I");
    break;
  case MNE::SCALL:
    sprintf(disassembly, "%-8s", "SCALL");
    running = false;
    break;
  case MNE::SBREAK:
    sprintf(disassembly, "%-8s", "SBREAK");
    running = false;
    break;
  case MNE::RDCYCLE:
    sprintf(disassembly, "%-8s", "RDCYCLE");
    break;
  case MNE::RDCYCLEH:
    sprintf(disassembly, "%-8s", "RDCYCLEH");
    break;
  case MNE::RDTIME:
    sprintf(disassembly, "%-8s", "RDTIME");
    break;
  case MNE::RDTIMEH:
    sprintf(disassembly, "%-8s", "RDTIMEH");
    break;
  case MNE::RDINSTRRET:
    sprintf(disassembly, "%-8s", "RDINSTRRET");
    break;
  case MNE::RDINSTRETH:
    sprintf(disassembly, "%-8s", "RDINSTRETH");
    break;
  case MNE::MUL: { // multiplica rs1 por rs2 e guarda em rd os 32 bits menores
    int64_t res = (int32_t)registers.readReg(ins.getRs1()) *
                  (int32_t)registers.readReg(ins.getRs2());
    uint32_t value = (int32_t)res;
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "MUL",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::MULH: { // multiplica rs1 por rs2 e guarda em rd os 32 bits maiores
                    // (signed)
    int64_t res = (int32_t)registers.readReg(ins.getRs1()) *
                  (int32_t)registers.readReg(ins.getRs2());
    uint32_t value = (int32_t)(res >> 32);
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "MULH",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::MULHU: { // multiplica rs1 por rs2 e guarda em rd os 32 bits maiores
                     // (unsigned)
    int64_t res =
        registers.readReg(ins.getRs1()) * registers.readReg(ins.getRs2());
    uint32_t value = (uint32_t)(res >> 32);
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "MULHU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::MULHSU: { // multiplica rs1 por rs2 e guarda em rd os 32 bits
                      // maiores
                      // (signed x unsigned)
    int64_t res = (int32_t)registers.readReg(ins.getRs1()) *
                  (uint32_t)registers.readReg(ins.getRs2());
    uint32_t value = (uint32_t)(res >> 32);
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "MULHSU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::DIV: { // divide rs1 por rs2 e guarda em rd (signed)
    int32_t value = (int32_t)registers.readReg(ins.getRs1()) /
                    (int32_t)registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "DIV",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::DIVU: { // divide rs1 por rs2 e guarda em rd (unsigned)
    uint32_t value = (uint32_t)registers.readReg(ins.getRs1()) /
                     (uint32_t)registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "DIVU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::REM: { // divide rs1 por rs2 e guarda o resto em rd (signed)
    uint32_t value = (uint32_t)registers.readReg(ins.getRs1()) %
                     (uint32_t)registers.readReg(ins.getRs2());
    uint32_t dividend_sign = (ins.getRs1() >> 31);
    int32_t value_s;
    if (dividend_sign == 1)
      value_s = value * -1;
    else
      value_s = value;
    registers.writeReg(ins.getRd(), value_s);
    sprintf(disassembly, "%-8s %s, %s, %s", "REM",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::REMU: { // divide rs1 por rs2 e guarda o resto em rd (unsigned)
    uint32_t value = (uint32_t)registers.readReg(ins.getRs1()) %
                     (uint32_t)registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    sprintf(disassembly, "%-8s %s, %s, %s", "REMU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  }
}

std::ostringstream processor_t::doLogLine(instruction_t ins) {
  std::ostringstream out;
  // Monta PC
  out << "[PC=" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << ins.getInsAddress() << "] "; 
  // Instrução PC
  out << "[" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << ins.getIns() << "] "; 
  // Monta rd
  uint32_t rd = (ins.getIns() >> 7) & 31;
  out << "[x" << std::dec << std::setw(2) << std::setfill('0') << rd << "=";
  out << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << registers.readReg(rd) << "] "; 
  // Monta rs1
  uint32_t rs1 = (ins.getIns() >> 15) & 31;
  out << "[x" << std::dec << std::setw(2) << std::setfill('0') << rs1 << "=";
  out << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << registers.readReg(rs1) << "] "; 
  // Monta rs2
  uint32_t rs2 = (ins.getIns() >> 20) & 31;
  out << "[x" << std::dec << std::setw(2) << std::setfill('0') << rs2 << "=";
  out << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << registers.readReg(rs2) << "] "; 
  // Disassembly (De acordo com os slides)
  out << disassembly;
  return out;
}