CC:=arm-linux-gnueabihf-gcc
LD:=arm-linux-gnueabihf-ld
COPY:=arm-linux-gnueabihf-objcopy
DUMP:=arm-linux-gnueabihf-objdump
objs := start.o main.o

ledc.bin:$(objs)
	$(LD) -Timx6ull.lds -o ledc.elf $^
	$(COPY) -O binary -S ledc.elf $@
	$(DUMP) -D -m arm ledc.elf > ledc.dis

%.o:%.s
	$(CC) -Wall -nostdlib -c -o $@ $<

%.o:%.S
	$(CC) -Wall -nostdlib -c -o $@ $<

%.o:%.c
	$(CC) -Wall -nostdlib -c -o $@ $<

clean:
	rm  -rf $(objs) ledc.elf ledc.dis ledc.bin