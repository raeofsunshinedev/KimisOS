CC := gcc
AS := nasm
CFLAGS:=-g -c -m32 -fno-pie -mno-sse -O3 -D __bits__=32 -Wall -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-incompatible-pointer-types -Wno-address-of-packed-member -Wno-discarded-qualifiers -fno-stack-protector -mno-red-zone -mno-sse -mno-sse2 -ffreestanding -nostdlib -mno-mmx
CFLAGS_MODULE:=-g -c -m32 -fpie -pie -mno-sse -O3 -D __bits__=32 -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-incompatible-pointer-types -Wno-address-of-packed-member -Wno-discarded-qualifiers -fno-stack-protector -mno-red-zone -mno-sse -mno-sse2 -ffreestanding -nostdlib -mno-mmx
BL_ASFLAGS := -f bin
LDFLAGS_MODULE:=-melf_i386 -e init --no-dynamic-linker -static -nostdlib
SRCS := $(wildcard src/kernel/*/*.c)
OBJS := $(patsubst src/kernel/%.c, bin/kernel/%.o, $(SRCS))

all: bootloader tools kernel
	cp bin/bootloader.bin image.bin
	qemu-img resize -f raw image.bin 64M
	tar --format=ustar -cf initrd.rd idm.elf ifsm.elf initrc.conf
# 	./diskwrite idm.elf ifsm.elf kernel.elf initrc.conf -o image.bin
	./diskwrite initrd.rd kernel.elf -o image.bin
	sudo mount image.bin mount
# 	sudo mkdir mount/mod # for kernel modules
# 	sudo mkdir mount/bin # for binary executables
# 	sudo mkdir mount/sys # for system files
# 	./diskwrite -v -o image.bin
	# ./diskwrite -v -l kernel.elf idm.elf -o image.bin
	qemu-system-i386 -hda image.bin -hdb test.bin --no-reboot --no-shutdown -m 32m -smp 2 -serial mon:stdio -D intlog.txt -d int
	sudo umount mount

tools:
	$(CC) tools/diskwrite.c -o diskwrite

mount: bootloader tools kernel
	cp bin/bootloader.bin image.bin
	qemu-img resize -f raw image.bin 64M
	tar --format=ustar -cf initrd.rd idm.elf ifsm.elf initrc.conf
	./diskwrite initrd.rd kernel.elf -o image.bin
	sudo mount image.bin mount
	@echo "\e[31mrun 'sudo umount mount' to unmount\e[0m"

bootloader:
	$(AS) src/bootloader/main.s $(BL_ASFLAGS) -o bin/bootloader.bin
run:
	qemu-system-i386 -hda image.bin -hdb test.bin --no-reboot --no-shutdown -m 32m -smp 2 -serial mon:stdio -D intlog.txt -d int
	
# kernel: $(OBJS)
kernel:
	nasm src/kernel/entry.s -o bin/kernel/entry.o -f elf32
	gcc src/kmodules/disk_driver.c $(CFLAGS_MODULE) -o bin/modules/disk_driver.o -m32
	gcc src/kmodules/fs_driver.c $(CFLAGS_MODULE) -o bin/modules/fs_driver.o -m32
	sh c_build_helper.sh
	nasm src/kernel/arch_i386/idt.s -o bin/kernel/idt.o -felf32
	# ld -T linker.ld bin/kernel/entry.o bin/kernel/*.o -melf_i386
	ld -T linker.ld bin/kernel/*.o -melf_i386 -o kernel.elf -static
	ld -T linker.ld -o kernel_interface.elf -r -R kernel.elf -melf_i386
	ld -pie bin/modules/disk_driver.o bin/kernel/string.o -o idm.elf -melf_i386 -e init --no-dynamic-linker -static -nostdlib
# 	ld bin/modules/fs_driver.o -o ifsm.elf -melf_i386
	ld -pie bin/modules/fs_driver.o bin/kernel/string.o -o ifsm.elf -melf_i386 -e init --no-dynamic-linker -static -nostdlib
# %.o: $(SRCS)
# 	mkdir -p bin/kernel/$(shell dirname $@)
# 	$(CC) $(CFLAGS) $< -o $@

init:
	mkdir mount
	mkdir bin
	mkdir bin/boot
	mkdir bin/kernel
	mkdir bin/modules
	touch test.bin
	qemu-img resize test.bin 64M

.PHONY: tools makefile src/

clean:
	rm diskwrite
	rm image.bin
	rm test.bin
	rm -rf bin/
	rm -rf mount/
	rm *.elf
	rm intlog.txt