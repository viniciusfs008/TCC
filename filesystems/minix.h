#ifndef MINIX_H
#define MINIX_H

#include "../types.h"

#define MINIX3_MAGIC            0x4d5a
#define MINIX_BLOCK_SIZE        1024
#define INODE_SIZE              64
#define INODES_PER_BLOCK        (MINIX_BLOCK_SIZE / INODE_SIZE)  // 16
#define DIRENTRY_SIZE           64
#define DIRENTRIES_PER_BLOCK    (MINIX_BLOCK_SIZE / DIRENTRY_SIZE)  // 16
#define PTRS_PER_BLOCK          (MINIX_BLOCK_SIZE / 4)            // 256 ponteiros por bloco indireto

// Máscaras do campo `mode` do inode, seguindo o padrão POSIX.
// O campo mode de 16 bits combina permissões (rwxrwxrwx) e tipo de arquivo.
// S_IFMT isola apenas os bits de tipo (os 4 bits mais significativos).
// Uso: (inode.mode & S_IFMT) == S_IFDIR
#define S_IFMT   0170000    // Máscara para extrair o TIPO do arquivo (bits 12-15)
#define S_IFDIR  0040000    // Diretório  
#define S_IFREG  0100000    // Arquivo regular/comum 

// ============================
// Superbloco do Minix 3
// Localização: offset 1024 no disco (bloco 1)
// ============================
typedef struct MinixSuperBlock {
    uint32 ninodes;         // Número total de inodes
    uint16 pad0;
    uint16 imap_blocks;     // Blocos usados pelo bitmap de inodes
    uint16 zmap_blocks;     // Blocos usados pelo bitmap de zonas
    uint16 first_data_zone; // Número do primeiro bloco de dados
    uint16 log_zone_size;   // log2(tamanho da zona / tamanho do bloco)
    uint16 pad1;
    uint32 max_size;        // Tamanho máximo de arquivo em bytes
    uint32 zones;           // Número total de zonas
    uint16 magic;           // Magic number: deve ser 0x4d5a para Minix 3
    uint16 pad2;
    uint16 block_size;      // Tamanho do bloco em bytes (geralmente 1024)
    uint8  disk_version;    // Versão do disco
} MinixSuperBlock;

// ============================
// Inode do Minix 3
// Tamanho: 64 bytes, 16 por bloco
// ============================
typedef struct MinixInode {
    uint16 mode;        // Tipo do arquivo + permissões
    uint16 nlinks;      // Número de links rígidos
    uint16 uid;         // ID do usuário dono
    uint16 gid;         // ID do grupo dono
    uint32 size;        // Tamanho do arquivo em bytes
    uint32 atime;       // Último acesso (unix timestamp)
    uint32 mtime;       // Última modificação
    uint32 ctime;       // Mudança de status
    uint32 zones[10];   // Ponteiros para zonas de dados:
                        //   zones[0..6]: diretas (7 blocos × 1024 = 7 KB)
                        //   zones[7]:    indireta simples (256 ponteiros → 256 KB)
                        //   zones[8]:    duplamente indireta (256² ponteiros → 64 MB)
                        //   zones[9]:    triplamente indireta (256³ ponteiros → 16 GB)
} MinixInode;

// ============================
// Entrada de diretório do Minix 3
// Tamanho: 64 bytes, 16 por bloco
// ============================
typedef struct MinixDirEntry {
    uint32 inode;       // Número do inode do arquivo
    uint8  name[60];    // Nome do arquivo (nulo-terminado)
} MinixDirEntry;

// ============================
// Funções públicas
// ============================

// Lê e valida o superbloco do disco.
// Retorna 1 em sucesso (magic correto), 0 em falha.
int minix_read_superblock(uint32 dev, MinixSuperBlock *sb);

// Lê o inode de número `inode_num` do disco.
// Retorna 1 em sucesso, 0 em falha.
int minix_read_inode(uint32 dev, MinixSuperBlock *sb, uint32 inode_num, MinixInode *inode_out);

// Lê `size` bytes a partir de `offset` do arquivo descrito por `inode`.
// Armazena o resultado em `buffer`. Retorna o número de bytes lidos.
uint32 minix_read_file(uint32 dev, MinixSuperBlock *sb, MinixInode *inode,
                       uint8 *buffer, uint32 size, uint32 offset);

// Mapeia e imprime recursivamente a árvore do sistema de arquivos
void minix_print_tree(uint32 dev, MinixSuperBlock *sb, uint32 inode_num, int depth, char *name);

#endif
