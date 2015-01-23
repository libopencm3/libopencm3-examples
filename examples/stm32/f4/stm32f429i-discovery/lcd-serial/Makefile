OBJS = sdram.o clock.o console.o lcd-spi.o gfx.o

BINARY = lcd-serial

# we use sin/cos from the library
LDLIBS += -lm

LDSCRIPT = ../stm32f429i-discovery.ld

include ../../Makefile.include
