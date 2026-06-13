#include "types.h"
#include "defs.h"

// Buffer de teclado simples
volatile char console_buf[128];
volatile int console_buf_pos = 0;
volatile int console_line_ready = 0;

void console_handler() {
    int c;
    c = uartgetc();
    if (c == -1)
        return; 
    
    // Ignorar entradas enquanto a linha anterior não for processada
    // pelo shell para evitar buffer overflow e race conditions extremas.
    if (console_line_ready) {
        return; 
    }

    switch(c) {
        case 127: 
        case '\b':
            // backspace. 
            if (console_buf_pos > 0) {
                console_buf_pos--;
                uartputc('\b');
                uartputc(' ');
                uartputc('\b');
            }
            break;
        case '\r': 
            // Enter
            console_line_ready = 1;
            printf("\r\n"); 
            break;
        case 0x1b: // esc ANSI (setas, etc) ignorado no buffer principal por hora
            if( (c = uartgetc()) == 0x5B) {
                if((c = uartgetc()) == -1) return;
                switch(c) {
                    case 'A': printf("Seta para cima "); break;
                    case 'B': printf("Seta para baixo "); break;
                }
            }
            break;    
        default:
            if (console_buf_pos < 127) {
                console_buf[console_buf_pos++] = c;
                uartputc(c);  
            }
    }
}