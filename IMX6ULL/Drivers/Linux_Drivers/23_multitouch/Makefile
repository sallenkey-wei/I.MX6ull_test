KERNELDIR := /home/sallenkey/linux/IMX6ULL/linux/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek/
CURRENT_PATH := $(shell pwd)
CC := arm-linux-gnueabihf-gcc
MODULE_NAME := ft5x06
MODULE_SRCS :=
MODULE_OBJS := $(patsubst %.c, %.o, $(MODULE_SRCS))


obj-m := $(MODULE_NAME).o

$(MODULE_NAME)-objs := $(MODULE_OBJS)

build: kernel_modules $(APP)


kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules




APP_SRCS := ft5x06App.c

APP := $(patsubst %.c, %, $(APP_SRCS))

$(APP):$(APP_SRCS)
	$(CC) -o $@ $^
	

clean:
	rm $(APP) -f
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
