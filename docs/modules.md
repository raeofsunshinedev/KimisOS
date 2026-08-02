# Kernel Module API

## Overview

The kernel is composed of independent loadable modules.

Modules communicate through:

- The kernel module API
- `vfile_t` objects
- The kernel message dispatcher

Modules must not directly reference other modules. Hardware drivers, partition managers, and filesystem implementations communicate indirectly through kernel-managed interfaces.

Typical device flow:

```text
PCI Enumerator
      │
      ▼
Storage Driver
      │
      ├── Detects hardware
      ├── Creates a vfile_t device object
      └── Sends MESSAGE_DEVICE_ADD
                    │
                    ▼
Partition Manager
      │
      ├── Reads partition table
      ├── Creates partition vfile_t objects
      └── Exposes partitions through VFS
```

---

# Module Binary Requirements

All modules must be compiled as position-independent executables. Static module loading is not currently supported.

Because modules are relocated at load time, absolute addresses cannot be assumed during compilation.

Function pointers must not be assigned in global structures at compile time.

Incorrect:

```c
fileops_t disk_ops = {
    0,
    0,
    disk_write,
    disk_read,
    0,
    0
};
```

The function addresses may not be valid after relocation.

Correct:

```c
fileops_t disk_ops;

void init(KOS_MAPI_FP api, uint32_t version) {
    disk_ops.write = disk_write;
    disk_ops.read = disk_read;
}
```

This applies to all structures containing function pointers:

- `module_t`
- `fileops_t`
- Driver callback tables

Initialize these fields during `init()`.

---

# Module Lifecycle

Every module provides an initialization function:

```c
void init(KOS_MAPI_FP api, uint32_t api_version);
```

The kernel passes a function pointer providing access to the module API.

```c
typedef uint32_t (*KOS_MAPI_FP)(uint32_t function, ...);
```

Modules should store this pointer.

Example:

```c
static KOS_MAPI_FP api;

void init(KOS_MAPI_FP module_api, uint32_t version) {
    api = module_api;
}
```

Modules may provide:

```c
void fini(void);
```

which is called before unloading.

---

# Module Registration

Modules describe themselves using `module_t`.

Example:

```c
module_t module_data = {
    0,
    MODULE_ID, // For use by the driver itself. Will probably be used for dependency tracking eventually.
    "example", // Limited to 16 ASCII characters (16 bytes). Any more will corrupt the module_data structure.
    0,
    0,
    0,
};
```

Function pointers must be assigned during initialization.

```c
void init(KOS_MAPI_FP api, uint32_t version) {
    module_data.init_entry = init;
    module_data.fini = fini;

    api(MODULE_API_REGISTER,
        &module_data);
}
```

The kernel assigns a unique module key during registration.

The key is required when registering resources owned by the module.

---

# Virtual Files

All kernel-visible objects are represented by `vfile_t`.

Examples:

- Regular files
- Directories
- Block devices
- Partitions
- Mounted filesystems
- Pipes
- Links

```c
typedef struct virtual_file {
    char name[212];

    uint16_t flags;
    fileops_t *fileops;

    uint32_t refcount;

    uint32_t id;
    void *private;

    uint32_t size;
    uint32_t offset;

    uint16_t minimum_rw_size;
} vfile_t;
```

## Driver-Owned Fields

The kernel does not assign meaning to:

```c
uint32_t id;
uint32_t offset;
void *private;
```

These fields are owned by drivers and modules.

Common uses:

```text
id
 |
 +-- device index
 +-- controller index
 +-- module object index

offset
 |
 +-- physical offset
 +-- filesystem offset
 +-- driver-specific position

private
 |
 +-- pointer to driver state
 +-- pointer to filesystem metadata
 +-- pointer into memory-backed storage
 +-- pointer to module-specific structures
```

Modules should use these fields consistently with their names when possible, but the kernel does not enforce any meaning.

---

# File Operations

`vfile_t` operations are provided through `fileops_t`.

```c
typedef struct fileops {
    struct virtual_file *(*create)(...);
    int (*delete)(...);
    int (*write)(...);
    int (*read)(...);
    void (*close)(...);
    struct virtual_file *(*rfopen)(...);
} fileops_t;
```

Unused operations should be `NULL`.

Example initialization:

```c
fileops_t ops;

void init(...) {
    ops.write = device_write;
    ops.read = device_read;
}
```

---

# Device Creation

Hardware drivers expose devices by creating `vfile_t` objects.

Typical flow:

```text
Detect hardware
      │
      ▼
Create vfile_t
      │
      ▼
Assign file operations
      │
      ▼
Set driver metadata
      │
      ▼
MESSAGE_DEVICE_ADD
```

Example:

```c
vfile_t *device =
    fcreate(api,
            "/dev/disk/ide0",
            FS_FILE_SYSTEM);

device->fileops = &ide_fileops;
device->minimum_rw_size = 512;
device->id = drive_index;

api(MODULE_API_DISPATCH_MESSAGE,
    MESSAGE_DEVICE_ADD,
    device);
```

The driver does not call the partition manager directly.

The partition manager receives the device notification and creates additional `vfile_t` objects representing partitions.

---

# Message Handlers

Modules may receive kernel messages.

Register a handler:

```c
api(MODULE_MESSAGE_HANDLER,
    module.key,
    message_handler);
```

Handler format:

```c
int32_t handler(uint32_t message, ...);
```

Supported messages:

| Message | Arguments |
|---------|-----------|
| `MESSAGE_DEVICE_ADD` | `vfile_t *` |
| `MESSAGE_DEVICE_REMOVE` | `vfile_t *` |
| `MESSAGE_PARTITION_DETECT` | `vfile_t *` |
| `MESSAGE_PARTITION_REFRESH` | Reserved |
| `MESSAGE_MOUNT_FS` | `vfile_t *, char *, uint32_t` |
| `MESSAGE_UNMOUNT_FS` | `vfile_t *` |

Returning a value greater than or equal to zero indicates that the message was handled.

Returning a negative value allows other modules to process it.

---

# File API

Open a virtual file:

```c
vfile_t *file =
    (vfile_t *)api(MODULE_API_OPEN,
                   "/dev/disk/ide0");
```

Create a virtual file:

```c
vfile_t *file =
    (vfile_t *)api(MODULE_API_CREAT,
                   "/dev/example",
                   FS_FILE_SYSTEM);
```

Read:

```c
api(MODULE_API_READ,
    file,
    buffer,
    offset,
    count);
```

Write:

```c
api(MODULE_API_WRITE,
    file,
    buffer,
    offset,
    count);
```

Offsets and sizes are measured in bytes.

---

# Block Devices

Block devices must provide:

```c
vfile_t->minimum_rw_size
```

This describes the smallest valid read/write unit.

Example:

```c
disk->minimum_rw_size = 512;
```

Consumers must not assume a fixed sector size.

Example:

The partition manager module uses this value when converting partition LBAs into byte offsets.

```c
byte_offset =
    start_lba * parent->minimum_rw_size;
```

---

# Memory API

Allocate kernel memory (size allocated is in 4096-byte pages):

```c
void *ptr =
    (void *)api(MODULE_API_MALLOC,
                pages);
```

Free memory:

```c
api(MODULE_API_FREE,
    ptr);
```

Allocate 64 KiB-aligned physical memory:

```c
uint32_t address =
    api(MODULE_API_PMALLOC64K);
```

Map physical memory at a virtual address:

```c
api(MODULE_API_KMALLOC_PADDR,
    physical_address,
    size);
```

---

# Virtual Memory

Map memory:

```c
api(MODULE_API_MAP,
    virtual,
    physical,
    flags);
```

Unmap:

```c
api(MODULE_API_UNMAP,
    virtual);
```

Get physical address:

This only returns the start of the physical page.

```c
api(MODULE_API_PADDR,
    virtual);
```

---

# Interrupts

Register an interrupt handler:

```c
api(MODULE_API_ADDINT,
    irq,
    module.key,
    handler);
```

Remove an interrupt handler:

```c
api(MODULE_API_DELINT,
    irq,
    module.key);
```

Currently supported IRQ range:

```text
0-15
```

---

# Scheduler API

Block a process:

```c
api(MODULE_API_BLOCK_PID,
    pid);
```

Unblock:

```c
api(MODULE_API_UNBLOCK_PID,
    pid);
```

Get current process:

```c
uint32_t pid =
    api(MODULE_API_GET_CPID);
```

Check interrupt context:

```c
uint32_t irq =
    api(MODULE_API_IS_INTERRUPT);
```

---

# Logging

Print to the kernel log:

```c
api(MODULE_API_PRINT,
    MODULE_NAME,
    "Detected drive %u\n",
    id);
```

---

# Filesystem Flags

| Flag | Meaning |
|------|---------|
| `FS_FILE_READ_ONLY` | Read-only object |
| `FS_FILE_HIDDEN` | Hidden object |
| `FS_FILE_SYSTEM` | System object |
| `FS_FILE_IS_DIR` | Directory |
| `FS_FILE_ARCHIVE` | Archive |
| `FS_FILE_PIPE` | Pipe |
| `FS_FILE_LINK` | Link |
| `FS_FILE_MOUNT` | Physical device or mounted filesystem |

---

# Example: Storage Driver

A storage driver detects a disk:

```c
vfile_t *disk =
    fcreate(api,
            "/dev/disk/ide0",
            FS_FILE_SYSTEM);

disk->fileops = &ide_fileops;
disk->minimum_rw_size = 512;
disk->id = drive_id;

api(MODULE_API_DISPATCH_MESSAGE,
    MESSAGE_DEVICE_ADD,
    disk);
```

The partition manager receives:

```c
MESSAGE_DEVICE_ADD
```

and creates partition objects:

```text
/dev/disk/ide0
/dev/disk/ide0p0
/dev/disk/ide0p1
```

Each partition is another `vfile_t` object with its own operations.

Modules interact through these shared kernel objects rather than direct dependencies.