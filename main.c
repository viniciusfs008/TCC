#include "types.h"
#include "defs.h"
#include "console.h"
#include "riscv.h"
#include "memlayout.h"
#include "game/game.h"
#include "virtio/gpu.h"
#include "libs/math.h"
#include "virtio/rng.h"
#include "filesystems/minix.h"
#include "shell.h"


extern void mvector(void); // Rotina q salva os registradores em assembly
extern void msg_syscall(int);

void desenha_basico(){
        stroke_color(COLOR_RGB(0,0,0));
        stroke_weigth(5);
        draw_line(336,46, 78, 382);
        draw_line(740,390, 834,530);
        draw_line(740,530, 834,390);

        fill_color(TRANSPARENT);
        draw_triangle(319,179, 290,429, 599,441);
        
        fill_color(COLOR_HEX(0x333333));
        stroke_color(COLOR_HEX(0x00ff00));
        draw_triangle(787,239, 681,334, 801,334);

        stroke_weigth(0);
        fill_color(COLOR_HEX(0x223399));
        draw_rect(59,466,357,100);


        stroke_weigth(8);
        stroke_color(COLOR_HEX(0x000000));
        fill_color(COLOR_HEX(0x00ff00));
        draw_circle(611,580, 70);

        stroke_weigth(0);
        fill_color(COLOR_HEX(0xff0000));
        draw_rect(581,121, 100,100);
        draw_triangle(560,172,700,172,631,100);
        draw_triangle(560,172,700,172,631,241);

        fill_color(COLOR_HEX(0x223399));
        draw_string(
            "ABCDEFGHIJKLMNOPQRSTUVXWYZ\n"
            "abcdefghijklmnopqrstuvxwyz\n"
            "0123456789!#$%'()+,-.;=@\"*\n"
            "/\\:?[]{}",
            20, 664, 4, 0, 0);
}


void exemplo_linha(){
    stroke_color(BLACK);
    stroke_weigth(3);
    draw_line(20,20, 200,300);

    stroke_weigth(8);
    draw_line(120,20, 300, 300);

    stroke_color(COLOR_HEX(0x0000ff));
    draw_line(220,20, 400, 300);
}
void exemplo_triangulo(){
    stroke_weigth(0);
    stroke_color(BLACK);
    fill_color(0x00ffff);
    draw_triangle(20,20, 80, 280, 260, 110);
    // draw_triangle(20,20, 70, 220, 140,110);

    // stroke_weigth(3);
    // draw_triangle(220, 20, 270, 220, 340,110);

    // stroke_weigth(7);
    // fill_color(COLOR_HEX(0xff0000));
    // stroke_color(COLOR_HEX(0x0000ff));
    // draw_triangle(420, 20, 470, 220, 540,110);
}
void exemplo_retangulo(){
    stroke_weigth(0);
    stroke_color(BLACK);
    fill_color(0x00ffff);
    draw_rect(20,20, 100,200);

    stroke_weigth(3);
    draw_rect(160, 20, 100,200);

    stroke_weigth(7);
    fill_color(COLOR_HEX(0xff0000));
    stroke_color(COLOR_HEX(0x0000ff));
    draw_rect(300, 20, 100, 200);
}
void exemplo_circulo(){
    stroke_weigth(0);
    stroke_color(BLACK);
    fill_color(0x00ffff);
    draw_circle(150,150, 100);

    stroke_weigth(3);
    draw_circle(400, 150, 100);

    stroke_weigth(7);
    fill_color(COLOR_HEX(0xff0000));
    stroke_color(COLOR_HEX(0x0000ff));
    draw_circle(650, 150, 100);
}
// No final de entry() o sistema salta para cá no modo-S
void main() {
    // Teste do Filesystem Minix 3
    MinixSuperBlock sb;
    if (minix_read_superblock(1, &sb)) {
        printf("[Minix3] Superbloco OK! Magic: 0x%x\n", sb.magic);
        printf("[Minix3] Total de inodes: %d\n", sb.ninodes);
        printf("[Minix3] Tamanho do bloco: %d\n", sb.block_size);

        // Inicia o shell iterativo do usuario
        shell_run(1, &sb);
    } else {
        printf("[Minix3] Falha ao ler superbloco.\n");
    }
    // \uXXXX e \UXXXXXXXX são chamados universal-character-name
    // init_game();

    // bg_color(WHITE);
    // while(1){
    //     clear();
    //     exemplo_triangulo();

    //     gpu_update();

    // }

    for(;;);
}

// boot.s salta para cá no modo máquina (M-Mode)
void 
entry() {
    // gerenciamento do heap
    memory_init();
    printf("\n\n\n");
    //O fluxo de execução retorna para a função main em main.c
    w_mepc((uint64) main);
    // Configura o vetor de interrupções.
    // Quando há uma interrupção/exceção o fluxo é desviado para
    // o endereço presente no registrador mtvec
    uint64 x = (uint64) mvector;
    w_mtvec(x);
    // ativa memória virtual 
    kvminit();
    
    x = r_mstatus();
    // Zera apenas o campo MSTATUS.MPP (2 bits)
    x = x & ~MSTATUS_MPP_MASK;
    // Atribui o privilégio S-Mode ao campo MSTATUS.MPP
    x = x | MSTATUS_MPP_S;
    // Habilita interrupções
    // configura mstatus.MPIE == 1 para que as interrupções sejam habilitadas 
    // quando o sistema voltar para o modo supervisor (mstatus.MIE <- mstatus.MPIE)
    x = x | (1 << 7);
    x = x | (1 << 13); //habilita float operations
    w_mstatus(x);

    // Habilita interrupção do temporizador
    x = r_mie();
    x = x | (1 << 7); // MTIE
    // x = x | (1 << 5); // STIE (inútil: a interrupção temporizador modo-S não ocorre)
    x = x | (1 << 11); // MEIE (Interrupções externas)
    w_mie(x);
    // 1 interrupção por segundo
    //uint64 *mtimecmp = (uint64 *) CLINT_MTIMECMP(0);
    uint64 *mtime = (uint64 *) CLINT_MTIME;
    //printf("Valor de MTIME: %d\n", *mtime);
    
    uartinit();
    printf("UART Inicializado\n");
    plic_init();
    printf("PLIC inicializado\n");

    //process_init();
    //*mtimecmp = *mtime + 10000000;
    //printf("Valor de CLINT_MTIMECMP: %p\n", CLINT_MTIMECMP(0));
    printf("Descoberta de dispositivos...\n");
    virtio_probe();

    // bg_color(WHITE);
    // while(1){
    //     clear();
    //     exemplo_triangulo();
    //     gpu_update();
    // }
    asm volatile("mret"); // retorna para a função main no modo-S
}

void
test() {
    // printf("Fim bss:%p\n", (uint64*)bss_end);
    // int per = perimetro(3, 3, 4, 5);
    // printf("Perímetro triângulo:%d\n", per);
    // per = perimetro(4, 10, 10, 10, 10);
    // printf("Perímetro retângulo:%d\n", per);
    // printf("Perímetro Pentágono:%d\n", perimetro(5, 6, 3, 3, 4, 4));
    // printf("Ponteiro:%p\n", (uint64*)4096);
}