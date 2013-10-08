#
#  Name:    Makefile
#
#  Purpose: the make file of this project
#
#  Created By:         Lipeng<runangaozhong@163.com>
#  Created Date:       2013-10-05
#
#  ChangeList:
#  Created in 2013-10-05 by Lipeng;
#
#  This document and the information contained in it is confidential and 
#  proprietary to Unication Co., Ltd. The reproduction or disclosure, in 
#  whole or in part, to anyone outside of Unication Co., Ltd. without the 
#  written approval of the President of Unication Co., Ltd., under a 
#  Non-Disclosure Agreement, or to any employee of Unication Co., Ltd. who 
#  has not previously obtained written authorization for access from the 
#  individual responsible for the document, will have a significant 
#  detrimental effect on Unication Co., Ltd. and is expressly prohibited. 
#  
#
include ./config.mk

# Sources
SRCS = main.c stm32f4xx_it.c system_stm32f4xx.c syscalls.c mp3.c

# Audio
SRCS += Audio.c

# sonic
SRCS += sonic.c

# fft
SRCS += fft.c

# bpm
SRCS += bpm.c

# USB
SRCS += usbh_usr.c usb_bsp.c


CFLAGS += -std=gnu99 -Tstm32_flash.ld

###################################################

vpath %.c src
vpath %.a lib

ROOT=$(shell pwd)

# Includes
CFLAGS += -Iinc -Ilib/Core/cmsis -Ilib/Core/stm32
CFLAGS += -Ilib/Conf

# Library paths
LIBPATHS = -Llib/StdPeriph -Llib/USB_OTG
LIBPATHS += -Llib/USB_Host/Core -Llib/USB_Host/Class/MSC
LIBPATHS += -Llib/fat_fs
LIBPATHS += -Llib/helix

# Libraries to link
LIBS = -lm -lhelix -lfatfs -lstdperiph -lusbhostcore -lusbhostmsc -lusbcore

# Extra includes
CFLAGS += -Ilib/StdPeriph/inc
CFLAGS += -Ilib/USB_OTG/inc
CFLAGS += -Ilib/USB_Host/Core/inc
CFLAGS += -Ilib/USB_Host/Class/MSC/inc
CFLAGS += -Ilib/fat_fs/inc
CFLAGS += -Ilib/helix/pub

# add startup file to build
SRCS += lib/startup_stm32f4xx.s

OBJS = $(SRCS:.c=.o)

###################################################

.PHONY: lib proj

all: lib proj
	$(SIZE) $(OUTPATH)/$(PROJ_NAME).elf

lib:
	$(MAKE) -C lib FLOAT_TYPE=$(FLOAT_TYPE)

proj: 	$(OUTPATH)/$(PROJ_NAME).elf

$(OUTPATH)/$(PROJ_NAME).elf: $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBPATHS) $(LIBS)
	$(OBJCOPY) -O ihex $(OUTPATH)/$(PROJ_NAME).elf $(OUTPATH)/$(PROJ_NAME).hex
	$(OBJCOPY) -O binary $(OUTPATH)/$(PROJ_NAME).elf $(OUTPATH)/$(PROJ_NAME).bin
	$(OBJDUMP) -h -s -D $(OUTPATH)/$(PROJ_NAME).elf > $(OUTPATH)/dump.txt

clean:
	rm -f *.o
	rm -f $(OUTPATH)/$(PROJ_NAME).elf
	rm -f $(OUTPATH)/$(PROJ_NAME).hex
	rm -f $(OUTPATH)/$(PROJ_NAME).bin
	rm -f $(OUTPATH)/dump.txt
	$(MAKE) clean -C lib # Remove this line if you don't want to clean the libs as well
	
