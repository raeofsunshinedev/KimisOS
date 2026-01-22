# How to build this project

## To build and run in qemu:

simply run `make` That's it. It'll ask for your password to run `sudo mount image.bin`, to create a few directories (and eventually move in files that will be needed for installation/running). If you don't feel comfortable doing this through make, cause you don't trust the build script or something, you can run

`make tools && make bootloader && make kernel`

You'll then need to copy `bin/bootloader` to `image.bin` and resize the image. I suggest 64mb but its up to your discretion, as long as it's big enough. I usually do this by running `qemu-img resize -f raw image.bin 64M`.

Next, you'll need an `initrd`, named `initrd.rd`. Unless you don't want to be able to read and write to the disk, make sure to include `idm.elf` and `ifsm.elf` (or/and your own custom .elf formatted relocatable executable modules), as well as the config `initrc.conf` to be able to load these files. `initrd` will be mounted at the directory `/boot`, so any filenames must have `/boot` appended to them, if they are located on the `initrd`. The `initrd` MUST be a ustar-formatted image. My makefile assembles it using `tar --format=ustar -cf initrd.rd idm.elf ifsm.elf initrc.conf`.

Because linux mangles the names, and the BIOS bootloader does not support LFNs, or account for these mangled names, you'll need to run the custom tool I built to write the the disk image. This can be done by running `./diskwrite initrd.rd kernel.elf`. This will write the files to the filesystem in a way that does not mangle the names, granted the names are under 11 characters. If they are above 11 characters, the filenames will be truncated.

As a warning, there is no output to the screen currently, so if you would like to see what the kernel is doing, make sure to check the serial log, or include the option `-serial mon:stdio` if you are using qemu, to see it in the terminal. Please note that doing this way disables `ctrl+c` exiting, and you'll have to use `ctrl+a x` to exit.

running `make` will open `qemu`. If you'd like to build without opening `qemu` then use `make mount` instead. It'll ask for your password to mount the image to `./mount` to allow you to include any files that you won't need at boot time, like a shell, or application files.

<!-- followed by `cp bin/bootloader.bin image.bin`, `qemu-img resize -f raw image.bin 64M`, and `diskwrite initrd.rd kernel.elf -o image.bin`, which will then allow you to mount image.bin. Then, run `mk {mount}/mod {mount}/bin {mount}/sys` and copy and files that need to be copied into their respective folders (currently there are none other than `idm.elf`, `ifsm.elf`, and `kernel.elf`)

You MUST run the `diskwrite` tool BEFORE mounting, as it sets up the filesystem for the bootloader, and copies `idm.elf`, `ifsm.elf` and `kernel.elf` in a way that can be read by the bootloader.

To run, just use the command `qemu-system-i386 -hda image.bin {settings}`, with whatever settings you choose. I recommend using at least 32 megabytes of memory, and if you would like to see the kernel logs as they are writen to serial, with the option `-serial mon:stdio`

## To build, but NOT run in qemu

You could always just run `make` and close qemu immediately, or you could run `make mount`. It'll ask for your password again for the above reason, but you can just follow the above steps, minus running qemu if you don't feel comfortable letting the build script handle it. Again, it is imperitive that you run `diskwrite` as specified, or else the bootloader won't be able to find the files (as linux defaults to long file names, and my bootloader does not support them, and the short filenames are in a different style than expected (eg `kernel.elf` becomes `KERNEL  ELF`))
-->