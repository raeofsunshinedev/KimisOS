# Filesystems

## VFS, RAMFS, and mounting

No matter the configuration, '/' is the root directory of the RAMFS, which is responsible for the actual 'filesystem' functionality of everything between '/' and any other mount point. Mount points themselves are just files within the RAMFS.

The RAMFS itself by default includes the following directories (but others can be created and deleted at runtime):

```

/
|--tmp/
|--dev/
|--boot/
|--dev/disk

```

`/tmp/` is for volatile temporary data that does not need to persist beyond reboot. Any files stored in `/tmp/` will be lost on shutdown.

`/dev/` is for any devices. For example, the PCI enumerator proceeds to create
`/dev/pci/` and related subfolders to represent the various PCI busses that have been exposed.

`/boot` is where the `initrd.rd` is mounted. It contains boot time modules, like a disk driver to read from the boot disk, a filesystem module to mount the boot partition, and a partition manager to actually read and create virtual devices to represent the partitions stored on the disks. 

##  Opening, closing, and other file operations.

All files are expected to be represented as `vfile_t`. A definition of `vfile_t` is provided in the module header. Because of this expectation, and the lack of core kernel functionality. Filesystem modules are responsible for translating their native inode or file representation into `vfile_t`.

When a function is called, the VFS will delegate functionality to the file operations stored in `vfile.fileops`. If a file operation is not supported, it is expected to be represented by `0`.

There are only a few expectations about the implementation of the translation between native file representation and `vfile_t`:

- fileops struct must be set to the proper functions. Failure to do so will render the module useless.
- file.size must be set to the size of the file, in bytes.
- Directory sizes is should always be defined as `entry_count * sizeof(vfile_t)`, and does not necessarily correspond to on-disk representation.
- file.minimum_rw_size is the minimum number of bytes that may be read or written atomically. Reads should allocate at least this amount of backing storage. Writes smaller than this value may overwrite or zero adjacent data depending on the underlying device.
- A module may not unmount, nor delete a file if file.refcount is > 0; Doing so is considered a bug, and may lead to oprhaned file pointers.
- modules should increment file.refcount upon opening, and decrement upon closing.
- `fread()` and `fwrite()` should always return the size read, in bytes.
- calling `fwrite()` on a directory should always fail.
- Reading a directory returns a packed array of vfile_t structures. Partial entries must never be returned. If the requested size would end in the middle of an entry, the returned byte count shall be rounded down to the nearest multiple of sizeof(vfile_t).
