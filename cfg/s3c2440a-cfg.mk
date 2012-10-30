

COMMON_FLAGS +=  -fmessage-length=0 -fno-short-enums
#COMMON_FLAGS +=  -ffunction-sections -fdata-sections
COMMON_FLAGS +=  -Wall
# 需要.lst文件时加入此参数
#COMMON_FLAGS += -Wa,-adhlns="$@.lst"

COMMON_FLAGS += -DALIGN_SIZE=8
COMMON_FLAGS += -DMEM_STATS
LDFLAGS += -Wl,--start-group -lgcc -Wl,--end-group
# remove unused sections: -Xlinker --gc-sections
LDFLAGS += -Xlinker --gc-sections
#print removed sections : -Xlinker --print-gc-sections
#LDFLAGS += -Xlinker --print-gc-sections

CONFIG_SMALL_MEMLIB := y
