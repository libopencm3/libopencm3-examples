BINARY  = application

OBJS    = clock.o sdram.o
OBJS   += $(patsubst %.c, %.o, $(wildcard support/*.c))

CFLAGS   = -O3

DEVICE   = stm32f769nih6
FP_FLAGS = -mfloat-abi=hard -mfpu=fpv5-d16

include ../../Makefile.include
