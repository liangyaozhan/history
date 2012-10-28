# -*- coding: gb2312; -*- 
#########################################################################################################
## 文   件   名: rules.mk
##
## 创   建   人: liangyaozhan
##
## 版        本: 2.0
##
## 更  新 日  期:   Thu Apr 26 15:20:26 HKT 2012
##                编译时，使用源码全路径，这样才能在eclipse跳转到错误或者警告处。
##                为了支持make -j选项， 编译完成后再一次性链接，而且使用xargs来避免命令行参数太长的问题。
##                不同的库的目标文件放在不同的地方。
##
## 描        述: 此文件包含了许多规则，包括：编译C/C++和汇编源码文件、自动生存依赖关系、递归执行所有目录、
##              确保子目录下的Makefile存在、安装子目录的Makefile文件和清理。
##              提供的目标有：
##              make install_makefile 或者 make install_Makefile   : 安装与配置Makefile
##              make clean                                         : 执行当前目录和子目录的清理
##              make distclean                                     : 任何目录make 此目标都清理整个工程
##              make write_objs_to_makefile                        : 把目标文件列表写入Makefile，而不是在
##                                                                 编译时才用通配符查找。
## 
#########################################################################################################

ifeq ($(CPU),)
$(error "missing definition of CPU. for example: make CPU=arm1176jzf-s VERSION=release")
endif
ifeq ($(VERSION),)
$(error "missing definition of VERSION. for example: make CPU=arm1176jzf-s VERSION=release")
endif
export CPU

ifeq ($(findstring --no-print-directory,$(MAKE)),)
MAKE += --no-print-directory
endif
export MAKE

#编译器名字定义
CC       = $(CROSS_COMPILE)gcc
CPP      = $(CROSS_COMPILE)g++
AR       = $(CROSS_COMPILE)ar
LD       = $(CROSS_COMPILE)ld
OBJCOPY  = $(CROSS_COMPILE)objcopy
OBJDUMP  = $(CROSS_COMPILE)objdump
NM       = $(CROSS_COMPILE)nm
SIZE       = $(CROSS_COMPILE)size

# C/C++ asm都使用的编译参数:COMMON_FLAGS
COMMON_FLAGS += $(addprefix -I,$(INC_PATH))
COMMON_FLAGS += -mcpu=$(CPU)

# c flags
CFLAGS += $(COMMON_FLAGS)
# cpp flags
CPPFLAGS += $(COMMON_FLAGS)
# assembler flags
ASFLAGS += -x assembler-with-cpp 
ASFLAGS += -D__ASSEMBLY__
ASFLAGS += $(COMMON_FLAGS)

# AR flags
ARFLAGS = crvu

# 版本定义:debug 或者 release. 默认是debug
# debug版带调试信息,release 不带调试信息
#VERSION ?= debug

# 如果没有定义子目录，则使用所有子目录
ALL_SUB_PATH ?= $(wildcard */)
EXCLUDE_SUB_PATH1:=$(addsuffix /,$(EXCLUDE_SUB_PATH))
override ALL_SUB_PATH := $(filter-out $(EXCLUDE_SUB_PATH) $(EXCLUDE_SUB_PATH1),$(ALL_SUB_PATH))

# 缺省的库名字为
MODULE_NAME ?= $(DEFAULT_LIB_NAME)

# 定义位置：源码目录、目标文件目录和库目录
SRC_DIR  := $(shell pwd)
OUTPUT_SUFFIX ?=output/$(CPU)$(VERSION)
OBJ_DIR  := $(subst $(RTK_BASE),$(RTK_BASE)/$(OUTPUT_SUFFIX)/obj-$(MODULE_NAME:.a=),$(SRC_DIR))

LIB_DIR  := $(RTK_BASE)/$(OUTPUT_SUFFIX)
RELATIVE_DIR :=  $(patsubst $(RTK_BASE)/%,%,$(SRC_DIR))

COBJS += $(COBJS-y) 
COBJS += $(COBJS-yes)

CPPOBJS += $(CPPOBJS-y) 
CPPOBJS += $(CPPOBJS-yes)

SOBJS += $(SOBJS-y) 
SOBJS += $(SOBJS-yes)

# 带路径的目标文件(*.o)
COBJS_P   := $(addprefix $(OBJ_DIR)/,$(COBJS) )
CPPOBJS_P := $(addprefix $(OBJ_DIR)/,$(CPPOBJS) )
SOBJS_P   := $(addprefix $(OBJ_DIR)/,$(SOBJS) )

# 选择是否带调试信息
ifeq ($(VERSION),debug)
CFLAGS   += -DDEBUG=1 -g -gdwarf-2 -O0 
CPPFLAGS += -DDEBUG=1 -g -gdwarf-2 -O0 
ASFLAGS  += -DDEBUG=1 -g -gdwarf-2 -O0 
else
CFLAGS   += -DNDEBUG -Os
CPPFLAGS += -DNDEBUG -Os
ASFLAGS  += -DNDEBUG -Os
#LDFLAGS  += -s
endif


# 先取消再定义查找各种类型文件的目录
vpath %.c
vpath %.cpp
vpath %.S
vpath %.d
vpath %.a
vpath %.c   $(SRC_DIR)
vpath %.cpp $(SRC_DIR)
vpath %.S   $(SRC_DIR)
vpath %.o   $(OBJ_DIR)
vpath %.d   $(OBJ_DIR)
vpath %.a   $(LIB_DIR) $(SYS_LIB_DIR)
vpath %.elf $(LIB_DIR)

# 这些目标的创建顺序pre_building ==> $(MODULE_NAME) ==> DO_SUB_DIR_ALL ==> post_building
.PHONY:pre_building all post_building makesure_Makefile_exists DO_SUB_DIR_ALL SHOW_DONE_MSG SHOW_START_MSG
.PHONY: target_final
.PHONY: CREATE_ARCHIVE
all: pre_building DO_SUB_DIR_ALL post_building SHOW_DONE_MSG SHOW_START_MSG makesure_Makefile_exists


#  如果哪个目录定义目标TARGET，那么在改目录生成可执行文件，
#  如果编译库文件（没有定义TARGET），则由"ifneq ($(TARGET),)"括起来的定义可忽略.
ifneq ($(TARGET),)

MAKELEVEL:=0

# files to remove when cleaning
EX_CLEAN_FILE += $(TARGET).elf $(TARGET).map $(TARGET) $(TARGET).bin $(TARGET).lst

# 默认的连接脚本
LDSCRIPT ?= ldscript.lds
# 系统库
LIBS              += $(foreach dir,$(SYS_LIB_DIR), $(wildcard $(dir)/*.a)) 
LDFLAGS           += -T$(LDSCRIPT)
LDFLAGS           += -L.
all_local_libs    := $(wildcard $(LIB_DIR)/*.a)

# 在编译完所有子目录后，再make target_final_sym
# 这样做都是为了重新加载Makefile使得变量$(all_local_libs)
# 能包含所有库。
target_final: $(all_local_libs) $(LIBS)
	@echo "=======================  step 1  ============================="
	@echo "Creating \"$(TARGET).elf\""
	$(CC) -o $(LIB_DIR)/$(TARGET).elf \
          -Wl,-whole-archive $(all_local_libs) \
          -Wl,-no-whole-archive \
          -Wl,--start-group \
          $(LIBS) \
          -Wl,--end-group\
          $(LDFLAGS)

# $(TARGET).elf depends on
# $(LIBS) $(all_local_libs)
$(TARGET).elf:$(LIBS) $(all_local_libs)
	@$(MAKE) -C . target_final

$(TARGET).lst:$(TARGET).elf
	@echo "=======================  list ================================"
	@echo "Creating \"$@\""
	$(OBJDUMP) -h -S $< >$(LIB_DIR)/$@

$(TARGET).bin:$(TARGET).elf
	@echo "=======================  binary =============================="
	@echo "Creating \"$@\""
	@echo '$$(OBJCOPY) --gap-fill=0xff -O binary $(TARGET).elf $@'
	@$(OBJCOPY) --gap-fill=0xff -O binary $(LIB_DIR)/$(TARGET).elf $(LIB_DIR)/$@
	@$(SIZE) -A -x $(LIB_DIR)/$(TARGET).elf
	@echo "=======================  complete  ==========================="
	@echo "<<< Finish building, \t`date`"
#	$(OBJCOPY) --gap-fill=0xff --remove-section=.ARM.exidx -O binary $< $@

# 因为库可能会在子目录中更新，所以，需要重新加载获取$(TARGET).elf $(TARGET).bin等文件的时间戳
target_post_building:|CREATE_ARCHIVE
	@$(MAKE) -C . $(TARGET).bin

post_building: |target_post_building

endif ###  <-- ifneq ($(TARGET),)

pre_building:                 | SHOW_START_MSG
$(COBJS) $(CPPOBJS) $(SOBJS): | pre_building
DO_SUB_DIR_ALL:               | makesure_Makefile_exists 
CREATE_ARCHIVE:               | DO_SUB_DIR_ALL $(COBJS) $(CPPOBJS) $(SOBJS)
post_building:                | CREATE_ARCHIVE
SHOW_DONE_MSG:                | post_building

# 显示开始和结束时间,只在执行make的目录显示开始和结束时间
ifeq ($(MAKELEVEL),0)
SHOW_DONE_MSG:
	@echo "<<< Finish building '$(MODULE_NAME)', \t`date`"
SHOW_START_MSG:
	@echo ">>> Start  building '$(MODULE_NAME)', \t`date`"

CREATE_ARCHIVE: 
	@mkdir -p $(LIB_DIR) $(OBJ_DIR);find $(OBJ_DIR) -name "*.o" -print | xargs -s 16384 $(AR) $(ARFLAGS) $(LIB_DIR)/$(MODULE_NAME) 

else
SHOW_START_MSG:
SHOW_DONE_MSG:
CREATE_ARCHIVE:
endif

# 在编译OBJS和子目录前，可以使用此目标完成一些工作，
# 如果子目录不定义此目标，则使用本makefile定义的空目标
pre_building:


# 在编译OBJS和子目录后，可以使用此目标完成一些工作，
# 如果子目录不定义此目标，则使用本makefile定义的空目标
post_building:

#
# 编译源码文件规则定义
#

# C
$(COBJS):%.o:%.c
	@-rm -f $@
	@echo "    CC \"$(RELATIVE_DIR)/$<\""
	mkdir -p $(OBJ_DIR);$(CC) $(CFLAGS) $($@-FLAGS) -Wall -c $(SRC_DIR)/$< -o $(OBJ_DIR)/$@

# C++
$(CPPOBJS):%.o:%.cpp
	@-rm -f $@
	@echo "    CPP \"$(RELATIVE_DIR)/$<\""
	mkdir -p $(OBJ_DIR);$(CPP) $(CPPFLAGS) $($@-FLAGS) -Wall -c $(SRC_DIR)/$< -o $(OBJ_DIR)/$@

# assembler
$(SOBJS):%.o:%.S
	@-rm -f $@
	@echo "    AS \"$(RELATIVE_DIR)/$<\""
	mkdir -p $(OBJ_DIR);$(CPP)  $(ASFLAGS) $($@-FLAGS) -Wall -c $(SRC_DIR)/$< -o $(OBJ_DIR)/$@

# 安装Makefile和clean时不包含依赖文件
ifneq ($(findstring install_Makefile,$(MAKECMDGOALS)),install_Makefile)
ifneq ($(findstring install_makefile,$(MAKECMDGOALS)),install_makefile)
ifneq ($(findstring makesure_Makefile_exists,$(MAKECMDGOALS)),makesure_Makefile_exists)
ifneq ($(findstring clean,$(MAKECMDGOALS)),clean)
#
# 自动产生依赖关系
#

# C
$(COBJS:.o=.d):%.d:%.c
	@mkdir -p $(OBJ_DIR);$(CC) -MM $(CFLAGS) $($(@:.d=.o)-FLAGS) $< |\
    sed 's,\($*\)\.o[:]*,\1.o $@:,g' > $(OBJ_DIR)/$@

# C++
$(CPPOBJS:.o=.d):%.d:%.cpp
	@mkdir -p $(OBJ_DIR);$(CPP) -MM $(CPPFLAGS) $($(@:.d=.o)-FLAGS) $<|\
    sed 's,\($*\)\.o[:]*,\1.o $@:,g' > $(OBJ_DIR)/$@

# assembler
$(SOBJS:.o=.d):%.d:%.S
	@mkdir -p $(OBJ_DIR);$(CPP) -MM $(ASFLAGS) $($(@:.d=.o)-FLAGS) $< |\
    sed 's,\($*\)\.o[:]*,\1.o $@:,g' > $(OBJ_DIR)/$@

# do not include dependencies whlie doing "make clean"
sinclude $(COBJS_P:.o=.d)
sinclude $(CPPOBJS_P:.o=.d)
sinclude $(SOBJS_P:.o=.d)

Makefile:$(SOBJS:.o=.d) $(CPPOBJS:.o=.d) $(COBJS:.o=.d)

endif
endif
endif
endif

# 递归执行所有目录
.PHONY:DO_SUB_DIR_ALL $(ALL_SUB_PATH)
DO_SUB_DIR_ALL:$(ALL_SUB_PATH)
$(ALL_SUB_PATH):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

# 使用for循环对每个子目录都执行make clean
.PHONY:clean
clean:DO_SUB_DIR_ALL
	rm -f -r $(OBJ_DIR)
	rm -f $(EX_CLEAN_FILE) 

#
# 确保子目录下的Makefile存在。
# 如果由ALL_SUB_PATH定义的子目录中没有Makefile，则把Makefile模板复制该目录
.PHONY: makesure_Makefile_exists
makesure_Makefile_exists:$(addsuffix /Makefile,$(ALL_SUB_PATH))
$(addsuffix /Makefile,$(ALL_SUB_PATH)):
	@if [ ! -f $@ ]; then \
         cp -v $(RTK_BASE)/makefiles/makefiletemplate.mk $@;\
     fi;\
	$(MAKE) -C $(dir $@) makesure_Makefile_exists

# make distclean时：
#    1. 直接删除output目录。
#    2. 删除除了顶层目录以外所有子目录的Makefile文件
distclean:
	-rm -f $(RTK_BASE)/output -r 
	-find $(RTK_BASE) -name "*.[od]" -delete
#	-find $(RTK_BASE) -mindepth 2 -name "Makefile" -delete

.PHONY: write_objs_to_makefile
#
# 把.o文件写入Makefile,需要sed 支持 -i选项.
#
write_objs_to_makefile:
	@sed -i -e '/[ ]*COBJS[ \t].*[:]*=[ \t]*/s/$$(patsubst.*/$(COBJS)/g'\
            -e '/[ ]*CPPOBJS[ \t].*[:]*=[ \t]*/s/$$(patsubst.*/$(CPPOBJS)/g'\
            -e '/[ ]*SOBJS[ \t].*[:]*=[ \t]*/s/$$(patsubst.*/$(SOBJS)/g' Makefile
	@for i in $(ALL_SUB_PATH); do \
           $(MAKE) -C $$i write_objs_to_makefile;\
     done

# 除了当前目录的Makefile以外，所有子目录的Makefile都可以通过
# 执行make install_Makefile或者make install_makefile达到
# 自动安装Makefile的目的。
#    在自动安装Makefile的过程中，会扫描一个名为"tree.cfg"的配置文件，
# 每当遇到名为tree.cfg的文件，则tree.cfg所在目录及有子目录下的Makefile都会插入tree.cfg文件里面的内容.
# 也就是说，在一条路径上存在多个tree.cfg时
.PHONY: install_makefile install_Makefile
install_makefile install_Makefile:makesure_Makefile_exists
	if [ -f ./tree.cfg ]; then \
        TREE_FILENAME="$(shell pwd)/tree.cfg";\
		sed -e "/#<<tree\.cfg.*/r $$TREE_FILENAME" -e "s/#<<tree.cfg.*/MAKELEVEL=0/g" ./Makefile >./Makefile.tmp;\
        mv ./Makefile.tmp ./Makefile;\
    fi;\
    if [ -n "$$TREE_FILENAME" ]; then \
        sed -e "/#<<tree\.cfg.*/r $$TREE_FILENAME" -e "/#<<tree.cfg.*/d" ./Makefile >./Makefile.tmp;\
        mv ./Makefile.tmp ./Makefile;\
    fi;\
    for i in $(ALL_SUB_PATH); do \
        $(MAKE) -C $$i install_makefile TREE_FILENAME=$$TREE_FILENAME;\
    done;\

# 当sed支持-i选项时可以使用以下命令
#install_makefile install_Makefile:makesure_Makefile_exists
#	if [ -f ./tree.cfg ]; then \
#        TREE_FILENAME="$(shell pwd)/tree.cfg";\
#    fi;\
#    if [ -n "$$TREE_FILENAME" ]; then \
#        sed -i -e "/#<<tree\.cfg.*/r $$TREE_FILENAME" -e "/#<<tree.cfg.*/d" ./Makefile; fi;\
#    for i in $(ALL_SUB_PATH); do \
#        $(MAKE) -C $$i check_module_name TREE_FILENAME=$$TREE_FILENAME;\
#    done;\


# 查看预处理结果
# process_file = findfp.c
# process_result = process_result.c
# EX_CLEAN_FILE += $(process_result)
# post_building:$(process_result)
# $(process_result):$(process_file)
# 	$(CC) -E $(CFLAGS) $< >./$@

