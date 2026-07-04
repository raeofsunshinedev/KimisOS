#include "serial.h"
#include "../shared/memory.h"
#include "../shared/string.h"
#include "cpuio.h"
#include <stdarg.h>
#include <stdint.h>
// #include <string.h>

uint16_t com_ports[8];
uint8_t com_buffer[8][128];
uint8_t com_buffer_index[8] = {0};

void serial_init(){
    bda_t *bda = (void*)0x400;
    for(int i = 0; i < 4; i++){
        com_ports[i] = bda->com_ports[i];
    }
}
void serial_writeb(uint8_t port, uint8_t data){
    if(com_ports[port] == 0){
        com_buffer[port][com_buffer_index[port]++] = data;
    }
    outb(com_ports[port], data);
}
uint8_t serial_readb(uint8_t port){
    if(com_ports[port] == 0){
        return com_buffer[port][com_buffer_index[port]--];
    }
    return inb(com_ports[port]);
}
void serial_writereg(uint8_t port, uint8_t reg, uint8_t data){
    if(com_ports[port] == 0 || reg > 8) return;
    outb(com_ports[port] + reg, data);
}
uint8_t serial_readreg(uint8_t port, uint8_t reg){
    if(com_ports[port] == 0 || reg > 8) return -1;
    return inb(com_ports[port] + reg);
}

void debug_puts(char *string){
    while(*string){
        // serial_writeb(0, *(string++));
        outb(com_ports[0], *string++);
    }
}
void printf(char *str, ...){
    va_list vars;
    va_start(vars, str);
    vprintf(str, vars);
    va_end(vars);
}

void vprintf(char *string, va_list vars){
    while(*string){
        if(*string == '%'){
            string++;
            int32_t num = 0;
            char *ptr = 0;
            char numb[12];
            switch(*string){
                case 'd':
                    string++;
                    num = va_arg(vars, int32_t);
                    itoa(num, numb, 10);
                    debug_puts(numb);
                    continue;
                case 'x':
                    // debug_puts(":3");
                    string++;
                    num = va_arg(vars, int32_t);
                    // char hnumb[12];
                    itoa(num, numb, 16);
                    strpad(numb, '0', 8);
                    debug_puts(numb);
                    continue;
                case 's':
                    string++;
                    ptr = va_arg(vars, char *);
                    printf(ptr);
                    continue;
                case 'c':
                    string++;
                    num = va_arg(vars, int32_t);
                    printf((char[]){num, 0});
                    continue;
                case 'b':
                    string++;
                    num = va_arg(vars, int32_t);
                    printf(num ? "true" : "false");
            }
        }
        outb(com_ports[0], *string);
        string++;
    }
}