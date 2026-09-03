#pragma once
#include <stdint.h>

inline void outb(uint16_t port, uint8_t data){
    asm volatile ("outb %0, %1" :: "a"(data), "Nd"(port));
}

inline void outw(uint16_t port, uint16_t data){
    asm volatile ("outw %0, %1" :: "a"(data), "Nd"(port));
}

inline uint8_t inb(uint16_t port){
    uint8_t byte = 0;
    asm volatile ("inb %1, %0" : "=a"(byte) : "Nd"(port));
    return byte;
}

inline uint16_t inw(uint16_t port){
    uint16_t word = 0;
    asm volatile ("inw %1, %0" : "=a"(word) : "Nd"(port));
    return word;
}

inline void outl(uint16_t port, uint32_t data){
    asm volatile ("outl %0, %1" :: "a"(data), "Nd"(port));
}

inline uint32_t inl(uint16_t port){
    uint32_t byte = 0;
    asm volatile ("inl %1, %0" : "=a"(byte) : "Nd"(port));
    return byte;
}

inline uint32_t __rdtsc(){
    unsigned int lo, hi;
    
    __asm__ volatile (
        "rdtsc"
        : "=a"(lo), "=d"(hi)
    );
    
    return lo;
}