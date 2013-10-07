include ../../config.mk

###################################################

vpath %.c real

CFLAGS += -ffreestanding -nostdlib
CFLAGS += -Ireal -Ipub
	
SRCS = mp3dec.c mp3tabs.c bitstream.c buffers.c dct32.c dequant.c dqchan.c
SRCS += huffman.c hufftabs.c imdct.c polyphase.c scalfact.c
SRCS += stproc.c subband.c trigtabs_fixpt.c

OBJS = $(SRCS:.c=.o)

.PHONY: libhelix.a

all: libhelix.a

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^

libhelix.a: $(OBJS)
	$(AR) -r $@ $(OBJS)

clean:
	rm -f $(OBJS) libhelix.a
