CROSS_COMPILE 	?= arm-linux-gnueabihf-
TARGET 			?= clk

CC 				:= $(CROSS_COMPILE)gcc
LD				:= $(CROSS_COMPILE)ld
OBJCOPY			:= $(CROSS_COMPILE)objcopy
OBJDUMP			:= $(CROSS_COMPILE)objdump
CFLAGS			:= 

INCDIRS 		:= imx6ul       \
				   project      \
				   bsp/delay    \
				   bsp/led		\
				   bsp/beep		\
				   bsp/bsp_key  \
				   bsp/gpio     \
				   bsp/clk

SRC_DIRS        := imx6ul       \
				   project      \
				   bsp/delay    \
				   bsp/led		\
				   bsp/beep		\
				   bsp/bsp_key  \
				   bsp/gpio     \
				   bsp/clk

CFLAGS			+= $(patsubst %, -I %, $(INCDIRS))

C_SRCS 			:= $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
S_SRCS         	:= $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.S))

C_OBJS   		:= $(patsubst %.c, objs/%.o, $(notdir $(C_SRCS)))
S_OBJS			:= $(patsubst %.S, objs/%.o, $(notdir $(S_SRCS)))
OBJS			:= $(C_OBJS) $(S_OBJS)

VPATH			:= $(SRC_DIRS)

$(TARGET).bin : $(OBJS)
	$(LD) -Timx6ul.lds -o $(TARGET).elf $^
	$(OBJCOPY) -O binary -S $(TARGET).elf $@
	$(OBJDUMP) -D -m arm $(TARGET).elf > $(TARGET).dis

$(C_OBJS) : objs/%.o : %.c
	$(CC) -Wall -nostdlib -c -O2 $(CFLAGS) -o $@ $<

$(S_OBJS) : objs/%.o : %.S
	$(CC) -Wall -nostdlib -c -O2 -o $@ $<

.PHONY : clean

clean:
	rm -rf objs/*.o $(TARGET).dis $(TARGET).elf $(TARGET).bin load.imx
