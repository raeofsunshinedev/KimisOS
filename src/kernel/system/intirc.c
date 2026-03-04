#include "../shared/kstdlib.h"
#include "../shared/memory.h"
#include "../shared/string.h"
#include "modules.h"
#include "vfs.h"

//COMMANDS TO WRITE:
//link: create symlink from one file to another (useful for actually selecting where to mount things)
//exec: execute program in userspace

int is_num(char c){
    if(c < 48 || (c > 57 && c < 97)) return 0;
    if(c > 102) return 0;
    return 1;
}


void initrc_read(vfile_t *file){
    mlog("KERNEL", "Reading initrc:\n", MLOG_PRINT);
    char *ptr = file->access.data.ptr;
    uint32_t size = file->access.data.size_pgs * 4096;
    char statement[512];
    uint32_t i = 0;
    while(strcmp(statement, "END")){
        uint32_t s_i = 0;
        while(ptr[i] == ' ' || ptr[i] == '\t' || ptr[i] == '\n'){
            i++;
        }
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
        else if(!strcmp(statement, "ECHO")){
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
        else if(!strcmp(statement, "MODULE")){
            i++;
            char module_name[512];
            int j = 0;
            for(; j < 512 && ptr[i + j] && ptr[i+j] != '\n' && ptr[i + j] != ' '; j++){
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
        else if(!strcmp(statement, "MOUNT")){
            char mount_src_name[512] = {0};
            char mount_dest[512] = {0};
            uint32_t offset = 0;
            i++;
            int j = 0;
            for(; j < 512 && ptr[i + j] && ptr[i+j] != '\n' && ptr[i+j] != ' '; j++){
                mount_src_name[j] = ptr[i + j];
            }
            i+=j;
            mount_src_name[j] = 0;
            for(j = 1; ptr[i + j] && ptr[i + j] != '\n' && ptr[i + j] != ' '; j++){
                mount_dest[j-1] = ptr[i + j];
            }
            
            mount_dest[j-1] = 0;
            i += j;
            if(ptr[i] == ' '){
                i++;
                j = 0;
                char atoi_str[512] = {0};
                
                uint32_t base = 10;
                if(ptr[i + j + 1] == 'x'){
                    // printf("base 16\n");
                    i+=2;
                    base = 16;
                }
                else if(ptr[i+j] == '0' && ptr[i+j+1] == '0'){
                    // printf("Base 8!\n");
                    i += 2;
                    base = 8;
                }
                
                for(; j < 512 && ptr[i + j] && ptr[i+j] != '\n' && ptr[i + j] != ' ' && is_num(ptr[i+j]); j++){
                    atoi_str[j] = ptr[i + j];
                }
                i += j;
                atoi_str[j] = 0;
                
                offset = atoi(atoi_str, base);
                
                // printf("String: %s, Result: %d\n", atoi_str, offset);
                
            }
            
            // if(ptr[i] )
            
            // printf("TEST: %s, %s, %d\n", mount_src_name, mount_dest, ptr[i]);
            
            vfile_t *to_mount = fget_file(mount_src_name);
            dispatch_message(MESSAGE_MOUNT_FS, to_mount, mount_dest, offset);
        }
        else if(!strcmp(statement, "END")){
            return;
        }
        else{
            printf("FATAL ERROR: Unknown Instruction: %s\n", statement);
            asm("int $6");
        }
        // printf("%d\n", i);
        while(ptr[i] != '\n'){
            i++;
            if(i >= size){
                return;
            }
        }
        i++;
    }
}