#include <stdint.h>
#include "../shared/string.h"
//res needs to allocate enough space for 12 chars minimum
void itoa(int64_t data, char *res, uint8_t base){
    char carr[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char negative = 0;
    if(res == 0) return;
    if(data == 0){
        res[0] = '0';
        res[1] = 0;
        return;
    }
    if(data < 0 && base == 10){
        data *= -1;
        negative = 1;
    }
    int ptr = 0;
    char buffer[12] = {0};
    // res[11] = 0;
    if(data == 0){
        res[0] = '0';
        res[1] = 0;
    }
    uint32_t tmp = data;
    while(tmp){
        buffer[ptr++] = carr[tmp % base];
        tmp /= base;
    }
    uint32_t rindex = 0;
    res[ptr] = 0;
    while(ptr > 0){
        res[--ptr + negative] = buffer[rindex++];
    }
    res[0] += negative * '-';
    return;
}
void memcpy(char *src, char *dest, uint32_t c){
    for(uint32_t i = 0; i < c; i++){
        dest[i] = src[i];
    }
}
void strcpy(char *src, char *dest){
    if(src == 0 || dest == 0){
        return;
    }
    uint32_t len = strlen(src);
    for(int i = 0; i < len; i++){
        dest[i] = src[i];
    }
    dest[len] = 0;
}

void strpad(char *string, char padding, uint32_t length){
    if(strlen(string) >= padding) return;
    char tmp[length];
    for(int i = 0; i < length - strlen(string); i++){
        tmp[i] = padding;
    }
    strcpy(string, tmp + length - strlen(string));
    strcpy(tmp, string);
}

char res[256];

char* strtok(char* string, char d)
{
    static char* input = 0;

    if (string){
        input = string;
    }
    if (!input){
        return 0;
    }
    int i = 0;
    for (; input[i] != 0; i++) {
        if (input[i] != d){
            res[i] = input[i];
        }
        else {
            res[i] = '\0';
            input = input + i+1;
            return res;
        }
    }
    res[i] = '\0';
    input = 0;
    
    return res;
}

void *memset(void *ptr, int value, size_t num){
    for(size_t i = 0; i < num; i++){
        ((uint8_t *)ptr)[i] = value;
    }
    return ptr;
}