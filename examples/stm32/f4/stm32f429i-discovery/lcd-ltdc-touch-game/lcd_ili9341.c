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

#include "lcd_ili9341.h"
#include <libopencm3/cm3/nvic.h>

volatile int rx_pend;
volatile uint16_t spi_rx_buf;

void ili9341_spi5_isr()
{
	spi_rx_buf = SPI_DR(SPI5);
	--rx_pend;
}

/* send spi data */
static inline void ili9341_send_data(uint8_t data);

/* send spi command */
static inline void ili9341_send_command(uint8_t data);

/* interrupt driven send command via ssp */
void
ili9341_send_command(uint8_t data)
{
	ILI9341_WRX_RESET();
	ILI9341_SPI_SELECT();

	rx_pend++;
	spi_send(ILI9341_SPI, data);
	while (rx_pend);

	ILI9341_SPI_DESELECT();
}

/* interrupt driven send data via ssp */
void
ili9341_send_data(uint8_t data)
{
	ILI9341_WRX_SET();
	ILI9341_SPI_SELECT();

	rx_pend++;
	spi_send(ILI9341_SPI, data);
	while (rx_pend);

	ILI9341_SPI_DESELECT();
}



ili9341_options_t ili9341_opts;

/**
 * Setup LCD screen to be driven by ltdc
 *
 * surface buffers have ILI9341_BUFFERS_PER_LAYER elements
 * which point to memory areas where each have a size of
 * ILI9341_SURFACE_SIZE
 *
 * @param layer1_surface_buffers
 * @param layer2_surface_buffers
 */
void ili9341_init(
		uint16_t *layer1_surface_buffers[],
		uint16_t *layer2_surface_buffers[]
) {
	int i;

	/* Initialize pins used */
	ili9341_init_pins();
	/* SPI chip select high */
	ILI9341_SPI_DESELECT();
	/* Initialize LCD for LTDC */
	ili9341_init_lcd();
	/* Initialize LTDC */
	ili9341_init_ltdc();
	/* Initialize LTDC layers */
	ili9341_init_layers();

	ili9341_opts.width  = ILI9341_WIDTH;
	ili9341_opts.height = ILI9341_HEIGHT;
	ili9341_opts.current_layer = 0;
	ili9341_opts.layer1_opacity = 255;
	ili9341_opts.layer2_opacity = 0;
	ili9341_opts.current_layer1_surface_idx = 0;
	ili9341_opts.current_layer2_surface_idx = 0;
	for (i = 0; i < ILI9341_BUFFERS_PER_LAYER; i++) {
		ili9341_opts.layer1_surface_buffers[i] =
				layer1_surface_buffers[i];
		ili9341_opts.layer2_surface_buffers[i] =
				layer2_surface_buffers[i];
	}

	gfx_init(layer1_surface_buffers[0], ILI9341_WIDTH, ILI9341_HEIGHT);

	/* initially fill the screen white again to confuse people */
	/*
	for (i=0; i<ILI9341_BUFFERS_PER_LAYER; i++) {
		ili9341_set_layer1();
		gfx_fill_screen(GFX_COLOR_WHITE);
		ili9341_flip_layer1_buffer();

		ili9341_set_layer2();
		gfx_fill_screen(ILI9341_LAYER2_COLOR_KEY);
		ili9341_flip_layer2_buffer();

		ltdc_reload(LTDC_SRCR_RELOAD_VBR);
		while (LTDC_SRCR_IS_RELOADING());
	}
	ili9341_set_layer1();
	*/
}

/* Setup pins needed by ltdc 16-bit interface (also spi setup..) */
void ili9341_init_pins(void)
{
	/* Enable clocks */
	rcc_periph_clock_enable(ILI9341_WRX_CLK);
	rcc_periph_clock_enable(ILI9341_CS_CLK);
	rcc_periph_clock_enable(ILI9341_RST_CLK);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOF);
	rcc_periph_clock_enable(RCC_GPIOG);
	rcc_periph_clock_enable(RCC_SPI5);


	/* WRX pin */
	gpio_mode_setup(
			ILI9341_WRX_PORT,
			GPIO_MODE_OUTPUT,
			GPIO_PUPD_NONE,
			ILI9341_WRX_PIN
		);
	gpio_set_output_options(
			ILI9341_WRX_PORT,
			GPIO_OTYPE_PP,
			GPIO_OSPEED_100MHZ,
			ILI9341_WRX_PIN
		);
	/* SPI CS pin */
	gpio_mode_setup(
			ILI9341_CS_PORT,
			GPIO_MODE_OUTPUT,
			GPIO_PUPD_NONE,
			ILI9341_CS_PIN
		);
	gpio_set_output_options(
			ILI9341_CS_PORT,
			GPIO_OTYPE_PP,
			GPIO_OSPEED_100MHZ,
			ILI9341_CS_PIN
		);
	/* Reset pin */
	gpio_mode_setup(
			ILI9341_RST_PORT,
			GPIO_MODE_OUTPUT,
			GPIO_PUPD_PULLUP,
			ILI9341_RST_PIN
		);
	gpio_set_output_options(
			ILI9341_RST_PORT,
			GPIO_OTYPE_PP,
			GPIO_OSPEED_100MHZ,
			ILI9341_RST_PIN
		);
	/*  SPI5 clk and mosi */
	gpio_mode_setup(
			GPIOF,
			GPIO_MODE_AF,
			GPIO_PUPD_NONE,
			GPIO7 | GPIO9
		);
	gpio_set_af(
			GPIOF,
			GPIO_AF5,
			GPIO7 | GPIO9
		);

	/* setup spi */
	spi_init_master(
		SPI5,
		SPI_CR1_BAUDRATE_FPCLK_DIV_16,
		SPI_CR1_CPOL,
		SPI_CR1_CPHA,
		SPI_CR1_DFF_8BIT,
		SPI_CR1_MSBFIRST
	);
	spi_set_full_duplex_mode(SPI5);
	spi_disable_crc(SPI5);
	spi_enable_ss_output(SPI5);
	spi_enable(SPI5);
	spi_enable_rx_buffer_not_empty_interrupt(SPI5);
	spi_disable_tx_buffer_empty_interrupt(SPI5);
	spi_disable_error_interrupt(SPI5);
	nvic_enable_irq(NVIC_SPI5_IRQ);


	/* Parallel interface pins */

	/* Alternating functions */
	gpio_set_af(GPIOA, GPIO_AF14, GPIO3);
	gpio_set_af(GPIOA, GPIO_AF14, GPIO4);
	gpio_set_af(GPIOA, GPIO_AF14, GPIO6);
	gpio_set_af(GPIOA, GPIO_AF14, GPIO11);
	gpio_set_af(GPIOA, GPIO_AF14, GPIO12);
	gpio_set_af(GPIOB, GPIO_AF9,  GPIO0);
	gpio_set_af(GPIOB, GPIO_AF9,  GPIO1);
	gpio_set_af(GPIOB, GPIO_AF14, GPIO8);
	gpio_set_af(GPIOB, GPIO_AF14, GPIO9);
	gpio_set_af(GPIOB, GPIO_AF14, GPIO10);
	gpio_set_af(GPIOB, GPIO_AF14, GPIO11);
	gpio_set_af(GPIOC, GPIO_AF14, GPIO6);
	gpio_set_af(GPIOC, GPIO_AF14, GPIO7);
	gpio_set_af(GPIOC, GPIO_AF14, GPIO10);
	gpio_set_af(GPIOD, GPIO_AF14, GPIO3);
	gpio_set_af(GPIOD, GPIO_AF14, GPIO6);
	gpio_set_af(GPIOF, GPIO_AF14, GPIO10);
	gpio_set_af(GPIOG, GPIO_AF14, GPIO6);
	gpio_set_af(GPIOG, GPIO_AF14, GPIO7);
	gpio_set_af(GPIOG, GPIO_AF9,  GPIO10);
	gpio_set_af(GPIOG, GPIO_AF14, GPIO11);
	gpio_set_af(GPIOG, GPIO_AF9,  GPIO12);

	/* Common settings (aka. ltdc pin setup) */

	/* GPIOA
	 *	Blue5	VSYNC	Green2	Red4	 Red5 */
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ,
		GPIO3 | GPIO4 | GPIO6 | GPIO11 | GPIO12);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO3 | GPIO4 | GPIO6 | GPIO11 | GPIO12);

	/* GPIOB
	 *	Red3	Red6	Blue6	Blue7   Green4	 Green5 */
	gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ,
		GPIO0 | GPIO1 | GPIO8 | GPIO9 | GPIO10 | GPIO11);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO0 | GPIO1 | GPIO8 | GPIO9 | GPIO10 | GPIO11);

	/* GPIOC
	 *	HSYNC	Green6	Red2 */
	gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ,
		GPIO6 | GPIO7 | GPIO10);
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO6 | GPIO7 | GPIO10);

	/* GPIOD
	 *	Green7	Blue2 */
	gpio_set_output_options(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ,
		GPIO3 | GPIO6);
	gpio_mode_setup(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO3 | GPIO6);

	/* GPIOF
	 *	Enable */
	gpio_set_output_options(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ,
		GPIO10);
	gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO10);

	/* GPIOG
	 *	Red7	DOTCLK	Green3	 Blue3	  Blue4 */
	gpio_set_output_options(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ,
		GPIO6 | GPIO7 | GPIO10 | GPIO11 | GPIO12);
	gpio_mode_setup(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO6 | GPIO7 | GPIO10 | GPIO11 | GPIO12);


	/* see Errata-Sheet DM00068628, Rev.9, 2.1.10
	 *
	 * When PA12 is used as GPIO or alternate function in input or output
	 * mode, the data read from Flash memory can be corrupted.
	 * PA12 must be left unconnected on the PCB (configured as push-pull
	 * and held Low). You can use all the other GPIOs and all alternate
	 * functions except for the ones mapped on PA12. Use the OTG_HS
	 * peripheral in full-speed mode instead of the OTG_FS peripheral.
	 *
	 * This limitation is fixed in silicon revision 3.
	 *
	 * So using the display may be a bad idea..
	 * PA12 is R5 (from Red 2..7)
	 *
	 */
/* Disabling fix
 *
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO12);
	gpio_clear(GPIOA, GPIO12);
 *
 */
}
/* Init lcd-display (ili9341) */
void ili9341_init_lcd(void)
{
	ili9341_send_command(0xCA);
	ili9341_send_data(0xC3);
	ili9341_send_data(0x08);
	ili9341_send_data(0x50);
	ili9341_send_command(ILI9341_POWERB);
	ili9341_send_data(0x00);
	ili9341_send_data(0xC1);
	ili9341_send_data(0x30);
	ili9341_send_command(ILI9341_POWER_SEQ);
	ili9341_send_data(0x64);
	ili9341_send_data(0x03);
	ili9341_send_data(0x12);
	ili9341_send_data(0x81);
	ili9341_send_command(ILI9341_DTCA);
	ili9341_send_data(0x85);
	ili9341_send_data(0x00);
	ili9341_send_data(0x78);
	ili9341_send_command(ILI9341_POWERA);
	ili9341_send_data(0x39);
	ili9341_send_data(0x2C);
	ili9341_send_data(0x00);
	ili9341_send_data(0x34);
	ili9341_send_data(0x02);
	ili9341_send_command(ILI9341_PRC);
	ili9341_send_data(0x20);
	ili9341_send_command(ILI9341_DTCB);
	ili9341_send_data(0x00);
	ili9341_send_data(0x00);
	ili9341_send_command(ILI9341_FRC);
	ili9341_send_data(0x00);
	ili9341_send_data(0x1B);
	ili9341_send_command(ILI9341_DFC);
	ili9341_send_data(0x0A);
	ili9341_send_data(0xA2);
	ili9341_send_command(ILI9341_POWER1);
	ili9341_send_data(0x10);
	ili9341_send_command(ILI9341_POWER2);
	ili9341_send_data(0x10);
	ili9341_send_command(ILI9341_VCOM1);
	ili9341_send_data(0x45);
	ili9341_send_data(0x15);
	ili9341_send_command(ILI9341_VCOM2);
	ili9341_send_data(0x90);
	ili9341_send_command(ILI9341_MAC);
	ili9341_send_data(0xC8);
	ili9341_send_command(ILI9341_3GAMMA_EN);
	ili9341_send_data(0x00);
	ili9341_send_command(ILI9341_RGB_INTERFACE);
	ili9341_send_data(0xC2);
	ili9341_send_command(ILI9341_DFC);
	ili9341_send_data(0x0A);
	ili9341_send_data(0xA7);
	ili9341_send_data(0x27);
	ili9341_send_data(0x04);

	ili9341_send_command(ILI9341_COLUMN_ADDR);
	ili9341_send_data(0x00);
	ili9341_send_data(0x00);
	ili9341_send_data(0x00);
	ili9341_send_data(0xEF);

	ili9341_send_command(ILI9341_PAGE_ADDR);
	ili9341_send_data(0x00);
	ili9341_send_data(0x00);
	ili9341_send_data(0x01);
	ili9341_send_data(0x3F);
	ili9341_send_command(ILI9341_INTERFACE);
	ili9341_send_data(0x01);
	ili9341_send_data(0x00);
	ili9341_send_data(0x06);

	ili9341_send_command(ILI9341_GRAM);
	ili9341_delay(1000000);

	ili9341_send_command(ILI9341_GAMMA);
	ili9341_send_data(0x01);

	ili9341_send_command(ILI9341_PGAMMA);
	ili9341_send_data(0x0F);
	ili9341_send_data(0x29);
	ili9341_send_data(0x24);
	ili9341_send_data(0x0C);
	ili9341_send_data(0x0E);
	ili9341_send_data(0x09);
	ili9341_send_data(0x4E);
	ili9341_send_data(0x78);
	ili9341_send_data(0x3C);
	ili9341_send_data(0x09);
	ili9341_send_data(0x13);
	ili9341_send_data(0x05);
	ili9341_send_data(0x17);
	ili9341_send_data(0x11);
	ili9341_send_data(0x00);
	ili9341_send_command(ILI9341_NGAMMA);
	ili9341_send_data(0x00);
	ili9341_send_data(0x16);
	ili9341_send_data(0x1B);
	ili9341_send_data(0x04);
	ili9341_send_data(0x11);
	ili9341_send_data(0x07);
	ili9341_send_data(0x31);
	ili9341_send_data(0x33);
	ili9341_send_data(0x42);
	ili9341_send_data(0x05);
	ili9341_send_data(0x0C);
	ili9341_send_data(0x0A);
	ili9341_send_data(0x28);
	ili9341_send_data(0x2F);
	ili9341_send_data(0x0F);

	ili9341_send_command(ILI9341_SLEEP_OUT);
	ili9341_delay(1000000);
	ili9341_send_command(ILI9341_DISPLAY_ON);

	ili9341_send_command(ILI9341_GRAM);
}
/* Init/enable ltdc */
void ili9341_init_ltdc(void)
{
	/* Enable the LTDC Clock */
	rcc_periph_clock_enable(RCC_LTDC);

	/* Configure PLLSAI prescalers for LCD */
	/* Enable Pixel Clock */
	/* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
	/* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAI_N = 192 Mhz */
	/* PLLLCDCLK = PLLSAI_VCO Output/PLLSAI_R = 192/4 = 96 Mhz */
	/* LTDC clock frequency = PLLLCDCLK / RCC_PLLSAIDivR = 96/4 = 24 Mhz */
	rcc_pllsai_config(192, RCC_PLLSAICFGR_PLLSAIP_DIV2, 7, 4);

	/* this results in tearing */
	/*rcc_pllsai_postscalers(0, RCC_DCKCFGR_PLLSAIDIVR_DIVR_4); */
	/* this seems ideal.. */
	rcc_pllsai_postscalers(0, RCC_DCKCFGR_PLLSAIDIVR_DIVR_8);
	/* this results slightly moving screen */
	/*rcc_pllsai_postscalers(0, RCC_DCKCFGR_PLLSAIDIVR_DIVR_16);*/

	/* Enable PLLSAI Clock */
	rcc_pllsai_enable();

    /* Wait for PLLSAI activation */
	while (!rcc_pllsai_ready());


	/* Polarity configuration */
	ltdc_ctrl_enable(/* or |=? */
	/* Initialize the horizontal synchronization polarity as active low */
		LTDC_GCR_HSPOL_ACTIVE_LOW
	/* Initialize the vertical synchronization polarity as active low */
	  | LTDC_GCR_VSPOL_ACTIVE_LOW
	/* Initialize the data enable polarity as active low */
	  | LTDC_GCR_DEPOL_ACTIVE_LOW
	/* Initialize the pixel clock polarity as input pixel clock */
	  | LTDC_GCR_PCPOL_ACTIVE_LOW
	 );

	/* Configure R,G,B component values for LCD background color */
	ltdc_set_background_color(0, 0, 0);

	/* Timing configuration */
	ltdc_set_tft_sync_timings(
			 10,   2, /* sync time */
			 20,   2, /* 'blind' pixels left  and up */
			240, 320, /* display width/height, visible resolution */
			 10,   4  /* 'blind' pixels right and down */
		);
}
/* Config ltdc options */
void ili9341_init_layers(void)
{
	/* Windowing configuration */
	/* (see ltdc_set_tft_sync_timings)
	 * Horizontal start  = sync time h + 'blind' pixels left
	 *	Vertical start   = sync time v + 'blind' pixels up
	 *	Horizontal stop  = display width
	 *	Vertical stop    = display height
	 */
	ltdc_setup_windowing(LTDC_LAYER_1, 30, 4, 240, 320);
	ltdc_setup_windowing(LTDC_LAYER_2, 30, 4, 240, 320);

	/* Pixel Format configuration*/
	/* The stmf429i-board has the ili9341 connected via the 16-bit
	 * parallel interface and is configured in init_lcd (probably)
	 * to use RGB565 as color format
	 */
	ltdc_set_pixel_format(LTDC_LAYER_1, LTDC_LxPFCR_RGB565);
	ltdc_set_pixel_format(LTDC_LAYER_2, LTDC_LxPFCR_RGB565);

    /* Default Color configuration (configure A,R,G,B component values) */
	ltdc_set_default_colors(LTDC_LAYER_1, 0, 0, 0, 0);
	ltdc_set_default_colors(LTDC_LAYER_2, 0, 0, 0, 0);

    /* Configure blending factors */
    /* To use blending in without alpha in RGB565
     * is actually a bit hard to do :)
     */
    /* Here we tell the controller to do blending using the constant alpha
     * defined elsewhere.
     */
	/* bottom layer */
	/*ltdc_set_blending_factors(
			LTDC_LAYER_1,
			LTDC_LxBFCR_BF1_CONST_ALPHA,
			LTDC_LxBFCR_BF2_CONST_ALPHA
		);*/
	ltdc_set_blending_factors(
			LTDC_LAYER_1,
			LTDC_LxBFCR_BF1_PIXEL_ALPHA_x_CONST_ALPHA,
			LTDC_LxBFCR_BF2_PIXEL_ALPHA_x_CONST_ALPHA
		);
	/* Here we say to blend layer to with layer 1 according to their
	 * alpha channels. I don't know at the moment if color keying is
	 * affected by this
	 */
    /* top layer */
	ltdc_set_blending_factors(
			LTDC_LAYER_2,
			LTDC_LxBFCR_BF1_PIXEL_ALPHA_x_CONST_ALPHA,
			LTDC_LxBFCR_BF2_PIXEL_ALPHA_x_CONST_ALPHA
		);
	/* note: there is also the background color
	 *       (we deal with 3 layers in total)
	 */


	/* Color Keying
	 * Now we set the color key for layer 2 (top layer)
	 * The color keying feature replaces alpha-value for the defined
	 * color automatically with 0
	 */
	uint32_t rgb888_color_key =
			ltdc_get_rgb888_from_rgb565(ILI9341_LAYER2_COLOR_KEY);
	ltdc_set_color_key(
			LTDC_LAYER_2,
			(rgb888_color_key >> 16)&0xff,
			(rgb888_color_key >>  8)&0xff,
			(rgb888_color_key >>  0)&0xff
		);
	ltdc_layer_ctrl_enable(LTDC_LAYER_2, LTDC_LxCR_COLKEY_ENABLE);

    /* the length of one line of pixels in bytes + 3 then :
    Line Lenth = Active high width x number of bytes per pixel + 3
    Active high width         = LCD_PIXEL_WIDTH
    number of bytes per pixel = 2    (pixel_format : RGB565)
    */
	/* the pitch is the increment from the start of one line of pixels
	 * to the start of the next line in bytes, then :
	 *   Pitch = Active high width x number of bytes per pixel
    */
    /* I was too lazy to add makros for this */
	ltdc_set_fb_line_length(LTDC_LAYER_1, (240*2+3), (240*2));
	ltdc_set_fb_line_length(LTDC_LAYER_2, (240*2+3), (240*2));


	/* Configure the number of lines */
	ltdc_set_fb_line_count(LTDC_LAYER_1, 320);
	ltdc_set_fb_line_count(LTDC_LAYER_2, 320);

	/* Start Address configuration :
	 *   the LCD Frame buffer is defined on SDRAM
	 */
	ltdc_set_fbuffer_address(
			LTDC_LAYER_1,
			(uint32_t)ili9341_opts.layer1_surface_buffers[0]
		);

    /* Start Address configuration :
     *   the LCD Frame buffer is defined on SDRAM w/ Offset
     */
	ltdc_set_fbuffer_address(
			LTDC_LAYER_2,
			(uint32_t)ili9341_opts.layer2_surface_buffers[0]
		);


	/* All the layer configs are shadow registers which can be reloaded
	 * either immediately with LTDC_SRCR_RELOAD_IMR or in the vsync phase
	 * (i guess) with LTDC_SRCR_RELOAD_VBR
	 * if you want to do double buffering just set the new buffer address
	 * and say ltdc_reload(LTDC_SRCR_RELOAD_VBR)
	 */
	ltdc_reload(LTDC_SRCR_RELOAD_IMR);

	/* Enable foreground & background Layers */
	ltdc_layer_ctrl_enable(LTDC_LAYER_1, LTDC_LxCR_LAYER_ENABLE);
	ltdc_layer_ctrl_enable(LTDC_LAYER_2, LTDC_LxCR_LAYER_ENABLE);
	ltdc_reload(LTDC_SRCR_RELOAD_IMR);

	/* enable dithering to add artsy graphical artifacts */
	ltdc_ctrl_enable(LTDC_GCR_DITHER_ENABLE);

	/* turn ltdc on, uh yeah! */
	ltdc_ctrl_enable(LTDC_GCR_LTDC_ENABLE);

	/* finally make both layers visible
	 */
	ltdc_set_constant_alpha(LTDC_LAYER_1, 255);
	ltdc_set_constant_alpha(LTDC_LAYER_2, 255);
	ltdc_reload(LTDC_SRCR_RELOAD_IMR);

	/*
	 * alpha is set by either color_keying, constant_alpha or
	 * layer blending (which probably also includes color_keying)
	 * according to the blending options selected above
	 * color keying is enabled only for layer 2 with the key
	 * ILI9341_LAYER2_COLOR_KEY
	 */
}






/*
 * Interface stuff
 */

ili9341_driver_t ili9341_get_driver()
{
	return ILI9341_DRIVER_LTDC;
}


bool
ili9341_is_reloading()
{
	return LTDC_SRCR_IS_RELOADING();
}

void
ili9341_reload(uint32_t mode)
{
	ltdc_reload(mode);
}


void
ili9341_vsync()
{
	while (!ltdc_get_display_status(LTDC_CDSR_VSYNCS));
}


/* double buffering feature (2 buffers for each layer makes 2*2) */
void
ili9341_flip_layer1_buffer()
{
	ltdc_set_fbuffer_address(
			LTDC_LAYER_1,
			(uint32_t)ili9341_opts.layer1_surface_buffers[ili9341_opts.current_layer1_surface_idx]
		);

	ili9341_opts.current_layer1_surface_idx = (ili9341_opts.current_layer1_surface_idx + 1) % ILI9341_BUFFERS_PER_LAYER;
	/*ltdc_set_fbuffer_address(LTDC_LAYER_1, (uint32_t)ili9341_get_current_layer_buffer_address());*/
	/*ili9341_opts.current_local_layer1_buffer = (ili9341_opts.current_local_layer1_buffer+1)%ILI9341_BUFFERS_PER_LAYER;*/

	/* vsync or so by the controller shadow register update command */
	/*ltdc_reload(LTDC_SRCR_RELOAD_VBR);*/
}
void
ili9341_flip_layer2_buffer()
{
	ltdc_set_fbuffer_address(
			LTDC_LAYER_2,
			(uint32_t)ili9341_opts.layer2_surface_buffers[ili9341_opts.current_layer2_surface_idx]
		);

	ili9341_opts.current_layer2_surface_idx = (ili9341_opts.current_layer2_surface_idx + 1) % ILI9341_BUFFERS_PER_LAYER;

	/* vsync or so by the controller shadow register update command */
	/*ltdc_reload(LTDC_SRCR_RELOAD_VBR);*/
}


/* draw_pixel and fill draw on layer1 after this is called */
void
ili9341_set_layer1(void)
{
	/*ili9341_change_current_layer_offset(ILI9341_FRAME_OFFSET*(ili9341_opts.current_local_layer1_buffer));*/
	__gfx_state.surface = ili9341_opts.layer1_surface_buffers[ili9341_opts.current_layer1_surface_idx];
	ili9341_opts.current_layer = 0;
}

/* draw_pixel and fill draw on layer2 after this is called */
void
ili9341_set_layer2(void)
{
	/*ili9341_change_current_layer_offset(ILI9341_FRAME_OFFSET*(ili9341_opts.current_local_layer2_buffer+ILI9341_BUFFERS_PER_LAYER));*/
	__gfx_state.surface = ili9341_opts.layer2_surface_buffers[ili9341_opts.current_layer2_surface_idx];
	ili9341_opts.current_layer = 1;
}

/* set layer 1 alpha (to hide and stuff) */
void
ili9341_set_layer1_opacity(uint8_t opacity)
{
	ili9341_opts.layer1_opacity = opacity;
	ili9341_update_layer_opacity();
}

/* set layer 2 alpha (to hide and stuff) */
void
ili9341_set_layer2_opacity(uint8_t opacity)
{
	ili9341_opts.layer2_opacity = opacity;
	ili9341_update_layer_opacity();
}


/*
 * set alpha and reload the shadow register after opacity update
 * disables layer rendering if opacity is 0 (not really worth anything)
 */
void
ili9341_update_layer_opacity(void)
{
	if (ili9341_opts.layer1_opacity) {
		ltdc_layer_ctrl_enable(LTDC_LAYER_1, LTDC_LxCR_LAYER_ENABLE);
	} else {
		ltdc_layer_ctrl_disable(LTDC_LAYER_1, LTDC_LxCR_LAYER_ENABLE);
	}

	if (ili9341_opts.layer2_opacity) {
		ltdc_layer_ctrl_enable(LTDC_LAYER_2, LTDC_LxCR_LAYER_ENABLE);
	} else {
		ltdc_layer_ctrl_disable(LTDC_LAYER_2, LTDC_LxCR_LAYER_ENABLE);
	}

	ltdc_set_constant_alpha(LTDC_LAYER_1, ili9341_opts.layer1_opacity);
	ltdc_set_constant_alpha(LTDC_LAYER_2, ili9341_opts.layer2_opacity);
	ltdc_reload(LTDC_SRCR_RELOAD_VBR);
}





