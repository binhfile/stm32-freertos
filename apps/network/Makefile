TARGET=stm32f4-freertos
#TARGET=arm-linux


BINDIR=bin
SRCDIR=.
EXTRALIB_DIR=../../extralib
ASOURCES=
CSOURCES=
CXXSOURCES=
ifneq (,$(findstring stm32f4,$(TARGET)))
	TOOLCHAIN_DIR=/home/toolchain/stm32/gcc-arm-none-eabi-4_9-2015q3
	CROSS_COMPILE=arm-none-eabi-
	HW_DIR=../../opensource/STM32F4-Discovery_FW_V1.1.0
	
	ASOURCES+=$(shell find -L $(SRCDIR) -name '*.s')
	ASOURCES+=$(HW_DIR)/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f4xx.s
	ASOURCES+=$(EXTRALIB_DIR)/libc/string/stm32f4/memcpy_a.s
	CSOURCES+=$(HW_DIR)/Utilities/STM32F4-Discovery/stm32f4_discovery.c
	CSOURCES+=$(shell find -L $(SRCDIR)/bsp -name '*.c')
	CSOURCES+=$(shell find -L $(HW_DIR)/Libraries/STM32F4xx_StdPeriph_Driver -name '*.c')
	INC+=$(shell find -L $(SRCDIR)/bsp -name '*.h' -exec dirname {} \; | uniq)
	INC+=$(shell find -L $(HW_DIR) -name '*.h' -exec dirname {} \; | uniq)
	
	MCFLAGS=-mcpu=cortex-m4 -mthumb -mlittle-endian -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb-interwork
	CFLAGS=-nostdlib -Wall -ffunction-sections -fdata-sections -fno-builtin
	LDFLAGS =-nostdlib -T stm32_flash.ld --specs=nosys.specs $(MCFLAGS) -Wl,-Map=bin/app.map -Wl,-gc-sections --specs=nano.specs
	DEFS=-DUSE_STDPERIPH_DRIVER -DSTM32F4XX -DUSE_USB_OTG_FS
	CXXFLAGS+=-nostdlib -Wall -fno-exceptions -ffunction-sections -fdata-sections -fno-builtin -fno-rtti
			
	BINELF=outp.elf
    BINHEX=outp.hex
    BIN=outp.bin
endif
ifneq (,$(findstring freertos,$(TARGET)))
	OS_DIR=../../opensource/FreeRTOS
	CSOURCES+=$(shell find -L $(SRCDIR)/os -name '*.c')
	CSOURCES+=$(shell find -L $(EXTRALIB_DIR)/libc -name '*.c')
	CSOURCES+=$(shell find -L $(EXTRALIB_DIR)/linux/stm32f4_freertos -name '*.c')
	CSOURCES+=$(shell find -L $(EXTRALIB_DIR)/posix/stm32f4_freertos -name '*.c')
	CSOURCES+=$(shell find -L $(EXTRALIB_DIR)/special/stm32f4_freertos -name '*.c')
	CSOURCES+=$(shell find -L $(OS_DIR) -name '*.c')
	CSOURCES+=$(EXTRALIB_DIR)/posix/getopt.c
	INC+=$(shell find -L $(SRCDIR)/os -name '*.h' -exec dirname {} \; | uniq)
	INC+=$(shell find -L $(EXTRALIB_DIR) -name '*.h' -exec dirname {} \; | uniq)
	INC+=$(shell find -L $(OS_DIR) -name '*.h' -exec dirname {} \; | uniq)
	DEFS+=-DOS_FREERTOS
	
	CXXFLAGS+=-O2
    CFLAGS+=-O2
    LDFLAGS+=-O2
endif

ifneq (,$(findstring linux,$(TARGET)))
	TOOLCHAIN_DIR=/home/toolchain/tools-master/arm-bcm2708/arm-bcm2708-linux-gnueabi
	CROSS_COMPILE=bcm2708-
	OUTFILE=app_net
	DEFS+=-DOS_LINUX
	LDFLAGS+=-lrt -lpthread -ldl -T linux.ld
	
	CSOURCES+=$(shell find -L $(EXTRALIB_DIR)/special/linux -name '*.c')
	INC+=$(shell find -L $(EXTRALIB_DIR)/special/include -name '*.h' -exec dirname {} \; | uniq)
	
	CXXFLAGS+=-ggdb -g3
	CFLAGS+=-ggdb -g3
	LDFLAGS+=-ggdb -g3
endif

# GNU ARM Embedded Toolchain
CC=$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)gcc
CXX=$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)g++
LD=$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)ld
AR=$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)ar
AS=$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)as
CP=$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)objcopy
OD=$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)objdump
SIZE=$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)size
READELF=$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)readelf

CSOURCES+=$(shell find -L $(SRCDIR)/src -name '*.c')
CSOURCES+=$(shell find -L $(EXTRALIB_DIR)/apps -name '*.c')
CSOURCES+=$(shell find -L $(EXTRALIB_DIR)/lib -name '*.c')
CXXSOURCES+=$(shell find -L $(SRCDIR)/src -name '*.cpp')
INC+=$(shell find -L $(EXTRALIB_DIR)/apps -name '*.h' -exec dirname {} \; | uniq)
INC+=$(shell find -L $(EXTRALIB_DIR)/lib -name '*.h' -exec dirname {} \; | uniq)
INC+=$(shell find -L $(SRCDIR)/src -name '*.h' -exec dirname {} \; | uniq)
#INC+=$(shell find -L $(MESH_DIR) -name '*.h' -exec dirname {} \; | uniq)
#regex lib
SLRE_DIR=../../opensource/slre
CSOURCES+=$(shell find -L $(SLRE_DIR) -name '*.c')
INC+=$(shell find -L $(SLRE_DIR) -name '*.h' -exec dirname {} \; | uniq)

INCLUDES=$(INC:%=-I%)
# Create object list
OBJECTS+=$(CSRC_LIB:%.c=%.o)
OBJECTS+=$(ASOURCES:%.s=%.o)
OBJECTS+=$(CSOURCES:%.c=%.o)
OBJECTS+=$(CXXSOURCES:%.cpp=%.o)
# Define output files ELF & IHEX


###
# MCU FLAGS
# COMPILE FLAGS
CFLAGS  +=-c $(MCFLAGS) $(DEFS) $(INCLUDES)
CXXFLAGS  +=-c $(MCFLAGS) $(DEFS) $(INCLUDES)
###
# Optimizations

###
# Build Rules
.PHONY: all release release-memopt debug clean

all: release

ifneq (,$(findstring stm32f4-freertos,$(TARGET)))
release: $(BINDIR)/$(BINHEX) $(BINDIR)/$(BIN)
endif
ifneq (,$(findstring linux,$(TARGET)))
release: $(BINDIR)/$(OUTFILE)
endif
release-memopt: release

debug: CFLAGS+=-g
debug: LDFLAGS+=-g
debug: release

ifneq (,$(findstring stm32f4-freertos,$(TARGET)))
$(BINDIR)/$(BINHEX): $(BINDIR)/$(BINELF)
	@ $(CP) -O ihex $< $@ 
	@echo "Objcopy from ELF to IHEX complete"
$(BINDIR)/$(BIN): $(BINDIR)/$(BINELF)
	@ $(CP) -O binary $< $@ 
	@ $(OD) -S $(BINDIR)/$(BINELF) > $(BINDIR)/$(BINELF).lst
	@echo "Objcopy from ELF to BIN complete"	

$(BINDIR)/$(BINELF): $(OBJECTS)
	@ $(CC) $(LDFLAGS) $(OBJECTS) -o $@
	@echo "Linking complete!"
	$(SIZE) $(BINDIR)/$(BINELF)
	#$(READELF) -e $(BINDIR)/$(BINELF)
endif

$(BINDIR)/$(OUTFILE): $(OBJECTS)
	@ $(CC) $(LDFLAGS) $(OBJECTS) -o $@
	@echo "Linking complete!"
	$(SIZE) $(BINDIR)/$(OUTFILE)
%.o: %.c
	@$(CC) $(CFLAGS) $< -o $@
	@echo "CC "$<""
%.o: %.cpp
	@$(CXX) $(CFLAGS) $< -o $@
	@echo "CXX "$<""
%.o: %.s
	@$(CC) $(CFLAGS) $< -o $@
	@echo "ASM "$<""

clean:
	@ rm -f $(OBJECTS) $(BINDIR)/*
	
ifneq (,$(findstring stm32f4-freertos,$(TARGET)))
deploy:
ifeq ($(wildcard /opt/openocd/bin/openocd),)
	/usr/bin/openocd -f /usr/share/openocd/scripts/board/stm32f4discovery.cfg -c "program bin/"$(BINELF)" verify reset"
else
	/opt/openocd/bin/openocd -f /opt/openocd/share/openocd/scripts/board/stm32f4discovery.cfg -c "program bin/"$(BINELF)" verify reset"
endif
flash: $(BINDIR)/$(BIN)
	st-flash --reset write $(BINDIR)/$(BIN) 0x08000000
endif
