#include "minix.h"
#include "../virtio/block.h"
#include "../defs.h"
#include "../string.h"

// ============================
// minix_read_superblock
//
// Lê o superbloco do dispositivo `dev` e valida o magic number.
// O superbloco fica no offset 1024 no disco (segundo bloco de 1024 bytes).
//
// Retorna 1 se o magic for válido, 0 caso contrário.
// ============================
int minix_read_superblock(uint32 dev, MinixSuperBlock *sb) {
    // O dispositivo sempre escreve o bloco inteiro (1024 bytes), mas a struct
    // tem só ~32 bytes. Lemos para um buffer do tamanho do bloco e copiamos
    // apenas o início, senão o resto do bloco sobrescreveria a pilha.
    uint8 bloco_buf[MINIX_BLOCK_SIZE];
    if (!block_op(dev, bloco_buf, MINIX_BLOCK_SIZE, MINIX_BLOCK_SIZE, 0))
        return 0;
    memcpy(sb, bloco_buf, sizeof(MinixSuperBlock));

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
    if (!block_op(dev, (uint8 *) bloco_inodes, MINIX_BLOCK_SIZE, offset_disco, 0))
        return 0;

    // Extrai o inode específico dentro do bloco
    uint32 idx = (inode_num - 1) % INODES_PER_BLOCK;
    *inode_out = bloco_inodes[idx];

    return 1;
}

// ============================
// minix_ler_indireta
//
// Resolve um bloco através de `nivel` níveis de indireção.
// `zona` é o bloco de ponteiros do nível atual e `indice` é a posição
// relativa ao início da região coberta por ele.
//
// Cada bloco de ponteiros guarda PTRS_PER_BLOCK (256) números de zona.
// No nível 1 o ponteiro já aponta para o dado; nos níveis acima cada
// entrada cobre 256^(nivel-1) blocos, então dividimos o índice para
// escolher a entrada e descemos um nível com o resto.
//
//   nivel 1 (simples): 256 blocos         =    256 KiB
//   nivel 2 (dupla):   256 × 256 blocos   =     64 MiB
//   nivel 3 (tripla):  256³ blocos        =     16 GiB
//
// Retorna o número da zona de dados, ou 0 se não houver bloco alocado.
// ============================
static uint32 minix_ler_indireta(uint32 dev, uint32 zona, uint32 nivel, uint32 indice) {
    if (zona == 0)
        return 0;

    uint32 ponteiros[PTRS_PER_BLOCK];
    if (!block_op(dev, ponteiros, MINIX_BLOCK_SIZE, (uint64) zona * MINIX_BLOCK_SIZE, 0))
        return 0;

    if (nivel == 1)
        return ponteiros[indice];

    // Quantos blocos de dados cada entrada deste nível cobre
    uint32 alcance = 1;
    for (uint32 i = 1; i < nivel; i++)
        alcance *= PTRS_PER_BLOCK;

    return minix_ler_indireta(dev, ponteiros[indice / alcance], nivel - 1, indice % alcance);
}

// ============================
// minix_bmap
//
// Converte o número de um bloco lógico do arquivo (0, 1, 2, ...) no número
// da zona correspondente no disco.
//   - blocos 0..6: zonas diretas, zones[0..6]
//   - em seguida:  indireta simples (zones[7]), dupla (zones[8]) e tripla (zones[9])
//
// Retorna 0 se o bloco não estiver alocado.
// ============================
static uint32 minix_bmap(uint32 dev, MinixInode *inode, uint32 bloco) {
    if (bloco < 7)
        return inode->zones[bloco];
    bloco -= 7;

    uint32 capacidade = PTRS_PER_BLOCK;  // blocos cobertos pelo nível atual
    for (uint32 nivel = 1; nivel <= 3; nivel++) {
        if (bloco < capacidade)
            return minix_ler_indireta(dev, inode->zones[6 + nivel], nivel, bloco);
        bloco -= capacidade;
        capacidade *= PTRS_PER_BLOCK;
    }
    return 0;
}

// ============================
// minix_read_file
//
// Lê `size` bytes a partir de `offset` do arquivo descrito por `inode`.
// Para cada bloco lógico envolvido, minix_bmap encontra a zona no disco,
// passando pelas zonas diretas e pelas indiretas simples, dupla e tripla.
// Zonas não alocadas (buracos em arquivos esparsos) são lidas como zeros.
//
// Retorna o número de bytes efetivamente lidos.
// ============================
uint32 minix_read_file(uint32 dev, MinixSuperBlock *sb, MinixInode *inode,
                       uint8 *buffer, uint32 size, uint32 offset) {

    // Limita a leitura ao tamanho real do arquivo
    if (offset >= inode->size)
        return 0;
    if (size > inode->size - offset)
        size = inode->size - offset;

    uint32 bytes_lidos = 0;
    uint8 bloco_buf[MINIX_BLOCK_SIZE];

    while (bytes_lidos < size) {
        uint32 pos    = offset + bytes_lidos;
        uint32 bloco  = pos / MINIX_BLOCK_SIZE;   // bloco lógico do arquivo
        uint32 inicio = pos % MINIX_BLOCK_SIZE;   // deslocamento dentro do bloco

        uint32 a_copiar = MINIX_BLOCK_SIZE - inicio;
        if (a_copiar > size - bytes_lidos)
            a_copiar = size - bytes_lidos;

        uint32 zona = minix_bmap(dev, inode, bloco);
        if (zona == 0) {
            memset(bloco_buf, 0, MINIX_BLOCK_SIZE);
        } else if (!block_op(dev, bloco_buf, MINIX_BLOCK_SIZE, (uint64) zona * MINIX_BLOCK_SIZE, 0)) {
            break;  // Erro de E/S: devolve o que já foi lido
        }

        memcpy(buffer + bytes_lidos, bloco_buf + inicio, a_copiar);
        bytes_lidos += a_copiar;
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
