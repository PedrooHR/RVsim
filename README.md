## RVsim

Simulador do processador RISC-V para disciplina de Arquitetura de Computadores 2 na Unicamp.

O comportamento das instruções desse simulador foram montadas de acordo com o descrito no [ISA Specification](https://riscv.org/technical/specifications/). Esse simulador não implementa o disassembly de instruções em pseudo-instruções, e para conferência do corretude, seu comportamento foi avaliado comparando o log de execução de cada instrução com o log do Simulador [Spike](https://github.com/riscv/riscv-isa-sim). 

Nenhuma biblioteca adicional foi utilizada, nem mesmo para o *parsing* do arquivo elf. A memória máxima é igual a `UINT32_MAX` e é feita por mapping e não alocando um vetor para memória.
    
## Compilando e executando o programa

**Para compilar:**

`make $DINERO_INSTALL_DIR=<dinero-install-dir>`

**Para Executar:**

`./rvsim <elf_program> <janela_instruções:int> <bits_gshare:int>`

**Configuração do simulador:**
A configuração do tamanho da janela de instruções e da quantidade de bits do histórico do gshare é feita via argumentos do program. Demais configurações são feitas por variáveis de ambiente.

**Memória do programa:**
A memória é feita utilizando mapeamento, e não mais vetor, isso permite que endereçamento máxima seja igual a `UINT32_MAX`.

**Impressões:**
Por padrão, o simulador sempre imprime o número de ciclos total para execução e o número de instruções.  Algumas impressões extras são controladas por variáveis de ambientes, que podem ser configuradas:

- `export PRINT_INSTRUCTION_LOG=<bool>`: Imprime a linha de log para cada instruções (conforme Projeto 1).
- `export PRINT_INSTRUCTION_END_TIME=<bool>`: Imprime quando cada instrução fez commit. (Só se `PRINT_INSTRUCTION_LOG=true`).
- `export PRINT_CACHE_MISSES_LOG=<bool>`: Imprime a quantidade de misses de cada cache.
- `export PRINT_GSHARE_LOG=<bool>`: Imprime quantos acertos e quantos erros o gshare teve.

**Observações Importantes:**
- O simulador assume leitura antes do despacho.
- O simulador assume que todas as operações de memória são ordenadas e que só é possível fazer uma por vez (se uma operação de store/load leva 150 ciclos, a próximo acesso só começará após os 150 acesso). Por questões de simplificação, não é verificado o acesso concorrente a memória entre as operações de LOAD/STORE e o FETCH das instruções.

**Informações das caches (dineroIV)**
- `L1i`: (sub-)Blocos de tamanho 2^7, Tamanho total de 2^15. 
- `L1d`: (sub-)Blocos de tamanho 2^7, Tamanho total de 2^15. 
- `L2`: (sub-)Blocos de tamanho 2^10, Tamanho total de 2^18. 
- `L3`: (sub-)Blocos de tamanho 2^10, Tamanho total de 2^23. 
- Todas as caches são write-through, por simplificação.
- O simulador contará 10 ciclos adicionais se acontecer um miss na L1i ou L1d, 50 ciclos adicionais se acontecer um miss na L2 e 100 ciclos adicionais se acontecer um miss na L3.

## Sobre o simulador

Nesta versão, o simulador simula também a temporização gasta durante a execução do programa.

**Estágios levados em consideração:**

- `FETCH`: Busca até 4 instruções, possui branch predictor gshare e assume que o hardware é capaz de distinguir que é uma instrução de salto, funciona para Branches e Jumps.  
- `DECODE`: Simula um decode simples para RISC-V.
- `ALLOCATION`: Assume que utilizou-se register renaming para evitar dependências *NOME*.
- `ISSUE`: Controla a ordem de execução das instruções, dependências do tipo Read-after-Write (que não são corrigidas por register renaming) e distribui de acordo com as unidades funcionais disponíveis, também ordena operações de acesso a memória.
- `EXECUTE`: Executa cada instrução e devolve feedback gshare.
- `COMMIT`: determina o final de cada instrução. 

**Temporização de cada parte:**
- `FETCH`: 4 ciclos + ciclos excendente provenientes de miss nas caches
- `DECODE`: 1 ciclo
- `ALLOCATION`: 1 ciclo (2 estágios de alocação acontecem junto com a parte de `ISSUE`)
- `ISSUE`: 3 ciclos
- `EXECUTE`: 2 ciclos + ciclos excendente provenientes de miss nas caches
- `COMMIT`: 1 ciclo.

## Observações:

Foram observadas algumas diferenças quanto ao *desassembly* das instruções em relação ao Spike, como acontece nos exemplos a seguir:
  * AUPIC: (Ambas carregam o valor de 0x2000 no registrador gp)
    * Valor observado no Spike: "auipc   gp, 0x2" 
    * Valor observado no RVsim: "auipc   gp, 0x2000"
  * LUI: (Ambas carregam o valor de 0x11000 no registrador s0)
    * Valor observado no Spike: "lui     s0, 0x11" 
    * Valor observado no RVsim: "lui     s0, 0x11000"

A instrução *JAL - Jump and Link* foi considerada como instrução do tipo *U* ao invés de *UJ* como estão nos slides, uma vez que possui apenas *rd* e *imm*

O *disassembly* das instruções foi feito de acordo com o mostrado nos slides, portanto, diferente do *disassembly* do simulador Spike. Todos os valores foram colocados em decimal.