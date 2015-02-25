# README

This example program sends repeating sequence of characters "0123456789" on 
USART2 on the ST Nucleo F411RE eval board.

The sending is done in a blocking way.

Note that the Nucleo board exports the serial port as an ttyACM device on
Linux or a COM port on Windows. On Linux, if you can locate the correct
serial port as 

/dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_066CFF515056805087171825-if02

(note there will be differences based on the serial number of your board) 
this may also show up as /dev/ttyACM0 if it was the first serial port 
enumerated, if you have additional serial devices attached to the USB port
of your system they may show up as additional ttyACM1/ttyACM2/... etc. 

Connect to the port using `screen /dev/ttyACM0 115200` where you replace
the serial port with the one for your system. 

