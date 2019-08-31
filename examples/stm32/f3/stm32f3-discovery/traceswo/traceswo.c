/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Modified by Fernando Cortes <fermando.corcam@gmail.com>
 * modified by Guillermo Rivera <memogrg@gmail.com>
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
#include <libopencm3/cm3/tpiu.h>
#include <libopencm3/cm3/scs.h>
#include <libopencm3/cm3/itm.h>
#include <libopencm3/stm32/dbgmcu.h>


static void trace_setup(uint32_t baudrate)
{
    // enable TPIU registers
    SCS_DEMCR |= SCS_DEMCR_TRCENA;

    // set port protocol to NRZ (compatible with UART)
    TPIU_SPPR = TPIU_SPPR_ASYNC_NRZ;

    // turn off the formatter
    // this will prevent the output from being cluttered with multiplexing id's
    // (which can be very useful, but in this example we want to keep it simple)
    TPIU_FFCR &= ~TPIU_FFCR_ENFCONT;

    // calculate the prescaler for the given baudrate
    uint32_t prescaler = (rcc_ahb_frequency / baudrate) - 1;
    TPIU_ACPR = prescaler;

    // enable the TRACESWO pin in async mode (meaning: just use one pin, PB3)
    DBGMCU_CR |= DBGMCU_CR_TRACE_IOEN;
    DBGMCU_CR |= DBGMCU_CR_TRACE_MODE_ASYNC;

    // unlock ITM registers
    ITM_LAR = 0xC5ACCE55;

    // enable ITM with id 1
    ITM_TCR = (1 << 16) | ITM_TCR_ITMENA;

    // enable simulus port 1
    ITM_TER[0] = 1;
}


static void trace_send_blocking(char c)
{
    // wait until the fifo is empty
    while (!(ITM_STIM8(0) & ITM_STIM_FIFOREADY));                                                          

    // write the char into the fifo
    ITM_STIM8(0) = c;
}


int main(void)
{
	int i;

    // setup for a baudrate of 115200
    trace_setup(115200);

	while (1) {

        for(char c = 'a'; c <= 'z'; c ++){
            trace_send_blocking(c);
        }

        trace_send_blocking('\r');
        trace_send_blocking('\n');
            
        for (i = 0; i < 2000000; i++) /* Wait a bit. */
			__asm__("nop");

	}

	return 0;
}
