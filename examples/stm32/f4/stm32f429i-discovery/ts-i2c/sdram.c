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
 * This then is the initialization code extracted from the
 * sdram example.
 */
#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/fsmc.h>
#include "clock.h"
#include "sdram.h"

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
sdram_init(void) {
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
#if 1
	rcc_periph_clock_enable(RCC_FSMC);
#else
	rcc_peripheral_enable_clock(&RCC_AHB3ENR, RCC_AHB3ENR_FMCEN);
#endif

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
