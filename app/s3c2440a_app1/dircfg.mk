
TARGET:=rtk

LDFLAGS += -nostdlib -nostartfiles -Wl,-Map,$(LIB_DIR)/$(TARGET).map
LDFLAGS += -L$(SYS_LIB_DIR)
LDFLAGS += --entry _start
LDSCRIPT = $(RTK_BASE)/bsp/$(BSP_NAME)/ldscript.lds

LIB_DIR := $(RTK_BASE)/app/s3c2440a_app1/output
OUTPUT_SUFFIX := app/s3c2440a_app1/output/$(CPU)$(VERSION)

