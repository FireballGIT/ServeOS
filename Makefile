all: srv_os.img

srv_os.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > temp.bin
	truncate -s 1474560 temp.bin
	mv temp.bin srv_os.img

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel.bin: kernel_entry.o kernel.o
	ld -m i386pe -T linker.ld kernel_entry.o kernel.o -o kernel.tmp
	objcopy -O binary kernel.tmp kernel.bin
	rm kernel.tmp

kernel_entry.o: kernel_entry.asm
	nasm -f elf32 kernel_entry.asm -o kernel_entry.o

kernel.o: kernel.c
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -c kernel.c -o kernel.o

clean:
	rm -f *.bin *.o *.img *.tmp
