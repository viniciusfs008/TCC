#include "types.h"
#include "defs.h"
#include <stdarg.h>

static char *digits = "0123456789abcdef";


void printdouble(double val){
    //printa fixamente com uma precisao de 4 valores apos a virgula
    //no momento, para alterar a precisao, é so alterar a variavel
    int precisao = 4;
    for (int i = 0; i < precisao; i++){
        val *= 10;
    }
    int num = val; 
    int neg = 0;
    //se  o numero for negativo
    if (num < 0){
        num *= -1; //deixa ele positivo
        neg = 1; //e marca a flag como true
    }
    char s[32];
    int i = 0;
    if (num == 0){
        s[i++] = '0';
    }
    while(num){
        s[i++] = digits[num%10];
        num /= 10;
    }
    while (i < precisao){
        s[i++] = '0';
    }
    if (i == precisao){
        s[i++] = '0';
    }
    if (neg){
        s[i++] = '-';
    }
    while(i > precisao) {
        uartputc(s[--i]);
    }
    uartputc('.');
    while(i>0){
        uartputc(s[--i]);
    }
}


void printuns(unsigned int val){
    char s[32];
    int i = 0;
    if (val == 0){
        s[i++] = '0';
    }
    while(val){
        s[i++] = digits[val%10];
        val /= 10;
    }
    while(i > 0) {
        uartputc(s[--i]);
    }
}

void printlng(long val, int base) {
    long l;
    int i = 0;
    char s[66]; // !!! Para base 2, um long ocupa 64 bits. Para cada bit, 1 caracter, além do '0b' inicial

    if (val < 0) {
        l = -val;
    }
    else {
        l = val;
    }
    //Imprimir o número zero    
    if ( l == 0) {
        s[i++] = '0';
    }
    while(l) {
        s[i++] = digits[ l % base];
        l = l / base;
    }
    if (base == 16) {
        s[i++] = 'x';
        s[i++] = '0';
    }
    if (base == 2) {
        s[i++] = 'b';
        s[i++] = '0';
    }
    if (val < 0) s[i++] = '-';
    
    while(i > 0) {
        uartputc(s[--i]);
    }
}

void printptr(uint64 *ptr){
/* !!! é preciso implementar uma função separada para ponteiro porque a função que imprime int/long
        aceita como parâmetro um número com sinal, permitindo a impressão de um espaço de 2^ 63 bits.
        Mas, espere: o espaço de endereçamento do riscv é de 48 bits, certo? (verificar isso)
    */
   uint64 addr = (uint64) ptr;
   char s[64];
   int i = 0;
   while(addr != 0) {
       s[i++] =  digits[addr % 16]; 
       addr /= 16;
   }
   s[i++] = 'x';
   s[i++] = '0';
   while (--i >= 0) {
       uartputc(s[i]);
   }
}

void printunsl(uint64 val){
    char s[64];
    int i = 0;
    if (val == 0){
        s[i++] = '0';
    }
    while(val){
        s[i++] = digits[val%10];
        val /= 10;
    }
    while(i > 0) {
        uartputc(s[--i]);
    }
}

void printf(char *s, ...){
    va_list ap; // argument pointer 
    int c;
    char *str; 
    // https://www.gnu.org/software/libc/manual/html_node/Receiving-Arguments.html#Receiving-Arguments
    // ap aponta para o primeiro argumento opcional
    va_start(ap, s);
    for (; (c = *s) != 0; s++) {
        if (c == '%') { 
            c = *(s+1);
            if (c != 0) { // String mal-formatada terminando em '%' (ex.: "bla%") ?
                // É preciso saltar o caracter que indica a formatação, que não pode ser impresso. 
                // Ex.: Em %x, ignoramos o 'x' 
                s++;     
            }           
            switch(c) {
                // !!! implementar o 'l' ?
                 case '%':
                    uartputc(c);
                    break;
                 case 'd':
                  //va_arg retorna os sucessivos argumentos opcionais. A cada chamada, o próximo argumento
                    printlng(va_arg(ap, int), 10); 
                    break;
                 case 'x':
                    printlng(va_arg(ap, int), 16);
                    break;
                 case  'l':
                    printlng(va_arg(ap, long), 10);
                    break;
                 case 's':
                    str = va_arg(ap, char *);
                    while(*str != '\0') {
                        uartputc(*str++);
                    }               
                    break;
                 case 'p':
                    printptr(va_arg(ap, uint64 *)); 
                    break;
                case 'u':
                    printuns(va_arg(ap, uint32));
                    break;
                case 'h':
                    printunsl(va_arg(ap, uint64));
                    break;
                case 'c':
                    uartputc(va_arg(ap, int));
                    break;
                case 'f':
                    printdouble(va_arg(ap, double));
                    break;
                default:
                    printf("ERRO: printf\nArgumento inesperado\n"); 
            }
        }
        else {
            uartputc(c);
        }
    }
}

void panic(char *s) {
    printf("\u2671 panic - %s\n", s);
    for(;;);
}