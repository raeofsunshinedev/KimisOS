# Kernel Modules

Kernel modules are special pieces of software loaded into kernelspace, and persisting along with the kernel among all processes. These can be loaded at both runtime and during boot time. Boot time modules **must** be contained within the initrd, or be loaded by the `initrc` by a mounted filesystem.

## The API

When a module starts, two arguments are passed to it: The API's function pointer, and the version number. It is suggested that modules check the version number to make sure they are compatible with the API. Currently, all modules should check to make sure that the version number is `0`. Any number other than zero should be assumed incompatible, and the module should exit.

It is expected that every module calls `MODULE_API_REGISTER`, to obtain a `key`, which is required for certain operations, or making sure that the correct information is assigned to the correct module.

### API Functions

<!-- 
enum MODULE_API_FUNCS{
    
    MODULE_API_ADDFUNC,//adds function to kernel API handler
    MODULE_API_REGISTER, //registers the module as active using information provided from the module
    MODULE_API_DELFUNC, //delete function from kernel API handler
    MODULE_API_ADDINT, //set interrupt handler
    MODULE_API_DELINT, //delete interrupt handler
    MODULE_API_PRINT, //print to terminal
    MODULE_API_READ, //read from virtual file
    MODULE_API_WRITE, //write to virtual file
    MODULE_API_CREAT, //create a virtual file and assigns it to the the proper module (requires having a read and write function passed)
    MODULE_API_DELET, //delete a virtual file
    MODULE_API_OPEN,
    MODULE_API_MAP, //map physical address to virtual address
    MODULE_API_UNMAP, //unmap physical address to virtual address
    MODULE_API_PADDR, //get physical address of memory
    MODULE_API_MALLOC, //allocate memory in 4kb blocks
    MODULE_API_FREE, //free memory allocated by malloc
    MODULE_API_PMALLOC64K,
    MODULE_API_KMALLOC_PADDR,
    MODULE_MESSAGE_HANDLER,
};
-->
#### MODULE_API_ADDFUNC

Unused. Treated as no-op

#### MODULE_API_REGISTER

Arguments: `module_t *structure`

Obtains a key, returned in the `structure` structure. Structure argument shall reference a variable within the module of type `module_t`. Returns -1 if `structure` is null. Returns 0 on success

#### MODULE_API_ADDINT

Arguments: `uint32_t interrupt_index, uint32_t key, void (*interrupt_handler)(register_t* registers)`

Registers an interrupt handler to handle the IRQ corresponding with `interrupt_index`. Returns -1 if `interrupt_index` is above or equal to 16 or below 0. Returns `0` on success.

#### MODULE_API_DELINT

Arguments: `uint32_t interrupt_index, uint32_t key`

Deletes interrupt handler for IRQ `interrupt_index`. Returns -1 if the module does not own the interrupt handler.

#### MODULE_API_PRINT

Arguments: `char *name, char* string, ...`

Prints a formatted string attributed to `name`.

#### MODULE_API_READ

Arguments: `vfile_t *file, char *buffer, uint32_t offset, uint32_t count`

Reads `count` bytes from `file` into `buffer` starting at `offset`. Returns number of bytes read.

#### MODULE_API_WRITE

Arguments: `vfile_t *file, char *buffer, uint32_t offset, uint32_t count`

Writes `count` bytes from `buffer` into `file` starting at `offset`. Returns number of bytes written.

#### MODULE_API_CREAT

Arguments: `char *name, VFILE_TYPE ftype, void *arg1, void *arg2`

Creates a `file` using the arguments `arg1` and `arg2`.

If ftype is either of the following: `VFILE_POINTER`, `VFILE_DIRECTORY`, `VFILE_MOUNT`, or `VFILE_SYMLINK`,

`arg1` corresponds to a pointer to data, `arg2` is unused.

If ftype is either of the following: `VFILE_FILE`, or `VFILE_DEVICE`

`arg1` corresponds to the `read` function, and `arg2` corresponds to the `write` function

Returns the pointer to the new file in memory.

#### MODULE_API_DELET

Not implemented, yet. Always returns -1.

#### MODULE_API_OPEN

Arguments: `char *name`

Returns pointer to file in memory. If file does not exist, returns `NULL`

#### MODULE_API_MAP

Arguments: `void *vaddr, void *paddr, uint32_t flags`

Maps physical address `paddr` to virtual address `vaddr`, Returns 0. 

`flags` corresponds to the [page flags](https://osdev.wiki/wiki/paging).

#### MODULE_API_UNMAP

Arguments: `void *addr`

Unmaps `addr` from memory. Accesses to this memory after calling `MODULE_API_UNMAP` will result in a page fault.

#### MODULE_API_PADDR

Arguments: `void *addr`

Returns the physical address associated with the address `addr`.

#### MODULE_API_MALLOC

Arguments: `uint32_t sz_pages`

Allocates `sz_pages` pages of contiguous virtual memory. Returns pointer to the newly allocated memory.

#### MODULE_API_FREE

Arguments: `void *addr`

Deallocates memory in `addr`. Returns `NULL`

#### MODULE_API_PMALLOC64K

No Arguments.

Allocates 64kb of contiguous physical memory and returns the physical address.

#### MODULE_API_KMALLOC_PADDR

Arguments: `void *paddr, uint32_t size`

Allocates `size` pages starting at physical address `paddr`. Returns virtual address.

#### MODULE_API_MESSAGE_HANDLER 

Arguments: `uint32_t key, void (*handler)(uint32_t message, ...)`

Assigns and marks valid message handler to recieve messages from the kernel to kernel modules. Returns -1 if the key is invalid, otherwise, returns 0. No messages have been implemented, but a will update with a table of messages as necessary.

If a message handler cannot handle a message, it is required that the message handler returns -1.
