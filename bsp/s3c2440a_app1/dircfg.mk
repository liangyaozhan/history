
TARGET:=rtk

LDFLAGS += -nostdlib -nostartfiles -Wl,-Map,$(LIB_DIR)/$(TARGET).map
LDFLAGS += -L$(SYS_LIB_DIR)
LDFLAGS += --entry _start
LDSCRIPT = /home/lyzh/ubuntu/lyzh/source/rtk/bsp/s3c2440a_app1/s3c2440a/ldscript.lds

LIB_DIR := ./output
OUTPUT_SUFFIX := bsp/s3c2440a_app1/output/$(CPU)$(VERSION)

