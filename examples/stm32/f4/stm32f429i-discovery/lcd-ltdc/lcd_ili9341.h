/*
 * This file is part of the libopencm3 project.
 * 
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
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


#ifndef LTDC_ILI9341_H
#define LTDC_ILI9341_H

/**
 * Includes
 */

#include <stdint.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/f4/ltdc.h>

#include "gfx_locm3.h"

#include "sdram.h"

#ifndef ILI9341_LAYER2_COLOR_KEY
#define ILI9341_LAYER2_COLOR_KEY 0x000C //0x01ad
#endif

//Commands
#define ILI9341_RESET				0x01
#define ILI9341_SLEEP_OUT			0x11
#define ILI9341_GAMMA				0x26
#define ILI9341_DISPLAY_OFF			0x28
#define ILI9341_DISPLAY_ON			0x29
#define ILI9341_COLUMN_ADDR			0x2A
#define ILI9341_PAGE_ADDR			0x2B
#define ILI9341_GRAM				0x2C
#define ILI9341_MAC					0x36
#define ILI9341_PIXEL_FORMAT		0x3A
#define ILI9341_WDB					0x51
#define ILI9341_WCD					0x53
#define ILI9341_RGB_INTERFACE		0xB0
#define ILI9341_FRC					0xB1
#define ILI9341_BPC					0xB5
#define ILI9341_DFC					0xB6
#define ILI9341_POWER1				0xC0
#define ILI9341_POWER2				0xC1
#define ILI9341_VCOM1				0xC5
#define ILI9341_VCOM2				0xC7
#define ILI9341_POWERA				0xCB
#define ILI9341_POWERB				0xCF
#define ILI9341_PGAMMA				0xE0
#define ILI9341_NGAMMA				0xE1
#define ILI9341_DTCA				0xE8
#define ILI9341_DTCB				0xEA
#define ILI9341_POWER_SEQ			0xED
#define ILI9341_3GAMMA_EN			0xF2
#define ILI9341_INTERFACE			0xF6
#define ILI9341_PRC					0xF7



#ifndef ILI9341_SPI
#define ILI9341_SPI SPI5
#endif

#ifndef ILI9341_CS_PIN
//This pin is used on STM32F429 discovery board
#define ILI9341_CS_CLK				RCC_GPIOC
#define ILI9341_CS_PORT				GPIOC
#define ILI9341_CS_PIN				GPIO2
#endif

#ifndef ILI9341_WRX_PIN
//This pin is used on STM32F429 discovery board
#define ILI9341_WRX_CLK				RCC_GPIOD
#define ILI9341_WRX_PORT			GPIOD
#define ILI9341_WRX_PIN				GPIO13
#endif

//Reset pin
#ifndef ILI9341_RST_PIN
//Reset pin
#define ILI9341_RST_CLK				RCC_GPIOD
#define ILI9341_RST_PORT			GPIOD
#define ILI9341_RST_PIN				GPIO12
#endif

#define ILI9341_RST_SET()           gpio_set  (ILI9341_RST_PORT, ILI9341_RST_PIN)
#define ILI9341_RST_RESET()			gpio_clear(ILI9341_RST_PORT, ILI9341_RST_PIN)
#define ILI9341_SPI_IS_SELECTED()   (!gpio_get(ILI9341_CS_PORT,  ILI9341_CS_PIN))
#define ILI9341_SPI_DESELECT()      gpio_set  (ILI9341_CS_PORT,  ILI9341_CS_PIN)
#define ILI9341_SPI_SELECT()        gpio_clear(ILI9341_CS_PORT,  ILI9341_CS_PIN)
#define ILI9341_WRX_SET()           gpio_set  (ILI9341_WRX_PORT, ILI9341_WRX_PIN)
#define ILI9341_WRX_RESET()         gpio_clear(ILI9341_WRX_PORT, ILI9341_WRX_PIN)


/* LCD settings */
#define ILI9341_WIDTH 				240
#define ILI9341_HEIGHT				320


#define ILI9341_SURFACE_SIZE        ((ILI9341_WIDTH*ILI9341_HEIGHT)*sizeof(uint16_t))


/* double buffering (values other than 2 were not tested and make not too much sense..) */
#define ILI9341_BUFFERS_PER_LAYER 2



typedef struct {
	uint16_t width;
	uint16_t height;
	uint8_t current_layer;
	uint8_t layer1_opacity;
	uint8_t layer2_opacity;
	uint8_t current_layer1_surface_idx;
	uint8_t current_layer2_surface_idx;
	uint16_t *layer1_surface_buffers[ILI9341_BUFFERS_PER_LAYER];
	uint16_t *layer2_surface_buffers[ILI9341_BUFFERS_PER_LAYER];
} ili9341_options_t;

extern ili9341_options_t ili9341_opts;
void ili9341_init(uint16_t *layer1_surface_buffers[], uint16_t *layer2_surface_buffers[]);
void ili9341_spi5_isr(void);


/* Wait for vertical synchronisation */
static inline void ili9341_vsync(void);

/* Flip the double_buffer */
static inline void ili9341_flip_layer1_buffer(void);
static inline void ili9341_flip_layer2_buffer(void);

/* Simple delay */
static inline void ili9341_delay(volatile unsigned int delay);

/* Select layer to draw on with gfxcm3 */
static inline void ili9341_set_layer1(void);
static inline void ili9341_set_layer2(void);
static inline void ili9341_set_layer1_opacity(uint8_t opacity);
static inline void ili9341_set_layer2_opacity(uint8_t opacity);
static inline void ili9341_update_layer_opacity(void);



/* this is what vsync would probably look like */
static inline
void
ili9341_vsync() {
	while (!ltdc_get_display_status(LTDC_CDSR_VSYNCS));
}

static inline
uint16_t*
ili9341_get_current_layer_buffer_address(void);

static inline
uint16_t*
ili9341_get_current_layer_buffer_address() {
	return __gfx_state.surface;
}

/* double buffering feature (2 buffers for each layer makes 2*2) */
static inline
void
ili9341_flip_layer1_buffer() {
	ltdc_set_fbuffer_address(
			LTDC_LAYER_1,
			(uint32_t)ili9341_opts.layer1_surface_buffers[ili9341_opts.current_layer1_surface_idx]
		);

	ili9341_opts.current_layer1_surface_idx = (ili9341_opts.current_layer1_surface_idx + 1) % ILI9341_BUFFERS_PER_LAYER;

	/* vsync or so by the controller shadow register update command */
	//ltdc_reload(LTDC_SRCR_RELOAD_VBR);
}
static inline
void
ili9341_flip_layer2_buffer() {
	ltdc_set_fbuffer_address(
			LTDC_LAYER_2,
			(uint32_t)ili9341_opts.layer2_surface_buffers[ili9341_opts.current_layer2_surface_idx]
		);

	ili9341_opts.current_layer2_surface_idx = (ili9341_opts.current_layer2_surface_idx + 1) % ILI9341_BUFFERS_PER_LAYER;

	/* vsync or so by the controller shadow register update command */
	//ltdc_reload(LTDC_SRCR_RELOAD_VBR);
}


/* draw_pixel and fill draw on layer1 after this is called */
static inline
void
ili9341_set_layer1(void) {
	__gfx_state.surface = ili9341_opts.layer1_surface_buffers[ili9341_opts.current_layer1_surface_idx];
	ili9341_opts.current_layer = 0;
}

/* draw_pixel and fill draw on layer2 after this is called */
static inline
void
ili9341_set_layer2(void) {
	__gfx_state.surface = ili9341_opts.layer2_surface_buffers[ili9341_opts.current_layer2_surface_idx];
	ili9341_opts.current_layer = 1;
}

/* set layer 1 alpha (to hide and stuff) */
static inline
void
ili9341_set_layer1_opacity(uint8_t opacity) {
	ili9341_opts.layer1_opacity = opacity;
	ili9341_update_layer_opacity();
}

/* set layer 2 alpha (to hide and stuff) */
static inline
void
ili9341_set_layer2_opacity(uint8_t opacity) {
	ili9341_opts.layer2_opacity = opacity;
	ili9341_update_layer_opacity();
}

/*
 * set alpha and reload the shadow register after opacity update
 * disables layer rendering if opacity is 0 (not really worth anything)
 */
static inline
void
ili9341_update_layer_opacity(void) {
	if (ili9341_opts.layer1_opacity)
		ltdc_layer_ctrl_enable(LTDC_LAYER_1, LTDC_LxCR_LAYER_ENABLE);
	else
		ltdc_layer_ctrl_disable(LTDC_LAYER_1, LTDC_LxCR_LAYER_ENABLE);

	if (ili9341_opts.layer2_opacity)
		ltdc_layer_ctrl_enable(LTDC_LAYER_2, LTDC_LxCR_LAYER_ENABLE);
	else
		ltdc_layer_ctrl_disable(LTDC_LAYER_2, LTDC_LxCR_LAYER_ENABLE);


	ltdc_set_constant_alpha(LTDC_LAYER_1, ili9341_opts.layer1_opacity);
	ltdc_set_constant_alpha(LTDC_LAYER_2, ili9341_opts.layer2_opacity);
	ltdc_reload(LTDC_SRCR_RELOAD_VBR);
}


/* a fancy delay with a volatile argument */
static inline
void
ili9341_delay(volatile unsigned int delay) {
	for (; delay != 0; delay--);
}

#endif

