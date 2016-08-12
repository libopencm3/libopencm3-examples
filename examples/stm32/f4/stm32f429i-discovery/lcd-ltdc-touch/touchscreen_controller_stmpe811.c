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

#include "touchscreen_controller_stmpe811.h"

#ifndef NULL
#define NULL 0
#endif

/*static*/
stmpe811_t stmpe811;

void
stmpe811_setup_hardware() {

	rcc_periph_clock_enable(STMPE811_I2C_CLK);
	rcc_periph_clock_enable(STMPE811_I2C_SDA_CLK);
	rcc_periph_clock_enable(STMPE811_I2C_SCL_CLK);
	rcc_periph_clock_enable(STMPE811_I2C_INT_CLK);


	/* STMPE811 interrupt
	 * - INT (the board has a 4.7k pullup resistor for this pin)
	 */
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO15);

	/* I2C */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF,    GPIO_PUPD_NONE, GPIO8);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO8);
	gpio_set_af(GPIOA, GPIO_AF4, GPIO8);

	gpio_mode_setup(GPIOC, GPIO_MODE_AF,    GPIO_PUPD_NONE, GPIO9);
	gpio_set_output_options(GPIOC, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO9);
	gpio_set_af(GPIOC, GPIO_AF4, GPIO9);

	/* I2C setup */
	i2c_setup_master(STMPE811_I2C, 400000);
}

void
stmpe811_enable_interrupts() {
	nvic_set_priority(NVIC_EXTI15_10_IRQ, 0xff);

	exti_select_source(EXTI15, GPIOA);
	exti_set_trigger(EXTI15, EXTI_TRIGGER_FALLING);
	exti_enable_request(EXTI15);
	nvic_enable_irq(NVIC_EXTI15_10_IRQ);
}

void
stmpe811_setup() {
	memset((char *)&stmpe811, 0, sizeof(stmpe811_t));

	/** TODO removeme **/
	stmpe811.last_values_count = 0;
	stmpe811.touch_interrupts = 0;
	/*******************/

	/* soft reset */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_SYS_CTRL1, 2}, 2,
			NULL, 0);
	msleep_loop(10);
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_SYS_CTRL1, 0}, 2,
			NULL, 0);
	msleep_loop(2);


	/* Interrupts */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_INT_EN, 0b00001111}, 2,  /* enable FIFO and Touch interrupts */
			NULL, 0);
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_INT_CTRL, 0b011}, 2,     /* active low, flank interrupt, global int on */
			NULL, 0);


	/* ADC */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_ADC_CTRL1, 0b00011000}, 2, /* clk sample time 44, 12bit, internal ref */
			NULL, 0);

	msleep_loop(2);
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_ADC_CTRL2, 0b10}, 2,       /* ADC clk ~6.5MHz (/44=147kHz) */
			NULL, 0);
}

uint8_t
stmpe811_read_interrupt_status() {
	uint8_t s;
	/* read the status */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_INT_STA}, 1,
			&s, 1);
	return s;
}

void
stmpe811_start_temp_measurements() {
	/* clock setup */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			/*(uint8_t[]){STMPE811_REG_SYS_CTRL2, 0b0111}, 2,*/ /* turn on temperature sensor */
			(uint8_t[]){STMPE811_REG_SYS_CTRL2, 0b0110}, 2, /* turn on clocks (temp,!gpio,!tsc,adc) */
			NULL, 0);

	/* Temp */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_TEMP_CTRL, 0b111}, 2,  /* start temp measurement, measure forever (every 10ms) */
			NULL, 0);
}

uint16_t
stmpe811_read_temp_sample() {
	uint16_t temp;
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
		(uint8_t[]){STMPE811_REG_TEMP_DATA}, 1,
		(uint8_t *)&temp, 2);
	temp = (temp<<8)|(temp>>8); /* switch msb/lsb */
	/* *3Vio/7.51 == 1°C (for some reason this didn't work so i
	 * tinkered with the numbers a little :)) */
	return (uint16_t)((((uint32_t)temp*30000)/751)>>4);
}

void
stmpe811_stop_temp_measuements() {
	/* clock setup */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_SYS_CTRL2, 0b1111}, 2, /* turn on clocks (!temp,!gpio,!tsc,!adc) */
			NULL, 0);

	/* Temp */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_TEMP_CTRL, 0b000}, 2,  /* stop */
			NULL, 0);
}

#define x8(x) x>>8, x&255

void
stmpe811_start_tsc() {

	/* clock setup */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_SYS_CTRL2, 0b1100}, 2, /* turn on clocks (!temp,!gpio,tsc,adc) */
			NULL, 0);

	/* TSC setup */

	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_GPIO_ALT_FUNCT, 0b00001111}, 2, /* disable af on gpio */
			NULL, 0);

	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			/* 2 samples averaging, 500us touch detect delay, 500us settling time */
			(uint8_t[]){STMPE811_REG_TSC_CFG, 0b01011010}, 2,
			/* 2 samples averaging, 100us touch detect delay, 100us settling time */
			/*(uint8_t[]){STMPE811_REG_TSC_CFG,0b01010001}, 2,*/
			/* no averaging, 1ms touch detect delay, 50us settling time */
			/*(uint8_t[]){STMPE811_REG_TSC_CFG,0b00100001}, 2,*/
			/* no averaging, 1ms touch detect delay, 1ms settling time */
			/*(uint8_t[]){STMPE811_REG_TSC_CFG,0b00100011}, 2,*/
			NULL, 0);

	/* Z is not reported, see STMPE811_REG_TSC_CTRL config */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
		(uint8_t[]){STMPE811_REG_TSC_FRACT_Z, 0b100 }, 2, /* Fraction Z (z accuracy) 4:4 */
		NULL, 0);

	/* TODO maybe set shield for X- and Y- lines (no improvement..) */
	/*i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
		(uint8_t[]){STMPE811_REG_TSC_SHIELD,0b0101 }, 2, // Shield (Ground X- and Y-)
		NULL, 0);*/

	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			/* Max current on short  typ.50mA (max xxxmA)*/
			(uint8_t[]){STMPE811_REG_TSC_I_DRIVE, 0b1}, 2,
			/*(typ.20mA (max 35mA))*/
			/*(uint8_t[]){STMPE811_REG_TSC_I_DRIVE,0b0}, 2,*/
			NULL, 0);
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			/* No line is connected to GND */
			(uint8_t[]){STMPE811_REG_TSC_SHIELD, 0b0}, 2,
			NULL, 0);

	/* setup descriptions for portrait view (0°) */
	 /* bottom left  x, */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_WDW_BL_X, x8(STMPE811_X_MIN)}, 3,
			NULL, 0);
	 /* bottom left y, */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_WDW_BL_Y, x8(STMPE811_Y_MIN)}, 3,
			NULL, 0);
	 /* top right x, */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_WDW_TR_X, x8(STMPE811_X_MAX)}, 3,
			NULL, 0);
	 /* top right y, */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_WDW_TR_Y, x8(STMPE811_Y_MAX)}, 3,
			NULL, 0);

	/* FIFO interrupt threshold 1 samples */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_FIFO_TH, 1}, 2,
			NULL, 0);
	 /* Reset FIFO */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_FIFO_STA, 0b00000001}, 2,
			NULL, 0);
	/*msleep_loop(1);*/

	/* Start FIFO */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_FIFO_STA, 0x20}, 2,
			NULL, 0);

	/* Clear all the status pending bits if any */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_INT_STA, 0xFF}, 2,
			NULL, 0);

	 /* window tracking disabled (generate a sample every n-points move), XY acquisition, Enable */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_TSC_CTRL, 0b00000011}, 2,
			NULL, 0);

	msleep_loop(2);

	stmpe811.sample_read_state = STMPE811_TOUCH_STATE__TOUCHED_WAITING_FOR_TIMEOUT;
}


void
stmpe811_handle_interrupt() {
	/* initializations here are not needed.. */
	uint16_t i;
	/*uint16_t x,y,xmin=0,xmax=0,ymin=0,ymax=0, u,i,v;*/
	/*uint32_t all_x,all_y;*/

	/*
	if (!STMPE811_HAS_INTERRUPT()) {
		return;
	}
	*/

	/* read the status */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_INT_STA}, 1,
			stmpe811.rb, 1);

	/* clear the interrupts */
	i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
			(uint8_t[]){STMPE811_REG_INT_STA, stmpe811.rb[0]}, 2,
			NULL, 0);


	/* handle touch detected */
	if (stmpe811.rb[0]&STMPE811_INT__TOUCH_DET) {
		stmpe811.touch_interrupts++;

		i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
				(uint8_t[]){STMPE811_REG_TSC_CTRL}, 1,
				stmpe811.rb+1, 1);

		/* update touch state */
		stmpe811.current_touch_state = 0 != (stmpe811.rb[1]&STMPE811_TSC_CTRL__TOUCH_DET) ? STMPE811_TOUCH_STATE__TOUCHED : STMPE811_TOUCH_STATE__UNTOUCHED;

	}
	switch (stmpe811.sample_read_state) {
	case STMPE811_TOUCH_STATE__UNTOUCHED:
	case STMPE811_TOUCH_STATE__TOUCHED:
		stmpe811.touch.touched     = 0;
		stmpe811.sample_read_state = stmpe811.current_touch_state;
		break;
	case STMPE811_TOUCH_STATE__TOUCHED_WAITING_FOR_TIMEOUT:
		if (stmpe811.interrupt_timeout < mtime() && stmpe811.touch.touched == 0) {
			stmpe811.sample_read_state = stmpe811.current_touch_state;
		} else
		if (stmpe811.current_touch_state == STMPE811_TOUCH_STATE__TOUCHED) {
			stmpe811.interrupt_timeout = mtime() + STMPE811_INTERRUPT_TIMEOUT;
		}
		break;
	}

	/* handle fifo data */
	if (stmpe811.rb[0]&STMPE811_INT__FIFO_HAS_DATA) {
		i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
				(uint8_t[]){STMPE811_REG_FIFO_SIZE}, 1,
				stmpe811.rb+1, 1);

		/* fifo has data (rb[1] should never be 0 here!) */
		if (stmpe811.rb[1]) {
			stmpe811.last_values_count = stmpe811.rb[1];
			i = stmpe811.rb[1]*3;

			i2c_trx_retry(STMPE811_I2C, STMPE811_I2C_ADDRESS,
					(uint8_t[]){STMPE811_REG_TSC_DATA_XYZ_NA}, 1,
					stmpe811.rb+2, i);

			i += 2;
			/*for (u=2; u<i; u+=3) {
				x = ((uint16_t)(stmpe811.rb[u+0]    )<<4)|(stmpe811.rb[u+1]>>4);
				y = ((uint16_t)(stmpe811.rb[u+1]&0xf)<<8)|(stmpe811.rb[u+2]>>0);*/

			/* 1st stage of filtering.. last STMPE811_SAMPLES_HISTORY_LENGTH samples max 100 deviation (~3%) */
			stmpe811.last_valid_sample.x = ((uint16_t)(stmpe811.rb[i-3])     << 4)|(stmpe811.rb[i-2]>>4);
			stmpe811.last_valid_sample.y = ((uint16_t)(stmpe811.rb[i-2]&0xf) << 8)|(stmpe811.rb[i-1]>>0);
			stmpe811.last_valid_sample.z = (uint16_t)(stmpe811.rb[i-1] & 0xffff);
			stmpe811.last_valid_sample.sample_is_fresh = 1;
			stmpe811.drag_timeout        = mtime() + STMPE811_DRAG_TIMEOUT;

			switch (stmpe811.sample_read_state) {
			case STMPE811_TOUCH_STATE__UNTOUCHED: /* TODO unignore this.. */
			case STMPE811_TOUCH_STATE__TOUCHED:
				stmpe811.touch.touched = 1;
				stmpe811.touch.x = stmpe811.last_drag_sample.x = stmpe811.last_valid_sample.x;
				stmpe811.touch.y = stmpe811.last_drag_sample.y = stmpe811.last_valid_sample.y;
				stmpe811.touch.z = stmpe811.last_drag_sample.z = stmpe811.last_valid_sample.z;
				/*stmpe811.last_valid_sample.sample_is_fresh = 0;*/
				stmpe811.interrupt_timeout = mtime() + STMPE811_INTERRUPT_TIMEOUT;

				stmpe811.sample_read_state = STMPE811_TOUCH_STATE__TOUCHED_WAITING_FOR_TIMEOUT;
				break;
			case STMPE811_TOUCH_STATE__TOUCHED_WAITING_FOR_TIMEOUT:
				break;
			}
		}
	}
}

stmpe811_touch_state_t
stmpe811_get_current_touch_state(void) {
	return stmpe811.current_touch_state;
}
stmpe811_touch_t
stmpe811_get_touch_data(void) {
	stmpe811_touch_t touch = stmpe811.touch;
	stmpe811.touch.touched = 0;
	return touch;
}
stmpe811_drag_data_t
stmpe811_get_drag_data(void) {
	if ((stmpe811.last_valid_sample.sample_is_fresh)
	 && (stmpe811.sample_read_state == STMPE811_TOUCH_STATE__TOUCHED_WAITING_FOR_TIMEOUT)) {
		stmpe811_drag_data_t drag_data;
		stmpe811_valid_sample_t sample = stmpe811.last_valid_sample;
		drag_data.data_is_valid = 1;
		/* is more precise, because it has no accumulating conversion errors,
		 * but makes gui implementation harder..
		drag_data.dx = sample.x - stmpe811.touch.x;
		drag_data.dy = sample.y - stmpe811.touch.y;
		*/
		drag_data.dx = sample.x - stmpe811.last_drag_sample.x;
		drag_data.dy = sample.y - stmpe811.last_drag_sample.y;
		drag_data.dz = sample.z - stmpe811.last_drag_sample.z;
		stmpe811.last_drag_sample.x = sample.x;
		stmpe811.last_drag_sample.y = sample.y;
		stmpe811.last_drag_sample.z = sample.z;
		stmpe811.last_valid_sample.sample_is_fresh = 0;
		return drag_data;
	} else {
		return (stmpe811_drag_data_t) {
			.data_is_valid = stmpe811.drag_timeout > mtime(),
			.dx = 0, .dy = 0, .dz = 0
		};
	}
}
