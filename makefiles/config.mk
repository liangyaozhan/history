# -*- coding: gb2312; -*- 

# ָ������Ĵ���������:arm920t,arm1176jzf-s
# �ò����ڱ��������н��ԣ� -mcu=$(CPU)
# ��������������ָ����:make CPU=arm920t
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

# �汾����:debug ���� release. Ĭ����debug
# debug���������Ϣ,release ����������Ϣ
VERSION ?= debug
#VERSION ?= release

# Ĭ�Ͽ�����
DEFAULT_LIB_NAME:=librtk.a

# gnu������ǰ׺
CROSS_COMPILE =arm-none-eabi-
#CROSS_COMPILE=arm-rtk-eabi-

# ����SylixOS����Դ�빲�õ�ͷ�ļ�·��
# ע�⣺ ֻ��д��·���Ϳ��ԣ�������Ҫ��·��ǰ��"-I"��
# ����Ŀ¼$(RTK_BASE)��Ŀ¼makefiles���ڵ�Ŀ¼��
INC_PATH += .
INC_PATH += $(RTK_BASE)/include
INC_PATH += $(RTK_BASE)/bsp/$(BSP_NAME)

#INC_PATH += /home/lyzh/Sourcery_G++_Lite/lib/gcc/arm-none-eabi/4.5.1/include/
SYS_LIB_DIR := $(RTK_BASE)/output/$(CPU)$(VERSION)

# ����CԴ�ļ�ʱʹ��CFLAGS
# CFLAGS   += -DUEMF -DWEBS
# ����C++Դ�ļ�ʱʹ��CPPFLAGS
CPPFLAGS +=
# ����������Դ�ļ�ʱʹ��ASFLAGS
ASFLAGS  += 

# ����C/C++�ͻ���ļ����õ�FLAGS
#COMMON_FLAGS += -fomit-frame-pointer
COMMON_FLAGS += 

#COMMON_FLAGS +=  -fmessage-length=0
#COMMON_FLAGS +=  -ffunction-sections -fdata-sections
COMMON_FLAGS +=  -Wall
# ��Ҫ.lst�ļ�ʱ����˲���
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
