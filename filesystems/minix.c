#include "minix.h"
#include "../virtio/block.h"
#include "../defs.h"

// ============================
// minix_read_superblock
//
// Lê o superbloco do dispositivo `dev` e valida o magic number.
// O superbloco fica no offset 1024 no disco (segundo bloco de 1024 bytes).
//
// Retorna 1 se o magic for válido, 0 caso contrário.
// ============================
int minix_read_superblock(uint32 dev, MinixSuperBlock *sb) {
    block_op(dev, (uint8 *) sb, MINIX_BLOCK_SIZE, MINIX_BLOCK_SIZE, 0);
    if (sb->magic != MINIX3_MAGIC) {
        printf("minix: magic inválido (lido: 0x%x, esperado: 0x%x)\n",
               sb->magic, MINIX3_MAGIC);
        return 0;
    }
    return 1;
}

// ============================
// minix_read_inode
//
// Lê o inode de número `inode_num` do disco.
//
// Algoritmo:
//   - O número do bloco que contém o inode é:
//       bloco = (2 + imap_blocks + zmap_blocks) + (inode_num - 1) / INODES_PER_BLOCK
//   - O índice dentro do bloco é:
//       idx = (inode_num - 1) % INODES_PER_BLOCK
//
// Retorna 1 em sucesso, 0 em falha.
// ============================
int minix_read_inode(uint32 dev, MinixSuperBlock *sb, uint32 inode_num, MinixInode *inode_out) {
    if (inode_num == 0) {
        printf("minix: inode 0 é inválido\n");
        return 0;
    }

    // Calcula o offset do bloco que contém este inode
    uint32 inode_table_block = 2 + sb->imap_blocks + sb->zmap_blocks;
    uint32 bloco = inode_table_block + (inode_num - 1) / INODES_PER_BLOCK;
    uint32 offset_disco = bloco * MINIX_BLOCK_SIZE;

    // Lê o bloco inteiro (contém 16 inodes)
    MinixInode bloco_inodes[INODES_PER_BLOCK];
    block_op(dev, (uint8 *) bloco_inodes, MINIX_BLOCK_SIZE, offset_disco, 0);

    // Extrai o inode específico dentro do bloco
    uint32 idx = (inode_num - 1) % INODES_PER_BLOCK;
    *inode_out = bloco_inodes[idx];

    return 1;
}

// ============================
// minix_read_file
//
// Lê `size` bytes a partir de `offset` do arquivo descrito por `inode`.
// Suporta:
//   - Zonas diretas:   zones[0..6]  (7 blocos = 7 KB)
//   - Zona indireta simples: zones[7] (256 ponteiros de 4 bytes = 256 KB)
//
// Retorna o número de bytes efetivamente lidos.
// ============================
uint32 minix_read_file(uint32 dev, MinixSuperBlock *sb, MinixInode *inode,
                       uint8 *buffer, uint32 size, uint32 offset) {

    // Limita a leitura ao tamanho real do arquivo
    if (offset >= inode->size)
        return 0;
    if (offset + size > inode->size)
        size = inode->size - offset;

    uint32 bytes_lidos = 0;

    // Calcula em qual bloco do arquivo começa a leitura e
    // o byte de deslocamento dentro desse bloco
    uint32 bloco_inicial = offset / MINIX_BLOCK_SIZE;
    uint32 byte_offset   = offset % MINIX_BLOCK_SIZE;

    uint8 bloco_buf[MINIX_BLOCK_SIZE];

    // ---- Zonas diretas (zones[0..6]) ----
    for (uint32 i = 0; i < 7 && bytes_lidos < size; i++) {
        if (i < bloco_inicial) continue;  // Ainda não chegamos no bloco de início
        if (inode->zones[i] == 0) break;  // Zona não alocada: fim dos dados

        block_op(dev, bloco_buf, MINIX_BLOCK_SIZE, inode->zones[i] * MINIX_BLOCK_SIZE, 0);

        uint32 inicio = (i == bloco_inicial) ? byte_offset : 0;
        uint32 fim    = MINIX_BLOCK_SIZE;
        uint32 a_copiar = fim - inicio;

        if (bytes_lidos + a_copiar > size)
            a_copiar = size - bytes_lidos;

        for (uint32 j = 0; j < a_copiar; j++)
            buffer[bytes_lidos + j] = bloco_buf[inicio + j];

        bytes_lidos += a_copiar;
    }

    // ---- Zona indireta simples (zones[7]) ----
    if (bytes_lidos < size && inode->zones[7] != 0) {
        // Lê o bloco de ponteiros (256 × uint32)
        uint32 ponteiros[PTRS_PER_BLOCK];
        block_op(dev, (uint8 *) ponteiros, MINIX_BLOCK_SIZE, inode->zones[7] * MINIX_BLOCK_SIZE, 0);

        for (uint32 i = 0; i < PTRS_PER_BLOCK && bytes_lidos < size; i++) {
            uint32 bloco_abs = i + 7;  // Posição absoluta no arquivo (bloco 7, 8, ...)

            if (bloco_abs < bloco_inicial) continue;
            if (ponteiros[i] == 0) break;

            block_op(dev, bloco_buf, MINIX_BLOCK_SIZE, ponteiros[i] * MINIX_BLOCK_SIZE, 0);

            uint32 inicio = (bloco_abs == bloco_inicial) ? byte_offset : 0;
            uint32 a_copiar = MINIX_BLOCK_SIZE - inicio;

            if (bytes_lidos + a_copiar > size)
                a_copiar = size - bytes_lidos;

            for (uint32 j = 0; j < a_copiar; j++)
                buffer[bytes_lidos + j] = bloco_buf[inicio + j];

            bytes_lidos += a_copiar;
        }
    }

    return bytes_lidos;
}

// ============================
// minix_print_tree
//
// Lê iterativamente a árvore a partir de um Inode inicial e formata a exibição 
// recursiva no terminal do OS. 
// ============================
void minix_print_tree(uint32 dev, MinixSuperBlock *sb, uint32 inode_num, int depth, char *name) {
    MinixInode inode;
    if (!minix_read_inode(dev, sb, inode_num, &inode)) {
        return;
    }

    // Identação visual por nível da árvore
    for (int i = 0; i < depth; i++) {
        printf("  "); // Dois espaços por nível
    }

    // Uma máscara é utilizada no minix para limpar as flags de permissão
    // E sobrar apenas a informação do tipo de arquivo (0170000 octal = S_IFMT)
    uint16 file_type = inode.mode & S_IFMT;

    if (file_type == S_IFDIR) {
        printf("|-- %s/ \n", name);

        // Se for um diretório, o arquivo de dados dele será uma lista contínua
        // de estruturas MinixDirEntry
        uint32 read_bytes = 0;
        uint32 offset = 0;
        uint8 buf[MINIX_BLOCK_SIZE];

        while (offset < inode.size) {
            uint32 chunk = minix_read_file(dev, sb, &inode, buf, MINIX_BLOCK_SIZE, offset);
            if (chunk == 0) break; // Terminou o arquivo

            int dir_entries = chunk / DIRENTRY_SIZE;
            MinixDirEntry *dir_array = (MinixDirEntry *)buf;

            for (int i = 0; i < dir_entries; i++) {
                // Inodes desvinculados possuem valor 0 como inode. 
                if (dir_array[i].inode == 0) continue;

                // Não queremos fazer recursividade infinita nas pastas atuais "." e ".."
                if (dir_array[i].name[0] == '.' && dir_array[i].name[1] == '\0') continue;
                if (dir_array[i].name[0] == '.' && dir_array[i].name[1] == '.' && dir_array[i].name[2] == '\0') continue;

                // Chamada recursiva para os filhotes passando o nome.
                minix_print_tree(dev, sb, dir_array[i].inode, depth + 1, (char *)dir_array[i].name);
            }
            offset += chunk;
        }
    } else {
        // Se for arquivo normal
        printf("|-- %s (%d bytes)\n", name, inode.size);
    }
}
