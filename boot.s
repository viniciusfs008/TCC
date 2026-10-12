
.section .data 
.global STACK_START  # Torna visível o início da pilha para o código C
STACK_START: .dword stack_start 
.global STACK_END # Torna visível o fim da pilha para o código C
STACK_END: .dword stack_end
.section .text

.global _start
.equ STACK_SIZE, 0x4000           # tamanho da pilha por CPU (16 KiB)

_start:
    # setup stacks per hart
    csrr t0, mhartid            # obtém o ID do hart atual
    li a0, STACK_SIZE           # carrega o tamanho da pilha (16 KiB) para a0
    mul t1, t0, a0
    la sp, stack_end                
    sub sp, sp, t1              # define o endereço da pilha para o hart atual

    # Os hards com id != de 0 saltam para park
    csrr a0, mhartid                
    bnez a0, park                                          
    # !!! Deveria usar o call (no lugar de j) como no Xv6 porque ele permite o código em C 
    # !!!voltar para cá (p/ o endereço park)
    # Para a diferença entre as pseudo-instruções j e call consultar:
    # https://www.imperialviolet.org/2016/12/31/riscv.html
    call    entry                  # salta para o código C

park:
    wfi
    j park
# A pilha precisa ser colocada na seção bss. Se omitirmos a seção ela ficará na seção atual 
# (ex.: .text)
# O linker entende que a seção .text é r-x, mas a pilha precisar ser rw-. Ou seje, nos 
# cabeçalhos de programa do ELF .text terá estes flags indicando que o segmento .text na
# memória se comporta de forma diferente do segmento .bss. 
# Mas como no caso o programa é o próprio SO, estas diferenças não são respeitadas:qualquer 
# região da memória a partir de 0x80000000 podem ser lidas, executadas e escritas pelo kernel.
# Estas diferenças possivelmente servem para o kernel configurar o comportamento do segmento 
# para os programas
# .section .bss    
# Faz alinhamento de memória em potência de 2 (2**4 = 16 bytes), 
# porque a arquitetura RISC-V exige tal alinhamento para a pilha 
# .align 4
# stack_start:                        
# .skip STACK_SIZE * 4            
# stack_end:
