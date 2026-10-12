/* um script mínimo para o compilar o kernel 'Hello World' 
A seção de texto começa no endereço 0x80000000 de acordo com o que espera 
a márquina virtual RISC-V `virt` do Qemu
*/

OUTPUT_ARCH( "riscv" )
ENTRY( _start )

SECTIONS
{
    . = 0x80000000;
    /* seção de código */
    .text :
    {
        *(.text)
    }
    /* 
    Por padrão o Linker cria alguns segmentos (cabeçalhos).
    Com o alinhamento de 4096 bytes o linker criar um novo segmento (program header) no arquivo ELF 
    Para listar os cabeçalhos do programa:
     riscv64-unknown-elf-objdump -p kernel
    O arquivo fica então com 2 segmentos (sem o alinhamento fica com 1 apenas):
    1. (texto) r-x
    2. (dados inicializados e não inicializados) rw-
    */
    . = ALIGN(0x1000); 
    PROVIDE(text_end = .); /* Não seria antes de align? Não! */
    /* Seção de dados  */
    .data :
    {
        *(.data)
    }

    .bss : 
    {
        *(.bss)
        *(.sbss*) /*para quê serve este? */
        PROVIDE(bss_end = .);
    }
    /* A pilha é alinhada na memória em múltiplos de 16 (bytes) */
    . = ALIGN(16); 
    PROVIDE(stack_start = .);
    /* Deve ser STACK_SIZE (boot.s) × número de CPUs */
    PROVIDE(stack_end = stack_start + 0x10000); /* 0x4000 * 4: 16 KiB por CPU */
    /* PROVIDE(heap_start = stack_end); */
}
