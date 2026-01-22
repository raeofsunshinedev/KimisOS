# Kimi's Bootloader Documentation

## Basic Information

Kimi's Bootloader is a custom booting solution and the default bootloader for Kimi's OS on legacy BIOS systems. It loads the default x86 GDT to be used by the Kernel, and maps the kernel to any address as specified in it's ELF program header.

The bootloader also sets the video mode to the closest mode present to Standard definition, and obtains the BIOS memory map to be passed to the Kernel. 

### Features

It's not very feature rich, cause it's whole purpose was to do the bare minimum for what i needed for my own project. While I've made bootloaders before, they've typically only consisted of some code that gets a memory map, sets up a gdt, and says "screw it, i'll load these next 64 sectors, why not?" however i wanted to do everything a bit better, make things easier on me in the long run, and now i have a bootloader that does the same thing but

- searches and loads any file named "kernel.elf" into memory (only supports legacy filenames for now, LFNs to be supported at a later date

	(note: this only works in the root directory, if not found, the bootloader will simply hang and spit out some error code
- parses the ELF format for information on how to accurately map the Kernel in virtual memory

	(note: this only really works if segments are page-aligned. TBH even once I have C code working if I ever want to try and work with non-page-aligned ELF files...)

- Gathers a map of memory to pass to the loaded kernel

#### What I want to add

- Recursive directory searching

	I want to be able to load the kernel file from another directory, just to keep things more organized. Like booting from some file path like `/sys/kernel32.elf` instead. This kind of goes hand in hand with implementing LFNs, but i do NOT want to deal with those right now

- Config files
	
	Yeah, it's cool and all being able to load a file and properly map everything. Hell, someone could even theoretically write their own kernel and replace the kernel.elf file and it'd work fine upon restart, but why do that when I can be even more convoluted and choose to boot Linux from my glorious homebrew bootloader (theoretically)

- Module loading

	<!-- Since i want to have a modular kernel, I'll need to somehow load modules for disks and filesystems at boot time. While I would do this now, I don't have any modules to boot, nor a kernel to use said modules, so I'm going to hold off on this until I'm a bit into my kernel and start needing actual modules. (It's going to be So Fun making this work. Maybe I'll just load the file and have the kernel take care of symbol patching?) this also requires reading and parsing config files, which I am not ready to start working on... Yet. -->
	Currently, the bootloader only supports loading a second file alongside the kernel, named `initrd.rd` It is up to the person using the bootloader to either include `initrd.rd` within their filesystem, to include necesary files within their `initrd` and the kernel to accurately handle the `initrd` as needed. For Kimi's OS, this is done by making the `initrd` a tarball in ustar format containing the `initrc`, `ifsm.elf` and `idm.elf`. 

- Multiboot support

	Yeah passing information from a non-standard struct to my kernel is nice and all, but you know what else is nice? allowing my kernel to actually use other bootloaders other than those designed specifically for my kernel. I could try to detect whether or not i'm using my own struct or a multiboot struct, but why do that when I can just support one?

- Selecting video modes that most closely align with monitor's real size

	This would be great to have, and while I have implemented it in a previous bootloader, I don't understand the code anymore and am not comfortable just copy-pasting the entire thing and praying that it works somehow.

- Selecting better load/copy addresses

	On loading the kernel file, I just pick some arbitrary, pre-chosen spot and say "yeah, this is good enough", but in reality i have on average only 400-ish kb until i start to overwrite the EBDA (which is bad, i do enjoy having ACPI). Instead, I want to implement a function to actually look for a continuous point in memory that is large enough to hold the entire kernel (ELF headers and all) without accidentally overwriting important information or running into memory barriers/holes.
	
	Edit (11/21/24) I actually added this a *while* ago, but never updated the docs cause I'm forgetful

### A20 Line

Due to the fact that many x86 PCs POST without enabling the A20 line, the bootloader attempts to enable it through 3 distinct methods, starting with 

1. Enabling through BIOS

Kimi's Bootloader will first attempt to enable the A20 line via the BIOS before attempting any other method of enabling the A20 line. This method is the least likely to have any unintended side effects, which is why it is the first of the three methods to be attempted

2. Enabling via Fast A20 method

The bootloader then attempts to enable the A20 by using the fast A20 method

3. Enabling through keyboard

If all else fails, the bootloader will attempt to enable the a20 line by using the legacy way of enabling the A20 line - by using Intel's 8042 Keyboard controller. This is the most likely to succeed on any chipsets with a PS/2 controller still present, but will fail if one is not. 

### GDT

While any GDT loaded for the Kernel will work, once the Kernel wants to load user processes, it will fail to locate user segments in the GDT if the one loaded is not the one expected. Because of this, the Kernel will need to load it's own if the one it wants is not loaded by this bootloader. To allow for detection, the key "0xaa55" is written at the end of the GDT, marking the last segment invalid, but allowing for the detection of the correct GDT being pre-loaded. This detection will only occurr during boot time, and unexpected writes will cause the Kernel to crash.

#### GDT Layout:

|INDEX|ENTRY TYPE|
|-----|----------|
|0x00 |Null Entry|
|0x08 |Kernel Code (16)|
|0x10 |Kernel Code (32)|
|0x18 |Kernel Data (32)|
|0x20 |User Code (32)  |
|0x28 |User Data (32)  |
|0x30 |TSS			   |

### Error Codes

|Error Code|Description|
|----------|-----------|
|1         |A20 Line could not be enabled|
|2|Failed to read from disk|
|3|Corrupted/Invalid ELF formatted file|
|4|Failed to find suitable location in memory for kernel|
|5|Failed to locate kernel file|