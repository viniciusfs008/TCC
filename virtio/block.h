#ifndef BLOCK_H
#define BLOCK_H

#include "virtio.h"

#define VIRTIO_BLK_F_RO     (5)
#define MAX_BLOCK_DEVICES   (8)
#define NULL                (0)

// Tipos de requisição VirtIO Block
#define VIRTIO_BLK_T_IN           0
#define VIRTIO_BLK_T_OUT          1
#define VIRTIO_BLK_T_FLUSH        4

// Estrutura do cabeçalho de uma requisição de bloco (conforme especificação VirtIO)
typedef struct virtio_blk_req_header {
    uint32 blktype;
    uint32 reserved;
    uint64 sector;
} virtio_blk_req_header;

// Estrutura para os dados da requisição (simplificada para o driver)
typedef struct virtio_blk_req_data {
    void *data;
} virtio_blk_req_data;

// Estrutura para o status da requisição
typedef struct virtio_blk_req_status {
    uint8 status;
} virtio_blk_req_status;

// Estrutura consolidada para facilitar a alocação de uma requisição completa
typedef struct Request {
    virtio_blk_req_header header;
    virtio_blk_req_status status;
    // O buffer de dados (data) é passado separadamente para evitar cópias desnecessárias
} Request;

// Definição da estrutura BlockDevice
typedef struct BlockDevice {
    VirtQ *queue;
    uint64 dev;
    uint32 idx;
    uint32 ack_used_idx;
    int read_only;
} BlockDevice;

// Inicialização do dispositivo de bloco
int setup_block_device(uint64 ptr);

// Função para realizar uma operação de bloco
void block_op(uint32 dev, void *buffer, uint32 size, uint64 offset, int write);




#endif
