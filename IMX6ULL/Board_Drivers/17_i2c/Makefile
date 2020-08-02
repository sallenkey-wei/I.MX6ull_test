CROSS_COMPILE 	?= arm-linux-gnueabihf-
TARGET 			?= ap3216c

CC 				:= $(CROSS_COMPILE)gcc
LD				:= $(CROSS_COMPILE)ld
OBJCOPY			:= $(CROSS_COMPILE)objcopy
OBJDUMP			:= $(CROSS_COMPILE)objdump
CFLAGS			:= 

LIBPATH			:= -lgcc -L /usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/lib/gcc/arm-linux-gnueabihf/4.9.4/

INCDIRS 		:= imx6ul       \
				   project      \
				   bsp/delay    \
				   bsp/led		\
				   bsp/beep		\
				   bsp/gpio     \
				   bsp/clk		\
				   bsp/exit		\
				   bsp/int		\
				   bsp/epit		\
				   bsp/key      \
				   bsp/keyfilter \
				   bsp/uart     \
				   bsp/lcd		\
				   bsp/rtc		\
				   bsp/i2c		\
				   bsp/ap3216c		\
				   stdio/include

SRC_DIRS        := imx6ul       \
				   project      \
				   bsp/delay    \
				   bsp/led		\
				   bsp/beep		\
				   bsp/gpio     \
				   bsp/clk		\
				   bsp/exit		\
				   bsp/int		\
				   bsp/epit		\
				   bsp/key		\
				   bsp/keyfilter \
				   bsp/uart      \
				   bsp/lcd		 \
				   bsp/rtc		 \
				   bsp/i2c		\
				   bsp/ap3216c		\
				   stdio/lib

CFLAGS			+= $(patsubst %, -I %, $(INCDIRS))

C_SRCS 			:= $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
S_SRCS         	:= $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.S))

C_OBJS   		:= $(patsubst %.c, objs/%.o, $(notdir $(C_SRCS)))
S_OBJS			:= $(patsubst %.S, objs/%.o, $(notdir $(S_SRCS)))
OBJS			:= $(C_OBJS) $(S_OBJS)

VPATH			:= $(SRC_DIRS)

$(TARGET).bin : $(OBJS)
	$(LD) -Timx6ul.lds -o $(TARGET).elf $^ $(LIBPATH)
	$(OBJCOPY) -O binary -S $(TARGET).elf $@
	$(OBJDUMP) -D -m arm $(TARGET).elf > $(TARGET).dis

$(C_OBJS) : objs/%.o : %.c
	$(CC) -Wall -nostdlib -fno-builtin -Wa,-mimplicit-it=thumb -c -O2 $(CFLAGS) -o $@ $<

$(S_OBJS) : objs/%.o : %.S
	$(CC) -Wall -nostdlib -c -O2 -o $@ $<

.PHONY : clean

clean:
	rm -rf objs/*.o $(TARGET).dis $(TARGET).elf $(TARGET).bin load.imx
