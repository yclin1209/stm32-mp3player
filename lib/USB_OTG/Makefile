include ../../config.mk

###################################################

vpath %.c src

CFLAGS += -ffreestanding -nostdlib

# Includes
CFLAGS += -Iinc -I../Core/cmsis -I../Core/stm32
CFLAGS += -I../Conf

# Sources
SRCS = usb_core.c usb_dcd.c usb_dcd_int.c usb_hcd.c
SRCS += usb_hcd_int.c usb_otg.c

OBJS = $(SRCS:.c=.o)
LIBNAME = libusbcore.a

all: $(LIBNAME)

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(LIBNAME): $(OBJS)
	$(AR) -r $@ $(OBJS)

clean:
	rm -f $(OBJS) $(LIBNAME)
	
