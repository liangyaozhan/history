# -*- coding: gb2312; -*- 
#########################################################################################################
## ��   ��   ��: rules.mk
##
## ��   ��   ��: liangyaozhan
##
## ��        ��: 2.0
##
## ��  �� ��  ��:   Thu Apr 26 15:20:26 HKT 2012
##                ����ʱ��ʹ��Դ��ȫ·��������������eclipse��ת��������߾��洦��
##                Ϊ��֧��make -jѡ� ������ɺ���һ�������ӣ�����ʹ��xargs�����������в���̫�������⡣
##                ��ͬ�Ŀ��Ŀ���ļ����ڲ�ͬ�ĵط���
##
## ��        ��: ���ļ������������򣬰���������C/C++�ͻ��Դ���ļ����Զ�����������ϵ���ݹ�ִ������Ŀ¼��
##              ȷ����Ŀ¼�µ�Makefile���ڡ���װ��Ŀ¼��Makefile�ļ�������
##              �ṩ��Ŀ���У�
##              make install_makefile ���� make install_Makefile   : ��װ������Makefile
##              make clean                                         : ִ�е�ǰĿ¼����Ŀ¼������
##              make distclean                                     : �κ�Ŀ¼make ��Ŀ�궼������������
##              make write_objs_to_makefile                        : ��Ŀ���ļ��б�д��Makefile����������
##                                                                 ����ʱ����ͨ������ҡ�
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

#���������ֶ���
CC       = $(CROSS_COMPILE)gcc
CPP      = $(CROSS_COMPILE)g++
AR       = $(CROSS_COMPILE)ar
LD       = $(CROSS_COMPILE)ld
OBJCOPY  = $(CROSS_COMPILE)objcopy
OBJDUMP  = $(CROSS_COMPILE)objdump
NM       = $(CROSS_COMPILE)nm
SIZE       = $(CROSS_COMPILE)size

# C/C++ asm��ʹ�õı������:COMMON_FLAGS
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

# �汾����:debug ���� release. Ĭ����debug
# debug���������Ϣ,release ����������Ϣ
#VERSION ?= debug

# ���û�ж�����Ŀ¼����ʹ��������Ŀ¼
ALL_SUB_PATH ?= $(wildcard */)
EXCLUDE_SUB_PATH1:=$(addsuffix /,$(EXCLUDE_SUB_PATH))
override ALL_SUB_PATH := $(filter-out $(EXCLUDE_SUB_PATH) $(EXCLUDE_SUB_PATH1),$(ALL_SUB_PATH))

# ȱʡ�Ŀ�����Ϊ
MODULE_NAME ?= $(DEFAULT_LIB_NAME)

# ����λ�ã�Դ��Ŀ¼��Ŀ���ļ�Ŀ¼�Ϳ�Ŀ¼
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

# ��·����Ŀ���ļ�(*.o)
COBJS_P   := $(addprefix $(OBJ_DIR)/,$(COBJS) )
CPPOBJS_P := $(addprefix $(OBJ_DIR)/,$(CPPOBJS) )
SOBJS_P   := $(addprefix $(OBJ_DIR)/,$(SOBJS) )

# ѡ���Ƿ��������Ϣ
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


# ��ȡ���ٶ�����Ҹ��������ļ���Ŀ¼
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

# ��ЩĿ��Ĵ���˳��pre_building ==> $(MODULE_NAME) ==> DO_SUB_DIR_ALL ==> post_building
.PHONY:pre_building all post_building makesure_Makefile_exists DO_SUB_DIR_ALL SHOW_DONE_MSG SHOW_START_MSG
.PHONY: target_final
.PHONY: CREATE_ARCHIVE
all: pre_building DO_SUB_DIR_ALL post_building SHOW_DONE_MSG SHOW_START_MSG makesure_Makefile_exists


#  ����ĸ�Ŀ¼����Ŀ��TARGET����ô�ڸ�Ŀ¼���ɿ�ִ���ļ���
#  ���������ļ���û�ж���TARGET��������"ifneq ($(TARGET),)"�������Ķ���ɺ���.
ifneq ($(TARGET),)

MAKELEVEL:=0

# files to remove when cleaning
EX_CLEAN_FILE += $(TARGET).elf $(TARGET).map $(TARGET) $(TARGET).bin $(TARGET).lst

# Ĭ�ϵ����ӽű�
LDSCRIPT ?= ldscript.lds
# ϵͳ��
LIBS              += $(foreach dir,$(SYS_LIB_DIR), $(wildcard $(dir)/*.a)) 
LDFLAGS           += -T$(LDSCRIPT)
LDFLAGS           += -L.
all_local_libs    := $(wildcard $(LIB_DIR)/*.a)

# �ڱ�����������Ŀ¼����make target_final_sym
# ����������Ϊ�����¼���Makefileʹ�ñ���$(all_local_libs)
# �ܰ������п⡣
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

# ��Ϊ����ܻ�����Ŀ¼�и��£����ԣ���Ҫ���¼��ػ�ȡ$(TARGET).elf $(TARGET).bin���ļ���ʱ���
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

# ��ʾ��ʼ�ͽ���ʱ��,ֻ��ִ��make��Ŀ¼��ʾ��ʼ�ͽ���ʱ��
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

# �ڱ���OBJS����Ŀ¼ǰ������ʹ�ô�Ŀ�����һЩ������
# �����Ŀ¼�������Ŀ�꣬��ʹ�ñ�makefile����Ŀ�Ŀ��
pre_building:


# �ڱ���OBJS����Ŀ¼�󣬿���ʹ�ô�Ŀ�����һЩ������
# �����Ŀ¼�������Ŀ�꣬��ʹ�ñ�makefile����Ŀ�Ŀ��
post_building:

#
# ����Դ���ļ�������
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

# ��װMakefile��cleanʱ�����������ļ�
ifneq ($(findstring install_Makefile,$(MAKECMDGOALS)),install_Makefile)
ifneq ($(findstring install_makefile,$(MAKECMDGOALS)),install_makefile)
ifneq ($(findstring makesure_Makefile_exists,$(MAKECMDGOALS)),makesure_Makefile_exists)
ifneq ($(findstring clean,$(MAKECMDGOALS)),clean)
#
# �Զ�����������ϵ
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

# �ݹ�ִ������Ŀ¼
.PHONY:DO_SUB_DIR_ALL $(ALL_SUB_PATH)
DO_SUB_DIR_ALL:$(ALL_SUB_PATH)
$(ALL_SUB_PATH):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

# ʹ��forѭ����ÿ����Ŀ¼��ִ��make clean
.PHONY:clean
clean:DO_SUB_DIR_ALL
	rm -f -r $(OBJ_DIR)
	rm -f $(EX_CLEAN_FILE) 

#
# ȷ����Ŀ¼�µ�Makefile���ڡ�
# �����ALL_SUB_PATH�������Ŀ¼��û��Makefile�����Makefileģ�帴�Ƹ�Ŀ¼
.PHONY: makesure_Makefile_exists
makesure_Makefile_exists:$(addsuffix /Makefile,$(ALL_SUB_PATH))
$(addsuffix /Makefile,$(ALL_SUB_PATH)):
	@if [ ! -f $@ ]; then \
         cp -v $(RTK_BASE)/makefiles/makefiletemplate.mk $@;\
     fi;\
	$(MAKE) -C $(dir $@) makesure_Makefile_exists

# make distcleanʱ��
#    1. ֱ��ɾ��outputĿ¼��
#    2. ɾ�����˶���Ŀ¼����������Ŀ¼��Makefile�ļ�
distclean:
	-rm -f $(RTK_BASE)/output -r 
	-find $(RTK_BASE) -name "*.[od]" -delete
#	-find $(RTK_BASE) -mindepth 2 -name "Makefile" -delete

.PHONY: write_objs_to_makefile
#
# ��.o�ļ�д��Makefile,��Ҫsed ֧�� -iѡ��.
#
write_objs_to_makefile:
	@sed -i -e '/[ ]*COBJS[ \t].*[:]*=[ \t]*/s/$$(patsubst.*/$(COBJS)/g'\
            -e '/[ ]*CPPOBJS[ \t].*[:]*=[ \t]*/s/$$(patsubst.*/$(CPPOBJS)/g'\
            -e '/[ ]*SOBJS[ \t].*[:]*=[ \t]*/s/$$(patsubst.*/$(SOBJS)/g' Makefile
	@for i in $(ALL_SUB_PATH); do \
           $(MAKE) -C $$i write_objs_to_makefile;\
     done

# ���˵�ǰĿ¼��Makefile���⣬������Ŀ¼��Makefile������ͨ��
# ִ��make install_Makefile����make install_makefile�ﵽ
# �Զ���װMakefile��Ŀ�ġ�
#    ���Զ���װMakefile�Ĺ����У���ɨ��һ����Ϊ"tree.cfg"�������ļ���
# ÿ��������Ϊtree.cfg���ļ�����tree.cfg����Ŀ¼������Ŀ¼�µ�Makefile�������tree.cfg�ļ����������.
# Ҳ����˵����һ��·���ϴ��ڶ��tree.cfgʱ
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

# ��sed֧��-iѡ��ʱ����ʹ����������
#install_makefile install_Makefile:makesure_Makefile_exists
#	if [ -f ./tree.cfg ]; then \
#        TREE_FILENAME="$(shell pwd)/tree.cfg";\
#    fi;\
#    if [ -n "$$TREE_FILENAME" ]; then \
#        sed -i -e "/#<<tree\.cfg.*/r $$TREE_FILENAME" -e "/#<<tree.cfg.*/d" ./Makefile; fi;\
#    for i in $(ALL_SUB_PATH); do \
#        $(MAKE) -C $$i check_module_name TREE_FILENAME=$$TREE_FILENAME;\
#    done;\


# �鿴Ԥ������
# process_file = findfp.c
# process_result = process_result.c
# EX_CLEAN_FILE += $(process_result)
# post_building:$(process_result)
# $(process_result):$(process_file)
# 	$(CC) -E $(CFLAGS) $< >./$@

