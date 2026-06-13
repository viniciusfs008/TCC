#include "string.h"

void* memset(void *dst, int c, int len) {
    char *d = (char *) dst;
    for(int i = 0; i < len; i++) {
        d[i] = c;
    }
    return dst;
}

// ==============================================================
// strcpy
// Copia a string de origem para destino.
// ==============================================================
char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

// ==============================================================
// strcat
// Concatena src no final de dest.
// ==============================================================
char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d != '\0') d++;
    while ((*d++ = *src++) != '\0');
    return dest;
}

// ==============================================================
// strlen
// Calcula o tamanho de uma string (quantidade de caracteres
// até encontrar o caractere nulo '\0').
// ==============================================================
int strlen(const char *s) {
    int len = 0;
    while(s[len] != '\0') len++;
    return len;
}

// ==============================================================
// strcmp
// Compara duas strings caractere por caractere.
// Retorna 0 se forem iguais, >0 se a primeira for maior, ou <0 
// se for menor.
// ==============================================================
int strcmp(const char *s1, const char *s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// ==============================================================
// strncmp
// Funciona como o strcmp, mas compara no máximo 'n' caracteres.
// Utilizado para poder verificar qual comando o user digitou sem
// afetar o resto da string (ex: verificar apenas os 3 primeiros
// chars de "cd pasta").
// ==============================================================
int strncmp(const char *s1, const char *s2, int n) {
    while(n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if(n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void* memcpy(void *dst, const void *src, int len) {
    char *d = (char *) dst;
    const char *s = (const char *) src;
    for(int i = 0; i < len; i++) {
        d[i] = s[i];
    }
    return dst;
}