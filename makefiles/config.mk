# -*- coding: gb2312; -*- 

# 指定编译的处理器，如:arm920t,arm1176jzf-s
# 该参数在编译命令中将以： -mcu=$(CPU)
# 可以在命令行中指定如:make CPU=arm920t
# CPU     ?= arm926ej-s

#CPU     ?= arm1176jzf-s
#BSP_NAME ?= s3c6410x

CPU ?= arm920t
BSP_NAME ?= s3c2440a
ARMARCH  ?= armv4

#CPU     ?= arm926ej-s
#BSP_NAME ?= lpc3250
#ARMARCH  ?= armv4

#CPU     ?=cortex-m3
#BSP_NAME ?=stm32f10x
#ARMARCH  ?=armv7m

include $(RTK_BASE)/cfg/$(BSP_NAME)-cfg.mk

# 版本定义:debug 或者 release. 默认是debug
# debug版带调试信息,release 不带调试信息
VERSION ?= debug
#VERSION ?= release

# 默认库名字
DEFAULT_LIB_NAME:=librtk.a

# gnu工具链前缀
CROSS_COMPILE =arm-none-eabi-
#CROSS_COMPILE=arm-rtk-eabi-

# 编译SylixOS所有源码共用的头文件路径
# 注意： 只需写上路径就可以，而不需要在路径前加"-I"。
# 其中目录$(RTK_BASE)是目录makefiles所在的目录。
INC_PATH += .
INC_PATH += $(RTK_BASE)/include
INC_PATH += $(RTK_BASE)/bsp/$(BSP_NAME)

#INC_PATH += /home/lyzh/Sourcery_G++_Lite/lib/gcc/arm-none-eabi/4.5.1/include/
SYS_LIB_DIR := $(RTK_BASE)/output/$(CPU)$(VERSION)

# 编译C源文件时使用CFLAGS
# CFLAGS   += -DUEMF -DWEBS
# 编译C++源文件时使用CPPFLAGS
CPPFLAGS +=
# 编译汇编语言源文件时使用ASFLAGS
ASFLAGS  += 

# 编译C/C++和汇编文件共用的FLAGS
#COMMON_FLAGS += -fomit-frame-pointer
COMMON_FLAGS += 

#COMMON_FLAGS +=  -fmessage-length=0
#COMMON_FLAGS +=  -ffunction-sections -fdata-sections
COMMON_FLAGS +=  -Wall
# 需要.lst文件时加入此参数
#COMMON_FLAGS += -Wa,-adhlns="$@.lst"

LDFLAGS += -Wl,--start-group -lgcc -Wl,--end-group
# remove unused sections: -Xlinker --gc-sections
#LDFLAGS += -Xlinker --gc-sections
#print removed sections : -Xlinker --print-gc-sections
#LDFLAGS += -Xlinker --print-gc-sections
# Omit all symbo information (-s)

#LDFLAGS += -nostdlib -nostartfiles -Wl,-Map,$(LIB_DIR)/$(TARGET).map
#LDFLAGS += -L$(SYS_LIB_DIR)
#LDFLAGS += --entry Reset_Handler
#LDFLAGS += -Wl,-u,g_pfnVectors
