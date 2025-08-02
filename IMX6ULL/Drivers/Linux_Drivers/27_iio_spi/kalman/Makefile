CROSS_COMPILE	?= arm-linux-gnueabihf-
TARGET		  	?= icm20608App	

CC			  	:= $(CROSS_COMPILE)gcc
LD				:= $(CROSS_COMPILE)ld

CFLAGS			:=

LD_FLAGS		:= ./libs/meschach.a -lm

INC_DIRS		:= 	include \
					project

SRC_DIRS		:=	include \
					project

CFLAGS			+= $(patsubst %, -I %, $(INC_DIRS))

SRCS			:= $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
OBJS			:= $(patsubst %.c, %.o, $(SRCS))

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(LD_FLAGS)

$(OBJS) : %o : %c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm $(TARGET) $(OBJS) -f


.PHONY : clean

print:
	echo $(CFLAGS)
	echo $(SRCS)
	echo $(OBJS)

