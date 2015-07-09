OBJS = sdram.o clock.o lcd-spi.o gfx.o stmpe811.o

BINARY = ts-i2c

# we use sin/cos from the library
LDLIBS += -lm

LDSCRIPT = ../stm32f429i-discovery.ld

include ../../Makefile.include
