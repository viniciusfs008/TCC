#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "riscv.h"
#include "string.h"
#include "console.h"
#include "proc.h"
#include "param.h"


/* O espaço de endereçamento do próprio kernel é virtualizado 
   para efeitos de proteção do código/dados. O espaço é mapeado em cinco regiões:
    - texto, incluindo rodata:  (r-x)
    - dados + pilha + heap:     (rw-)      
    - UART (a região mapeada para o dispositivo UART)
    - CLINT (interrupções locais, como o temporizador mtime)
    - PLIC (região de memória do controlador de interrupçõe globais)
   text_end é o primeiro byte depois do fim do texto, servindo como divisor entre as 2 
   primeiras regiões
*/

extern char text_end[]; 
extern char stack_start[]; 
extern char stack_end[]; 



long total_pages;
long alloc_start;
uint64 *kernel_pagetable;
uint64 *kernel_heap; //Sem uso. Remover
TrapFrame trap_frame[NCPU]; // Um TF por CPU

#define HEAP_START  stack_end
#define HEAP_END    (uint8*)(KERNEL_START + MEMORY_LENGTH) 
#define TEXT_END    (uint8*)text_end
#define HEAP_SIZE   ((uint64) HEAP_END - (uint64) HEAP_START)
//Para controle da lista de páginas livres
#define FREEPG 0x1 // A página está livre
#define LASTPG 0x2 // A página é a última do bloco alocado

int
free_page(uint8 desc) {
    // A página livre tem o bit 0 = 1
    if (desc & FREEPG) 
        return 1;
    else
        return 0;
}

int
last_page(uint8 desc) {
    // A última página do bloco alocado tem o bit 1 = 1
    if (desc & LASTPG)
        return 1;    
    else
        return 0;
}

void
set_free_page_flag(uint8 *desc, uint8 freedom) {
    if (freedom)
        *desc = *desc | 0x01;
    else {
        *desc = *desc & (0xFF << 1); // 0xFE;
    }
}

void
set_last_page_flag(uint8 *desc, uint8 last) {
    if (last) {
        *desc = *desc | (1 << 1); // bit 1 = 1
    }
    else {
       *desc = *desc & ((0xFF << 2) | 1); // (0xFD)  bit 1 = 0
    }
}

// Converte o endereço de memória addr no seu endereço de página com
// arredondamento para baixo. 
uint64
page_round_down(uint64 addr) {
    return (addr & ~(PAGE_SIZE - 1));
}

// Converte o endereço de memória addr no seu endereço de página com
// arredondamento para cima
uint64 
page_round_up(uint64 addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

// Retorna o índice (L2, L1 ou L0) do endereço virtual de acordo com o nível
// Formato do VA: |L2 | L1 | L0| offset|, 
// onde L2 = L1 = L0 = 9 bits e offset = 12 bits.
int 
page_idx(uint64 va, int level) {
    return (va >> (PAGE_SHIFT + level * 9)) & PAGE_IDX_MASK;
}

// Retorna o endereço físico (end. de pág) armazenado em uma PTE
// (entrada da tabela de páginas)
// Uma PTE ocupa 64 bits:
// | reservado (10 bits) | PPN (44 bits) | flags (10 bits) | 
uint64
pte2pa(pte_t pte) {
    return (pte >> 10) << PAGE_SHIFT;
}

uint64
pa2pte(uint64 pa) {
    return ((pa >> PAGE_SHIFT) << 10);
}

// Aloca pages páginas da região do HEAP do kernel. 
// Retorna um ponteiro para o primeiro byte da página alocada
void *
alloc(int pages) {
    uint8 * ptr = (uint8 *) HEAP_START;
    int i;
    int count = 0;
    uint8 * desc = 0; // O descritor da primeira página do bloco alocado
    
    if (pages == 0)
        return 0;
    for(i = 0; i < total_pages; i++) {
        ptr = (uint8 *) HEAP_START + i;
        if(free_page(*ptr)) {
            if (count == 0) 
                desc = ptr;
            count++;
        }
        else {
            count = 0;
            desc = 0;
        }
        if (count == pages) break;
    }
    if (count < pages)
        desc = 0;
    if (desc != 0) {
        for (i = 0; i < pages; i++) {
            set_free_page_flag(desc+i, !FREEPG);
            set_last_page_flag(desc+i, !LASTPG);
        }
        set_last_page_flag(desc + (i-1), LASTPG);
        return (uint8 *)(alloc_start + ((uint64)desc - (uint64)HEAP_START) * PAGE_SIZE); 
    }
    return 0;
}

void dealloc(void *address){
    uint32 desc_number = (((uint64) address) - ((uint64) alloc_start))/PAGE_SIZE;
    uint8 *desc = (uint8*) (HEAP_START + desc_number);
    if (free_page(*desc)) return; //se a pagina está livre, retorna
    while(!last_page(*desc)){
        set_free_page_flag(desc, FREEPG);
        desc++;
    }
    set_free_page_flag(desc, FREEPG);
    set_last_page_flag(desc, !LASTPG);
}



// Buscar o PTE (de L0) correspondente ao VA (uma consulta), ou
// alocar uma página para a tabela de páginas, caso não exista 
// (criação da entrada)
pte_t * 
walk(pte_t *pagetable, uint64 va, int alloc_pg) {
    int level;
    // pagetable aponta sucessivas páginas de tabela de página: L2, L1 e depois L0.
    pte_t *pte;
    // va válidos vão de 0 a MAXVA - 1
    if (va >= MAXVA)
        panic("walk - espaço de endereçamento virtual maior que o permitido");
    for(level = 2; level > 0; level--) {
        
        pte = &pagetable[page_idx(va, level)];
        // kprintf("Level %d.  Endereco PTE %p. Índice da página:%d\n", level, pte, page_idx(va, level));
        if (*pte & PTE_V) {
            pagetable = (uint64 *) pte2pa(*pte);
        }
        else {
            if(!alloc_pg || (pagetable = (uint64 *) alloc(1)) == 0) {
                return 0;
            }
            // !!! zerar pagina alocada
            memset(pagetable, 0, PAGE_SIZE);
            *pte = pa2pte((uint64) pagetable) | PTE_V; 
        }
    }
    //pagetable aponta página de tabela de páginas L0
    return &pagetable[page_idx(va, 0)];
}

// Mapeia  páginas sequenciais de endereço virutal para páginas de endereço 
// físico. O endereço virtual (va) é usado para recuperar, via travessia das
//  tabelas de páginas, o PTE_L0 . O PTE_L0 recebe o endereço físico (pa) 
//  Estes 2 passos são repeditos para sz/PAGE_SIZE páginas 
int 
mappages(uint64 *pagetable, uint64 va, uint64 sz, uint64 pa, int perm) {
    uint64 addr, end;
    pte_t *pte;

    // sz é em bytes e  pode não ser multiplo de PAGE_SIZE
    addr = page_round_down(va); // Obtém end. da página
    // end = page_round_up(va+sz);
    end = page_round_up(addr+sz);
    while( addr < end) {
        if( (pte = walk(pagetable, va, 1)) == 0) {
            return -1;
        }
        if(*pte & PTE_V) { // A página já está sendo utilizada
            panic("mapppages: remap");
        }
        // Atribui o endereço físico à entrada PTE (L0) do VA
        // Configura permissões (rwx) e bit de validade
        *pte = pa2pte(pa) | perm | PTE_V;
        va += PAGE_SIZE;
        pa += PAGE_SIZE;
        addr += PAGE_SIZE;
    } 
    return 0; // Tudo OK
}

void
pages_init() {
    uint8 * ptr = (uint8 *) HEAP_START;
    int i;
    int reserved_pages; //páginas ocupadas pela lista de descritores

    total_pages = HEAP_SIZE / PAGE_SIZE;
    reserved_pages = total_pages / PAGE_SIZE;
    if(total_pages % PAGE_SIZE) {
        reserved_pages++;
    }
    total_pages -= reserved_pages;
    for (i = 0; i < total_pages; i++) {
        *ptr++ = FREEPG; 
    }
    // printf("HEAP START <pages_init>:%p\n", HEAP_START);
    // printf("Paginas da lista de desc: %d\n", reserved_pages);
    alloc_start = page_round_up((uint64)HEAP_START + reserved_pages * PAGE_SIZE); 
}

void
memory_init() {
    char* s = "É a VIda...";
    //printf("String:%s\n", s);
    pages_init();
}

void kvmmap(uint64 va, uint64 pa, uint64 sz, int perm) {
    
    if(mappages(kernel_pagetable, va, sz, pa, perm) != 0) {
        panic("Erro de mapeamento");
    }
}

void
get_mstatus_mpie(){
    // Verifica o estado de mstatus.MPIE
    uint64 reg = r_mstatus();
    reg = reg & (1L << 7);
    if(reg) {
        printf(TOMATO "MPIE habilitado\n");
    }
    else
    {
        printf(TOMATO "MPIE Desabilitado\n");
    }
}

//Mapeamento de identidade das regiões de memória acessadas pelo kernel
void 
kvminit(){
    // A área de heap para uso exclusivo dos objetos do kernel é de 64 *4096 = 256KiB
    kernel_heap = (uint64 *) alloc(64); // Por enquanto não há uso para isso
    // página raiz (L2) da tabela de páginas
    if((kernel_pagetable = (uint64*) alloc(1)) == 0) {
        panic("Erro ao alocar página L2 para tabela de página");
    }
    memset(kernel_pagetable, 0, PAGE_SIZE);
    //printf("<memory.c> Endereço da tab de página do kernel:%p\n", kernel_pagetable);
    // A região do código (text + rodata) é de leitura e execução
    kvmmap(KERNEL_START, KERNEL_START, (uint64) text_end - KERNEL_START, PTE_R | PTE_X);
    // A região de dados (dados + pilha + heap) é de leitura e escrita        
    kvmmap((uint64) text_end, (uint64) text_end, (KERNEL_START + MEMORY_LENGTH - (uint64) text_end), PTE_R|PTE_W);
    // Precisamos escrever na região de memória onde o UART está maepado
    kvmmap(UART, UART, PAGE_SIZE, PTE_R | PTE_W);

    //! No momento estes mapeamentos são irrelevantes porque a inicialização de
    //! dispositivos e tratamento de interrupções se dá no modo-M onde o MMU 
    //! está desativado
    // OS CSR ocupam 0x10000 de memória
    kvmmap(CLINT, CLINT, 0x10000, PTE_R | PTE_W);
    kvmmap(PLIC, PLIC, 0x400000, PTE_R | PTE_W);
    // os 8 dispositivos que o nosso SO reconhece estão separados por endereços 
    // de 0x1000 (4096) começando em 0x10001000 
    kvmmap(0x10001000 , 0x10001000 , 0x8000, PTE_R | PTE_W);

    //Configura o trap_frame para a CPU 0
    w_mscratch((uint64)&trap_frame[0]); 
    trap_frame[0].hartid = 0;
    // w_sscratch(r_mscratch()); //!!! Não é necessário por enquanto
    // o kernel e cada processo tem sua tabela de páginas. Quando o sistema 
    // chavera de um processo de volta para o kernel é preciso restaurar satp
    trap_frame[0].satp = MAKE_SATP(kernel_pagetable); 
    trap_frame[0].trap_stack = alloc(1); // Cria pilha para atender o trap
    memset(trap_frame[0].trap_stack, 0, 4096); 
    // Move para o final da página, porque a pilha cresce para trás
    trap_frame[0].trap_stack += 4096; 

    // Ativar modo Sv39 e informar o endereço da tabela raiz (L2)
    w_satp(MAKE_SATP(kernel_pagetable));
    
    get_mstatus_mpie();
    //invalida as entradas da TLB 
    sfence_vma(); 
}

// Cria uma tabela de páginas do usuário
uint64 *
uvmcreate()
{
  uint64 *pagetable;
  pagetable = (uint64 *) alloc(1);
  if (pagetable == 0)
    panic("uvmcreate: Não temos memória");
  memset(pagetable, 0, PAGE_SIZE);
  return pagetable;
}

void
memory_test() {
    char* p =  alloc(1);
    *p = 'O';
    *(p+1) = 'i';
    *(p+2) = '\n';
    char *p2 = alloc(2);
    printf("End. de p:\t%p\n", p);
    printf("Conteúdo de p:\t%s\n", p);
    printf("End. de p2:\t%p\n", p2);
    uint8 *desc;
    desc = (uint8*) HEAP_START;
    printf("DESCRITOR PÁG.\tSTATUS (0-OCUPADA  1-LIVRE  2-OCUPADA/ÚLTIMA)\n");
    for(int i = 0; i < 5; i ++) { //Os 5 primeiros descritores
        printf("%d\t  \t%d\t%p\n", i, *(desc+i), (i*PAGE_SIZE)+alloc_start); 
    }
    printf("Desalocando...\n");
    dealloc(p);
    dealloc(p2);

    printf("TEXT_END:\t%p\n", TEXT_END);
    printf("Início pilha:\t%p\n", stack_start);
    printf("Fim da pilha:\t%p\n", stack_end);
    printf("HEAP_START:\t%p\n", HEAP_START);
    printf("HEAP_END:\t%p\n", HEAP_END);
    printf("HEAP_SIZE:\t%p\n", (uint64*) HEAP_SIZE);
    printf("alloc_start:\t%p\n", (uint64*) alloc_start);
    printf(LIGHT_SEA_GREEN);
    printf(" --- PAGINAÇÃO (endereços virtuais e físicos dos segmentos) ---\n");
    pte_t *pte = walk(kernel_pagetable, KERNEL_START, 0);
    printf("TEXT_START\t\tVA:%p\tPA:%p\n", KERNEL_START, (uint64*)pte2pa(*pte));
    pte = walk(kernel_pagetable, (uint64) text_end, 0);
    printf("DATA_START\t\tVA:%p\tPA:%p\n", text_end, (uint64*)pte2pa(*pte));
    pte = walk(kernel_pagetable, (uint64) stack_start, 0);
    printf("KERNEL STACK_START\tVA:%p\tPA:%p\n", stack_start, (uint64*)pte2pa(*pte));
    pte = walk(kernel_pagetable, (uint64) HEAP_START, 0);
    printf("HEAP_START\t\tVA:%p\tPA:%p\n", HEAP_START, (uint64*)pte2pa(*pte));
    printf(CR);
}