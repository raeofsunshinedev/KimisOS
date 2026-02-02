CFLAGS="-g -c -m32 -fno-pie -mno-sse -O3 -D __bits__=32 -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-incompatible-pointer-types -Wno-address-of-packed-member -Wno-discarded-qualifiers -fno-stack-protector -mno-red-zone -mno-sse -mno-sse2 -ffreestanding -nostdlib -mno-mmx"
LDFLAGS_i386="-Ttext 0xc0000000 --oformat binary -melf_i386"

cd src/kernel/
gcc kmain.c $CFLAGS -o ../../bin/kernel/kmain.o
for d in ./*/; do
    if [[ "$d" != "./obj/" ]]; then
        echo "\033[1;32mCompiling files in $d\033[0m"
        for f in $d*.c; do
            echo -e "\033[1;36mfile: $f\033[0m"
            SUF=".c"
            SUF_REM=${f%"$SUF"}
            END=${SUF_REM#"$d"}
            # doing this, '#' removes prefix, and '%' removes suffix
            gcc $f $CFLAGS -o ../../bin/kernel/$END.o
        done
    fi
done

cd ../../

# find -name bin/ *.o | xargs echo
# echo ../obj/*.o

# ld ../obj/kentry.o obj/*.o -T linker.ld -melf_i386 -o ../obj/kernel.bin 