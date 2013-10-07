include ../../config.mk

vpath %.c src
vpath %.c src/option

CFLAGS += -ffreestanding -nostdlib

# Includes
CFLAGS += -I../Core/cmsis -I../Core/stm32
CFLAGS += -Iinc -I../Conf

# Sources
SRCS = ff.c fattime.c ccsbcs.c

OBJS = $(SRCS:.c=.o)
LIBNAME = libfatfs.a

all: $(LIBNAME)

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(LIBNAME): $(OBJS)
	$(AR) -r $@ $(OBJS)

clean:
	rm -f $(OBJS) $(LIBNAME)
	
