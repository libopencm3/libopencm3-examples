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

#ifndef CODEBASE_DRIVERS_TOUCHSCREEN_CONTROLLER_STMPE811_H_
#define CODEBASE_DRIVERS_TOUCHSCREEN_CONTROLLER_STMPE811_H_


#ifndef NULL
#define NULL 0
#endif

#include <string.h>
#include <stdint.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "i2c.h"




/**
 * STMPE811 definitions
 */

#ifndef STMPE811_I2C
#define STMPE811_I2C           I2C3
#define STMPE811_I2C_CLK       RCC_I2C3
/*#define STMPE811_I2C_RST       RST_I2C3*/

#define STMPE811_I2C_SDA_CLK   RCC_GPIOA
#define STMPE811_I2C_SDA_PORT  GPIOA
#define STMPE811_I2C_SDA_PIN   GPIO8
#define STMPE811_I2C_SCL_CLK   RCC_GPIOC
#define STMPE811_I2C_SCL_PORT  GPIOC
#define STMPE811_I2C_SCL_PIN   GPIO9
#define STMPE811_I2C_INT_CLK   RCC_GPIOA
#define STMPE811_I2C_INT_PORT  GPIOA
#define STMPE811_I2C_INT_PIN   GPIO15

#define STMPE811_I2C_ADDRESS   (0b1000001)

#endif

/* registers */
#define STMPE811_REG_CHIP_ID          0x00
#define STMPE811_REG_ID_VER           0x02
#define STMPE811_REG_SYS_CTRL1        0x03
#define STMPE811_REG_SYS_CTRL2        0x04
#define STMPE811_REG_SPI_CFG          0x08
#define STMPE811_REG_INT_CTRL         0x09
#define STMPE811_REG_INT_EN           0x0A
#define STMPE811_REG_INT_STA          0x0B
#define STMPE811_REG_GPIO_INT_EN      0x0C
#define STMPE811_REG_GPIO_INT_STA     0x0D
#define STMPE811_REG_ADC_INT_EN       0x0E
#define STMPE811_REG_ADC_INT_STA      0x0F
#define STMPE811_REG_GPIO_SET_PIN     0x10
#define STMPE811_REG_GPIO_CLR_PIN     0x11
#define STMPE811_REG_GPIO_MP_STA      0x12
#define STMPE811_REG_GPIO_DIR         0x13
#define STMPE811_REG_GPIO_ED          0x14
#define STMPE811_REG_GPIO_RE          0x15
#define STMPE811_REG_GPIO_FE          0x16
#define STMPE811_REG_GPIO_ALT_FUNCT   0x17
#define STMPE811_REG_ADC_CTRL1        0x20
#define STMPE811_REG_ADC_CTRL2        0x21
#define STMPE811_REG_ADC_CAPT         0x22
#define STMPE811_REG_ADC_DATA_CH0     0x30
#define STMPE811_REG_ADC_DATA_CH1     0x32
#define STMPE811_REG_ADC_DATA_CH2     0x34
#define STMPE811_REG_ADC_DATA_CH3     0x36
#define STMPE811_REG_ADC_DATA_CH4     0x38
#define STMPE811_REG_ADC_DATA_CH5     0x3A
#define STMPE811_REG_ADC_DATA_CH6     0x3C
#define STMPE811_REG_ADC_DATA_CH7     0x3E
#define STMPE811_REG_TSC_CTRL         0x40
#define STMPE811_REG_TSC_CFG          0x41
#define STMPE811_REG_WDW_TR_X         0x42
#define STMPE811_REG_WDW_TR_Y         0x44
#define STMPE811_REG_WDW_BL_X         0x46
#define STMPE811_REG_WDW_BL_Y         0x48
#define STMPE811_REG_FIFO_TH          0x4A
#define STMPE811_REG_FIFO_STA         0x4B
#define STMPE811_REG_FIFO_SIZE        0x4C
#define STMPE811_REG_TSC_DATA_X       0x4D
#define STMPE811_REG_TSC_DATA_Y       0x4F
#define STMPE811_REG_TSC_DATA_Z       0x51
#define STMPE811_REG_TSC_DATA_XYZ     0x52
#define STMPE811_REG_TSC_DATA_XYZ_NA  0xD7
#define STMPE811_REG_TSC_FRACT_Z      0x56
#define STMPE811_REG_TSC_DATA         0x57
#define STMPE811_REG_TSC_I_DRIVE      0x58
#define STMPE811_REG_TSC_SHIELD       0x59
#define STMPE811_REG_TEMP_CTRL        0x60
#define STMPE811_REG_TEMP_DATA        0x61
#define STMPE811_REG_TEMP_TH          0x63

/* interesting bitmasks */
#define STMPE811_INT__TOUCH_DET       (1<<0)
#define STMPE811_INT__FIFO_TH         (1<<1)
#define STMPE811_INT__FIFO_OFLOW      (1<<2)
#define STMPE811_INT__FIFO_FULL       (1<<3)
#define STMPE811_INT__FIFO_EMPTY      (1<<4)
#define STMPE811_INT__TEMP_SENS       (1<<5)
#define STMPE811_INT__ADC             (1<<6)
#define STMPE811_INT__GPIO            (1<<7)

#define STMPE811_INT__FIFO_HAS_DATA   ( \
			STMPE811_INT__FIFO_TH | \
			STMPE811_INT__FIFO_OFLOW | \
			STMPE811_INT__FIFO_FULL \
		)

#define STMPE811_TSC_CTRL__TOUCH_DET  (1<<7)



/**
 * Touch controller setup
 */
/*
 *
 */
#define STMPE811_X_MIN 300
#define STMPE811_X_MAX 3850
#define STMPE811_Y_MIN 400
#define STMPE811_Y_MAX 3900

/* STMPE811_INTERRUPT_TIMEOUT ms no stmpe811-interrupt
 * shall occur until touch data is considered to set touched again :P */
#define STMPE811_INTERRUPT_TIMEOUT 300
#define STMPE811_DRAG_TIMEOUT      300 /* values bigger than STMPE811_INTERRUPT_TIMEOUT make no sense..*/

typedef enum {
	STMPE811_TOUCH_STATE__UNTOUCHED = 0,                /* set by init / timeout in interrupt */
	STMPE811_TOUCH_STATE__TOUCHED,                      /* set by touch interrupt */
	STMPE811_TOUCH_STATE__TOUCHED_WAITING_FOR_TIMEOUT,  /* set by */
} stmpe811_touch_state_t;

typedef struct  {
	int16_t x, y, z;
} stmpe811_xyz_sample_t;
typedef struct  {
	uint32_t touched;
	int16_t x, y, z;
} stmpe811_touch_t;
typedef struct  {
	uint32_t sample_is_fresh;
	int16_t x, y, z;
} stmpe811_valid_sample_t;
typedef struct  {
	uint32_t data_is_valid;
	int16_t dx, dy, dz;
} stmpe811_drag_data_t;
typedef struct {
	/*uint8_t  init;*/
	stmpe811_touch_t        touch;
	stmpe811_touch_state_t  sample_read_state;
	stmpe811_touch_state_t  current_touch_state;
	stmpe811_valid_sample_t last_valid_sample;
	stmpe811_xyz_sample_t   last_drag_sample;
	uint32_t                touch_interrupts;
	uint32_t                last_values_count;
	/*uint16_t xmin,xmax,ymin,ymax; */ /* made this constant */
	uint64_t                interrupt_timeout;
	uint64_t                drag_timeout;
	uint8_t                 rb[128*4+2]; /* fifo read buffer */
} stmpe811_t;

void
stmpe811_setup_hardware(void);
void
stmpe811_enable_interrupts(void);

void
stmpe811_setup(void); /*, save_data_t *save_data);*/
uint8_t
stmpe811_read_interrupt_status(void);
void
stmpe811_start_temp_measurements(void);
uint16_t
stmpe811_read_temp_sample(void);
void
stmpe811_stop_temp_measuements(void);
void
stmpe811_start_tsc(void);

void
stmpe811_handle_interrupt(void);
stmpe811_touch_state_t
stmpe811_get_current_touch_state(void);
stmpe811_touch_t
stmpe811_get_touch_data(void);
stmpe811_drag_data_t
stmpe811_get_drag_data(void);


/* REMOVE ME!  DEBUG */
extern stmpe811_t stmpe811;

#endif /* CODEBASE_DRIVERS_TOUCHSCREEN_CONTROLLER_STMPE811_H_ */
