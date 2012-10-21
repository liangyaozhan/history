
TARGET:=rtk

INC_PATH += $(RTK_BASE)/bsp/stm32f10x/
INC_PATH += $(RTK_BASE)/bsp/stm32f10x/Libraries/CMSIS/CM3/CoreSupport
INC_PATH += $(RTK_BASE)/bsp/stm32f10x/Libraries/STM32F10x_StdPeriph_Driver/inc
INC_PATH += $(RTK_BASE)/bsp/stm32f10x/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x

LDFLAGS += -nostdlib -nostartfiles -Wl,-Map,$(LIB_DIR)/$(TARGET).map
LDFLAGS += -L$(SYS_LIB_DIR)
LDFLAGS += --entry Reset_Handler
LDFLAGS += -Wl,-u,g_pfnVectors
LDSCRIPT = $(RTK_BASE)/bsp/stm32f10x/stm32_rom.ld

LIB_DIR := $(RTK_BASE)/app/stm32-example2/output
OUTPUT_SUFFIX := app/stm32-example2/output/$(CPU)$(VERSION)

