# README

This directory contains examples for the stm32f429i discovery board.
The PCB should be labelled either MB1075B, and contains a user USB
port, 2.4" QVGA TFT LCD display, 64Mbits external SDRAM and ST MEMS gyroscope.

http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/PF259090

# Chuck McManis README

These examples are designed to demonstrate the use of libopencm3
with the STM32F4Discovery-DISCO board. This board has a 2.2"
TFT LCD touchscreen on it, a MEMS gyroscope, and 8MB of SDRAM.

If you move through the examples in this order, the code from
the previous example will be used in the next example:

0. miniblink - verify that you can build a program, link it, and download it to
   the board. Blinks the GREEN LED at about 2Hz

1. tick\_blink - Clock setup, Systick setup, LED GPIO setup and blinking.

2. usart\_console - Program a USART on the board as a console (requires
   clonsing jumpers on your discovery board connecting USART1 to the
   programmer)

3. usart\_irq\_console - Program a USART on the board as a console with an
   interrupt driven receive routine. This allows you to interrupt execution
   with ^C as you can on a Linux process.

4. sdram - SDRAM setup, using the usb port as a console, which sets up the
   SDRAM

5. spi - Serial Peripheral Interface example which talks to the MEMS gyroscope
   on the DISCO board.

6. lcd-serial - Activates the TFT using the SPI port (serial) and holds a frame
   buffer in the SDRAM area.

7. lcd - Now uses the new LCD "driver" peripheral to refresh the contents with
   what is in memory, very fast, write in memory and it appears on screen.

8. dma2d - The 2D graphics accelerator device which displays various animations
   on the LCD using code from all of the previous examples.
