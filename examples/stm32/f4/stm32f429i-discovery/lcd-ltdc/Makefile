OBJS = clock.o gfx_locm3.o lcd_ili9341.o sdram.o Tamsyn5x9b_9.o Tamsyn5x9r_9.o

BINARY = application

# we use sin/cos from the library
LDLIBS += -lm

LDSCRIPT = ../stm32f429i-discovery.ld

include ../../Makefile.include
