###
TOOLCHAIN_DIR=/home/toolchain/stm32/gcc-arm-none-eabi-4_9-2015q3
# GNU ARM Embedded Toolchain
CC=$(TOOLCHAIN_DIR)/bin/arm-none-eabi-gcc
LD=$(TOOLCHAIN_DIR)/bin/arm-none-eabi-ld
AR=$(TOOLCHAIN_DIR)/bin/arm-none-eabi-ar
AS=$(TOOLCHAIN_DIR)/bin/arm-none-eabi-as
CP=$(TOOLCHAIN_DIR)/bin/arm-none-eabi-objcopy
OD=$(TOOLCHAIN_DIR)/bin/arm-none-eabi-objdump
SIZE=$(TOOLCHAIN_DIR)/bin/arm-none-eabi-size
READELF=$(TOOLCHAIN_DIR)/bin/arm-none-eabi-readelf

###
# Directory Structure
BINDIR=bin
INCDIR=inc
SRCDIR=src
EXTRALIB_DIR=../extralib
OS_DIR=../opensource/FreeRTOS
HW_DIR=../opensource/STM32F4-Discovery_FW_V1.1.0
#MESH_DIR=../opensource/LwMesh
###
# Find source files
ASOURCES=$(shell find -L $(SRCDIR) -name '*.s')
ASOURCES+=$(HW_DIR)/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f4xx.s

CSOURCES+=$(shell find -L $(SRCDIR) -name '*.c')
CSOURCES+=$(shell find -L $(EXTRALIB_DIR)/src -name '*.c')
CSOURCES+=$(HW_DIR)/Utilities/STM32F4-Discovery/stm32f4_discovery.c
CSOURCES+=$(shell find -L $(OS_DIR) -name '*.c')
CSOURCES+=$(shell find -L $(HW_DIR)/Libraries/STM32F4xx_StdPeriph_Driver -name '*.c')
#CSOURCES+=$(shell find -L $(MESH_DIR) -name '*.c')
# Find header directories
INC=$(shell find -L . -name '*.h' -exec dirname {} \; | uniq)
INC+=$(shell find -L $(OS_DIR) -name '*.h' -exec dirname {} \; | uniq)
INC+=$(shell find -L $(HW_DIR) -name '*.h' -exec dirname {} \; | uniq)
INC+=$(shell find -L $(EXTRALIB_DIR)/include -name '*.h' -exec dirname {} \; | uniq)
#INC+=$(shell find -L $(MESH_DIR) -name '*.h' -exec dirname {} \; | uniq)
INCLUDES=$(INC:%=-I%)
# Create object list
OBJECTS+=$(CSRC_LIB:%.c=%.o)
OBJECTS+=$(ASOURCES:%.s=%.o)
OBJECTS+=$(CSOURCES:%.c=%.o)
# Define output files ELF & IHEX
BINELF=outp.elf
BINHEX=outp.hex
BIN=outp.bin

###
# MCU FLAGS
MCFLAGS=-mcpu=cortex-m4 -mthumb -mlittle-endian -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb-interwork
# COMPILE FLAGS
DEFS=-DUSE_STDPERIPH_DRIVER -DSTM32F4XX
CFLAGS  =-c $(MCFLAGS) $(DEFS) $(INCLUDES) -nostdlib -Wall
# LINKER FLAGS
LDSCRIPT= stm32_flash.ld
LDFLAGS =-nostdlib -T $(LDSCRIPT) --specs=nosys.specs $(MCFLAGS) -Wl,-Map=bin/app.map

###
# Optimizations
CXXFLAGS+=-fno-exceptions -O2 -ffunction-sections -fdata-sections -fno-builtin -fno-rtti
CFLAGS+=-O3 -ffunction-sections -fdata-sections -fno-builtin 
LDFLAGS+=-O3 -Wl,-gc-sections --specs=nano.specs
###
# Build Rules
.PHONY: all release release-memopt debug clean

all: release

release: $(BINDIR)/$(BINHEX) $(BINDIR)/$(BIN)

release-memopt: release

debug: CFLAGS+=-g
debug: LDFLAGS+=-g
debug: release

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

%.o: %.c
	@ $(CC) $(CFLAGS) $< -o $@
	@echo "CC "$<""

%.o: %.s
	@ $(CC) $(CFLAGS) $< -o $@
	@echo "ASM "$<""

clean:
	@ rm -f $(OBJECTS) $(BINDIR)/*

deploy:
ifeq ($(wildcard /opt/openocd/bin/openocd),)
	/usr/bin/openocd -f /usr/share/openocd/scripts/board/stm32f4discovery.cfg -c "program bin/"$(BINELF)" verify reset"
else
	/opt/openocd/bin/openocd -f /opt/openocd/share/openocd/scripts/board/stm32f4discovery.cfg -c "program bin/"$(BINELF)" verify reset"
endif
flash: $(BINDIR)/$(BIN)
	st-flash --reset write $(BINDIR)/$(BIN) 0x08000000
