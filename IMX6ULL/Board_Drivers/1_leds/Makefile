CC:=arm-linux-gnueabihf-gcc
LD:=arm-linux-gnueabihf-ld
COPY:=arm-linux-gnueabihf-objcopy
DUMP:=arm-linux-gnueabihf-objdump


led.bin:led.s
	$(CC) -g -c led.s -o led.o
	$(LD) -Ttext 0x87800000 led.o -o led.elf
	$(COPY) -O binary -S -g led.elf led.bin
	$(DUMP) -D led.elf > led.dis

clean:
	rm -rf *.o led.bin led.elf led.dis
