#include "../types.h"
#include "../defs.h"
#include "virtio.h"
#include "../memlayout.h"
#include "../console.h"
#include "gpu.h"
#include "rng.h"
#include "keyboard.h"
#include "block.h"

Device_id map_irqs[8] = {
    NO_DEVICE,
    NO_DEVICE,
    NO_DEVICE,
    NO_DEVICE,
    NO_DEVICE,
    NO_DEVICE,
    NO_DEVICE,
    NO_DEVICE
};

void set_descriptor(VirtQDescriptor *desc, uint64 address, uint32 length, uint16 flags, uint16 next){
    desc->address = address;
    desc->length = length;
    desc->flags = flags;
    desc->next = next;
}


void set_bit(uint64 base, MMIOReg reg, uint32 mask){
    uint32 number = *(uint32*)(base + reg);
    number |= mask;
    *(uint32*)(base + reg) = number;
}

void write_to_reg(uint64 base, MMIOReg reg, uint32 value){
    *(uint32*)(base + reg) = value;
}

uint32 read_from_reg(uint64 base, MMIOReg reg){
    uint32 val = *(uint32*)(base + reg);
    return val;
}


#define CPU_CLOCK   (10000000)
#define TIME_MS     10000





void virtio_probe() {
    uint64 addr;
    uint32 *ptr;
    uint32 magicvalue;
    uint32 deviceid;
    int irq_index;
    printf(CR);
    for(addr = VIRTIO_START; addr <= VIRTIO_END; addr += VIRTIO_STRIDE) {
        printf("Sondando endereço VIRTIO %p ...\n", (uint64*) addr);
        ptr = (uint32*)addr;
        magicvalue = *ptr;
        // No deslocamento 8 temos o ID do disposito 
        deviceid = read_from_reg(addr, DEVICE_ID);
        // Se o dispositov está conectado o deslocamento 0x000 contém o número
        // 0x74_72_69_76
        irq_index = (addr >> 12 & 0xf) - 1;

        if (magicvalue != VIRTIO_MAGIC) {
            printf("Dispositivo virtio não encontrado \n");
        }
        else if (deviceid == 0) {
            printf( INDIAN_RED "\tDispositivo não conectado\n" CR);
        }
        else {
            map_irqs[irq_index] = deviceid;
            switch(deviceid) {
                case ENTROPY_DEVICE_ID:
                    printf(
                        LIGHT_SEA_GREEN 
                        "\tDispositivo entropia (Gerador de Número Aleatório) encontrado\n" 
                        CR
                    );
                    setup_rng(addr);
                    break;
                case GPU_DEVICE_ID:
                    printf(LIGHT_SEA_GREEN "\tDispositivo GPU  encontrado\n" CR);
                    gpu_device* d = setup_gpu_device(addr);
                    init_gpu_device(d);
                    break;
                case INPUT_DEVICE_ID:
                    printf(LIGHT_SEA_GREEN "\tDispositivo de Entrada encontrado\n" CR);
                    setup_input_device(addr);
                    break;
                case BLOCK_DEVICE_ID:
                    printf(LIGHT_SEA_GREEN "\tDispositivo de bloco encontrado\n" CR);
                    int teste = setup_block_device(addr);
                    if (teste == true)
                        printf("DEU CERTO🥹");
                    else
                        printf("DEU ERRADO😵");
                    break;
                default:
                    printf("Dispositivo virtio não identificado foi encontrado\n");
                    break;        
            }
            printf(CR);
        }
    }
    printf("\n\n\n");
}


void handle_virt_int(int irq){
    printf(PINK_RED);
    Device_id device = map_irqs[irq];
    switch(device){
        case INPUT_DEVICE_ID:
            keyboard_int(); //caso seja implementado mouse
            //vai ter que ser implementado uma funcao que
            //controla as interrupcoes do mouse
            break;
        case GPU_DEVICE_ID:
            //printf("GPU\n");
            break;
        case ENTROPY_DEVICE_ID:
            //printf("ENTROPY\n");
            break;
        case BLOCK_DEVICE_ID:
            printf("BLOCO\n");
            break;           
        default:
            printf("DISPOSITIVO VIRTIO DESCONHECIDO\n");
            break;
    }
    printf(CR);
}