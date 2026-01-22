#include "kstdlib.h"
#include <stdarg.h>
#include "memory.h"

char* log_info_types[] = {
    "\b",
    "| DEBUG",
    "\b",
    "| WARNING",
    "| ERROR"
};

void vmlog(char *module, char *str, uint32_t type, va_list vars){
    if(!type || type > 4){
        return;
    }
    printf("[ %s %s ]", module, log_info_types[type]);
    vprintf(str, vars);
}

void mlog(char *module, char *str, uint32_t type, ...){
    va_list vars;
    va_start(vars, type);
    if(!type || type > 4){
        return;
    }
    printf("[ %s %s ]", module, log_info_types[type]);
    vprintf(str, vars);
}

//set initializer to 0 to create an uninitialized vector of size elements
vector_t *init_vector(void *vecptr, uint32_t sizeof_elements, uint32_t elements, void *initializer){
    vector_t *ret = vecptr;
    if (vecptr == 0){
        ret = kmalloc(1);
    }
    ret->sizeof_elements = sizeof_elements;
    ret->size = elements;
    uint32_t pages = (elements * sizeof_elements)/4096 + 1;
    if(elements > 0) ret->ptr = kmalloc(pages);
    if(initializer == 0) return ret;
    for(uint8_t i = 0; i < elements * sizeof_elements; i++){
        ((uint8_t *)(ret->ptr))[i] = ((uint8_t *)initializer)[i];
    }
    return ret;
}

void *vector_get(uint32_t pos, vector_t *vector){
    if(pos > vector->size){
        return 0;
    }
    return vector->ptr + (pos*vector->sizeof_elements);
}

//pos MUST be within vector bounds;
void vector_set(uint32_t pos, vector_t *vector, void *new_element){
    if(pos > vector->size){
        return;
    }
    for(uint32_t i = 0; i < vector->sizeof_elements; i++){
        ((uint8_t *)(vector->ptr))[i] = ((uint8_t *)new_element)[i];
    }
}
// void vector_push(vector_t *vector, void *new_element){
//     if((vector->size * vector->sizeof_elements)/4096 > ((vector->size + 1) * vector->sizeof_elements)/4096){
//         vector->size++;
//         void *newptr = kmalloc((vector->size * vector->sizeof_elements)/4096);
//         for(uint8_t i = 0; i < (vector->size - 1) * vector->sizeof_elements; i++){
//             ((uint8_t *)(newptr))[i] = ((uint8_t *)vector->ptr)[i * vector->size];
//         }
//     }
//     for(uint32_t i = 0; i < vector->sizeof_elements; i++){
//         ((uint8_t *)(vector->ptr))[vector->size + i] = ((uint8_t *)new_element)[vector->size + i];
//     }
//     printf(" (vector push ) %d", vector->size);
// }
void vector_push(vector_t *vector, void *new_element){
    if(((vector->size + 1) * vector->sizeof_elements) >> 12 != (vector->size * vector->sizeof_elements)){
        void *newptr = kmalloc((vector->size * vector->sizeof_elements)/4096);
        for(uint8_t i = 0; i < (vector->size - 1) * vector->sizeof_elements; i++){
            ((uint8_t *)(newptr))[i] = ((uint8_t *)vector->ptr)[i * vector->size];
        }
    }
    for(uint32_t i = 0; i < vector->sizeof_elements; i++){
        ((uint8_t *)(vector->ptr))[vector->size + i] = ((uint8_t *)new_element)[i];
    }
    vector->size++;
}
void vector_pop(uint32_t pos, vector_t *vector, void *element){
    for(uint32_t i = 0; i < vector->sizeof_elements; i++){
        ((uint8_t *)element)[i] = ((uint8_t *)(vector->ptr))[i];
    }
    vector->size--;
}