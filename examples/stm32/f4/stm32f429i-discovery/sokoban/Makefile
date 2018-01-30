###############################################################
# Makefile to compile sokoban game for STM32 DISCOVERY using libopencm3
#
# Since there are some additional tasks to compile, i use an independent Makefile
# There is still the Makefile.bk that uses the included files like the other examples.
# I use the same Makefile to compile in Linux and in Windows.

## Copyright (C) 2018 Marcos Augusto Stemmer <mastemmer@gmail.com>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##
################################################################
TARGET	 = sokoban
CSOURCES = sokoban.c uart-int.c sdram.c mprintf.c ili9341c.c i2c3_touch.c levels.c
ASOURCES =
HEADERS	 = ili9341.h sdram.h uart.h i2c3_touch.h
BITMAPS = guy.bmp wall.bmp box.bmp box2.bmp bigguy.bmp
LDSCRIPT	= stm32f429i-discovery.ld
LIBNAME		= opencm3_stm32f4
DEFS		+= -DSTM32F4

STLINK_UTILITY_DIR = "C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility"

##################################################
# Don't show commands; use 'make V=1' to show.
ifneq ($(V),1)
Q := @
endif
# Check if in Windows
ifeq ($(shell echo "check_quotes"),"check_quotes")
WINDOWS := yes
else
WINDOWS := no
endif
#####################################

ifeq ($(WINDOWS),yes)
# In Windows I use a fixed path to libopencm3 #
OPENCM3_DIR	= C:\arm-none-eabi\lib\libopencm3
else

## This script to serach for OPENCM3_DIR was taken from file rules.mk ##
ifeq ($(strip $(OPENCM3_DIR)),)
# user has not specified the library path, so we try to detect it

# where we search for the library
LIBPATHS := ./libopencm3 ../../../../libopencm3 ../../../../../libopencm3

OPENCM3_DIR := $(wildcard $(LIBPATHS:=/locm3.sublime-project))
OPENCM3_DIR := $(firstword $(dir $(OPENCM3_DIR)))

ifeq ($(strip $(OPENCM3_DIR)),)
$(warning Cannot find libopencm3 library in the standard search paths.)
$(error Please specify it through OPENCM3_DIR variable!)
endif
endif

ifeq ($(V),1)
$(info Using $(OPENCM3_DIR) path to library)
endif
endif

INCLUDE_DIR	= $(OPENCM3_DIR)/include
LIB_DIR		= $(OPENCM3_DIR)/lib
SCRIPT_DIR	= $(OPENCM3_DIR)/scripts

#####################################
ifeq ($(WINDOWS),yes)
PREFIX		?= c:\arm-none-eabi\bin\arm-none-eabi
DIRLOC =
else
PREFIX		?= arm-none-eabi
DIRLOC = ./
endif

CC		:= $(PREFIX)-gcc
CXX		:= $(PREFIX)-g++
LD		:= $(PREFIX)-gcc
AR		:= $(PREFIX)-ar
AS		:= $(PREFIX)-as
OBJCOPY		:= $(PREFIX)-objcopy
OBJDUMP		:= $(PREFIX)-objdump
GDB		:= $(PREFIX)-gdb
###############################################################################
# C flags

CFLAGS		+= -Os -g
CFLAGS		+= -Wextra -Wshadow -Wimplicit-function-declaration
CFLAGS		+= -Wredundant-decls -Wstrict-prototypes
CFLAGS		+= -fno-common -ffunction-sections -fdata-sections
###############################################################################
# C & C++ preprocessor and linker common flags

CPPFLAGS	+= -MD
CPPFLAGS	+= -Wall -Wundef
CPPFLAGS	+= -I$(INCLUDE_DIR) $(DEFS)
FP_FLAGS	?= -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS	= -mthumb -mcpu=cortex-m4 $(FP_FLAGS)

# Suffix rules
COBJ = $(CSOURCES:.c=.o)
AOBJ = $(ASOURCES:.S=.o)
OBJECTS = $(COBJ) $(AOBJ)
###############################################################################
# Linker flags

LDFLAGS		+= --static -nostartfiles
LDFLAGS		+= -L$(LIB_DIR)
LDFLAGS		+= -T$(LDSCRIPT)
LDFLAGS		+= -Wl,-Map=$(*).map
LDFLAGS		+= -Wl,--gc-sections

###############################################################################
# Used libraries

LDLIBS		+= -l$(LIBNAME)
LDLIBS		+= -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

### Targets: ##########
hex: $(TARGET).hex
bin: $(TARGET).bin

#Convert elf to hex
%.hex: %.elf
	@echo HEX  $<
	$(Q)$(OBJCOPY) -O ihex $< $@

%.bin: %.elf
	@echo BIN  $<
	$(Q)$(OBJCOPY) -Obinary $< $@

bmp2c: bmp2c.c
	gcc -o bmp2c bmp2c.c -Wall

bmp.c: bmp2c $(BITMAPS)
	$(DIRLOC)bmp2c $(BITMAPS) > bmp.c

# Call c compiler
%.o: %.c $(HEADERS) bmp.c
	@echo CC   $<
	$(Q)$(CC) -c $(CFLAGS) $(CPPFLAGS) $(ARCH_FLAGS) -o $@ $<

# Linker
$(TARGET).elf: $(OBJECTS) $(LDSCRIPT) $(LIB_DIR)/lib$(LIBNAME).a
	@echo LD   $(OBJECTS)
	$(Q)$(LD) $(LDFLAGS) $(ARCH_FLAGS) $(OBJECTS) $(LDLIBS) -o $(TARGET).elf

ifeq ($(WINDOWS),yes)
clean:
	-del $(OBJECTS) *.d $(TARGET).hex $(TARGET).bin $(TARGET).elf .map bmp2c.exe bmp.c
# program flash using ST-LINK_CLI for Windows
flash: $(TARGET).hex
	$(STLINK_UTILITY_DIR)\ST-LINK_CLI -V -P $(TARGET).hex 0x8000000
else
clean:
	-rm -f $(OBJECTS) *.d $(TARGET).hex $(TARGET).bin $(TARGET).elf .map bmp2c bmp.c
# Program flash using st-flash for Linux
flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x8000000
endif

.PHONY:	clean flash hex bin
