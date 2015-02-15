#include "lcd-spi.h"

/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Initialize the ST Micro TFT Display for DMA video using the SPI port
 */
#include <stddef.h>
#include <stdio.h>

#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#include "clock.h"

#define LCD_SPI SPI5

/*
 * This is an ungainly workaround (aka hack) basically I want to know
 * when the SPI port is 'done' sending all of the bits out, and it is
 * done when it has clocked enough bits that it would have received a
 * byte. Since we're using the SPI port in write_only mode I am not
 * collecting the "received" bytes into a buffer, but one could of
 * course. I keep track of how many bytes should have been returned
 * by decrementing the 'rx_pend' volatile. When it reaches 0 we know
 * we are done.
 */

static volatile int rx_pend;
static volatile uint16_t spi_rx_buf;

/*
 * This is the ISR we use. Note that the name is based on the name
 * in the irq.json file of libopencm3 plus the "_isr" extension.
 */
void
spi5_isr(void)
{
	spi_rx_buf = SPI_DR(SPI5);
	--rx_pend;
}

/*
 * For the STM32-DISCO board, SPI pins in use:
 *  N/C  - RESET
 *  PC2  - CS (could be NSS but won't be)
 *  PF7  - SCLK (AF5) SPI5
 *  PD13 - DATA / CMD*
 *  PF9  - MOSI (AF5) SPI5
 */

/*
 * void lcd_command(cmd, delay, args, arg_ptr)
 *
 * All singing all dancing 'do a command' feature. Basically it
 * sends a command, and if args are present it sets 'data' and
 * sends those along too.
 */
static void
lcd_command(uint8_t cmd, int delay, int n_args, const uint8_t *args)
{
	uint32_t timeout;
	int i;

	gpio_clear(GPIOC, GPIO2); /* Select the LCD */
	rx_pend++;
	spi_send(SPI5, cmd);
	/* We need to wait until it is sent, if we turn on the Data
	 * line too soon, it ends up confusing the display to thinking
	 * its a data transfer, as it samples the D/CX line on the last
	 * bit sent.
	 */
	for (timeout = 0; (timeout < 1000) && (rx_pend); timeout++) {
		continue;
	}
	rx_pend = 0;		/* sometimes, at 10Mhz we miss this */
	if (n_args) {
		gpio_set(GPIOD, GPIO13); /* Set the D/CX pin */
		for (i = 0; i < n_args; i++) {
			rx_pend++;
			spi_send(SPI5, *(args+i));
		}
		/* This wait so that we don't pull CS too soon after
		 * sending the last byte of data.
		 */
		for (timeout = 0; (timeout < 1000) && (rx_pend); timeout++) {
			continue;
		}
	}
	gpio_set(GPIOC, GPIO2);	   /* Turn off chip select */
	gpio_clear(GPIOD, GPIO13); /* always reset D/CX */
	if (delay) {
		milli_sleep(delay); /* wait, if called for */
	}
}

/* Notes on the less obvious ILI9341 commands: */

/*
 * ILI9341 datasheet, pp 46-49:
 *
 *     RCM[1:0} = 0b10    command 0xb0
 *     DPI[2:0] = 0b110   command 0x3a
 *     RIM      = 0       command 0xf6
 *     PCDIV    = ????    command 0xB6
 *
 * Pp 239-240:
 *     external fosc = DOTCLK / (2 * (PCDIV + 1))
 *
 * ("Cube" is how the STM32F4Cube demo software sets the register.
 *  "Chuck" is ChuckM's lcd-serial demo, first revision.)
 *
 * Command 0x3A: COLMOD: Pixel Format Set  LCD_PIXEL_FORMAT
 *                Reset              Cube   Chuck
 *      DPI[2:0]   110 (18 bit/pix)   110    101 (16 bit/pix)
 *      DBI[2:0]   110 (18 bit/pix)   110    101 (16 bit/pix)
 *
 * Command 0xB0: RGB Interface Signal      LCD_RGB_INTERFACE
 *                Reset              Cube
 *      Bypass:      0 (direct)        1 (memory)
 *      RCM[1:0]    10                10
 *      VSPL         0 (low)           0
 *      HSPL         0 (low)           0
 *      DPL          0 (rising)        1 (falling)
 *      EPL          1 (low)           0 (high)
 *
 * Command 0xB6: Display Function Control  LCD_DFC
 *                Reset              Cube 0A A7 27 04
 *      PTG[1:0]    10                10
 *      PT[1:0]     10                10
 *      REV          1                 1
 *      GS           0                 0
 *      SS           0 (S1->S720)      1 (S720->S1)
 *      SM           0                 0
 *      ISC[3:0]  0010 (5 frames)   0111 (15 frames)
 *      NL[5:0]   100111          100111
 *      PCDIV[5:0]   ?            000100
 *   S720->S1 moves the origin from the lower left corner to lower right
 *   (viewing the board so the silkscreen is upright)
 *
 * Command 0xF6: Interface Control         LCD_INTERFACE
 *               Reset              Cube  01 00 06
 *      MY_EOR       0                 0
 *      MX_EOR       0                 0
 *      MV_EOR       0                 0
 *      BGR_EOR      0                 0
 *      WEMODE       1 (wrap)          1
 *      EPF[1:0]    00                00
 *      MDT[1:0]    00                00
 *      ENDIAN       0 (MSB first)     0
 *      DM[1:0]     00 (int clk)      01 (RGB ifc)
 *      RM           0 (sys ifc)       1 (RGB ifc)
 *      RIM          0 (1 xfr/pix)     0
 */

/* ILI9341 command definitions */

/* Regulative[sic] Command Set */
#define ILI_NOP                 0x00
#define ILI_RESET               0x01
#define ILI_RD_DID              0x04
#define ILI_RD_STS              0x09
#define ILI_RD_PWR_MODE         0x0a
#define ILI_RD_MADCTL           0x0b
#define ILI_RD_PXL_FMT          0x0c
#define ILI_PD_IMG_FMT          0x0d
#define ILI_RD_SIG_MODE         0x0e
#define ILI_RD_DIAG_RSLT        0x0f
#define ILI_ENTER_SLEEP         0x10
#define ILI_SLEEP_OUT           0x11
#define ILI_PARTIAL_ON          0x12
#define ILI_NORMAL_MODE_ON      0x13
#define ILI_INVERSE_ON          0x20
#define ILI_INVERSE_OFF         0x21
#define ILI_GAMMA_SET           0x26
#define ILI_DISP_OFF            0x28
#define ILI_DISP_ON             0x29
#define ILI_CAS                 0x2a
#define ILI_PAS                 0x2b
#define ILI_MEM_WRITE           0x2c
#define ILI_COLOR_SET           0x2d
#define ILI_MEM_READ            0x2e
#define ILI_PARTIAL_AREA        0x30
#define ILI_VERT_SCROLL_DEF     0x33
#define ILI_TEAR_EFF_OFF        0x34
#define ILI_TEAR_EFF_ON         0x35
#define ILI_MEM_ACC_CTL         0x36
#define ILI_V_SCROLL_START      0x37
#define ILI_IDLE_OFF            0x38
#define ILI_IDLE_ON             0x39
#define ILI_PIX_FMT_SET         0x3a
#define ILI_WR_MEM_CONT         0x3c
#define ILI_RD_MEM_CONT         0x3e
#define ILI_SET_TEAR_LINE       0x44
#define ILI_GET_SCANLINE        0x45
#define ILI_WR_BRIGHTNESS       0x51
#define ILI_RD_BRIGHTNESS       0x52
#define ILI_WR_CTRL             0x53
#define ILI_RD_CTRL             0x54
#define ILI_WR_CABC             0x55
#define ILI_RD_CABC             0x56
#define ILI_WR_CABC_MIN         0x5e
#define ILI_RD_CABC_MAX         0x5f
#define ILI_RD_ID1              0xda
#define ILI_RD_ID2              0xdb
#define ILI_RD_ID3              0xdc

/* Extended Command Set */
#define ILI_RGB_IFC_CTL         0xb0
#define ILI_FRM_CTL_NORM        0xb1
#define ILI_FRM_CTL_IDLE        0xb2
#define ILI_FRM_CTL_PART        0xb3
#define ILI_INVERSE_CTL         0xb4
#define ILI_PORCH_CTL           0xb5
#define ILI_FUNC_CTL            0xb6
#define ILI_ENTRY_MODE_SET      0xb7
#define ILI_BL_CTL_1            0xb8
#define ILI_BL_CTL_2            0xb9
#define ILI_BL_CTL_3            0xba
#define ILI_BL_CTL_4            0xbb
#define ILI_BL_CTL_5            0xbc
#define ILI_BL_CTL_7            0xbe
#define ILI_BL_CTL_8            0xbf
#define ILI_PWR_CTL_1           0xc0
#define ILI_PWR_CTL_2           0xc1
#define ILI_VCOM_CTL_1          0xc5
#define ILI_VCOM_CTL_2          0xc7
#define ILI_NV_MEM_WR           0xd0
#define ILI_NV_MEM_PROT_KEY     0xd1
#define ILI_NV_MEM_STATUS_RD    0xd2
#define ILI_RD_ID4              0xd3
#define ILI_POS_GAMMA           0xe0
#define ILI_NEG_GAMMA           0xe1
#define ILI_GAMMA_CTL_1         0xe2
#define ILI_GAMMA_CTL_2         0xe3
#define ILI_IFC_CTL             0xf6

/*
 * This structure defines the sequence of commands to send
 * to the Display in order to initialize it. The AdaFruit
 * folks do something similar, it helps when debugging the
 * initialization sequence for the display.
 */

#define MAX_INLINE_ARGS (sizeof(uint8_t *))
struct tft_command {
	uint16_t delay;		/* If you need a delay after */
	uint8_t cmd;		/* command to send */
	uint8_t n_args;		/* How many arguments it has */
	union {
		uint8_t args[MAX_INLINE_ARGS]; /* The first four arguments */
		const uint8_t *aptr; /* More than four arguemnts */
	};
};

static const uint8_t pos_gamma_args[] = { 0x0F, 0x29, 0x24, 0x0C, 0x0E,
					  0x09, 0x4E, 0x78, 0x3C, 0x09,
					  0x13, 0x05, 0x17, 0x11, 0x00 };
static const uint8_t neg_gamma_args[] = { 0x00, 0x16, 0x1B, 0x04, 0x11,
					  0x07, 0x31, 0x33, 0x42, 0x05,
					  0x0C, 0x0A, 0x28, 0x2F, 0x0F };

/*
 * These are the commands we're going to send to the
 * display to initialize it. We send them all, in sequence
 * with occasional delays. Commands that require data bytes
 * as arguments, indicate how many bytes to pull out the
 * above array to include.
 *
 * The sequence was pieced together from the ST Micro demo
 * code, the data sheet, and other sources on the web.
 */
#define EXPERIMENT 1
const struct tft_command initialization[] = {
	{  0, ILI_PWR_CTL_1,        1, .args = { 0x10 } },
	{  0, ILI_PWR_CTL_2,        1, .args = { 0x10 } },
	{  0, ILI_VCOM_CTL_1,       2, .args = { 0x45, 0x15 } },
	{  0, ILI_VCOM_CTL_2,       1, .args = { 0x90 } },
	{  0, ILI_MEM_ACC_CTL,      1, .args = { 0x08 } },
	{  0, ILI_RGB_IFC_CTL,      1, .args = { 0xc0 } },
	{  0, ILI_IFC_CTL,          3, .args = { 0x01, 0x00, 0x06 } },
	{  0, ILI_GAMMA_SET,        1, .args = { 0x01 } },
	{  0, ILI_POS_GAMMA,       15, .aptr = pos_gamma_args },
	{  0, ILI_NEG_GAMMA,       15, .aptr = neg_gamma_args },
	{ +5, ILI_SLEEP_OUT,        0, .args = {} },
	{  0, ILI_DISP_ON,          0, .args = {} },
};

static void
initialize_display(const struct tft_command cmds[], size_t cmd_count)
{
	size_t i;

	for (i = 0; i < cmd_count; i++) {
		uint8_t arg_count = cmds[i].n_args;
		const uint8_t *args = cmds[i].args;
		if (arg_count > MAX_INLINE_ARGS) {
			args = cmds[i].aptr;
		}
		lcd_command(cmds[i].cmd, cmds[i].delay, arg_count, args);
	}
}

/*
 * void lcd_spi_init(void)
 *
 * Initialize the SPI port, and the through that port
 * initialize the LCD controller. Note that this code
 * will expect to be able to draw into the SDRAM on
> * the board, so the sdram much be initialized before
 * calling this function.
 *
 * SPI Port and GPIO Defined - for STM32F4-Disco
 *
 * LCD_CS      PC2
 * LCD_SCK     PF7
 * LCD_DC      PD13
 * LCD_MOSI    PF9
 * LCD_SPI     SPI5
 * LCD_WIDTH   240
 * LCD_HEIGHT  320
 */
void
lcd_spi_init(void)
{
	uint32_t tmp;

	/*
	 * Set up the GPIO lines for the SPI port and
	 * control lines on the display.
	 */
	rcc_periph_clock_enable(RCC_GPIOC | RCC_GPIOD | RCC_GPIOF);

	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2);
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

	gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7 | GPIO9);
	gpio_set_af(GPIOF, GPIO_AF5, GPIO7 | GPIO9);

	rx_pend = 0;
	/* Implement state management hack */
	nvic_enable_irq(NVIC_SPI5_IRQ);

	rcc_periph_clock_enable(RCC_SPI5);
	/* This should configure SPI5 as we need it configured */
	tmp = SPI_SR(LCD_SPI);
	SPI_CR2(LCD_SPI) |= (SPI_CR2_SSOE | SPI_CR2_RXNEIE);

	/* device clocks on the rising edge of SCK with MSB first */
	tmp = SPI_CR1_BAUDRATE_FPCLK_DIV_4 | /* 10.25Mhz SPI Clock (42M/4) */
	      SPI_CR1_MSTR |                 /* Master Mode */
	      SPI_CR1_BIDIOE |               /* Write Only */
	      SPI_CR1_SPE;                   /* Enable SPI */

	SPI_CR1(LCD_SPI) = tmp; /* Do it. */
	if (SPI_SR(LCD_SPI) & SPI_SR_MODF) {
		SPI_CR1(LCD_SPI) = tmp; /* Re-writing will reset MODF */
		fprintf(stderr, "Initial mode fault.\n");
	}

	/* Set up the display */
	initialize_display(initialization,
			   sizeof(initialization) / sizeof(initialization[0]));
}
