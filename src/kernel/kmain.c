#include "shared/memory.h"
#include "shared/string.h"
#include "shared/kstdlib.h"
#include "shared/interrupts.h"
#include "shared/config.h"
#include "drivers/serial.h"
#include "drivers/pic.h"
#include "drivers/cpuio.h"
#include "drivers/pci.h"
#include "system/scheduler.h"
#include "system/modules.h"
#include "system/elf.h"
#include "system/vfs.h"
#include "drivers/ustar.h"
#include "system/initrc.h"
#include "system/ramfs.h"
#include "shared/spinlock.h"

kernel_info_t *boot_info = 0;

void pid0(){
    for(;;);
}
void sysinit(){
    mlog("KERNEL", "PID 1 Started\n", MLOG_PRINT);
    pci_init();
    // modules_init(boot_info, 0);
    read_initrd(boot_info->initrd);
    modules_init();
    vfile_t *initrc = fopen("/boot/initrc.conf");
    mlog("RAE", "\033[1;32mDid you remember to migrate your modules to the new API?\033[0m\n", MLOG_PRINT);
    if(initrc){
        mlog("KERNEL", "Found initrc\n", MLOG_PRINT);
        initrc_read(initrc);
    }
    else{
        mlog("KERNEL", "ERROR: Initrc could not be located!\n", MLOG_PRINT);
    }
    const kernel_config_t config = get_config_const();
    heap_init(config.kernel_heap_size);
    
    char *test1 = kmalloc(1);
    char *test2 = kmalloc(1);
    printf("Post alloc\n");
    for(uint32_t i = 0; i < 4096; i++){
        test1[i] = 0;
    }
    vfile_t *test = fopen("/dev/disk/ide0");
    printf("Test ptr: %x\n", test1);
    
    printf("Bleh\n");
    for(;;);
}
extern void kmain(kernel_info_t *kernel_info){
    config_init();
    serial_init();
    pm_init(kernel_info);
    ramfs_init();
    mlog("KERNEL", "Initializing IDT\n", MLOG_PRINT);
    idt_load();
    pic_init(0x20);
    pic_setmask(0x0, PIC1_DATA);
    pic_setmask(0x0, PIC2_DATA);
    // printf("Test start!\n");
    // kmalloc(0x4000);
    // printf("Test end!\n");
    vfs_init();
    fcreate("/tmp", FS_FILE_IS_DIR);
    fcreate("/dev", FS_FILE_IS_DIR);
    fcreate("/boot", FS_FILE_IS_DIR);
    fcreate("/dev/disk", FS_FILE_IS_DIR);
    mlog("KERNEL", "Initializing Scheduler & starting PID 1\n", MLOG_PRINT);
    boot_info = kernel_info;
    scheduler_init();
    // printf("Post init pre start\n");
    //scheduler doesn't work if there is no PID0, and I don't know why.
    thread_start(pid0);
    thread_start(sysinit);
    // for(;;);
    enable_interrupts();
    for(;;);//we actually **shouldn't** return, like, ever. That's bad.
    return;
}