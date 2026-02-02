#include "../shared/kstdlib.h"
#include "../shared/memory.h"
#include "../shared/string.h"
#include "modules.h"
#include "vfs.h"

//COMMANDS TO WRITE:
//link: create symlink from one file to another (useful for actually selecting where to mount things)
//exec: execute program in userspace


void initrc_read(vfile_t *file){
    mlog("KERNEL", "Reading initrc:\n", MLOG_PRINT);
    char *ptr = file->access.data.ptr;
    uint32_t size = file->access.data.size_pgs * 4096;
    char statement[512];
    uint32_t i = 0;
    while(strcmp(statement, "END")){
        uint32_t s_i = 0;
        while(ptr[i] != ' ' && ptr[i] != '\n' && ptr[i]){
            statement[s_i++] = ptr[i];
            i++;
        }
        statement[s_i]=0;
        if(statement[0] == '#'){
            while(ptr[i] != '\n' && ptr[i]){
                i++;
            }
        }
        if(!strcmp(statement, "ECHO")){
            //this is purely for testing purposes.
            printf("[ INITRC ]");
            i++;
            while(ptr[i] != '\n' && ptr[i]){
                if(ptr[i] != '\"'){
                    printf("%c", ptr[i]);
                }
                i++;
            }
            printf("\n");
        }
        if(!strcmp(statement, "MODULE")){
            i++;
            char module_name[512];
            int j = 0;
            for(; j < 512 && ptr[i + j] && ptr[i+j] != '\n'; j++){
                module_name[j] = ptr[i + j];
            }
            i+=j;
            module_name[j] = 0;
            // printf("%s\n", module_name);
            vfile_t *module = fget_file(module_name);
            if(!module){
                printf("Error: Could not find module in Initrc.conf: %s\n", module_name);
                continue;
            }
            module_start(module->access.data.ptr);
        }
        // printf("%d\n", i);
        if(i >= size){
            return;
        }
        i++;
    }
}