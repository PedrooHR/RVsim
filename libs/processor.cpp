#include "processor.h"

#include <algorithm>
#include <bitset>
#include <iomanip>
#include <iostream>

#include "utils.h"

//// IMPORTANTE: SEGUNDO A ESPECIFICAÇÃO, TODO IMEDIATO É SIGN-EXTENDED

processor_t::processor_t(memory_t *mem, uint32_t entry_point, int n_ins) {
  memory = mem;
  PC = entry_point;
  gshare = new gshare_t(10, 1024);
  registers.writeReg(2, memory->getTotalSize() - 4); // ajusta stack pointer
  cycle = 0;

  number_i = (n_ins > 0) ? n_ins : 1;

  // Inicialização das variáveis de controle da simulação
  used = new bool[number_i];
  pred = new bool[number_i];
  ri = new uint32_t[number_i];
  pc = new uint32_t[number_i];
  cycles = new uint32_t[number_i];
  ds = new std::string[number_i];
  d = new char *[number_i];
  for (int i = 0; i < number_i; i++)
    d[i] = new char[50];
  ins = new instruction_t[number_i];
}

void processor_t::executeProgram() {
  running = true;
  uint32_t tcycles = 0, tinst = 0;

  while (running) {
    // Reinicio das váriaveis de controle
    for (int i = 0; i < number_i; i++) {
      used[i] = false;
      pred[i] = false;
    }
    // ====================================================================== //
    // ESTAGIO DE FETCH
    // ====================================================================== //
    // O estágio de fetch é normalmente divido em 4 outros estágios levando
    // normalmente 4 ciclos para a execução, se a instrução estiver fora da
    // cache de instruções, temos 10 ciclos adicionais para consultar a cache
    // L2, se tiver miss na L2, mais 30 ciclos para checar a L3 e caso haja miss
    // na L3, mais 100 ciclos para checar a memoria RAM.
    for (int i = 0; i < number_i; i++)
      cycles[i] = Fetch(&ri[i], &pc[i], &pred[i]);

    // TODO: Perguntar se como conseguimos fazer o fetch de 4 instruções, se
    // fazemos uma depois da outra e contam o ciclos de pegar todas as
    // instruções, ou se contabilizamos só a mais longa (ex: se temos dois
    // acessos a ram para pegar o valor da instrução e colocar na cache)

    // Sempre haverá pelo menos uma instrução
    uint32_t c_max = cycles[0];
    for (int i = 0; i < number_i; i++)
      c_max = (c_max > cycles[i]) ? c_max : cycles[i];
    tcycles += c_max;

    // ====================================================================== //
    // ESTAGIO DE DECODE
    // ====================================================================== //
    // O estágio de decode do RISC-V é bem simples, como as instruções tem
    // tamanho fixo, podemos fazer o decode todo numa única vez, então gastamos
    // 1 ciclos para fazer o decode.
    for (int i = 0; i < number_i; i++) {
      instruction_t p(ri[i], pc[i]);
      ins[i] = p;
    }

    // Aqui temos a mesma pergunta, a principio, temos hardware pra fazer decode
    // de (até) 4 instruções
    tcycles += 1;

    // ====================================================================== //
    // ESTAGIO DE ALLOCATION
    // ====================================================================== //
    // No estágio de allocation fazemos register renaming, aqui não precisamos
    // necessáriamente fazer register renaming, mas podemos assumir que as
    // instruções podem ser executadas em qualquer ordem, desde que não tenham
    // uma dependencia do tipo leitura após escrita. (read after write)
    // Vamos assumir que o register renaming é feito mas não implementá-lo
    // explicitamente. Com isso, estamos livres de todas as dependencias de
    // NOME

    // TODO: conferir se podemos assumir que o renaming sempre vai ser 1 ciclo
    tcycles += 1;

    // ====================================================================== //
    // ESTAGIO DE ISSUE
    // ====================================================================== //
    // No estágio de Issue verifica-se os recursos que estão disponíveis e
    // alocamos o que vai ser usado pra executar cada instrução. Estamos
    // assumindo que o processador é fora de ordem e que register renaming é
    // feito, mas ainda precisamos alocar as instruções de acordo com os
    // recursos disponíveis e as dependencias que não são de NOME.
    // Vamos considerar que as instruções podem ser executadas fora de ordem
    // e que o reoordering vai ser feito no estágio de commit

    // TODO

    // ====================================================================== //
    // ESTAGIO DE EXECUÇÃO
    // ====================================================================== //
    // O estágio de execução é responsável por executar todas as instruções
    // Descartamos as instruções subsequentes em caso de uma predição de salto
    // falhar.
    // Vamos executar todas as instruções em ordem para que o resultado da
    // simulação seja correto, apesar de estarmos considerando register renaming
    //

    // Modelo com branch prediction
    // Sempre executamos a primeira instrução
    used[0] = true;
    for (int i = 0; i < number_i; i++) {
      if (used[i]) {
        // Executa a instrução e cria a linha de log
        cycles[i] = Execute(ins[i], d[i]);
        ds[i] = doLogLine(ins[i], d[i]);
        // Verificamos se a instruções foi uma branch, se sim, precisamos
        // devolver o feedback para o gshare
        if (is_branch) {
          // se acertarmos a predição de salto, vamos executar a próxima
          // instrução, se não, vamos eliminar todas as próximas instruções do
          // bloco
          if (branched == pred[i])
            if (i < 3)
              used[i + 1] = true;
          // devolve o feedback pro gshare
          gshare->feedback(branched, pc[i], PC);
        } else {
          // Se não for a instrução de encerrar o programa, como não temos um
          // branch, vamos executar a próxima instrução.
          if (running)
            if (i < 3)
              used[i + 1] = true;
        }
      }
    }

    // ====================================================================== //
    // CONTABILIZAÇÃO DOS CICLOS NA EXECUÇÃO
    // (SIMULA O CONTROLE FEITO DO ESTAGIO DE ISSUE)
    // ====================================================================== //
    // Temos duas questões a verificar, a primeira é relação de dependencias,
    // a outra é se temos uma operação de load/store, como estamos falando de
    // um processador in-order, as operações de load/store demoram mais que 1
    // ciclo. Também tem que verificar

    // TODO

    // ====================================================================== //
    // ESTAGIO DE COMMIT
    // ====================================================================== //
    // No estágio de commit vamos assumir que o reoordering é feito, no caso, 
    // se a impressão de instruções estiver ligada, vamos imprimir a ordem 
    // correta de execução.
    for (int i = 0; i < number_i; i++) {
      if (used[i]) {
        tinst++;
        // std::cout << ds[i] << std::endl;
      }
    }
    tcycles += 1;

    // ====================================================================== //
    cycle += tcycles;
  }
  std::cout << "Total number of cycles: " << cycle << std::endl;
  std::cout << "Total number of instructions: " << tinst << std::endl;
  std::cout << "gshare->getHits(): " << gshare->getHits() << std::endl;
  std::cout << "gshare->getErrors(): " << gshare->getErrors() << std::endl;
  std::cout << "memory->printCacheMisses(): " << std::endl;
  memory->printCacheMisses();
}

uint32_t processor_t::Fetch(uint32_t *raw_instruction, uint32_t *pc_address,
                            bool *pred) {
  // Fetch do PC atual
  uint8_t mem_value;
  uint32_t lcycles = 4;
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
  uint32_t lcycle = 1;
  // Branched controla se a instrução fez uma branch ou não, se sim, precisamos
  // invalidar as instruções subsequentes que fazem parte do mesmo block
  branched = false;
  is_branch = false;
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

int gshare_t::getHits() { return hits; }

int gshare_t::getErrors() { return errors; }