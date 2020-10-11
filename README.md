## RVsim

Simulador do processador RISC-V para disciplina de Arquitetura de Computadores 2 na Unicamp.

O comportamento das instruções desse simulador foram montadas de acordo com o descrito no [ISA Specification](https://riscv.org/technical/specifications/). Esse simulador não implementa o disassembly de instruções em pseudo-instruções, e para conferência do corretude, seu comportamento foi avaliado comparando o log de execução de cada instrução com o log do Simulador [Spike](https://github.com/riscv/riscv-isa-sim). Contudo, foram observadas algumas diferenças quanto ao desassembly das instruções, como acontece nos exemplos a seguir:
  * AUPIC: (Ambas carregam o valor de 0x2000 no registrador gp)
    * Valor observado no Spike: "auipc   gp, 0x2" 
    * Valor observado no RVsim: "auipc   gp, 0x2000"
  * LUI: (Ambas carregam o valor de 0x11000 no registrador s0)
    * Valor observado no Spike: "lui     s0, 0x11" 
    * Valor observado no RVsim: "lui     s0, 0x11000"
    
Nenhuma biblioteca adicional foi utilizada, nem mesmo para o *parsing* do arquivo elf. A memória alocada pro programa é de 256MiB, nenhum tipo de *base address* foi utilizado para a memória.
    
## Compilando e executando o programa

Para compilar:
`g++ rvsim/*.cpp libs/*.cpp -I include/ -o rvsim_exe`

Para Executar:
`rvsim_exe <elf_file>`
