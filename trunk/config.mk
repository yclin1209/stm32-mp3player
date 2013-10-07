#
#  Name:    config.mk
#
#  Purpose: the configuration file of this project
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

###################################################
# 				Project name
###################################################
PROJ_NAME=stm32_mp3player
OUTPATH=build

###################################################
#     		the compiler definitions
###################################################
BINPATH=/opt/arm-2011.09/bin/

CC=$(BINPATH)/arm-none-eabi-gcc
OBJCOPY=$(BINPATH)/arm-none-eabi-objcopy
OBJDUMP=$(BINPATH)/arm-none-eabi-objdump
SIZE=$(BINPATH)/arm-none-eabi-size

###################################################
# Check for valid float argument
# NOTE that you have to run make clan after
# changing these as hardfloat and softfloat are not
# binary compatible
###################################################
ifneq ($(FLOAT_TYPE), hard)
ifneq ($(FLOAT_TYPE), soft)
#override FLOAT_TYPE = hard
override FLOAT_TYPE = soft
endif
endif


###################################################
#     	compiler flags
###################################################
CFLAGS  = -std=gnu99 -g -O3 -Wall

CFLAGS += -mlittle-endian -mthumb -mthumb-interwork -nostartfiles -mcpu=cortex-m4

ifeq ($(FLOAT_TYPE), hard)
CFLAGS += -fsingle-precision-constant -Wdouble-promotion
CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
else
CFLAGS += -msoft-float
endif
