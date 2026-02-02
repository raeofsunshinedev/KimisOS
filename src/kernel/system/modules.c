#include "modules.h"
#include "../shared/kstdlib.h"
#include "elf.h"
#include "vfs.h"
#include "../shared/memory.h"
#include "scheduler.h"
#include <stdarg.h>

#define MODULE_NAME "MLOADER"

vector_t *modules;
vector_t *module_deps;

uint32_t module_api(uint32_t func, ...){
    va_list vars;
    va_start(vars, func);
    uint32_t return_value = 0;
    switch(func){
        case MODULE_API_REGISTER:
            //do something
            module_t *structure = va_arg(vars, void *);
            if(structure == 0){
                return_value =  -1;
            }
            uint32_t tmp = pm_alloc();
            (structure->key) = (modules->size ^ 190507) + tmp << 12 ^ 4405648937 ^ 8592807313 >> 5;
            structure->key &= 0xffffff00 | modules->size;
            vector_push(modules, structure);
            pm_free(tmp);
            return_value = 0;
            printf("vector size -> %d", structure->key);
            break;
            case MODULE_API_ADDINT:
            uint32_t int_index = va_arg(vars, uint32_t);
            uint32_t key = va_arg(vars, uint32_t);
            void *interrupt_handler = va_arg(vars, void *);
            if(int_index >= 16 || int_index < 0 /* || get_irq_handler(int_index)*/){
                
                return_value = -1;
                break;
            }
            // printf("installed %d", int_index);
            install_irq_handler(interrupt_handler, int_index);
            module_t *tmpmodule = vector_get(key & 0xff, modules);
            tmpmodule->interrupts |= 1 << int_index;
            return_value = 0;
            // printf("Added key\n");
            break;
        case MODULE_API_DELINT:
            int_index = va_arg(vars, uint32_t);
            key = va_arg(vars, uint32_t);
            tmpmodule = vector_get(key & 0xff, modules);
            if(key == tmpmodule->key){
                install_irq_handler(0, int_index);
                tmpmodule->interrupts &= ~(1 << int_index);
            }
            else{
                return_value = -1;
                break;
            }
            return_value = 0;
            break;
        case MODULE_API_PRINT:
            // vmlog(MODULE_NAME, "Testing modules calling kernel functions\n", MLOG_PRINT, vars);
            char *name = va_arg(vars, char *);
            char *string = va_arg(vars, char *);
            vmlog(name, string, MLOG_PRINT, vars);
            break;
        case MODULE_API_READ:
            vfile_t *file = va_arg(vars, vfile_t *);
            char *buffer = va_arg(vars, char *);
            uint32_t offset = va_arg(vars, uint32_t);
            uint32_t count = va_arg(vars, uint32_t);
            return_value = fread(file, buffer, offset, count);
            break;
        case MODULE_API_WRITE:
            file = va_arg(vars, vfile_t *);
            buffer = va_arg(vars, char *);
            offset = va_arg(vars, uint32_t), count = va_arg(vars, uint32_t);
            return_value = fwrite(file, buffer, offset, count);
            break;
        case MODULE_API_CREAT:
            name = va_arg(vars, char *);
            VFILE_TYPE ftype = va_arg(vars, VFILE_TYPE);
            void *arg1, *arg2;
            arg1 = va_arg(vars, void *);
            arg2 = va_arg(vars, void *);
            return_value = (uint32_t)fcreate(name, ftype, arg1, arg2);
            break;
        case MODULE_API_DELET:
            return_value = -1;
            break;
        case MODULE_API_OPEN:
            name = va_arg(vars, char *);
            return_value = (uint32_t)fget_file(name);
            break;
        case MODULE_API_MAP:
            return_value = 0;
            void *vaddr = va_arg(vars, void *);
            void *paddr = va_arg(vars, void *);
            uint32_t flags = va_arg(vars, uint32_t);
            map(vaddr, paddr, flags);
            break;
        case MODULE_API_UNMAP:
            vaddr = va_arg(vars, void *);
            unmap(vaddr);
            break;
        case MODULE_API_PADDR:
            void *addr = va_arg(vars, void *);
            return_value = get_paddr(addr);
            break;
        case MODULE_API_MALLOC: 
            uint32_t sz_pgs = va_arg(vars, uint32_t);
            return_value = (uint32_t)kmalloc(sz_pgs);
            break;
        case MODULE_API_FREE:
            void *ptr = va_arg(vars, void *);
            kfree(ptr);
            return_value = 0;
            break;
        case MODULE_API_PMALLOC64K:
            return_value = pm_alloc_64kaligned();
            break;
        case MODULE_API_KMALLOC_PADDR:
            paddr = (void *)va_arg(vars, uint32_t);
            uint32_t size = va_arg(vars, uint32_t);
            return_value = (uint32_t)kmalloc_page_paddr((uint32_t)paddr, size);
            break;
        case MODULE_MESSAGE_HANDLER:
            key = va_arg(vars, uint32_t);
            void *handler = va_arg(vars, void (*)());
            for(int i = 0; i < modules->size; i++){
                module_t* module = ((module_t *)vector_get(i, modules));
                if(module->key == key){
                    module->message_handler = handler;
                    return_value = 0;
                    break;
                }
                return_value = -1;
            }
            break;
        case MODULE_API_BLOCK_PID:
            uint32_t pid = va_arg(vars, uint32_t);
            set_pid_blocked(pid);
            break;
        case MODULE_API_UNBLOCK_PID:
            pid = va_arg(vars, uint32_t);
            set_pid_unblocked(pid);
            break;
        case MODULE_API_GET_CPID:
            return_value = get_current_pid();
            break;
        case MODULE_API_IS_INTERRUPT:
            return get_in_interrupt();
            break;
    }
    va_end(vars);
    return return_value;
}

void dispatch_message(uint32_t message, ...){
    va_list args;
    va_start(args, message);
    
    va_end(args);
}

void module_start(void *ptr){
    void (*entry)(void* api, uint32_t version) = load_elf(ptr, PT_SYS);
    (*entry)(module_api, 0);
}
void modules_init(){
    mlog(MODULE_NAME, "Initializing Boot-Time Modules\n", MLOG_PRINT);
    // kernel_module_t *module = kernel_info->loaded_modules;
    // uint32_t i = 0;
    modules = init_vector(0, sizeof(module_t), 0, 0);
    // while(module){
    //     if(!module->ptr){
    //         continue;
    //     }
    //     if(module->flags & MODULE_INITRC){
    //         fcreate("initrc", VFILE_POINTER, module->ptr, 1);
    //         continue;
    //     }
    //     void (*entry)(void* api, uint32_t version) = load_elf(module->ptr, PT_SYS);
    //     mlog(MODULE_NAME, "Loaded module with ID %x\n", MLOG_PRINT, module->id);
    //     // module->type |= 0x8000;//set present flag in type
    //     module->flags |= MODULE_PRESENT;
    //     module = module->link;
    //     (*entry)(module_api, 0);
    // }
}