OBJS     = clock.o gfx_locm3.o lcd_ili9341.o sdram.o
OBJS    += fonts/Tamsyn5x9b_9.o fonts/Tamsyn5x9r_9.o
OBJS    += vector_gfx/bezier.o
OBJS    += i2c.o touchscreen_controller_stmpe811.o

BINARY   = application

CFLAGS   = -O3 -g

# we use sin/cos from the library
LDLIBS   = -lm

LDSCRIPT = ../stm32f429i-discovery.ld

include ../../Makefile.include
