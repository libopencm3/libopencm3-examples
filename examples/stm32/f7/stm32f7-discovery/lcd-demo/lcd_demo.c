/* Includes ------------------------------------------------------------------*/
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/ltdc.h>
#include "RGB_565_480_272.h"
#include "rk043fn48h.h"

/* Private function prototypes -----------------------------------------------*/
static void lcd_config(void); 
static void lcd_pinmux(void); 
static void lcd_clock(void); 
static void lcd_config_layer(void);

/* Private functions ---------------------------------------------------------*/

void lcd_pinmux(void)
{
    /* Enable the LTDC Clock */
    rcc_periph_clock_enable(RCC_LTDC);

    /* Enable GPIOs clock */
    rcc_periph_clock_enable(RCC_GPIOE);
    rcc_periph_clock_enable(RCC_GPIOG);
    rcc_periph_clock_enable(RCC_GPIOI);
    rcc_periph_clock_enable(RCC_GPIOJ);
    rcc_periph_clock_enable(RCC_GPIOK);

    /*** LTDC Pins configuration ***/
    gpio_mode_setup(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO4);
    gpio_set_output_options(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO4);
    gpio_set_af(GPIOE, GPIO_AF14, GPIO4);

    /* GPIOG configuration */
    gpio_mode_setup(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO12);
    gpio_set_output_options(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO12);
    gpio_set_af(GPIOG, GPIO_AF9, GPIO12);

    /* GPIOI LTDC alternate configuration */
    gpio_mode_setup(GPIOI, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15);
    gpio_set_output_options(GPIOI, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15);
    gpio_set_af(GPIOI, GPIO_AF14, GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15);

    /* GPIOJ configuration */
    gpio_mode_setup(GPIOJ, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |
                                                         GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9 |
                                                         GPIO10 | GPIO11 | GPIO13 | GPIO14 | GPIO15);
    gpio_set_output_options(GPIOJ, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |
                                                         GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9 |
                                                         GPIO10 | GPIO11 | GPIO13 | GPIO14 | GPIO15);
    gpio_set_af(GPIOJ, GPIO_AF14, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |
                                                         GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9 |
                                                         GPIO10 | GPIO11 | GPIO13 | GPIO14 | GPIO15);

    /* GPIOK configuration */
    gpio_mode_setup(GPIOK, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO0 | GPIO1 | GPIO2 | GPIO4 | GPIO5 | GPIO6 | GPIO7);
    gpio_set_output_options(GPIOK, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO0 | GPIO1 | GPIO2 | GPIO4 | GPIO5 | GPIO6 | GPIO7);
    gpio_set_af(GPIOK, GPIO_AF14, GPIO0 | GPIO1 | GPIO2 | GPIO4 | GPIO5 | GPIO6 | GPIO7);
  
    /* LCD_DISP GPIO configuration */
    gpio_mode_setup(GPIOI, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
    gpio_set_output_options(GPIOI, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO12);

    /* LCD_BL_CTRL GPIO configuration */
    gpio_mode_setup(GPIOK, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);
    gpio_set_output_options(GPIOK, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO3);

    /* Assert display enable LCD_DISP pin */
    gpio_set(GPIOI, GPIO12);

    /* Assert backlight LCD_BL_CTRL pin */
    gpio_set(GPIOK, GPIO3);
}

static void lcd_config_layer(void)
{   
  /* Windowing configuration */ 
  ltdc_setup_windowing(LTDC_LAYER_2, 480, 272);

  /* Specifies the pixel format */
  ltdc_set_pixel_format(LTDC_LAYER_2, LTDC_LxPFCR_RGB565);

  /* Default color values */
  ltdc_set_default_colors(LTDC_LAYER_2, 0, 0, 0, 0);
  /* Constant alpha */
  ltdc_set_constant_alpha(LTDC_LAYER_2, 255);

  /* Blending factors */
  ltdc_set_blending_factors(LTDC_LAYER_2, LTDC_LxBFCR_BF1_CONST_ALPHA, LTDC_LxBFCR_BF2_CONST_ALPHA);

  /* Framebuffer address */
  ltdc_set_fbuffer_address(LTDC_LAYER_2, (uint32_t)&RGB_565_480_272.pixel_data);

  /* Configures the color frame buffer pitch in byte */
  ltdc_set_fb_line_length(LTDC_LAYER_2, 2*480, 2*480); /* RGB565 is 2 bytes/pixel */

  /* Configures the frame buffer line number */
  ltdc_set_fb_line_count(LTDC_LAYER_2, 272);

  /* Enable layer 1 */
  ltdc_layer_ctrl_enable(LTDC_LAYER_2, LTDC_LxCR_LAYER_ENABLE);

  /* Sets the Reload type */
  ltdc_reload(LTDC_SRCR_IMR);
}

/**
  * @brief LCD Configuration.
  * @note  This function Configure tha LTDC peripheral :
  *        1) Configure the Pixel Clock for the LCD
  *        2) Configure the LTDC Timing and Polarity
  *        3) Configure the LTDC Layer 1 :
  *           - The frame buffer is located at FLASH memory
  *           - The Layer size configuration : 480x272                      
  * @retval
  *  None
  */
static void lcd_config(void)
{ 
  /* LTDC Initialization */
  ltdc_ctrl_disable(LTDC_GCR_HSPOL_ACTIVE_HIGH); /* Active Low Horizontal Sync */
  ltdc_ctrl_disable(LTDC_GCR_VSPOL_ACTIVE_HIGH); /* Active Low Vertical Sync */
  ltdc_ctrl_disable(LTDC_GCR_DEPOL_ACTIVE_HIGH); /* Active Low Date Enable */
  ltdc_ctrl_disable(LTDC_GCR_PCPOL_ACTIVE_HIGH); /* Active Low Pixel Clock */
  
  /* Configure the LTDC */  
  ltdc_set_tft_sync_timings(RK043FN48H_HSYNC, RK043FN48H_VSYNC,
			                RK043FN48H_HBP,   RK043FN48H_VBP,
			                RK043FN48H_WIDTH, RK043FN48H_HEIGHT,
			                RK043FN48H_HFP,   RK043FN48H_VFP);

  ltdc_set_background_color(0, 0, 0);
  ltdc_ctrl_enable(LTDC_GCR_LTDC_ENABLE);
  
  /* Configure the Layer*/
  lcd_config_layer();

}


static void lcd_clock(void)
{
  /* LCD clock configuration */
  /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
  /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
  /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/5 = 38.4 Mhz */
  /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 38.4/4 = 9.6Mhz */

  /* Disable PLLSAI */
  RCC_CR &= ~RCC_CR_PLLSAION;
  while((RCC_CR & (RCC_CR_PLLSAIRDY))) {};

  /* N and R are needed,
   * P and Q are not needed for LTDC */
  RCC_PLLSAICFGR &= ~RCC_PLLSAICFGR_PLLSAIN_MASK;
  RCC_PLLSAICFGR |= 192 << RCC_PLLSAICFGR_PLLSAIN_SHIFT;
  RCC_PLLSAICFGR &= ~RCC_PLLSAICFGR_PLLSAIR_MASK;
  RCC_PLLSAICFGR |= 5 << RCC_PLLSAICFGR_PLLSAIR_SHIFT;
  RCC_DCKCFGR1 &= ~RCC_DCKCFGR1_PLLSAIDIVR_MASK;
  RCC_DCKCFGR1 |= RCC_DCKCFGR1_PLLSAIDIVR_DIVR_4;

  /* Enable PLLSAI */
  RCC_CR |= RCC_CR_PLLSAION;
  while(!(RCC_CR & (RCC_CR_PLLSAIRDY))) {};
}

int main(void)
{
  lcd_pinmux();
  lcd_clock(); 
  lcd_config(); /* Configure LCD : Only one layer is used */

  while (1) {}
}

