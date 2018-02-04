
.PHONY:	test project all clean install

VERSION					:= Debug
#VERSION				:= Release

PRJ_NAME				:= sm-02

SRC						:= main.c
SRC						+= common.c
SRC						+= figure.c
SRC						+= font.c
SRC						+= image.c
SRC						+= egl_util.c
SRC						+= draw.c

VPATH					:= 
INC 					:= 
DEFINE					:= 
LIB						:= stdc++
LIB						+= brcmGLESv2 brcmEGL bcm_host vcos vchiq_arm m png freetype

OPTIMIZE_LEVEL			:= 2
DEBUG_LEVEL				:= gdb

TOOL_PATH				:= C:\SysGCC\Raspberry\arm-linux-gnueabihf
SHELL					:= C:\msys32\usr\bin\sh.exe

##################################################################

ROOT					:= $(shell pwd)

BINDIR 					:= C:\SysGCC\Raspberry\bin
CROSS_PREFIX			:= arm-linux-gnueabihf-
CC						:= $(CROSS_PREFIX)gcc
AS						:= $(CROSS_PREFIX)gcc
LD						:= $(CROSS_PREFIX)gcc
CPPC					:= $(CROSS_PREFIX)g++
AR						:= $(CROSS_PREFIX)ar
RANLIB					:= $(CROSS_PREFIX)ranlib
SIZE					:= $(CROSS_PREFIX)size
OBJCOPY					:= $(CROSS_PREFIX)objcopy
OBJDUMP					:= $(CROSS_PREFIX)objdump

BIN_DIR					:= bin
DEBUG_DIR				:= Debug
RELEASE_DIR				:= Release
INC_DIR					:= inc
OBJ_DIR					:= obj
SRC_DIR					:= src

VPATH					+= $(INC_DIR) $(OBJ_DIR) $(SRC_DIR)
INCLUDE_PATH			:= $(INC) $(INC_DIR) $(COMM_INC_DIR) $(TOOL_PATH)/include
LIB_PATH				:= $(TOOL_PATH)/lib
GLLIB_PATH				:= lib

LIBFLAGS				:= $(addprefix -L,$(LIB_PATH)) $(addprefix -L,$(GLLIB_PATH)) $(addprefix -l,$(LIB))
ARFLAGS					:= rcs
 
CCFLAGS					:= -Wall
CCFLAGS					+= $(addprefix -D,$(DEFINE)) $(addprefix -I,$(INCLUDE_PATH))
CCFLAGS					+= -ffunction-sections -fdata-sections
 
ASFLAGS					:= -Wall
ASFLAGS					+= $(addprefix -D,$(DEFINE)) $(addprefix -I,$(INCLUDE_PATH))

ifeq ($(VERSION),Debug)
IMAGE					:= $(BIN_DIR)/$(DEBUG_DIR)/$(PRJ_NAME)
CCFLAGS					+= -g$(DEBUG_LEVEL) -O0 -DDEBUG
ASFLAGS					+= -g$(DEBUG_LEVEL) -O0 -DDEBUG
endif

ifeq ($(VERSION),Release)
IMAGE					:= $(BIN_DIR)/$(RELEASE_DIR)/$(PRJ_NAME)
CCFLAGS					+= -O$(OPTIMIZE_LEVEL)
ASFLAGS					+= -O0
endif

CPPCFLAGS				:= $(CCFLAGS)
CPPCFLAGS				+= -x c++ -Weffc++
CPPCFLAGS				+= -std=gnu++0x

LDFLAGS					:= -Wl,-Map,$(IMAGE).map,--cref -Wl,--gc-sections

OBJECTS					:= $(addprefix $(OBJ_DIR)/$(VERSION)/,$(patsubst %.c, %.o,$(filter %.c,$(SRC))))
OBJECTS					+= $(addprefix $(OBJ_DIR)/$(VERSION)/,$(patsubst %.cpp, %.o,$(filter %.cpp,$(SRC))))
OBJECTS					+= $(addprefix $(OBJ_DIR)/$(VERSION)/,$(patsubst %.s, %.o,$(filter %.s,$(SRC))))
OBJECTS					+= $(addprefix $(OBJ_DIR)/$(VERSION)/,$(patsubst %.S, %.o,$(filter %.S,$(SRC))))

#############################################################################

test: 
	$(CC) --version
	$(MAKE) --version
	@echo $(OBJECTS)
		
project:
	test  -d $(BIN_DIR) || mkdir $(BIN_DIR) $(BIN_DIR)/$(DEBUG_DIR) $(BIN_DIR)/$(RELEASE_DIR)
	test  -d $(OBJ_DIR) || mkdir $(OBJ_DIR) $(OBJ_DIR)/$(DEBUG_DIR) $(OBJ_DIR)/$(RELEASE_DIR)
	test  -d $(INC_DIR) || mkdir $(INC_DIR)
	test  -d $(SRC_DIR) || mkdir $(SRC_DIR)

install: $(IMAGE)
	cp $(IMAGE) out/
    

all: $(VERSION)
	
$(VERSION): exe lst size install
	@echo "--------------------- COMPLETE -----------------------"

exe:$(IMAGE)

lst:$(IMAGE).lst

size:$(IMAGE)
	@echo
	@echo $@
	@echo "------------------------------------------------------"
	$(SIZE) $(IMAGE)

$(IMAGE).lst:$(IMAGE)
	@echo
	@echo $@
	@echo "------------------------------------------------------"
	$(OBJDUMP) -h -S -z $<  > $@

$(IMAGE):$(OBJECTS)
	@echo
	@echo $@
	@echo "------------------------------------------------------"
	$(LD) $(CCFLAGS) $(LDFLAGS) $^ -o $@ $(LIBFLAGS)

$(OBJ_DIR)/$(VERSION)/%.o:$(SRC_DIR)/%.c
	@echo
	@echo $<
	@echo "------------------------------------------------------"
	$(CC) $(CCFLAGS) -MD -c $< -o $@

$(OBJ_DIR)/$(VERSION)/%.o:%.cpp
	@echo
	@echo $<
	@echo "------------------------------------------------------"
	$(CPPC) $(CPPCFLAGS) -MD -c $< -o $@

$(OBJ_DIR)/$(VERSION)/%.o:%.s
	@echo
	@echo $<
	@echo "------------------------------------------------------"
	$(AS) $(ASFLAGS) -c $< -o $@
	
$(OBJ_DIR)/$(VERSION)/%.o:%.S
	@echo
	@echo $<
	@echo "------------------------------------------------------"
	$(AS) $(ASFLAGS) -c $< -o $@

include $(wildcard $(OBJ_DIR)/$(VERSION)/*.d)

clean:
	rm -f $(OBJECTS)
	rm -f $(patsubst %.o, %.d,$(OBJECTS))
	rm -f $(IMAGE) $(IMAGE).map $(IMAGE).lst
	@echo "--------------------- COMPLETE -----------------------"
	
