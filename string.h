#ifndef STRING_H
#define STRING_H

#include "types.h"

void *memset(void *, int, int);
void *memcpy(void *, const void *, int);
int strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);

#endif