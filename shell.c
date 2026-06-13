#include "shell.h"
#include "console.h"
#include "string.h"
#include "defs.h"
#include "game/game.h"


// Mantem na memoria a nossa posicao atual nas pastas do Minix.
// O valor 1 é fixo, no Minix 3 o Root Directory ("/") é sempre o Inode 1.
uint32 current_inode = 1; 

extern volatile char console_buf[];
extern volatile int console_buf_pos;
extern volatile int console_line_ready;

// ==============================================================
// readline
// Fica esperando a flag do console iterar. Em OS mais complexos, isso daria yield() na thread!
// ==============================================================
int readline(char *buf, int max_len) {
    // Espera ocupada pela interrupção preencher
    while (!console_line_ready) {
        // busy wait
    }
    
    // Transfere o buffer global para o buffer do processo local
    int len = console_buf_pos;
    if (len >= max_len) len = max_len - 1;
    
    memcpy(buf, (void*)console_buf, len);
    buf[len] = '\0';
    
    // Reseta o estado para capturar nova linha
    console_buf_pos = 0;
    console_line_ready = 0;
    
    return len;
}

// ==============================================================
// find_inode_by_name
// Basicamente um pequeno buscador dentro de uma pasta para checar
// em todas as folhas de dados de todos os blocos possíveis dessa pasta
// se existe a exata "palavra/nome" solicitada no Inode atual
// Retorna o index se achar, 0 se não achar.
// ==============================================================
uint32 find_inode_by_name(uint32 dev, MinixSuperBlock *sb, uint32 dir_inode_num, const char *target_name) {
    MinixInode inode;
    
    // Leitura original
    if (!minix_read_inode(dev, sb, dir_inode_num, &inode)) {
        return 0; // abort
    }

    // Apenas diretórios podem ter nome de arquivos guardados dentro deles (struct DirEntry).
    // Então checo a mask do modo se ele realmente é diretório!
    if ((inode.mode & S_IFMT) != S_IFDIR) {
        return 0;
    }

    uint32 offset = 0;
    uint8 buf[MINIX_BLOCK_SIZE];

    // Faz a busca lendo o arquivo como fazemos desde blocos 
    while (offset < inode.size) {
        uint32 chunk = minix_read_file(dev, sb, &inode, buf, MINIX_BLOCK_SIZE, offset);
        if (chunk == 0) break; 

        // Descobrindo quantas strings estao ali naquele bloco e transformando em ponteiros
        int dir_entries = chunk / DIRENTRY_SIZE;
        MinixDirEntry *dir_array = (MinixDirEntry *)buf;

        for (int i = 0; i < dir_entries; i++) {
            
            // O 0 indica vazio
            if (dir_array[i].inode == 0) continue;
            
            // Compara os char* manualmente
            int len = strlen(target_name);
            if (strncmp((const char*)dir_array[i].name, target_name, len) == 0 && dir_array[i].name[len] == '\0') {
                return dir_array[i].inode;
            }
        }
        offset += chunk;
    }
    
    return 0; 
}


// ==============================================================
// shell_run
// Lógica de loop infinito onde interpretamos o CLI em formato de árvore parecida
// com linux, processando ls, cd, cat, tree, game ou help
// ==============================================================
void shell_run(uint32 dev, MinixSuperBlock *sb) {
    char cmd[128]; // Max length de texto do input de cada terminal no loop iterativo
    char current_path[128]; 
    strcpy(current_path, "/");

    printf(SPRING_GREEN "\n  ____                           _             ___  ____  \n" COLOR_RESET);
    printf(SPRING_GREEN " |  _ \\ ___  _ __   ___ __ _  __| | ___  _ __ / _ \\/ ___| \n" COLOR_RESET);
    printf(SPRING_GREEN " | |_) / _ \\| '_ \\ / __/ _` |/ _` |/ _ \\| '__| | | \\___ \\ \n" COLOR_RESET);
    printf(SPRING_GREEN " |  _ < (_) | | | | (_| (_| | (_| | (_) | |  | |_| |___) |\n" COLOR_RESET);
    printf(SPRING_GREEN " |_| \\_\\___/|_| |_|\\___\\__,_|\\__,_|\\___/|_|   \\___/|____/ \n" COLOR_RESET);
    printf(CORAL "               Shell Interativo - TCC RoncadorOS\n\n" COLOR_RESET);
    printf("Comandos: " LIGHT_SEA_GREEN "ls" COLOR_RESET " | " LIGHT_SEA_GREEN "cd <dir>" COLOR_RESET " | " LIGHT_SEA_GREEN "cat <file>" COLOR_RESET " | " LIGHT_SEA_GREEN "tree" COLOR_RESET " | " LIGHT_SEA_GREEN "game" COLOR_RESET " | " LIGHT_SEA_GREEN "help" COLOR_RESET "\n\n");

    while (1) {
        printf(SPRING_GREEN "roncadorOS" COLOR_RESET ":" CORAL "%s" COLOR_RESET "$ ", current_path);
        
        readline(cmd, sizeof(cmd));

        // Pular Enter sem digitação
        if (strlen(cmd) == 0) continue;

        // Comando 1: 'ls'
        if (strcmp(cmd, "ls") == 0) {
            MinixInode inode;
            minix_read_inode(dev, sb, current_inode, &inode);
            
            uint32 offset = 0;
            uint8 buf[MINIX_BLOCK_SIZE];

            // Ler exatamente todos os dados do Inode e tentar quebrar os sub_nomes em strings!
            while (offset < inode.size) {
                uint32 chunk = minix_read_file(dev, sb, &inode, buf, MINIX_BLOCK_SIZE, offset);
                if (chunk == 0) break;

                int dir_entries = chunk / DIRENTRY_SIZE;
                MinixDirEntry *dir = (MinixDirEntry *)buf;

                // Aqui varremos os itens do diretório do Root e imprimirmos um a um.
                for (int i = 0; i < dir_entries; i++) {
                    if (dir[i].inode == 0) continue;
                    
                    // Checa se o Inode dessa entrada é de arquivo ou diretorio para decorar (adicionando '/')
                    MinixInode child;
                    minix_read_inode(dev, sb, dir[i].inode, &child);
                    if ((child.mode & S_IFMT) == S_IFDIR) {
                        printf(LIGHT_SEA_GREEN " %s/" COLOR_RESET "  ", dir[i].name);
                    } else {
                        printf(" %s   ", dir[i].name);
                    }
                }
                offset += chunk;
            }
            printf("\n");
        }

        // Comando 2: 'cd'
        // Base lógica: Se passar "/" direto a gente reinicia para o root. Se escrever alguma pasta extra a 
        // gente chama aquele buscador ali de cima até o target e verifica que ele tambem seja diretório!
        else if (strncmp(cmd, "cd ", 3) == 0) {
            char *target = cmd + 3; // Ignora visualmente os 3 primeiros chars ('c', 'd', ' ')!

            if (strcmp(target, "/") == 0) {
                current_inode = 1;
                strcpy(current_path, "/");
            } else {
                uint32 target_inode = find_inode_by_name(dev, sb, current_inode, target);
                
                if (target_inode != 0) {
                    MinixInode tgt;
                    minix_read_inode(dev, sb, target_inode, &tgt);
                    // Comprovando com a mascara de pastas
                    if ((tgt.mode & S_IFMT) == S_IFDIR) {
                        current_inode = target_inode;
                        
                        // Atualiza a representação do caminho
                        if (strcmp(target, ".") == 0) {
                            // não faz nada com o current_path
                        } else if (strcmp(target, "..") == 0) {
                            // Subir de diretório (apaga até a última '/')
                            int len = strlen(current_path);
                            if (len > 1) { // Só volta se já não for a propria raiz "/"
                                // Remove a última barra barra se ela estiver na ponta
                                if (current_path[len-1] == '/') {
                                    current_path[len-1] = '\0';
                                    len--;
                                }
                                // procura de tras pra frente e corta na proxima barra
                                while (len > 0 && current_path[len] != '/') len--;
                                current_path[len+1] = '\0';
                                if (len == 0) strcpy(current_path, "/");
                            }
                        } else {
                            // Entrou em um novo diretório filho, concatena ele:
                            // Só coloca a barra se a string atual ja nao termina com barra
                            int l = strlen(current_path);
                            if (l > 0 && current_path[l-1] != '/') strcat(current_path, "/");
                            strcat(current_path, target);
                        }
                        
                    } else {
                        printf(TOMATO "cd: %s nao e um diretorio\n" COLOR_RESET, target);
                    }
                } else {
                    printf(TOMATO "cd: pasta %s nao encontrada\n" COLOR_RESET, target);
                }
            }
        }

        // Comando 3: 'cat'
        // É a mesmissima lógica do CD. Procura pelo nome fornecido pelo usuário e checa se o 
        // target encontrado é realmento um arquivo de texto ao inves de uma pasta. Quando ele achar,
        // ele da leitura pura do minix_read_file e plota tudo em caracteres no console!
        else if (strncmp(cmd, "cat ", 4) == 0) {
            char *target = cmd + 4; // Ignorando ('c', 'a', 't', ' ')
            uint32 target_inode = find_inode_by_name(dev, sb, current_inode, target);
            
            if (target_inode != 0) {
                MinixInode tgt;
                minix_read_inode(dev, sb, target_inode, &tgt);
                
                // Mascára de Arquivos Limpos no Minix.
                if ((tgt.mode & S_IFMT) == S_IFREG) {
                    uint32 offset = 0;
                    uint8 buf[MINIX_BLOCK_SIZE];
                    // Ficar lendo chunk por chunk contínuamente para casos o file > bloco fixo !
                    while (offset < tgt.size) {
                        uint32 chunk = minix_read_file(dev, sb, &tgt, buf, MINIX_BLOCK_SIZE - 1, offset);
                        if (chunk == 0) break;
                        buf[chunk] = '\0';
                        printf("%s", buf);
                        offset += chunk;
                    }
                    printf("\n");
                } else {
                     printf(TOMATO "cat: %s e um diretorio\n" COLOR_RESET, target);
                }
            } else {
                printf(TOMATO "cat: %s nao encontrado\n" COLOR_RESET, target);
            }
        } 
        
        // Comando 4: 'tree'
        // Imprime a árvore recursiva a partir do diretório atual
        else if (strcmp(cmd, "tree") == 0) {
            minix_print_tree(dev, sb, current_inode, 0, current_path);
        }
        // Comando 5: 'help'
        else if (strcmp(cmd, "help") == 0) {
            printf("Comandos: " LIGHT_SEA_GREEN "ls" COLOR_RESET " - lista arquivos\n");
            printf("          " LIGHT_SEA_GREEN "cd <dir>" COLOR_RESET " - entra em um diretorio\n");
            printf("          " LIGHT_SEA_GREEN "cat <file>" COLOR_RESET " - exibe conteudo de um arquivo\n");
            printf("          " LIGHT_SEA_GREEN "tree" COLOR_RESET " - arvore de arquivos a partir do diretorio atual\n");
            printf("          " LIGHT_SEA_GREEN "game" COLOR_RESET " - inicia o Tetris (requer VNC)\n");
        }
        
        // Comando 6: 'game'
        else if (strcmp(cmd, "game") == 0) {
            printf(CORAL "Iniciando Tetris... conecte-se via VNC na porta 5901\n" COLOR_RESET);
            init_game();
        }
        else {
             printf(TOMATO "comando nao encontrado: " COLOR_RESET "%s\n", cmd);
        }

        // Pula mais uma linha ao final de cada execução do loop para ficar visualmente limpo
        printf("\n");
    }
}
