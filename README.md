## RVsim

Simulador do processador RISC-V para disciplina de Arquitetura de Computadores 2 na Unicamp.

O comportamento das instruções desse simulador foram montadas de acordo com o descrito no [ISA Specification](https://riscv.org/technical/specifications/). Esse simulador não implementa o disassembly de instruções em pseudo-instruções, e para conferência do corretude, seu comportamento foi avaliado comparando o log de execução de cada instrução com o log do Simulador [Spike](https://github.com/riscv/riscv-isa-sim). 

Nenhuma biblioteca adicional foi utilizada, nem mesmo para o *parsing* do arquivo elf. A memória alocada pro programa é de 256MiB, nenhum tipo de *base address* foi utilizado para a memória.
    
## Compilando e executando o programa

Para compilar:
`g++ libs/*.cpp -I include/ -o rvsim`

Para Executar:
`./rvsim <elf_program>`

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