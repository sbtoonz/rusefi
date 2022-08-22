#****************************************************************************************
#|  Description: Makefile for GNU ARM Embedded toolchain.
#|    File Name: makefile
#|
#|---------------------------------------------------------------------------------------
#|                          C O P Y R I G H T
#|---------------------------------------------------------------------------------------
#|   Copyright (c) 2021  by Feaser    http://www.feaser.com    All rights reserved
#|
#|---------------------------------------------------------------------------------------
#|                            L I C E N S E
#|---------------------------------------------------------------------------------------
#| This file is part of OpenBLT. OpenBLT is free software: you can redistribute it and/or
#| modify it under the terms of the GNU General Public License as published by the Free
#| Software Foundation, either version 3 of the License, or (at your option) any later
#| version.
#|
#| OpenBLT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
#| without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#| PURPOSE. See the GNU General Public License for more details.
#|
#| You have received a copy of the GNU General Public License along with OpenBLT. It 
#| should be located in ".\Doc\license.html". If not, contact Feaser to obtain a copy.
#|
#****************************************************************************************
SHELL = sh

#|--------------------------------------------------------------------------------------|
#| Configure project name                                                               |
#|--------------------------------------------------------------------------------------|
PROJ_NAME=openblt_$(PROJECT_BOARD)

#|--------------------------------------------------------------------------------------|
#| Configure tool path                                                                  |
#|--------------------------------------------------------------------------------------|
# Configure the path to where the arm-none-eabi-gcc program is located. If the program
# is available on the path, then the TOOL_PATH variable can be left empty.
# Make sure to add a fordward slash at the end. Note that on Windows it should be in the
# 8.3 short pathname format with forward slashes. To obtain the pathname in the 8.3
# format, open the directory in the Windows command prompt and run the following command:
#  cmd /c for %A in ("%cd%") do @echo %~sA 
TOOL_PATH=$(TRGT)

#|--------------------------------------------------------------------------------------|
#| Configure sources paths                                                              |
#|--------------------------------------------------------------------------------------|
PROJECT_DIR=.
OPENBLT_TRGT_DIR=$(PROJECT_DIR)/ext/openblt/Target
OPENBLT_BOARD_DIR=$(PROJECT_DIR)/config/boards/$(PROJECT_BOARD)/openblt
ifeq ($(PROJECT_CPU),ARCH_STM32F4)
	OPENBLT_PORT_DIR=$(PROJECT_DIR)/hw_layer/ports/stm32/stm32f4/openblt
else ifeq ($(PROJECT_CPU),ARCH_STM32F7)
	OPENBLT_PORT_DIR=$(PROJECT_DIR)/hw_layer/ports/stm32/stm32f7/openblt
else ifeq ($(PROJECT_CPU),ARCH_STM32H7)
	OPENBLT_PORT_DIR=$(PROJECT_DIR)/hw_layer/ports/stm32/stm32h7/openblt
endif

#|--------------------------------------------------------------------------------------|
#| Collect helpers                                                                      |
#|--------------------------------------------------------------------------------------|
# Recursive wildcard function implementation. Example usages:
#   $(call rwildcard, , *.c *.h)   
#     --> Returns all *.c and *.h files in the current directory and below
#   $(call rwildcard, /lib/, *.c)
#     --> Returns all *.c files in the /lib directory and below
rwildcard = $(strip $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d)))

#|--------------------------------------------------------------------------------------|
#| Include port and board files                                                         |
#|--------------------------------------------------------------------------------------|
PROJ_FILES=
include $(OPENBLT_PORT_DIR)/port.mk
include $(OPENBLT_BOARD_DIR)/oblt_board.mk

#|--------------------------------------------------------------------------------------|
#| Collect bootloader core files                                                        |
#|--------------------------------------------------------------------------------------|
PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/*.c)
PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/*.h)

PROJ_FILES += $(PROJECT_DIR)/hw_layer/openblt/blt_conf.h
PROJ_FILES += $(PROJECT_DIR)/hw_layer/openblt/hooks.c
PROJ_FILES += $(PROJECT_DIR)/hw_layer/openblt/led.c
PROJ_FILES += $(PROJECT_DIR)/hw_layer/openblt/led.h
PROJ_FILES += $(PROJECT_DIR)/hw_layer/openblt/shared_params.c
PROJ_FILES += $(PROJECT_DIR)/hw_layer/openblt/shared_params.h

# CPU-dependent sources
ifeq ($(PROJECT_CPU),ARCH_STM32F4)
	# Collect bootloader port files
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM4_STM32F4/*.c)
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM4_STM32F4/*.h)
	# Collect bootloader port compiler specific files
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM4_STM32F4/GCC/*.c)
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM4_STM32F4/GCC/*.h)
	# LD file
	LFLAGS      = -Wl,-script="$(PROJECT_DIR)/hw_layer/ports/stm32/stm32f4/openblt/STM32F4xx.ld"
	# Port specific options
	PORTFLAGS  += -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
else ifeq ($(PROJECT_CPU),ARCH_STM32F7)
	# Collect bootloader port files
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM7_STM32F7/*.c)
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM7_STM32F7/*.h)
	# Collect bootloader port compiler specific files
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM7_STM32F7/GCC/*.c)
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM7_STM32F7/GCC/*.h)
	# LD file
	LFLAGS     += -Wl,-script="$(PROJECT_DIR)/hw_layer/ports/stm32/stm32f7/openblt/STM32F7xx.ld"
	# Port specific options
	PORTFLAGS   = -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16
else ifeq ($(PROJECT_CPU),ARCH_STM32H7)
	# Collect bootloader port files
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM7_STM32H7/*.c)
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM7_STM32H7/*.h)
	# Collect bootloader port compiler specific files
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM7_STM32H7/GCC/*.c)
	PROJ_FILES += $(wildcard $(OPENBLT_TRGT_DIR)/Source/ARMCM7_STM32H7/GCC/*.h)
	# LD file
	LFLAGS      = -Wl,-script="$(PROJECT_DIR)/hw_layer/ports/stm32/stm32h7/openblt/STM32H7xx.ld"
	# Port specific options
	PORTFLAGS  += -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16
endif

OPTFLAGS    = -Os
STDFLAGS    =  -fno-strict-aliasing
STDFLAGS   += -fdata-sections -ffunction-sections -Wall -g3
CFLAGS      = $(PORTFLAGS) $(BRDFLAGS) $(STDFLAGS) $(OPTFLAGS)
CFLAGS     += -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER
CFLAGS     += $(INC_PATH)
AFLAGS      = $(CFLAGS)
LFLAGS     += $(PORTFLAGS) $(BRDFLAGS) $(STDFLAGS) $(OPTFLAGS)

#|--------------------------------------------------------------------------------------|
#| Common options for toolchain binaries                                                |
#|--------------------------------------------------------------------------------------|
LFLAGS     += -Wl,-Map=$(BIN_PATH)/$(PROJ_NAME).map
LFLAGS     += -specs=nano.specs -Wl,--gc-sections $(LIB_PATH)
OFLAGS      = -O srec
ODFLAGS     = -x
SZFLAGS     = -B -d
RMFLAGS     = -f

#|--------------------------------------------------------------------------------------|
#| Toolchain binaries                                                                   |
#|--------------------------------------------------------------------------------------|
RM = rm
CC = $(TOOL_PATH)arm-none-eabi-gcc
LN = $(TOOL_PATH)arm-none-eabi-gcc
OC = $(TOOL_PATH)arm-none-eabi-objcopy
OD = $(TOOL_PATH)arm-none-eabi-objdump
AS = $(TOOL_PATH)arm-none-eabi-gcc
SZ = $(TOOL_PATH)arm-none-eabi-size

#|--------------------------------------------------------------------------------------|
#| Filter project files
#|--------------------------------------------------------------------------------------|
PROJ_ASRCS  = $(filter %.s,$(foreach file,$(PROJ_FILES),$(notdir $(file))))
PROJ_CSRCS  = $(filter %.c,$(foreach file,$(PROJ_FILES),$(notdir $(file))))
PROJ_CHDRS  = $(filter %.h,$(foreach file,$(PROJ_FILES),$(notdir $(file))))

#|--------------------------------------------------------------------------------------|
#| Set important path variables                                                         |
#|--------------------------------------------------------------------------------------|
VPATH    = $(foreach path,$(sort $(foreach file,$(PROJ_FILES),$(dir $(file)))) $(subst \,/,$(OBJ_PATH)),$(path) :)
OBJ_PATH = $(PROJECT_DIR)/build-openblt/obj
BIN_PATH = $(PROJECT_DIR)/build-openblt
INC_PATH = $(patsubst %/,%,$(patsubst %,-I%,$(sort $(foreach file,$(filter %.h,$(PROJ_FILES)),$(dir $(file))))))
LIB_PATH  = 

#|--------------------------------------------------------------------------------------|
#| Define targets                                                                       |
#|--------------------------------------------------------------------------------------|
AOBJS = $(patsubst %.s,%.o,$(PROJ_ASRCS))
COBJS = $(patsubst %.c,%.o,$(PROJ_CSRCS))

#|--------------------------------------------------------------------------------------|
#| Make ALL                                                                             |
#|--------------------------------------------------------------------------------------|
.PHONY: all
all: $(BIN_PATH)/$(PROJ_NAME).srec $(BIN_PATH)/$(PROJ_NAME).hex $(BIN_PATH)/$(PROJ_NAME).bin

$(BIN_PATH)/$(PROJ_NAME).srec : $(BIN_PATH)/$(PROJ_NAME).elf
	@mkdir -p $(@D)
	@$(OC) $< $(OFLAGS) $@
	@$(OD) $(ODFLAGS) $< > $(BIN_PATH)/$(PROJ_NAME).map
	@echo +++ Summary of memory consumption:
	@$(SZ) $(SZFLAGS) $<
	@echo +++ Build complete [$(notdir $@)]

$(BIN_PATH)/$(PROJ_NAME).hex : $(BIN_PATH)/$(PROJ_NAME).elf
	@mkdir -p $(@D)
	@$(OC) -O ihex $< $@

$(BIN_PATH)/$(PROJ_NAME).bin : $(BIN_PATH)/$(PROJ_NAME).elf
	@mkdir -p $(@D)
	@$(OC) -O binary $< $@

$(BIN_PATH)/$(PROJ_NAME).elf : $(AOBJS) $(COBJS)
	@mkdir -p $(@D)
	@echo +++ Linking [$(notdir $@)]
	@echo $(patsubst %.o,$(OBJ_PATH)/%.o,$(^F))
	@$(LN) $(LFLAGS) -o $@ $(patsubst %.o,$(OBJ_PATH)/%.o,$(^F)) $(LIBS)

#|--------------------------------------------------------------------------------------|
#| Compile and assemble                                                                 |
#|--------------------------------------------------------------------------------------|
$(AOBJS): %.o: %.s $(PROJ_CHDRS) $(OBJ_PATH)
	@mkdir -p $(@D)
	@echo +++ Assembling [$(notdir $<)]
	@$(AS) $(AFLAGS) -c $< -o $(OBJ_PATH)/$(@F)

$(COBJS): %.o: %.c $(PROJ_CHDRS) $(OBJ_PATH)
	@mkdir -p $(@D)
	@echo +++ Compiling [$(notdir $<)]
	@$(CC) $(CFLAGS) -c $< -o $(OBJ_PATH)/$(@F)

$(OBJ_PATH):
	@echo Compiler Options
	@echo $(CC) -c $(CFLAGS) main.c -o main.o
	@mkdir -p $(OBJ_PATH)

#|--------------------------------------------------------------------------------------|
#| Make CLEAN                                                                           |
#|--------------------------------------------------------------------------------------|
.PHONY: clean
clean: 
	@echo +++ Cleaning build environment
	@$(RM) $(RMFLAGS) $(foreach file,$(AOBJS),$(OBJ_PATH)/$(file))
	@$(RM) $(RMFLAGS) $(foreach file,$(COBJS),$(OBJ_PATH)/$(file))
	@$(RM) $(RMFLAGS) $(patsubst %.o,%.lst,$(foreach file,$(COBJS),$(OBJ_PATH)/$(file)))
	@$(RM) $(RMFLAGS) $(BIN_PATH)/$(PROJ_NAME).elf $(BIN_PATH)/$(PROJ_NAME).map
	@$(RM) $(RMFLAGS) $(BIN_PATH)/$(PROJ_NAME).srec
	@$(RM) $(RMFLAGS) $(BIN_PATH)/$(PROJ_NAME).bin
	@$(RM) $(RMFLAGS) $(BIN_PATH)/$(PROJ_NAME).hex
	@echo +++ Clean complete