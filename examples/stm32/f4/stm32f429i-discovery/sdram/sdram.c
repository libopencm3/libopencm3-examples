/*
 * sdram.c - SDRAM controller example
 *
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Chuck McManis <cmcmanis@mcmanis.com>
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

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/fsmc.h>
#include "clock.h"
#include "console.h"

#define SDRAM_BASE_ADDRESS ((uint8_t *)(0xd0000000))

void sdram_init(void);

#ifndef NULL
#define NULL	(void *)(0)
#endif

/*
 * This is just syntactic sugar but it helps, all of these
 * GPIO pins get configured in exactly the same way.
 */
static struct {
	uint32_t	gpio;
	uint16_t	pins;
} sdram_pins[6] = {
	{GPIOB, GPIO5 | GPIO6 },
	{GPIOC, GPIO0 },
	{GPIOD, GPIO0 | GPIO1 | GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15},
	{GPIOE, GPIO0 | GPIO1 | GPIO7 | GPIO8 | GPIO9 | GPIO10 |
			GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15 },
	{GPIOF, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO11 |
			GPIO12 | GPIO13 | GPIO14 | GPIO15 },
	{GPIOG, GPIO0 | GPIO1 | GPIO4 | GPIO5 | GPIO8 | GPIO15}
};

static struct sdram_timing timing = {
	.trcd = 2,		/* RCD Delay */
	.trp = 2,		/* RP Delay */
	.twr = 2,		/* Write Recovery Time */
	.trc = 7,		/* Row Cycle Delay */
	.tras = 4,		/* Self Refresh Time */
	.txsr = 7,		/* Exit Self Refresh Time */
	.tmrd = 2,		/* Load to Active Delay */
};

/*
 * Initialize the SD RAM controller.
 */
void
sdram_init(void)
{
	int i;
	uint32_t cr_tmp, tr_tmp; /* control, timing registers */

	/*
	* First all the GPIO pins that end up as SDRAM pins
	*/
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOE);
	rcc_periph_clock_enable(RCC_GPIOF);
	rcc_periph_clock_enable(RCC_GPIOG);

	for (i = 0; i < 6; i++) {
		gpio_mode_setup(sdram_pins[i].gpio, GPIO_MODE_AF,
				GPIO_PUPD_NONE, sdram_pins[i].pins);
		gpio_set_output_options(sdram_pins[i].gpio, GPIO_OTYPE_PP,
				GPIO_OSPEED_50MHZ, sdram_pins[i].pins);
		gpio_set_af(sdram_pins[i].gpio, GPIO_AF12, sdram_pins[i].pins);
	}

	/* Enable the SDRAM Controller */
	rcc_periph_clock_enable(RCC_FSMC);

	/* Note the STM32F429-DISCO board has the ram attached to bank 2 */
	/* Timing parameters computed for a 168Mhz clock */
	/* These parameters are specific to the SDRAM chip on the board */

	cr_tmp  = FMC_SDCR_RPIPE_1CLK;
	cr_tmp |= FMC_SDCR_SDCLK_2HCLK;
	cr_tmp |= FMC_SDCR_CAS_3CYC;
	cr_tmp |= FMC_SDCR_NB4;
	cr_tmp |= FMC_SDCR_MWID_16b;
	cr_tmp |= FMC_SDCR_NR_12;
	cr_tmp |= FMC_SDCR_NC_8;

	/* We're programming BANK 2, but per the manual some of the parameters
	 * only work in CR1 and TR1 so we pull those off and put them in the
	 * right place.
	 */
	FMC_SDCR1 |= (cr_tmp & FMC_SDCR_DNC_MASK);
	FMC_SDCR2 = cr_tmp;

	tr_tmp = sdram_timing(&timing);
	FMC_SDTR1 |= (tr_tmp & FMC_SDTR_DNC_MASK);
	FMC_SDTR2 = tr_tmp;

	/* Now start up the Controller per the manual
	 *	- Clock config enable
	 *	- PALL state
	 *	- set auto refresh
	 *	- Load the Mode Register
	 */
	sdram_command(SDRAM_BANK2, SDRAM_CLK_CONF, 1, 0);
	msleep(1); /* sleep at least 100uS */
	sdram_command(SDRAM_BANK2, SDRAM_PALL, 1, 0);
	sdram_command(SDRAM_BANK2, SDRAM_AUTO_REFRESH, 4, 0);
	tr_tmp = SDRAM_MODE_BURST_LENGTH_2				|
				SDRAM_MODE_BURST_TYPE_SEQUENTIAL	|
				SDRAM_MODE_CAS_LATENCY_3		|
				SDRAM_MODE_OPERATING_MODE_STANDARD	|
				SDRAM_MODE_WRITEBURST_MODE_SINGLE;
	sdram_command(SDRAM_BANK2, SDRAM_LOAD_MODE, 1, tr_tmp);

	/*
	 * set the refresh counter to insure we kick off an
	 * auto refresh often enough to prevent data loss.
	 */
	FMC_SDRTR = 683;
	/* and Poof! a 8 megabytes of ram shows up in the address space */
}

/*
 * This code are some routines that implement a "classic"
 * hex dump style memory dump. So you can look at what is
 * in the RAM, alter it (in a couple of automated ways)
 */
void dump_byte(uint8_t b);
void dump_long(uint32_t l);
uint8_t *dump_line(uint8_t *, uint8_t *);
uint8_t *dump_page(uint8_t *, uint8_t *);


/* make a nybble into an ascii hex character 0 - 9, A-F */
#define HEX_CHAR(x)	((((x) + '0') > '9') ? ((x) + '7') : ((x) + '0'))

/* send an 8 bit byte as two HEX characters to the console */
void dump_byte(uint8_t b)
{
	console_putc(HEX_CHAR((b >> 4) & 0xf));
	console_putc(HEX_CHAR(b & 0xf));
}

/* send a 32 bit value as 8 hex characters to the console */
void dump_long(uint32_t l)
{
	int i = 0;
	for (i = 0; i < 8; i++) {
		console_putc(HEX_CHAR((l >> (28 - i * 4)) & 0xf));
	}
}

/*
 * dump a 'line' (an address, 16 bytes, and then the
 * ASCII representation of those bytes) to the console.
 * Takes an address (and possiblye a 'base' parameter
 * so that you can offset the address) and sends 16
 * bytes out. Returns the address +16 so you can
 * just call it repeatedly and it will send the
 * next 16 bytes out.
 */
uint8_t *
dump_line(uint8_t *addr, uint8_t *base)
{
	uint8_t *line_addr;
	uint8_t b;
	uint32_t tmp;
	int i;

	line_addr = addr;
	tmp = (uint32_t)line_addr - (uint32_t) base;
	dump_long(tmp);
	console_puts(" | ");
	for (i = 0; i < 16; i++) {
		dump_byte(*(line_addr+i));
		console_putc(' ');
		if (i == 7) {
			console_puts("  ");
		}
	}
	console_puts("| ");
	for (i = 0; i < 16; i++) {
		b = *line_addr++;
		console_putc(((b > 126) || (b < 32)) ? '.' : (char) b);
	}
	console_puts("\n");
	return line_addr;
}


/*
 * dump a 'page' like the function dump_line except this
 * does 16 lines for a total of 256 bytes. Back in the
 * day when you had a 24 x 80 terminal this fit nicely
 * on the screen with some other information.
 */
uint8_t *
dump_page(uint8_t *addr, uint8_t *base)
{
	int i;
	for (i = 0; i < 16; i++) {
		addr = dump_line(addr, base);
	}
	return addr;
}

/*
 * This example initializes the SDRAM controller and dumps
 * it out to the console. You can do various things like
 * (FI) fill with increment, (F0) fill with 0, (FF) fill
 * with FF. NP (next page), PP (prev page), NL (next line),
 * (PL) previous line, and (?) for help.
 */
int
main(void)
{
	int i;
	uint8_t *addr;
	char	c;

	clock_setup();
	console_setup();
	sdram_init();

	console_puts("SDRAM Example.\n");
	console_puts("Original data:\n");
	addr = (uint8_t *)(0xd0000000);
	(void) dump_page(addr, NULL);
	addr = SDRAM_BASE_ADDRESS;
	for (i = 0; i < 256; i++) {
		*(addr + i) = i;
	}
	console_puts("Modified data (with Fill Increment)\n");
	addr = SDRAM_BASE_ADDRESS;
	addr = dump_page(addr, NULL);
	while (1) {
		console_puts("CMD> ");
		switch (c = console_getc(1)) {
		case 'f':
		case 'F':
			console_puts("Fill ");
			switch (c = console_getc(1)) {
			case 'i':
			case 'I':
				console_puts("Increment\n");
				for (i = 0; i < 256; i++) {
					*(addr+i) = i;
				}
				dump_page(addr, NULL);
				break;
			case '0':
				console_puts("Zero\n");
				for (i = 0; i < 256; i++) {
					*(addr+i) = 0;
				}
				dump_page(addr, NULL);
				break;
			case 'f':
			case 'F':
				console_puts("Ones (0xff)\n");
				for (i = 0; i < 256; i++) {
					*(addr+i) = 0xff;
				}
				dump_page(addr, NULL);
				break;
			default:
				console_puts("Unrecognized Command, press ? for help\n");
			}
			break;
		case 'n':
		case 'N':
			console_puts("Next ");
			switch (c = console_getc(1)) {
			case 'p':
			case 'P':
				console_puts("Page\n");
				addr += 256;
				dump_page(addr, NULL);
				break;
			case 'l':
			case 'L':
				console_puts("Line\n");
				addr += 16;
				dump_line(addr, NULL);
				break;
			default:
				console_puts("Unrecognized Command, press ? for help\n");
			}
			break;
		case 'p':
		case 'P':
			console_puts("Previous ");
			switch (c = console_getc(1)) {
			case 'p':
			case 'P':
				console_puts("Page\n");
				addr -= 256;
				dump_page(addr, NULL);
				break;
			case 'l':
			case 'L':
				console_puts("Line\n");
				addr -= 16;
				dump_line(addr, NULL);
				break;
			default:
				console_puts("Unrecognized Command, press ? for help\n");
			}
			break;
		case '?':
		default:
			console_puts("Help\n");
			console_puts(" n p - dump next page\n");
			console_puts(" n l - dump next line\n");
			console_puts(" p p - dump previous page\n");
			console_puts(" p l - dump previous line\n");
			console_puts(" f 0 - fill current page with 0\n");
			console_puts(" f i - fill current page with 0 to 255\n");
			console_puts(" f f - fill current page with 0xff\n");
			console_puts(" ? - this message\n");
			break;
		}
	}
}
