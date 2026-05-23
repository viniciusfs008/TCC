#include "block.h"

// Array para armazenar dispositivos de bloco
BlockDevice BLOCK_DEVICES[MAX_BLOCK_DEVICES]; // Supondo que 8 é o número máximo de dispositivos de bloco

// Função para configurar o dispositivo de bloco
int setup_block_device(uint64 addr) {

    // [Driver] Inicialização do Dispositivo
    // 1. Reinicie o dispositivo (escreva 0 no status)
    write_to_reg(addr, STATUS, STATUS_RESET);

    // 2. Configure o bit de status ACKNOWLEDGE
    set_bit(addr, STATUS, STATUS_ACKNOWLEDGE);

    // 3. Configure o bit de status DRIVER
    set_bit(addr, STATUS, STATUS_DRIVER);

    // 4. Leia os bits de feature do dispositivo, escreva um subconjunto de
    // bits de feature entendidos pelo SO e pelo driver no dispositivo.
    uint32 host_features = read_from_reg(addr, HOST_FEATURES);
    uint32 guest_features = host_features & ~(1 << VIRTIO_BLK_F_RO);
    // Determina se a flag VIRTIO_BLK_F_RO está definida nos recursos do host
    int ro = (host_features & (1 << VIRTIO_BLK_F_RO)) != 0;
    write_to_reg(addr, GUEST_FEATURES, guest_features);

    // 5. Configure o bit de status FEATURES_OK
    set_bit(addr, STATUS, STATUS_FEATURES_OK);

    // 6. Releia o status para garantir que FEATURES_OK ainda esteja definido.
    // Caso contrário, não suporta nossas features.
    uint32 status_ok = read_from_reg(addr, STATUS);

    // Se o campo de status não tiver mais FEATURES_OK definido,
    // isso significa que o dispositivo não conseguiu aceitar
    // os recursos que solicitamos. Portanto, isso é
    // considerado um estado "falha".
    if (!(status_ok & STATUS_FEATURES_OK)) {
        printf("falha nos recursos...");
        write_to_reg(addr, STATUS, STATUS_FAILED);
        return false;
    }

    // 7. Execute a configuração específica do dispositivo.
    // Defina o número da fila. Devemos garantir que o
    // tamanho da fila seja válido porque o dispositivo só pode
    // aceitar um certo tamanho.
    uint32 qnmax = read_from_reg(addr, QUEUE_NUM_MAX);
    write_to_reg(addr, QUEUE_NUM, QNUM);
    if (QNUM > qnmax) {
        printf("falha no tamanho da fila...");
        return false;
    }

    // Primeiro, se o array de dispositivos de bloco estiver vazio, crie-o!
    // Adicionamos 4095 para arredondar para cima e depois fazemos uma divisão
    // inteira para truncar o decimal. Não adicionamos 4096,
    // porque se for exatamente 4096 bytes, obteríamos duas
    // páginas, não uma.
    uint32 num_pages = (sizeof(VirtQ) + PAGE_SIZE - 1) / PAGE_SIZE;
    // Alocamos uma página para cada dispositivo. Esta será a
    // descrição onde podemos comunicar com o dispositivo de bloco.
    // Ainda usaremos um registro MMIO (em particular, QueueNotify)
    // para realmente informar ao dispositivo que colocamos algo na memória.
    // Também precisamos ter cuidado com a ordenação de memória.
    // Não queremos emitir um aviso antes que todas as escritas
    // de memória tenham terminado. Vamos examinar isso mais tarde,
    // mas precisamos do que é chamado de "cercas" ou barreiras de memória.
    write_to_reg(addr, QUEUE_SEL, 0);
    // O alinhamento é muito importante aqui. Este é o alinhamento
    // de endereço de memória entre os anéis disponíveis e usados.
    // Se isso estiver errado, nós e o dispositivo nos referiremos
    // a endereços de memória diferentes e, portanto, obteremos os dados errados no anel usado.
    VirtQ *queue_ptr = (VirtQ*) alloc(num_pages);
    uint32 queue_pfn = ((uint64)queue_ptr)/PAGE_SIZE; 
    write_to_reg(addr, GUEST_PAGE_SIZE, PAGE_SIZE);
    // QueuePFN é um número de página física, no entanto,
    // parece que para o QEMU temos que escrever o endereço de memória completo.
    // Este é um endereço de memória física onde nós
    // (o SO) e o dispositivo de bloco têm em comum para
    // fazer e receber solicitações.
    write_to_reg(addr, QUEUE_PFN,  queue_pfn);

    // Estrutura para armazenar os dados do dispositivo de bloco
    BlockDevice bd;
    bd.queue = queue_ptr;
    bd.dev = addr;
    bd.idx = 0;
    bd.ack_used_idx = 0;
    bd.read_only = ro;

    // Armazenar os dados do dispositivo de bloco na lista de dispositivos de bloco
    BLOCK_DEVICES[bd.idx] = bd;

    // Define o bit DRIVER_OK no status. O dispositivo está agora "ativo"
    set_bit(addr, STATUS, STATUS_DRIVER_OK);

    return true;
}

// Função para realizar operações de leitura/escrita no dispositivo de bloco
void block_op(uint32 dev, void *buffer, uint32 size, uint64 offset, int write) {
    if (dev == 0 || dev > MAX_BLOCK_DEVICES || BLOCK_DEVICES[dev - 1].queue == NULL) {
        printf("Dispositivo inválido!\n");
        return;
    }

    BlockDevice *bdev = &BLOCK_DEVICES[dev - 1];

    // Verifica se o dispositivo é somente leitura e estamos tentando escrever
    if (bdev->read_only && write) {
        printf("Tentando escrever em um dispositivo somente leitura!\n");
        return;
    }

    // Calcula o setor baseado no offset
    uint64 sector = offset / 512;

    // Aloca a estrutura de requisição (1 página é o suficiente)
    Request *blk_request = (Request *) alloc(1);

    // Configura a requisição
    blk_request->header.sector = sector;
    blk_request->header.blktype = write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    blk_request->header.reserved = 0;
    blk_request->status.status = 111; // Marcador para indicar pendência

    // Adiciona os descritores à fila do dispositivo
    uint32 head = bdev->idx;
    uint32 next_idx;

    // 1. Descritor do cabeçalho
    next_idx = (bdev->idx + 1) % QNUM;
    set_descriptor(&bdev->queue->desc[bdev->idx], (uint64) &blk_request->header, sizeof(blk_request->header),
                   VIRTQ_DESC_F_NEXT, next_idx);
    bdev->idx = next_idx;

    // 2. Descritor dos dados
    next_idx = (bdev->idx + 1) % QNUM;
    set_descriptor(&bdev->queue->desc[bdev->idx], (uint64) buffer, size,
                   VIRTQ_DESC_F_NEXT | (write ? 0 : VIRTQ_DESC_F_WRITE), next_idx);
    bdev->idx = next_idx;

    // 3. Descritor do status
    next_idx = (bdev->idx + 1) % QNUM;
    set_descriptor(&bdev->queue->desc[bdev->idx], (uint64) &blk_request->status, sizeof(blk_request->status),
                   VIRTQ_DESC_F_WRITE, 0); // Fim da cadeia
    bdev->idx = next_idx;

    // Adiciona o cabeçalho (head) ao anel disponível (Available Ring)
    bdev->queue->av.ring[bdev->queue->av.index % QNUM] = head;
    bdev->queue->av.index++;

    // Notifica o dispositivo sobre a requisição
    write_to_reg(bdev->dev, QUEUE_NOTIFY, 0);

    // Espera o dispositivo processar comparando o índice do anel 'used'
    // O dispositivo incrementa bdev->queue->used.index quando completa uma requisição
    while (bdev->ack_used_idx == bdev->queue->used.index);
    
    // Atualiza nosso índice de reconhecimento
    bdev->ack_used_idx++;

    // Libera a memória da requisição
    dealloc(blk_request);
}