/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Oliver Meier <h2obrain@gmail.com>
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

#include "i2c.h"

static uint32_t dummy __attribute__((unused));

volatile uint32_t stummy;

void i2c_setup_master(uint32_t i2c, uint32_t clock_speed)
{
	/* Disable the I2C before changing any configuration. */
	i2c_reset(i2c); /* rcc_periph_reset_pulse(i2c_RST); */
	i2c_peripheral_disable(i2c);

	/* analog filter enabled, digital filter 4*Tpclk1 */
#define DIGITAL_FILTER 2
	I2C_FILTR(i2c) = DIGITAL_FILTER;

	/* TODO fix timing!!!! */
	/*i2c_set_clock_frequency(i2c,    I2C_CR2_FREQ_42MHZ); // APB @42MHz?*/
	i2c_set_clock_frequency(i2c, CLOCK_SETUP.apb1_frequency/1000000);
	if (clock_speed <= 100000) { /* <=100000 according to ref. manual,
									<88000 according to errata sheet */
		i2c_set_standard_mode(i2c);
		i2c_set_ccr(i2c, CLOCK_SETUP.apb1_frequency/(clock_speed*2));
		stummy = CLOCK_SETUP.apb1_frequency/(clock_speed*2);
		i2c_set_dutycycle(i2c, I2C_CCR_DUTY_DIV2);
		i2c_set_trise(i2c, (CLOCK_SETUP.apb1_frequency/1000000) + 1 + DIGITAL_FILTER);
	} else {
		i2c_set_fast_mode(i2c);
		i2c_set_ccr(i2c, CLOCK_SETUP.apb1_frequency/(clock_speed*25));
		stummy = CLOCK_SETUP.apb1_frequency/(clock_speed*25);
		i2c_set_dutycycle(i2c, I2C_CCR_DUTY_16_DIV_9);
		i2c_set_trise(i2c, (((CLOCK_SETUP.apb1_frequency/1000000) * 300) / 1000) + 1 + DIGITAL_FILTER);
	}
	/*i2c_set_own_7bit_slave_address(i2c, 0); */


	i2c_peripheral_enable(i2c);
}



/* Clearing ADDR condition sequence. */
#define i2c_clear_address(i2c) { dummy = I2C_SR1(i2c); dummy = I2C_SR2(i2c); }

static
void
i2c_wait_until_probably_safe(uint32_t i2c);
static
void
i2c_send_start_safe(uint32_t i2c);
static
void
i2c_send_stop_safe(uint32_t i2c);
static
void
i2c_recovery(uint32_t i2c);


static
i2c_flag_wait_error
i2c_wait_for_sr1_flag_set(uint32_t i2c, uint32_t flag);
static
i2c_flag_wait_error
i2c_wait_while_busy(uint32_t i2c);


void
i2c_wait_until_probably_safe(uint32_t i2c) {
	uint32_t timeout = I2C_FLAG_WAIT_TIMEOUT;
	while (
		(I2C_CR1(i2c) & (I2C_CR1_STOP | I2C_CR1_START | I2C_CR1_PEC))
	 && timeout
	) {
		timeout--;
	}
	if (!timeout) {
		stummy = I2C_CR1(i2c);
		i2c_recovery(i2c);
	}
}
void
i2c_send_start_safe(uint32_t i2c) {
	i2c_wait_until_probably_safe(i2c);
	i2c_send_start(i2c);
}
void
i2c_send_stop_safe(uint32_t i2c) {
	/* sending stop is bad most of the time!! */
	i2c_wait_until_probably_safe(i2c);
	I2C_CR1(i2c) |= I2C_CR1_STOP;
}
void
i2c_recovery(uint32_t i2c) {
	uint32_t timeout;
	/* recover from glitch or so */
	I2C_CR1(i2c) |= I2C_CR1_SWRST;
	/* wait a long time..  */
	timeout = I2C_FLAG_WAIT_TIMEOUT;
	while (timeout) {
		timeout--;
	}
	I2C_CR1(i2c) &= ~I2C_CR1_SWRST;
}


i2c_flag_wait_error
i2c_wait_for_sr1_flag_set(uint32_t i2c, uint32_t flag) {
	uint32_t timeout = I2C_FLAG_WAIT_TIMEOUT;
	while (!(I2C_SR1(i2c) & (flag | I2C_SR1_AF)) && timeout) {
		timeout--;
	}
	stummy = I2C_SR1(i2c);
	if ((I2C_SR1(i2c) & I2C_SR1_AF) || !timeout) {
		stummy = I2C_SR2(i2c);


		/* TODO <<< check for AF error */
		/* TODO make a recovery function.. */
		/*i2c3_setup();*/
		/*for (timeout=10; timeout; timeout--) {*/
		if (I2C_SR1(i2c) & I2C_SR1_AF) {
			i2c_send_start_safe(i2c);
			while (!(I2C_SR1(i2c) & I2C_SR1_SB));
		} else {
			i2c_recovery(i2c);
		}
		/*}*/

		/*sending stop is only allowed after a full byte..*/
		/*i2c_send_stop_safe(i2c);*/

		/*stummy = I2C_SR1(i2c);*/
		return I2C_FLAG_WAIT_UNKNOWN_ERROR;
	}
	return I2C_FLAG_WAIT_NO_ERROR;
}
i2c_flag_wait_error
i2c_wait_while_busy(uint32_t i2c) {
	uint32_t timeout = I2C_FLAG_WAIT_TIMEOUT;
	while (i2c_is_busy(i2c) && timeout) {
		timeout--;
	}
	if ((I2C_SR1(i2c) & I2C_SR1_AF) || !timeout) {
		i2c_recovery(i2c);
		return I2C_FLAG_WAIT_UNKNOWN_ERROR;
	}
	return I2C_FLAG_WAIT_NO_ERROR;
}

volatile uint32_t i2c_failed_horribly;

void
i2c_trx_retry(uint32_t i2c, uint8_t address,
		const uint8_t *write_data,  uint32_t write_len,
		      uint8_t  read_data[], uint32_t read_len
) {
	uint32_t retries = I2C_MAX_RETRIES;
	while (
		i2c_trx(i2c, address, write_data, write_len, read_data, read_len)
	 && retries) {
		retries--;
	}
	if (!retries) {
		i2c_failed_horribly = 1;
	}
}

i2c_flag_wait_error
i2c_trx(uint32_t i2c, uint8_t address,
		const uint8_t *write_data,  uint32_t write_len,
		      uint8_t  read_data[], uint32_t read_len
) {
	/* TODO make it work :) */

	if (i2c_wait_while_busy(i2c)) {
		return I2C_FLAG_WAIT_UNKNOWN_ERROR;
	}

	/* enable error IT */
	/*I2C_CR2(i2c) |= I2C_CR2_ITERREN;*/

	/* Sending the data_p. */
	if (write_len) {
		i2c_wait_until_probably_safe(i2c);
		/* send start */
		I2C_CR1(i2c) |= I2C_CR1_START; /*i2c_send_start(i2c);*/
		if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_SB)) {
			return I2C_FLAG_WAIT_UNKNOWN_ERROR;
		}

		/* send address */
		i2c_send_7bit_address(i2c, address, I2C_WRITE);
		/* Waiting for address is transferred. */
		if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_ADDR)) {
			return I2C_FLAG_WAIT_UNKNOWN_ERROR;
		}

		/* clear ADDR flag by reading SR2 */
		dummy = I2C_SR2(i2c);
		/*i2c_clear_address(i2c);*/

		/*i2c_send_data(i2c, *write_data++); write_len--;*/ /*write first byte*/
		while (write_len) {
			/*if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_BTF)) return I2C_FLAG_WAIT_UNKNOWN_ERROR;*/
			if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_TxE)) {
				return I2C_FLAG_WAIT_UNKNOWN_ERROR;
			}
			i2c_send_data(i2c, *write_data++); write_len--;
			if ((I2C_SR1(i2c) & I2C_SR1_BTF) && write_len) {
				i2c_send_data(i2c, *write_data++); write_len--;
			}
		}

		if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_TxE)) {
			return I2C_FLAG_WAIT_UNKNOWN_ERROR;
		}
	}
	if (!read_len) {
		i2c_send_stop_safe(i2c);
	} else {
		/* Receiving data_p */
		i2c_wait_until_probably_safe(i2c);
		/*I2C_CR1(i2c) |= I2C_CR1_ACK;*/ /*enable ack*/
		i2c_send_start(i2c);         /* send repeated start */
		if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_SB)) {
			return I2C_FLAG_WAIT_UNKNOWN_ERROR;
		}

		i2c_send_7bit_address(i2c, address, I2C_READ);
		/* Waiting for address is transferred. */
		if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_ADDR)) {
			return I2C_FLAG_WAIT_UNKNOWN_ERROR;
		}

		/* Start receiving */
		switch (read_len) {
		case 1:
			I2C_CR1(i2c) &= ~I2C_CR1_ACK; /* disable ack */
			/* Disable all active IRQs around ADDR clearing and STOP programming because the EV6_3
			   software sequence must complete before the current byte end of transfer */
			__disable_irq();
			i2c_clear_address(i2c);
			I2C_CR1(i2c) |= I2C_CR1_STOP;
			__enable_irq();
			break;
		case 2:
			I2C_CR1(i2c) |= I2C_CR1_POS; /* this may cause trouble? has it to be set before start? */
			/* EV6_1: The acknowledge disable should be done just after EV6,
			 that is after ADDR is cleared, so disable all active IRQs around ADDR clearing and
			 ACK clearing */
			__disable_irq();
			i2c_clear_address(i2c);
			I2C_CR1(i2c) &= ~I2C_CR1_ACK;
			__enable_irq();
			/*
			I2C_CR1(i2c) &= ~I2C_CR1_ACK;
			I2C_CR1(i2c) |= I2C_CR1_POS;
			i2c_clear_address(i2c);
			*/
			break;
		default:
			I2C_CR1(i2c) |= I2C_CR1_ACK;
			i2c_clear_address(i2c);
			break;
		}
		while (read_len) {
			switch (read_len) {
			case 1:
				if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_RxNE)) {
					return I2C_FLAG_WAIT_UNKNOWN_ERROR;
				}
				*read_data++ = I2C_DR(i2c); read_len--;
				break;
			case 2:
				if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_BTF)) {
					return I2C_FLAG_WAIT_UNKNOWN_ERROR;
				}
				/* TODO i2c_send_stop_safe(i2c); */
				__disable_irq();
				I2C_CR1(i2c) |= I2C_CR1_STOP;
				*read_data++ = I2C_DR(i2c); read_len--;
				__enable_irq();
				*read_data++ = I2C_DR(i2c); read_len--;
				break;
			case 3: /*???!*/
				if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_BTF)) {
					return I2C_FLAG_WAIT_UNKNOWN_ERROR;
				}
				I2C_CR1(i2c) &= ~I2C_CR1_ACK;
				*read_data++ = I2C_DR(i2c); read_len--;
				/* goto case 2 to finish.. */
				break;
			default:
				if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_BTF)) {
					return I2C_FLAG_WAIT_UNKNOWN_ERROR;
				}
				*read_data++ = I2C_DR(i2c); read_len--;
				/*
				if (i2c_wait_for_sr1_flag_set(i2c, I2C_SR1_RxNE)) return I2C_FLAG_WAIT_UNKNOWN_ERROR;
				*read_data++ = I2C_DR(i2c); read_len--;
				if (I2C_SR1(i2c) & I2C_SR1_BTF) { // read another byte
					*read_data++ = I2C_DR(i2c); read_len--;
				}
				 */
				break;
			}
		}
		i2c_wait_until_probably_safe(i2c);
		I2C_CR1(i2c) &= ~I2C_CR1_POS;
	}

	/*return i2c_wait_while_busy(i2c);*/ /* busy checking is done at the beginning of this function.. */
	return I2C_FLAG_WAIT_NO_ERROR;
}

