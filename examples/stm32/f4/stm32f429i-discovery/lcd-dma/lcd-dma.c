#include <inttypes.h>
#include <stdio.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/ltdc.h>

#include "clock.h"
#include "console.h"
#include "lcd-spi.h"
#include "sdram.h"

#define LCD_WIDTH  240
#define LCD_HEIGHT 320
#define REFRESH_RATE 70 /* Hz */

#define HSYNC       10
#define HBP         20
#define HFP         10

#define VSYNC        2
#define VBP          2
#define VFP          4

// Layer 1 (bottom layer) is ARGB8888 format, full screen.

typedef uint32_t layer1_pixel;
layer1_pixel *const lcd_layer1_frame_buffer = (void *)SDRAM_BASE_ADDRESS;
#define LCD_LAYER1_PIXEL_SIZE (sizeof (layer1_pixel))
#define LCD_LAYER1_WIDTH  LCD_WIDTH
#define LCD_LAYER1_HEIGHT LCD_HEIGHT
#define LCD_LAYER1_PIXELS (LCD_LAYER1_WIDTH * LCD_LAYER1_HEIGHT)
#define LCD_LAYER1_BYTES  (LCD_LAYER1_PIXELS * LCD_LAYER1_PIXEL_SIZE)

// Layer 2 (top layer) is I8, 64x64 pixels.

// Pin assignments
//  R2      = PC10, AF14
//  R3      = PB0,  AF09
//  R4      = PA11, AF14
//  R5      = PA12, AF14
//  R6      = PB1,  AF09
//  R7      = PG6,  AF14
//
//  G2      = PA6,  AF14
//  G3      = PG10, AF09
//  G4      = PB10, AF14
//  G5      = PB11, AF14
//  G6      = PC7,  AF14
//  G7      = PD3,  AF14
//
//  B2      = PD6,  AF14
//  B3      = PG11, AF11
//  B4      = PG12, AF09
//  B5      = PA3,  AF14
//  B6      = PB8,  AF14
//  B7      = PB9,  AF14

// More pins...
//  ENABLE  = PF10, AF14
//  DOTCLK  = PG7,  AF14
//  HSYNC   = PC6,  AF14
//  VSYNC   = PA4,  AF14
//  CSX     = PC2         used in lcd-spi
//  RDX     = PD12        not used: read SPI
//  TE      = PD11        not used: tearing effect interrupt
//  WRX_DCX = PD13        used in lcd-spi
//  DCX_SCL = PF7         used in lcd-spi
//  SDA     = PF9         used in lcd-spi
//  NRST    = NRST

static void lcd_dma_init(void)
{
    // init GPIO clocks
    rcc_periph_clock_enable(RCC_GPIOA | RCC_GPIOB | RCC_GPIOC |
                            RCC_GPIOD | RCC_GPIOF | RCC_GPIOG);

    // set GPIO pin modes
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO3 | GPIO4 | GPIO6 | GPIO11 | GPIO12);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO3 | GPIO4 | GPIO6 | GPIO11 | GPIO12);
    gpio_set_af(GPIOA, GPIO_AF14, GPIO3 | GPIO4 | GPIO6 | GPIO11 | GPIO12);
                            
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO0 | GPIO1 | GPIO8 | GPIO9 | GPIO10 | GPIO11);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO0 | GPIO1 | GPIO8 | GPIO9 | GPIO10 | GPIO11);
    gpio_set_af(GPIOB, GPIO_AF9, GPIO0 | GPIO1);
    gpio_set_af(GPIOB, GPIO_AF14, GPIO8 | GPIO9 | GPIO10 | GPIO11);

    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO6 | GPIO7 | GPIO10);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO6 | GPIO7 | GPIO10);
    gpio_set_af(GPIOC, GPIO_AF14, GPIO6 | GPIO7 | GPIO10);

    gpio_mode_setup(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO3 | GPIO6);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO3 | GPIO6);
    gpio_set_af(GPIOD, GPIO_AF14, GPIO3 | GPIO6);

    gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO10);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO10);
    gpio_set_af(GPIOF, GPIO_AF14, GPIO10);

    gpio_mode_setup(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO6 | GPIO7 | GPIO10 | GPIO11 | GPIO12);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO6 | GPIO7 | GPIO10 | GPIO11 | GPIO12);
    gpio_set_af(GPIOG, GPIO_AF9, GPIO10 | GPIO12);
    gpio_set_af(GPIOG, GPIO_AF14, GPIO6 | GPIO7 | GPIO11);

    // Configure the pixel clock.
    // uint32_t total_v = VSYNC + VBP + LCD_HEIGHT + VFP;
    // uint32_t total_h = HSYNC + HBP + LCD_WIDTH + HFP;
    // uint32_t clocks_per_frame = total_v * total_h;
    // uint32_t pixclock = clocks_per_frame * REFRESH_RATE;
    // pixclock = pixclock;
    uint32_t sain = 192;
    uint32_t saiq = RCC_PLLSAICFGR &
                   (RCC_PLLSAICFGR_PLLSAIQ_MASK << RCC_PLLSAICFGR_PLLSAIQ_MASK);
    uint32_t sair = 4;
    RCC_PLLSAICFGR = (sain << RCC_PLLSAICFGR_PLLSAIN_SHIFT |
                      saiq |
                      sair << RCC_PLLSAICFGR_PLLSAIR_SHIFT);
    // printf("RCC DCKCFGR = %#08" PRIx32 "\n", RCC_DCKCFGR);
    RCC_DCKCFGR |= RCC_DCKCFGR_PLLSAIDIVR_DIV8;
    // printf("RCC DCKCFGR = %#08" PRIx32 "\n", RCC_DCKCFGR);
    RCC_CR |= RCC_CR_PLLSAION;
    while ((RCC_CR & RCC_CR_PLLSAIRDY) == 0)
        continue;
    RCC_APB2ENR |= RCC_APB2ENR_LTDCEN;

    // Configure the Synchronous timings: VSYNC, HSNC, Vertical and
    // Horizontal back porch, active data area, and the front porch
    // timings.
    LTDC_SSCR = ((HSYNC - 1) << LTDC_SSCR_HSW_SHIFT |
                 (VSYNC - 1) << LTDC_SSCR_VSH_SHIFT);
    LTDC_BPCR = ((HSYNC + HBP - 1) << LTDC_BPCR_AHBP_SHIFT |
                 (VSYNC + VBP - 1) << LTDC_BPCR_AVBP_SHIFT);
    LTDC_AWCR = ((HSYNC + HBP + LCD_WIDTH - 1) << LTDC_AWCR_AAW_SHIFT |
                 (VSYNC + VBP + LCD_HEIGHT - 1) << LTDC_AWCR_AAH_SHIFT);
    LTDC_TWCR = (HSYNC + HBP + LCD_WIDTH + HFP - 1) << LTDC_TWCR_TOTALW_SHIFT |
                (VSYNC + VBP + LCD_HEIGHT + VFP - 1) << LTDC_TWCR_TOTALH_SHIFT;

    // Configure the synchronous signals and clock polarity.
    // hsync low, vsync low, DE high, pixclock falling edge
    // LTDC_GCR |= LTDC_GCR_HSPOL;
    // LTDC_GCR |= LTDC_GCR_VSPOL;
    // LTDC_GCR |= LTDC_GCR_DEPOL;
    LTDC_GCR |= LTDC_GCR_PCPOL;

    // If needed, configure the background color.
    LTDC_BCCR = 0x000000FF;

    // Configure the needed interrupts.
    // (none for now)

    // Configure the Layer 1 parameters.
    // (Layer 1 is the bottom layer.)
    {
        // The Layer window horizontal and vertical position
        // (use default)
        uint32_t h_start = HSYNC + HBP + 0;
        uint32_t h_stop = HSYNC + HBP + LCD_LAYER1_WIDTH - 1;
        LTDC_L1WHPCR = h_stop << LTDC_LxWHPCR_WHSPPOS_SHIFT |
                       h_start << LTDC_LxWHPCR_WHSTPOS_SHIFT;
        uint32_t v_start = VSYNC + VBP + 0;
        uint32_t v_stop = VSYNC + VBP + LCD_LAYER1_HEIGHT - 1;
        LTDC_L1WVPCR = v_stop << LTDC_LxWVPCR_WVSPPOS_SHIFT |
                       v_start << LTDC_LxWVPCR_WVSTPOS_SHIFT;

        // The pixel input format
        // use ARGB8888.
        LTDC_L1PFCR = LTDC_LxPFCR_PF_ARGB8888;

        // The color frame buffer start address
        LTDC_L1CFBAR = (uint32_t)lcd_layer1_frame_buffer;

        // The line length and pitch of the color frame buffer
        uint32_t pitch = LCD_LAYER1_WIDTH * LCD_LAYER1_PIXEL_SIZE;
        uint32_t length = LCD_LAYER1_WIDTH * LCD_LAYER1_PIXEL_SIZE + 3;
        LTDC_L1CFBLR = pitch << LTDC_LxCFBLR_CFBP_SHIFT |
                       length << LTDC_LxCFBLR_CFBLL_SHIFT;

        // The number of lines of the color frame buffer
        LTDC_L1CFBLNR = LCD_HEIGHT;

        // If needed, load the CLUT with the RGB values and its address
        // (not using CLUT)

        // If needed, configure the default color and blending factors
        // LTDC_L1DCCR = 0xFF0000FF;
        LTDC_L1CACR = 0x000000FF;
        LTDC_L1BFCR = LTDC_LxBFCR_BF1_PIXEL_ALPHA_x_CONSTANT_ALPHA |
                      LTDC_LxBFCR_BF2_PIXEL_ALPHA_x_CONSTANT_ALPHA;
        // LTDC_L1BFCR = LTDC_LxBFCR_BF1_CONSTANT_ALPHA |
        //               LTDC_LxBFCR_BF2_CONSTANT_ALPHA;
    }

    // Configure the Layer 2 parameters.
    {
        // The Layer window horizontal and vertical position
        // The pixel input format
        // The color frame buffer start address
        // The line length and pitch of the color frame buffer
        // The number of lines of the color frame buffer
        // If needed, load the CLUT with the RGB values and its address
        // If needed, configure the default color and blending factors
    }

    // Enable Layer1 and if needed the CLUT
    LTDC_L1CR |= LTDC_LxCR_LEN;

    // Enable Layer2 and if needed the CLUT

    // If needed, enable dithering and/or color keying.

    // Reload the shadow registers to active registers.
    LTDC_SRCR |= LTDC_SRCR_IMR;

    // Enable the LCD-TFT controller.
    LTDC_GCR |= LTDC_GCR_LTDCEN;
}

static void test_screen(void)
{
    int row, col;
    
    for (row = 0; row < LCD_LAYER1_HEIGHT; row++) {
        for (col = 0; col < LCD_LAYER1_WIDTH; col++) {
            size_t i = row * LCD_LAYER1_WIDTH + col;
            uint8_t a = (row + col) & 0xFF;
            uint8_t r = (row * 10) & 0xFF;
            uint8_t g = col & 0xFF;
            uint8_t b = (row - 2 * col) & 0xFF;
            // a = 0xFF;
            // r = 0x00;
            // g = 0;
            // g = (uint8_t)((uint32_t)g & 0xFF);
            // b = 0;
            uint32_t pix = a << 24 | r << 16 | g << 8 | b << 0;
            if (row == 0 || col == 0 || row == 319 || col == 239)
                pix = 0xFFFFFFFF;
            else if (row < 20 && col < 20)
                pix = 0xFF000000;
            lcd_layer1_frame_buffer[i] = pix;
        }
    }
}

int main(void)
{
    // init timers.
    clock_setup();
    // set up USART 1.
    console_setup(115200);
    console_stdio_setup();
    // set up SDRAM.
    sdram_init();
    // console_puts("Hello\n");

    printf("Hello stdio!\n");

    lcd_dma_init();
    lcd_spi_init();

    printf("Initialized.\n");

    test_screen();

    while (1)
        continue;
}
