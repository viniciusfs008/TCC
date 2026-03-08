#ifndef BLOCK_H
#define BLOCK_H

#include "virtio.h"

#define VIRTIO_BLK_F_RO     (5)
#define MAX_BLOCK_DEVICES   (8)
#define NULL                (0)


// Definição da estrutura BlockDevice
typedef struct BlockDevice {
    VirtQ *queue;
    uint64 *dev;
    uint32 idx;
    uint32 ack_used_idx;
    int read_only;
} BlockDevice;

// Inicialização do dispositivo de bloco
int setup_block_device(uint64 ptr);

// Função para realizar uma operação de bloco
void block_op(uint32 dev, uint8 *buffer, uint32 size, uint64 offset, int write);




#endif
