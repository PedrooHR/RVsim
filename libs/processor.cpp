#include "processor.h"

#include <cstring>

#include <algorithm>
#include <bitset>
#include <iomanip>
#include <iostream>
#include <string>

#include "utils.h"

// VARIAVEIS DE AMBIENTE QUE CONTROLAM IMPRESSAO
bool PRINT_INSTRUCTION_LOG = false;
bool PRINT_INSTRUCTION_END_TIME = false;
bool PRINT_CACHE_MISSES_LOG = false;
bool PRINT_GSHARE_LOG = false;

processor_t::processor_t(memory_t *mem, uint32_t entry_point, int n_ins) {
  // Ve se deseja imprimir o log de instruções
  if (char *env = getenv("PRINT_INSTRUCTION_LOG")) 
    if (strcmp(env, "true") == 0) 
      PRINT_INSTRUCTION_LOG = true;
  if (char *env = getenv("PRINT_INSTRUCTION_END_TIME")) 
    if (strcmp(env, "true") == 0) 
      PRINT_INSTRUCTION_END_TIME = true;
  // Ve se deseja imprimir o log de misses da cache
  if (char *env = getenv("PRINT_CACHE_MISSES_LOG")) 
    if (strcmp(env, "true") == 0) 
      PRINT_CACHE_MISSES_LOG = true;
  // Ve se deseja imprimir o log do gshare
  if (char *env = getenv("PRINT_GSHARE_LOG")) 
    if (strcmp(env, "true") == 0) 
      PRINT_GSHARE_LOG = true;

  memory = mem;
  PC = entry_point;
  gshare = new gshare_t(10, 1024);
  registers.writeReg(2, memory->getTotalSize() - 4); // ajusta stack pointer
  cycle = 0;

  number_i = (n_ins > 0) ? n_ins : 1;

  ALU.clear();
  ALU.resize(NUMBER_OF_ALU, 0);
  AGU.clear();
  AGU.resize(NUMBER_OF_AGU, 0);
  BRU.clear();
  BRU.resize(NUMBER_OF_BRU, 0);
  memory_avail = 0;
}

void processor_t::executeProgram() {
  running = true;
  uint32_t tcycles = 0, tinst = 0;
  uint32_t previous_finish = 0;
  uint32_t branch_penalty = 0;
  uint32_t linst = 0;
  uint32_t commit_count = 0;
  while (running) {
    // Devolve o ciclo em que o fetch é feito
    uint32_t time_instruction = (linst / number_i) + branch_penalty; 
    uint32_t instruction_start = time_instruction;

    // ====================================================================== //
    // ESTAGIO DE FETCH (min 4 ciclos)
    // ====================================================================== //
    uint32_t raw_instruction, pc_instruction;
    bool prediction;
    time_instruction += Fetch(&raw_instruction, &pc_instruction, &prediction);

    // ====================================================================== //
    // ESTAGIO DE DECODE
    // ====================================================================== //
    time_instruction += BASE_DECODE_DURATION;
    instruction_t instruction(raw_instruction, pc_instruction);

    // ====================================================================== //
    // ESTAGIO DE ALLOCATION
    // ====================================================================== //
    time_instruction += BASE_ALLOC_DURATION;

    // ====================================================================== //
    // ESTAGIO DE ISSUE (3 ciclos)
    // ====================================================================== //
    uint32_t rs1 = instruction.getRs1();
    uint32_t rs2 = instruction.getRs2();
    uint32_t rd  = instruction.getRd();
    FU fu = instruction.getFU();
    // Dependência de dados
    uint32_t rs1_avail = (rs1 != -1) ? registers.checkUW(rs1) : 0;
    uint32_t rs2_avail = (rs2 != -1) ? registers.checkUW(rs2) : 0;

    // Indendependente das dependencias, temos pelo menos 3 ciclos
    time_instruction += BASE_ISSUE_DURATION;  
    
    // Se tiver alguma dependência de dados, vamos postegar o inicio.
    if (rs1_avail > time_instruction || rs2_avail > time_instruction) 
      time_instruction = std::max(rs1_avail, rs2_avail); 

    uint32_t res_avail;
    bool mem_acess = false;
    switch (fu) {
      case FU::ALU: 
        res_avail = getNextALU(time_instruction);
        break;
      case FU::AGU:
        mem_acess = true;
        res_avail = getNextAGU(time_instruction);
        break;
      case FU::BRU:
        res_avail = getNextBRU(time_instruction);
        break;
      case FU::NONE:
        break;
    }

    // se res_avail <= time instruction significa que temos uma unidade
    // funcional disponível assim que a parte de issue acabar, se res_avail >
    // time_instruction significa que só teremos recursos disponíveis a partir
    // de res_avail, então a instrução fica em stall até lá.
    if (res_avail > time_instruction)
      time_instruction = res_avail;

    // ====================================================================== //
    // ESTAGIO DE EXECUÇÃO (min 2 ciclos)
    // ====================================================================== //
    time_instruction += BASE_EXECUTE_DURATION;
    // Executa a instrução e cria a linha de log
    char disassembly[50];
    uint32_t extra_cicles = Execute(instruction, disassembly);
    std::string instruction_log = doLogLine(instruction, disassembly);

    // Feedback gshare
    if (is_branch) {
      // Acerto
      if (branched != prediction) {
        // Penalidade pelo erro - Cada vez que erramos significa que perdemos um
        // tanto de ciclos
        branch_penalty += BRANCH_PREDICTION_PEN;
        linst = ((linst / number_i) + 1) * number_i;
      }
      gshare->feedback(branched, pc_instruction, PC);
    }
    
    if (mem_acess) {
      // remove o write back temporariamente
      time_instruction -= 1;
      // Teremos extra_cicles se for acesso a MEM
      uint32_t mem_avail = getNextMEM(time_instruction, extra_cicles);
      // se mem_avail > time_instruction, significa que o recurso de memoria
      // está ocupado, temos que esperar, assim o tempo de
      if (mem_avail > time_instruction)
        time_instruction = mem_avail;

      // Adiciona os ciclos extras de acesso a memória
      time_instruction += extra_cicles;

      // Readiciona o writeback
      time_instruction += 1;
    }

    // Write back 
    if (wrote)
      // -1 assumindo que se le no mesmo ciclo que se escreve
      registers.setUW(rd, time_instruction - 1);

    // ====================================================================== //
    // ESTAGIO DE COMMIT (1 ciclo)
    // ====================================================================== //
    time_instruction += BASE_COMMIT_DURATION;
    uint32_t commited_at;
    // Checa commit da instrução
    if (time_instruction > previous_finish) {
      commited_at = time_instruction;
      previous_finish = time_instruction;
      commit_count = 1;
    } else {
      commit_count++;
      if (commit_count > 4) {
        commit_count = 1;
        previous_finish += BASE_COMMIT_DURATION;
      }
      commited_at = previous_finish;
    }

    // Contador de instruções
    tinst++;
    linst++;

    // Imprime log da instrução
    if (PRINT_INSTRUCTION_LOG) {
      std::cout << std::left << std::setw(93) << instruction_log;
      if (PRINT_INSTRUCTION_END_TIME) {
        std::cout << " | FINISHED AT: " << std::setw(5) << time_instruction;
        std::cout << "| COMMITED AT: " << std::setw(5) << commited_at;
      }
      std::cout << std::endl;
    }
    // ====================================================================== //
  }
  std::cout << "\nINFORMAÇÕES SOBRE CICLOS E #INSTRUÇÕES:\n";
  std::cout << "  Total number of cycles: " << previous_finish << std::endl;
  std::cout << "  Total number of instructions: " << tinst << std::endl;
  if (PRINT_GSHARE_LOG) {
    std::cout << "\nINFORMAÇÕES SOBRE GSHARE:\n";
    std::cout << "  Acertos: " << gshare->getHits() << std::endl;
    std::cout << "  Erros: " << gshare->getErrors() << std::endl;
  }
  if (PRINT_CACHE_MISSES_LOG) {
    std::cout << "\nINFORMAÇÕES SOBRE MISSES NAS CACHES:\n";
    memory->printCacheMisses();
  }
}

uint32_t processor_t::Fetch(uint32_t *raw_instruction, uint32_t *pc_address,
                            bool *pred) {
  // Fetch do PC atual
  uint8_t mem_value;
  uint32_t lcycles = BASE_FETCH_DURATION;
  uint32_t value = 0;
  for (int i = 0; i < 4; i++) {
    lcycles += memory->readMem(PC + i, &mem_value, ACCESS_TYPE::INSTRUCTION);
    value += mem_value << (8 * i);
  }
  *pc_address = PC;
  *raw_instruction = value;
  *pred = false; // vamos assumir que não é um salto

  // BRANCH PREDICTION
  // Por enquanto vamos assumir que há hardware pra verificar se a instrução
  // se trata de um jump no próprio FETCH, se não, precisaríamos testar se é ou
  // não um jump

  // Vamos presumir inicialmente que não tomaremos a branch
  PC = PC + 4;

  // Agora testaremos se vamos tomar a branch, se sim PC será alterado
  uint32_t opcode = value & 127; // 7 bits de opcode
  if (opcode == 0x0063 || opcode == 0x006F || opcode == 0x0067) {
    // Sabemos que temos uma instrução de jump
    // gshare olhará o histórico pra ver se vamos tomar a branch ou não,
    // inicialmente todas as branches não vão ser tomadas
    bool branch = gshare->checkBranch();
    if (branch) {
      // Se vamos tomar a branch, precisamos saber para qual endereço, que está
      // guardado na btb. Vamos indexar o BTB pelo PC. Temos umas btb de 1024
      // entradas, então os bits 11..2 (inclusive) do PC indexam as entradas da
      // btb e os bits 31..12 (inclusive) são a tag. Se o PC e a tag não baterem
      // cancelamos o salto
      int address = gshare->getBTB(PC);
      if (address > 0) {
        // Se a btb retornar um endereço vamos utilizá-lo
        PC = address;
        // Se temos um address pra salto, vamos marcar como branched
        *pred = true;
      }
    }
  }
  return lcycles;
}

uint32_t processor_t::Execute(instruction_t ins, char *disassembly) {
  // Todas as unidades funcionais precisam de pelo menos 1 ciclo pra execução,
  // acessos a memória podem demorar mais.
  uint32_t lcycle = 0;
  // Branched controla se a instrução fez uma branch ou não, se sim, precisamos
  // invalidar as instruções subsequentes que fazem parte do mesmo block
  branched = false;
  is_branch = false;
  wrote = false;
  switch (ins.getOperation()) {
  case MNE::LUI: { // Coloca o imediato no rd (preenche os 12 lower bits com 0)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    registers.writeReg(ins.getRd(), imm_se);
    wrote = true;
    sprintf(disassembly, "%-8s %s, %d", "LUI",
            register_name[ins.getRd()].c_str(), imm_se);
    break;
  }
  case MNE::AUIPC: { // Adiciona o imediato ao address da instrução e coloca no
                     // rd (preenche os 12 lower bits com 0)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    registers.writeReg(ins.getRd(), imm_se + ins.getInsAddress());
    wrote = true;
    sprintf(disassembly, "%-8s %s, %d", "AUIPC",
            register_name[ins.getRd()].c_str(), imm_se);
    break;
  }
  case MNE::LB: { // Carrega 1 byte do address (rs1 + imm) em rd (sign extend)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint8_t mem_value;
    uint32_t value = 0;
    // Vamos considerar que gastamos pelo menos 1 ciclo para acessar a cache
    lcycle += 1;
    for (int i = 0; i < 1; i++) {
      lcycle += memory->readMem(registers.readReg(ins.getRs1()) + imm_se + i,
                                &mem_value, ACCESS_TYPE::DATA);
      value += mem_value << (8 * i);
    }
    value = sign_extend(value, 8);
    registers.writeReg(ins.getRd(), value);
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "LB",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::LH: { // Carrega 2 bytes do address (rs1 + imm) em rd (sign extend)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint8_t mem_value;
    uint32_t value = 0;
    // Vamos considerar que gastamos pelo menos 1 ciclo para acessar a cache
    lcycle += 1;
    for (int i = 0; i < 2; i++) {
      lcycle += memory->readMem(registers.readReg(ins.getRs1()) + imm_se + i,
                                &mem_value, ACCESS_TYPE::DATA);
      value += mem_value << (8 * i);
    }
    value = sign_extend(value, 16);
    registers.writeReg(ins.getRd(), value);
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "LH",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::LW: { // Carrega 4 byte do address (rs1 + imm) em rd
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint8_t mem_value;
    uint32_t value = 0;
    // Vamos considerar que gastamos pelo menos 1 ciclo para acessar a cache
    lcycle += 1;
    for (int i = 0; i < 4; i++) {
      lcycle += memory->readMem(registers.readReg(ins.getRs1()) + imm_se + i,
                                &mem_value, ACCESS_TYPE::DATA);
      value += mem_value << (8 * i);
    }
    registers.writeReg(ins.getRd(), value);
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "LW",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::LBU: { // Carrega 1 byte do address (rs1 + imm) em rd (zero extend)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint8_t mem_value;
    uint32_t value = 0;
    // Vamos considerar que gastamos pelo menos 1 ciclo para acessar a cache
    lcycle += 1;
    for (int i = 0; i < 1; i++) {
      lcycle += memory->readMem(registers.readReg(ins.getRs1()) + imm_se + i,
                                &mem_value, ACCESS_TYPE::DATA);
      value += mem_value << (8 * i);
    }
    registers.writeReg(ins.getRd(), value);
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "LBU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::LHU: { // Carrega 2 bytes do address (rs1 + imm) em rd (zero extend)
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint8_t mem_value;
    uint32_t value = 0;
    // Vamos considerar que gastamos pelo menos 1 ciclo para acessar a cache
    lcycle += 1;
    for (int i = 0; i < 2; i++) {
      lcycle += memory->readMem(registers.readReg(ins.getRs1()) + imm_se + i,
                                &mem_value, ACCESS_TYPE::DATA);
      value += mem_value << (8 * i);
    }
    registers.writeReg(ins.getRd(), value);
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "LHU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::SB: { // store byte na memoria
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = registers.readReg(ins.getRs2()) & 255; // 1 byte
    // Vamos considerar que gastamos pelo menos 1 ciclo para acessar a cache
    lcycle += 1;
    lcycle += memory->writeMem(registers.readReg(ins.getRs1()) + imm_se, value,
                               ACCESS_TYPE::DATA);
    sprintf(disassembly, "%-8s %s, %s, %d", "SB",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::SH: { // store half word na memoria
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = registers.readReg(ins.getRs2()) & 4095; // 2 byte
    // Vamos considerar que gastamos pelo menos 1 ciclo para acessar a cache
    lcycle += 1;
    lcycle += memory->writeMem(registers.readReg(ins.getRs1()) + imm_se, value,
                               ACCESS_TYPE::DATA);
    lcycle += memory->writeMem(registers.readReg(ins.getRs1()) + imm_se + 1,
                               value >> 8, ACCESS_TYPE::DATA);
    sprintf(disassembly, "%-8s %s, %s, %d", "SH",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::SW: { // store word na memoria
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    uint32_t value = registers.readReg(ins.getRs2()); // 4 bytes
    // Vamos considerar que gastamos pelo menos 1 ciclo para acessar a cache
    lcycle += 1;
    lcycle += memory->writeMem(registers.readReg(ins.getRs1()) + imm_se, value,
                               ACCESS_TYPE::DATA);
    lcycle += memory->writeMem(registers.readReg(ins.getRs1()) + imm_se + 1,
                               value >> 8, ACCESS_TYPE::DATA);
    lcycle += memory->writeMem(registers.readReg(ins.getRs1()) + imm_se + 2,
                               value >> 16, ACCESS_TYPE::DATA);
    lcycle += memory->writeMem(registers.readReg(ins.getRs1()) + imm_se + 3,
                               value >> 24, ACCESS_TYPE::DATA);
    sprintf(disassembly, "%-8s %s, %s, %d", "SW",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::SLL: { // shift rs1 left (lower 5 bits de rs2)
    uint32_t shammt = registers.readReg(ins.getRs2()) & 31;
    uint32_t value = registers.readReg(ins.getRs1()) << shammt;
    registers.writeReg(ins.getRd(), value);
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %s", "SLL",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  case MNE::SLLI: { // shift rs1 left shammt e guarda e rd
    uint32_t value = registers.readReg(ins.getRs1()) << ins.getShammt();
    registers.writeReg(ins.getRd(), value);
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "SLLI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), ins.getShammt());
    break;
  }
  case MNE::SRL: { // shift rs1 right (lower 5 bits de rs2)
    uint32_t shammt = registers.readReg(ins.getRs2()) & 31;
    uint32_t value = registers.readReg(ins.getRs1()) >> shammt;
    registers.writeReg(ins.getRd(), value);
    wrote = true;
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
    wrote = true;
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
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "SRAI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), ins.getShammt());
    break;
  }
  case MNE::ADD: { // soma rs1 e rs2 e guarda em rd
    int32_t value = (int32_t)registers.readReg(ins.getRs1()) +
                    (int32_t)registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    wrote = true;
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
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "ADDI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::SUB: { // subtrai rs2 de rs1 e guarda em rd
    int32_t value = (int32_t)registers.readReg(ins.getRs1()) -
                    (int32_t)registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    wrote = true;
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
    wrote = true;
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
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "XORI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::OR: { // bitwise or entre rs1 e rs2 e guarda em rd
    uint32_t value =
        registers.readReg(ins.getRs1()) | registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    wrote = true;
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
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "ORI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::AND: { // bitwise and entre rs1 e rs2 e guarda em rd
    uint32_t value =
        registers.readReg(ins.getRs1()) & registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), value);
    wrote = true;
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
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "ANDI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::SLT: { // rs1 < rs2 ? (com sinal)
    bool lessthan = (int32_t)registers.readReg(ins.getRs1()) <
                    (int32_t)registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), lessthan == true ? 1 : 0);
    wrote = true;
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
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "SLTI",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::SLTU: { // rs1 < rs2 ? (sem sinal)
    bool lessthan =
        registers.readReg(ins.getRs1()) < registers.readReg(ins.getRs2());
    registers.writeReg(ins.getRd(), lessthan == true ? 1 : 0);
    wrote = true;
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
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %d", "SLTIU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(), imm_se);
    break;
  }
  case MNE::BEQ: { // salta se igual (atual + imm sig)
    is_branch = true;
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if (registers.readReg(ins.getRs1()) == registers.readReg(ins.getRs2())) {
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
      branched = true;
    }
    sprintf(disassembly, "%-8s %s, %s, %d", "BEQ",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::BNE: { // salta se diferente (atual + imm sig)
    is_branch = true;
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if (registers.readReg(ins.getRs1()) != registers.readReg(ins.getRs2())) {
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
      branched = true;
    }
    sprintf(disassembly, "%-8s %s, %s, %d", "BNE",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::BLT: { // salta se rs1 < rs2 (atual + imm sig) - sig compare
    is_branch = true;
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if ((int32_t)(registers.readReg(ins.getRs1())) <
        (int32_t)(registers.readReg(ins.getRs2()))) {
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
      branched = true;
    }
    sprintf(disassembly, "%-8s %s, %s, %d", "BLT",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::BGE: { // salta se rs1 >= rs2 (atual + imm sig) - sig compare
    is_branch = true;
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if ((int32_t)(registers.readReg(ins.getRs1())) >=
        (int32_t)(registers.readReg(ins.getRs2()))) {
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
      branched = true;
    }
    sprintf(disassembly, "%-8s %s, %s, %d", "BGE",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::BLTU: { // salta se rs1 < rs2 (atual + imm sig) - unsig compare
    is_branch = true;
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if (registers.readReg(ins.getRs1()) < registers.readReg(ins.getRs2())) {
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
      branched = true;
    }
    sprintf(disassembly, "%-8s %s, %s, %d", "BLTU",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::BGEU: { // salta se rs1 >= rs2 (atual + imm sig) - unsig compare
    is_branch = true;
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    if (registers.readReg(ins.getRs1()) >= registers.readReg(ins.getRs2())) {
      PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
      branched = true;
    }
    sprintf(disassembly, "%-8s %s, %s, %d", "BGEU",
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str(), imm_se);
    break;
  }
  case MNE::JAL: { // jump (atual + imm) and link pc+4
    is_branch = true;
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    registers.writeReg(ins.getRd(), ins.getInsAddress() + 4);
    wrote = true;
    PC = (uint32_t)((int32_t)ins.getInsAddress() + imm_se);
    branched = true;
    sprintf(disassembly, "%-8s %s, %d", "JAL",
            register_name[ins.getRd()].c_str(), imm_se);
    break;
  }
  case MNE::JALR: { // jump (rs1 + imm) and link pc+4
    is_branch = true;
    int32_t imm_se = sign_extend(ins.getImm(), ins.getImmSize());
    registers.writeReg(ins.getRd(), ins.getInsAddress() + 4);
    wrote = true;
    PC = (uint32_t)((int32_t)registers.readReg(ins.getRs1()) + imm_se);
    branched = true;
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
    wrote = true;
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
    wrote = true;
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
    wrote = true;
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
    wrote = true;
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
    wrote = true;
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
    wrote = true;
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
    wrote = true;
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
    wrote = true;
    sprintf(disassembly, "%-8s %s, %s, %s", "REMU",
            register_name[ins.getRd()].c_str(),
            register_name[ins.getRs1()].c_str(),
            register_name[ins.getRs2()].c_str());
    break;
  }
  }
  return lcycle;
}

std::string processor_t::doLogLine(instruction_t ins, char *disassembly) {
  std::ostringstream out;
  // Monta PC
  out << "[PC=" << std::uppercase << std::hex << std::setw(8)
      << std::setfill('0') << ins.getInsAddress() << "] ";
  // Instrução PC
  out << "[" << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
      << ins.getIns() << "] ";
  // Monta rd
  uint32_t rd = (ins.getIns() >> 7) & 31;
  out << "[x" << std::dec << std::setw(2) << std::setfill('0') << rd << "=";
  out << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
      << registers.readReg(rd) << "] ";
  // Monta rs1
  uint32_t rs1 = (ins.getIns() >> 15) & 31;
  out << "[x" << std::dec << std::setw(2) << std::setfill('0') << rs1 << "=";
  out << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
      << registers.readReg(rs1) << "] ";
  // Monta rs2
  uint32_t rs2 = (ins.getIns() >> 20) & 31;
  out << "[x" << std::dec << std::setw(2) << std::setfill('0') << rs2 << "=";
  out << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
      << registers.readReg(rs2) << "] ";
  // Disassembly (De acordo com os slides)
  out << disassembly;
  return out.str();
}

uint32_t processor_t::getNextALU(uint32_t time) {
  int min = 0;
  for (unsigned int i = 0; i < ALU.size(); i++) {
    if (ALU[i] < ALU[min])
      min = i;
  }
  int temp = ALU[min];
  ALU[min] = ((temp < time) ? time : temp) + CICLES_ALU;
  return temp;
}

uint32_t processor_t::getNextAGU(uint32_t time) {
  int min = 0;
  for (unsigned int i = 0; i < AGU.size(); i++) {
    if (AGU[i] < AGU[min])
      min = i;
  }
  int temp = AGU[min];
  AGU[min] = ((temp < time) ? time : temp)  + CICLES_AGU;
  return temp;
}

uint32_t processor_t::getNextBRU(uint32_t time) {
  int min = 0;
  for (unsigned int i = 0; i < BRU.size(); i++) {
    if (BRU[i] < BRU[min])
      min = i;
  }
  int temp = BRU[min];
  BRU[min] = ((temp < time) ? time : temp)  + CICLES_BRU;
  return temp;
}

uint32_t processor_t::getNextMEM(uint32_t time, uint32_t time_used) {
  int temp = memory_avail;
  memory_avail = ((temp < time) ? time : temp) + time_used;
  return temp;
}

/***************************************************************************
Gshare
***************************************************************************/
gshare_t::gshare_t(int psize, int pbtb_size) {
  size = psize; // Em bits
  btb_size = pbtb_size;
  table_entries = 2;
  for (int i = 1; i < size; i++)
    table_entries *= 2;

  hits = 0;
  errors = 0;

  // Inicializa as estruturas do gshare
  h_table.clear();
  h_table.resize(table_entries, 0);
  btb.clear();
  btb.resize(btb_size, 0);
  btb_tag.clear();
  btb_tag.resize(btb_size, 0);
  // Limpa a queue
  while (!branch_h.empty())
    branch_h.pop();
  history = 0;
  l_history = 0;
}

bool gshare_t::checkBranch() {
  // l_history significa que temos branches dos quais ainda nao recebemos
  // feedback, ou seja, estamos em um branch subsequente, então vamos assumir
  // que o branch anterior acertou e vamos usar aquele o histórico ainda sem
  // feedback.

  // l_history e history sempre terão apenas a quantidade de bits do gshare
  int entry = (l_history != history ? l_history : history);

  bool result = h_table[entry];

  // Vamos adicionar um histórico pra receber o feedback. Sabendo a entry e o
  // resultado, podemos arrumar o histórico posteriormente no feedback
  branch_h.push(std::pair<int, bool>(entry, result));

  // Nosso histórico momentaneo passa a ser o histórico atual + decisão atual
  l_history = (l_history << 1) + (result ? 1 : 0);
  // para efeitos de comparação, vamos manter sempre a quantidade de bits do
  // gshare
  l_history = l_history & (table_entries - 1);

  return result;
}

uint32_t gshare_t::getBTB(uint32_t address) {
  uint32_t btb_entry = (address >> 2) & (btb_size - 1); // pega os bits 11 .. 2
  uint32_t tag = (address >> 12);                       // pega os bits 31 .. 12

  if (btb_tag[btb_entry] == tag) {
    // btb bate com PC passado, returna o endereço que está na tbt
    return btb[btb_entry];
  } else {
    // btb não bate com PC passado
    return 0;
  }
}

void gshare_t::feedback(bool branched, uint32_t pc, uint32_t address) {
  // O feedback é chamado sempre que temos uma instrução de salto, antes de
  // executar a próxima instrução (em processador in-order)

  // Aqui saberamos o histórico antigo e a decisão que tomamos no branch
  // prediction
  std::pair<int, bool> branch_h_entry = branch_h.front();
  branch_h.pop();
  int entry = branch_h_entry.first;      // <pair>.first é a entry
  bool decision = branch_h_entry.second; // <pair>.second foi a decisão tomada

  // Independente se acertamos ou não a decisão, se tivemos um branch, vamos
  // atualizar a btb para se tiver um branch futuro termos o endereço certo de
  // salto
  if (branched) {
    uint32_t btb_entry = (pc >> 2) & (btb_size - 1); // pega os bits 11..2
    uint32_t tag = (pc >> 12);                       // pega os bits 31 .. 12

    // Atualiza o endereço de salto
    btb[btb_entry] = address;
    // Atualiza o tag
    btb_tag[btb_entry] = tag;
  }

  // Independente se acertamos ou não, vamos atualizar o histórico pra incluir a
  // decisão correta do branch que acabamos de executar
  history = (history << 1) + (branched ? 1 : 0);
  history = history & (table_entries - 1);

  // Se erramos na decisão, precisamos atualiza a entry que gerou a decisão
  // errada, e podemos limpar a fila de branches subsequentes já que as
  // instruções serão descartadas. Atualizamos também o histórico momentaneo
  // Caso acertarmos no branch, mais nada precisa ser corrigido.
  if (decision != branched) {
    // Arruma entry
    h_table[entry] = branched;
    // Limpa fila de feedback
    while (!branch_h.empty())
      branch_h.pop();
    // Atualiza l_history porque fazemos clean dos branches subsequentes e não
    // há mais nenhum branch pra dar feedback
    l_history = history;
    // Incrementa o contador de erros de previsão
    errors++;
  } else {
    // Caso a predição tenha acertado, incrementamos o contador de acertos do
    // gshare
    hits++;
  }

  // std::cout << "[Gshare] -> "
  //           << (decision != branched ? " Error; " : " Hit; ")
  //           << (branched ? "Saltou; " : "Não saltou; ")
  //           << "Historico atual: " << std::bitset<10>(history)
  //           << std::endl;
}

int gshare_t::getHits() { 
  return hits; 
}

int gshare_t::getErrors() { 
  return errors; 
}