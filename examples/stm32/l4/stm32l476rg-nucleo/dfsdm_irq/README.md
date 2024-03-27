This demonstrates the DFSDM on the L476RG nucleo board.

* printf via usart1 TX on Pin A9 115200@8n1
* this example assumes that there is a sigma delta modulator connected to pins B1 (CLK) and C2 (DATA) e.g. TI AMC1306
* pll configuration from hsi
* dfsdm0_isr reads the data regularily as soon as they are available.
