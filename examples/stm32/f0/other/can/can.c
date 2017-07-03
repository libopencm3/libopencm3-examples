/*
 * This file is NOT part of the libopencm3 project. The project maintainers
 * might feel free to include it into their examples. Otherwise it might
 * just rest here for users interested in it.
 *
 * At the time of writing this, you need a patched libopencm3, enabling
 * CAN for STM32F0 as suggested in
 * https://github.com/hallo-alex/libopencm3/commit/13cf8bac005d6b85aaaaa12ce0f5bf56578d0426
 *
 *
 * This file is derived from examples/stm32/f1/obldc/can/can.c
 *
 * Additions/changes 2017 by Alex Kist
 *
 * Original copyright:
 * Copyright (C) 2010 Thomas Otto <tommi@viadmin.org>
 * Copyright (C) 2010 Piotr Esden-Tempski <piotr@esden.net>
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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/can.h>


/* 500kbit/s -> CAN_BTR = 0x001c0005 source:
 * http://www.bittiming.can-wiki.info/ */
#define BTR500k 0x001c0005

/* to get the values needed by can_init from a BTR register value */
#define TS1FROMBTRVAL(value)	(value & 0xF0000)
#define TS2FROMBTRVAL(value)	(value & 0x700000)
#define SJWFROMBTRVAL(value)	(value & 0x3000000)
#define BRPFROMBTRVAL(value)	((value & 0x1FF)+1)


struct can_tx_msg {
	uint32_t std_id;
	uint32_t ext_id;
	uint8_t ide;
	uint8_t rtr;
	uint8_t dlc;
	uint8_t data[8];
};

struct can_rx_msg {
	uint32_t std_id;
	uint32_t ext_id;
	uint8_t ide;
	uint8_t rtr;
	uint8_t dlc;
	uint8_t data[8];
	uint8_t fmi;
};

struct can_tx_msg can_tx_msg;
struct can_rx_msg can_rx_msg;


static void systick_setup(void)
{
	/* 72MHz / 8 => 9000000 counts per second */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);

	/* 9000000/9000 = 1000 overflows per second - every 1ms one interrupt */
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(8999);

	systick_interrupt_enable();

	/* Start counting. */
	systick_counter_enable();
}

static void can_setup(void)
{
	/* Enable peripheral clocks. */
	rcc_periph_clock_enable(RCC_GPIOB);

	rcc_periph_clock_enable(RCC_CAN);

	/* Configure CAN pin: RX Alternate Function (input pull-up). */
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO8);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO8);

	/* Configure CAN pin: TX Alternate Function */
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO9);

	/* Reset CAN. */
	can_reset(CAN1);

	/* CAN cell init. */
	if (can_init(CAN1,
			false,     /* TTCM: Time triggered comm mode? */
			true,      /* ABOM: Automatic bus-off management? */
			false,     /* AWUM: Automatic wakeup mode? */
			true,      /* NART: No automatic retransmission? */
			false,     /* RFLM: Receive FIFO locked mode? */
			false,     /* TXFP: Transmit FIFO priority? */
			SJWFROMBTRVAL(BTR500k),
			TS1FROMBTRVAL(BTR500k),
			TS2FROMBTRVAL(BTR500k),
			BRPFROMBTRVAL(BTR500k),
			false,     /* Loopback */
			false)) {  /* Silent */

		/* can init returned a non zero value
		 * this can be caused by wrong configuration of the alternate
		 * function pins,or other reasons keeping the can module from
		 * reading 11 recessive bits for leaving initialization phase
		 */
		while (1);
	}
	/* CAN filter 0 init. */
	can_filter_id_mask_32bit_init(CAN1,
			0,     /* Filter ID */
			0,     /* CAN ID */
			0,     /* CAN ID mask */
			0,     /* FIFO assignment (here: FIFO0) */
			true); /* Enable the filter. */

	/* NVIC setup. */
	nvic_enable_irq(NVIC_CEC_CAN_IRQ);
	nvic_set_priority(NVIC_CEC_CAN_IRQ, 1);

	/* Enable CAN RX interrupt. */
	can_enable_irq(CAN1, CAN_IER_FMPIE0);
}


uint8_t rxcnt;

void sys_tick_handler(void)
{
	static int temp32 = 0;
	static uint8_t data[8] = {0, 1, 2, 0, 0, 0, 0, 0};

	/* We call this handler every 1ms so 1000ms = 1s on/off. */
	if (++temp32 != 1000) {
		return;
	}

	temp32 = 0;

	/* Transmit CAN frame. */
	data[0]++;

	data[7] = rxcnt; /* send count of received messages in last byte */
	/* "running your can transmission directly from within the systick
	 *  IRQ doesn't seem like very good style to use as an example" [karlp]
	 *
	 * As this is a derived work from examples/stm32/f1/obldc/can/can.c
	 * it uses the same send mechanism as the original
	 */
	can_transmit(CAN1,
			0,     /* (EX/ST)ID: CAN ID */
			false, /* IDE: CAN ID extended? */
			false, /* RTR: Request transmit? */
			8,     /* DLC: Data length */
			data);

}

void cec_can_isr(void)
{
	uint32_t id, fmi;
	bool ext, rtr;
	uint8_t length, data[8];

	can_receive(CAN1, 0, false, &id, &ext, &rtr, &fmi, &length, data);

	can_fifo_release(CAN1, 0);
	rxcnt++;
}

int main(void)
{
	rcc_clock_setup_in_hsi_out_48mhz();
	can_setup();
	systick_setup();

	while (1);

	return 0;
}
