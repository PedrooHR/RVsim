## RVsim

Simulador do processador RISC-V para disciplina de Arquitetura de Computadores 2 na Unicamp.

O comportamento das instruções desse simulador foram montadas de acordo com o descrito no [ISA Specification](https://riscv.org/technical/specifications/). Esse simulador não implementa o disassembly de instruções em pseudo-instruções, e para conferência do corretude, seu comportamento foi avaliado comparando o log de execução de cada instrução com o log do Simulador [Spike](https://github.com/riscv/riscv-isa-sim). 

Nenhuma biblioteca adicional foi utilizada, nem mesmo para o *parsing* do arquivo elf. A memória alocada pro programa é de 256MiB, nenhum tipo de *base address* foi utilizado para a memória.
    
## Compilando e executando o programa

Para compilar:
`make $DINERO_INSTALL_DIR=<dinero-install-dir>`

Para Executar:
`./rvsim <elf_program>`

## Sobre o simulador

Nesta versão, o simulador simula também a temporização gasta durante a execução do programa. Bugs nas instruções de LBU e LHU foram consertados.

Apesar de o simulador não simular todos os estágios de pipeline, os seguinte formato é representado (lembrando que esses estágios podem ainda conter sub-estágios), lembrando que o processador é super-escalar, testado para até 4 instruções simultâneas:

***TODO***: Colocar diagrama dos estágios de pipeline.

**Estágios levados em consideração:**

- `FETCH`: Busca até 4 instruções, possui branch predictor gshare e assume que o hardware é capaz de distinguir que é uma instrução de salto, funciona para Branches e Jumps.  
- `DECODE`: Simula um decode simples para RISC-V.
- `ALLOCATION`: Assume que utilizou-se register renaming para evitar dependências *NOME*.
- `ISSUE`: Controla a ordem de execução das instruções, dependências do tipo Read-after-Write (que não são corrigidas por register renaming) e distribui de acordo com as unidades funcionais disponíveis
- `EXECUTE`: Executa cada instrução (não executa instruções provenientes de saltos errados).
- `COMMIT`: determina o final de cada instrução. 

**Temporização de cada estágio:**

- `FETCH`: Base de 4 ciclos para fazer o fetch de 4 instruções*.
- `DECODE`: 1 ciclo para execução do decode simples.
- `ALLOCATION`: 1 ciclo, assumindo que é o tempo necessário para realizar o register renaming.
- `ISSUE`: 1 ciclo, assumindo que é o tempo necessário para alocação dos recursos e ordenação das instruções.
- `EXECUTE`: 1 ciclo para cada unidade funcional utilizada, mais 1 ciclo se tiver acesso a memória*.
- `COMMIT`: 1 ciclo para reoordenar as instruções para terminá-las na ordem correta.

\* 10 ciclos adicionais se acontecer um miss na L1i ou L1d, 50 ciclos adicionais se acontecer um miss na L2 e 100 ciclos adicionais se acontecer um miss na L3.

**Configuração do simulador:**
***TODO***

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