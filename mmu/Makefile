objs := mmu.o init.o main.o

mmu.bin : $(objs)
	/home/publish/mayue/gcc-3.4.5-glibc-2.3.6/bin/arm-linux-ld -Tmmu.lds -o mmu_elf $^
	/home/publish/mayue/gcc-3.4.5-glibc-2.3.6/bin/arm-linux-objcopy -O binary -S mmu_elf $@
	/home/publish/mayue/gcc-3.4.5-glibc-2.3.6/bin/arm-linux-objdump -D -m arm mmu_elf > mmu.dis
	
%.o:%.c
	/home/publish/mayue/gcc-3.4.5-glibc-2.3.6/bin/arm-linux-gcc -Wall -O2 -c -o $@ $<

%.o:%.s
	/home/publish/mayue/gcc-3.4.5-glibc-2.3.6/bin/arm-linux-gcc -Wall -O2 -c -o $@ $<

clean:
	rm -f mmu.bin mmu_elf mmu.dis *.o		
	
