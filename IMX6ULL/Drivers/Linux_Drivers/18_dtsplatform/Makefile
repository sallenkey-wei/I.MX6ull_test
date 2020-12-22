KERNELDIR := /home/sallenkey/linux/IMX6ULL/linux/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek/
CURRENT_PATH := $(shell pwd)
CC := arm-linux-gnueabihf-gcc

obj-m := dtsled.o

APP_SRCS := ledApp.c

APP := $(patsubst %.c, %, $(APP_SRCS))

build: kernel_modules $(APP)

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

$(APP):$(APP_SRCS)
	$(CC) -o $@ $^
	

clean:
	rm $(APP) -f
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
