#ifndef SHELL_H
#define SHELL_H

#include "types.h"
#include "filesystems/minix.h"

// ==============================================================
// shell_run
// 
// Função principal para iniciar o console interativo do usuário.
// Recebe o identificador do dispositivo do disco (dev) e um
// ponteiro para o superbloco que foi carregado antecipadamente
// no momento de boot em main.c.
// ==============================================================
void shell_run(uint32 dev, MinixSuperBlock *sb);

#endif
