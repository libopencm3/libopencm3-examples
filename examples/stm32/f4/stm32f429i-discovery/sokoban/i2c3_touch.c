/**************************************************************************
 * STM32F429 Discovery programming using gcc and libopencm3
 * Read Touch Screen
 * by Marcos Augusto Stemmer
*********************************************************************
 * Touch Screen STMPE811 conected to I2c3 using pins
 * PA8=SCL3	PC9=SDA3	PA15=TP_INT1 (STMP811 interrupt)
 * References:
 * www.st.com/resource/en/datasheet/CD00186725.pdf
 * www.st.com/resource/en/application_note/cd00203648.pdf
*********************************************************************/
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/syscfg.h>

#include "uart.h"
#include "ili9341.h"
#include "i2c3_touch.h"

//#define _DEBUG

uint32_t hi2c = I2C3;
extern volatile uint32_t milisec;
#define STMP811_DEV_ADDRESS 0x41
/* Local function prototypes */
int i2c3_set_reg(uint8_t reg);
int i2c3_read_array(uint8_t reg, uint8_t *data, int nb);

/***********************************
 * Initialize interface I2C3
 * PA8=SCL3	PC9=SDA3
 * *********************************/
void i2c3_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOA | RCC_GPIOC);
/* Pins PA8=SCL3 PC9=SDA3 of I2C3 as AF4 */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO8);
	gpio_set_output_options(GPIOC, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO9);
	gpio_set_af(GPIOA, GPIO_AF4, GPIO8);
	gpio_set_af(GPIOC, GPIO_AF4, GPIO9);
	rcc_periph_clock_enable(RCC_I2C3);
	i2c_peripheral_disable(I2C3);
	i2c_reset(I2C3);
	i2c_set_standard_mode(I2C3);
	i2c_enable_ack(I2C3);
	i2c_set_dutycycle(I2C3, I2C_CCR_DUTY_DIV2); /* default, no need to do this really */
	i2c_set_clock_frequency(I2C3, I2C_CR2_FREQ_24MHZ);
	/* divider ccr = 120 = 24MHz / (100kHz * 2) */
	i2c_set_ccr(I2C3, 120);
	i2c_set_trise(I2C3, 10);
	i2c_peripheral_enable(I2C3);
}

/* Auxiliary function to set STMP811 register number (internal use only) */
int i2c3_set_reg(uint8_t reg)
{
	uint32_t reg32;
	milisec=0;
	while ((I2C_SR2(hi2c) & I2C_SR2_BUSY)) {
		if(milisec >= 100) return -1;
	}

	i2c_send_start(hi2c);
	/* Wait for start-bit in master mode */
	milisec=0;
	while (!((I2C_SR1(hi2c) & I2C_SR1_SB)
		& (I2C_SR2(hi2c) & (I2C_SR2_MSL | I2C_SR2_BUSY)))){
		if(milisec >= 100) return -2;
	};

	i2c_send_7bit_address(hi2c, STMP811_DEV_ADDRESS, I2C_WRITE);

	/* Wait to send i2c address */
	milisec=0;
	while (!(I2C_SR1(hi2c) & I2C_SR1_ADDR)){
		if(milisec >= 100) return -3;
	};

	reg32 = I2C_SR2(hi2c);
	(void)reg32;
	/* Write register number */
	i2c_send_data(hi2c, reg);
	milisec=0;
	while (!(I2C_SR1(hi2c) & (I2C_SR1_BTF))){
		if(milisec >= 100) return -4;
	};
	return 0;
}

/***************************************************/
/* Write to a 8 bit reg STMP811 */
/***************************************************/
int i2c3_write_reg8(uint8_t reg, uint8_t val)
{
	if(i2c3_set_reg(reg)) return -1;
	i2c_send_data(hi2c, val);
	milisec=0;
	while (!(I2C_SR1(hi2c) & (I2C_SR1_BTF | I2C_SR1_TxE))){
		if(milisec >= 100) return -5;
	};
	i2c_send_stop(hi2c);
	return 0;
}

/******************************************/
/*Read a 8 bit reg STMP811 */
/******************************************/
int i2c3_read_reg8(uint8_t reg)
{
	uint32_t reg32, result;
	if(i2c3_set_reg(reg)) return -1;
	i2c_send_start(hi2c);
	/* Wait for start-bit in master mode */
	milisec=0;
	while (!((I2C_SR1(hi2c) & I2C_SR1_SB)
		& (I2C_SR2(hi2c) & (I2C_SR2_MSL | I2C_SR2_BUSY)))){
		if(milisec >= 100) return -10;
	};
	i2c_send_7bit_address(hi2c, STMP811_DEV_ADDRESS, I2C_READ);
	/* Wait to send i2c address */
	milisec=0;
	while (!(I2C_SR1(hi2c) & I2C_SR1_ADDR)){
		if(milisec >= 100) return -11;
	};

	i2c_disable_ack(hi2c);

	/* Dummy read to exit address condition */
	reg32 = I2C_SR2(hi2c);
	(void)reg32;
	i2c_send_stop(hi2c);

	milisec=0;
	while (!(I2C_SR1(hi2c) & I2C_SR1_RxNE)){
		if(milisec >= 100) return -12;
	};
	result = i2c_get_data(hi2c);

	i2c_enable_ack(hi2c);
	I2C_SR1(hi2c) &= ~I2C_SR1_AF;
	return (int)result;
}

/*******************************************/
/* Read a 16 bit register of STMP811 */
/*******************************************/
int i2c3_read_reg16(uint8_t reg)
{
	uint32_t reg32, result;
	if(i2c3_set_reg(reg)) return -1;
	i2c_send_start(hi2c);
	/* Wait for start-bit in master mode */
	milisec=0;
	while (!((I2C_SR1(hi2c) & I2C_SR1_SB)
		& (I2C_SR2(hi2c) & (I2C_SR2_MSL | I2C_SR2_BUSY)))){
		if(milisec >= 100) return -17;
	};
	/* Send i2c address */
	i2c_send_7bit_address(hi2c, STMP811_DEV_ADDRESS, I2C_READ);
	milisec=0;
	while (!(I2C_SR1(hi2c) & I2C_SR1_ADDR)){
		if(milisec >= 100) return -18;
	};
	i2c_enable_ack(hi2c);
	/* Dummy read to exit address condition */
	reg32 = I2C_SR2(hi2c);
	(void)reg32;
	milisec=0;
	while (!(I2C_SR1(hi2c) & I2C_SR1_RxNE)){
		if(milisec >= 100) return -19;
	};
	
	result = (i2c_get_data(hi2c) << 8);	/* Read first byte */

	i2c_disable_ack(hi2c);	/* Read last byte with NACK */
	i2c_send_stop(hi2c);
	milisec=0;
	while (!(I2C_SR1(hi2c) & I2C_SR1_RxNE)){
		if(milisec >= 100) return -20;
	};
	result |= i2c_get_data(hi2c);		/* Get second byte */

	i2c_enable_ack(hi2c);
	I2C_SR1(hi2c) &= ~I2C_SR1_AF;
	return (int)result;
}

/*****************************************
 * Read many bytes STMP811
 * ***************************************/
int i2c3_read_array(uint8_t reg, uint8_t *data, int nb)
{
	uint32_t reg32;
	int k;
	if(i2c3_set_reg(reg)) return -1;
	i2c_send_start(hi2c);
	milisec=0;
	while (!((I2C_SR1(hi2c) & I2C_SR1_SB)
		& (I2C_SR2(hi2c) & (I2C_SR2_MSL | I2C_SR2_BUSY)))){
		if(milisec >= 100) return -17;
	};

	i2c_send_7bit_address(hi2c, STMP811_DEV_ADDRESS, I2C_READ);
	milisec=0;
	while (!(I2C_SR1(hi2c) & I2C_SR1_ADDR)){
		if(milisec >= 100) return -18;
	};

	i2c_enable_ack(hi2c);
	reg32 = I2C_SR2(hi2c);
	(void)reg32;
	milisec=0;
	while (!(I2C_SR1(hi2c) & I2C_SR1_RxNE)){
		if(milisec >= 100) return -19;
	};
	k = 0;
	while(nb--) {
		if(nb == 0) {
			i2c_disable_ack(hi2c);
			i2c_send_stop(hi2c);
		} else	i2c_enable_ack(hi2c);

		milisec=0;
		while (!(I2C_SR1(hi2c) & I2C_SR1_RxNE)){
			if(milisec >= 100) return -19;
		};
		data[k++] = i2c_get_data(hi2c);
	}
	return 0;
}

/*********************************************
 * Sepecific of the touch-screen
 * *******************************************/

volatile int ts_x[8], ts_y[8], ts_z[8];
volatile int ts_size;

/*  PA_15 falling edge interrupt service routine */
void exti15_10_isr(void)
{
	int intflag, k, k4;
	int size;
	uint8_t data[32];
	if(exti_get_flag_status(EXTI15)) {
		//gpio_toggle(GPIOG, GPIO14);
		intflag = i2c3_read_reg8(0x0b);
		size = -1;
		if((intflag & 0x02) ) {
			size = i2c3_read_reg8(0x4c);
			if(size > 8) size = 8;
			if(size > 0) {
				i2c3_read_array(0xd7, data, 4*size);
				for(k=0; k < size; k++) {
					k4=4*k;
					ts_x[k] = (data[k4]<<4) | ((data[k4+1]>>4) & 0x0f);
					ts_y[k] = ((data[k4+1] << 8) & 0xf00) | data[k4+2];
					ts_z[k] = data[k4+3];	/* rz indica a pressao do toque */
					}
				i2c3_write_reg8(0x0b, 0x02);	/* clear buffer theshold interrupt flag */
				}
			if((intflag & 0x01)) {	/* clear touch interrupt bit */
				i2c3_write_reg8(0x0b, 0x01);
				}
			}
		ts_size = size;		
	}
	exti_reset_request(EXTI15);
}

struct regset {
	uint8_t reg,val;
};

/*******************************************************
 * Touch screen initialization sequence
 * Retorn 0 if ok; -1 on error;	
********************************************************/

int touch_init(void)
{
	int k;
	static const struct regset reginit[]={
		{0x04,0x0c},	/* Turn on touch-screen and ADC */
		{0x0a,0x02},	/* Enable interrupts: TOUCH_DETECT and FIFO_THRESHOLD */
		{0x20,0x48},	/* Select sample time: 72 Clocks */
		{0x21,0x01},	/* Select ADC Clock speed: 3.25 MHz */
		{0x17,0x00},	/* Select I/O pins to primary function */
		{0x41,0x9a},	/* Select 500us touch detect delay */
		{0x4a,0x04},	/* FIFO interrupt threshold: 4 points */
		{0x4b,0x01},	/* Clear FIFO */
		{0x4b,0x00},	/* Set FIFO back to normal operation */
		{0x56,0x07},	/* Set Z data format */
		{0x58,0x00},	/* Maximum current=20mA, If value=01 then maxcurrent = 50mA */
		{0x40,0x01},	/* Oparate in XYZ mode */
		{0x0b,0xff},	/* Clear interrupt flags */
		{0x09,0x01}};	/* Enable touch-detect interrupt */
	i2c3_init();
	k = i2c3_read_reg16(0);	/* Ask for device ID (should be 0x0811) */
	if(k != 0x0811) return -1;
	for(k = 0; k < 14; k++) {
		if(i2c3_write_reg8(reginit[k].reg, reginit[k].val)) return -1;
	}
	ts_size = -1;
	
/* Enable GPIOA bit 15 interrupt */
	rcc_periph_clock_enable(RCC_SYSCFG);
	exti_select_source(EXTI15, GPIOA);
	exti_set_trigger(EXTI15, EXTI_TRIGGER_FALLING);
	nvic_set_priority(NVIC_EXTI15_10_IRQ, 15);
	exti_enable_request(EXTI15);
	nvic_enable_irq(NVIC_EXTI15_10_IRQ);
	return 0;
}


/************************************************************************
 * Raw read of the touch screen ADC; no mapping to LCD coordenates
 * Return ts_size: Number of points read
 *************************************************************************/
int touch_read_raw_xy(int *x, int *y, int *z)
{
	int k, kmax, k4;
	int mx, my, mz, x2, y2, max;
	int size;
//	EXTI_IMR &= ~(1 << 15);		/* diable EXTI15 */
	exti_disable_request(EXTI15);
	size = ts_size;
	ts_size = -1;
	if(size < 3) {	/* No average if less than 3 points */
		*x = ts_x[0]; *y = ts_y[0]; *z = ts_z[0];
		return size;
	}
	/* Addition to compute average */
	mx = ts_x[0]; my = ts_y[0]; mz = ts_z[0];
	for(k=1; k < size; k++) {
		mx += ts_x[k];
		my += ts_y[k];
		mz += ts_z[k];
	}
	/* Find maximum deviation from average */
	max = kmax = 0; 
	for(k = 0; k < size; k++){
		x2 = (ts_x[k] - mx/size);
		y2 = (ts_y[k] - my/size);
		k4 = x2*x2 + y2*y2;
		if(k4 > max) { max = k4; kmax = k; }
	}
	/* Exclude point of maximum deviation */
	if(max > 200) {
		mx -= ts_x[kmax];
		my -= ts_y[kmax];
		mz -= ts_z[kmax];
		size--;
	}
	exti_enable_request(EXTI15);
	*x = mx/size; *y = my/size; *z = mz/size;
	return size;
}

/* These global variables are coeficients used to map points to LCD coordenates */
int multix=0x4000, multiy=0x4000, somax=0, somay=0;

enum state_name { INITIAL, CALIBRA1, CALIBRA2, DONE };

/* Distance of calibration points from the screen edge */
#define CAL_CX1	30
#define CAL_CY1 40
#define CAL_CX2 (LCD_WIDTH-40)
#define CAL_CY2 (LCD_HIGHT-40)

/**********************************************************
 * Check how many new points are available.
 * Return -1 if there is no new point
 * ********************************************************/
inline int touch_get_size(void)
{
	return ts_size;
}

void draw_bmp(int x0, int y0, uint16_t *pbits);
uint16_t *pbigguy_img;

/***************************************************
 * Initialize LCD and touch screen, including calibration procedure
 * *************************************************/
int touch_calibra(struct txtinfo *ptxt)
{
	int k2, state, ee;
	int tx1=-1,ty1=-1;
	int tx2=-1,ty2=-1;
	int ix, iy, iz;
	ptxt->f_size = 1;
	ptxt->f_size = 1;
	ptxt->f_bgcolor = lcd_cor565(0xff, 0xff,0xdf);
	ptxt->f_fgcolor = 0;
	ptxt->f_line = 76;
	ptxt->f_size = 2;
	ptxt->f_column = 40;
	lcd_init(ptxt);
	if(touch_init()) {
		mprintf(lcd_putchar,"Erro");
		return -1;
	}
	mprintf(lcd_putchar,"Sokoban");
	ptxt->f_size = 1;
	ptxt->f_line = 110;
	ptxt->f_column = 30;
	mprintf(lcd_putchar,"by Marcos A. Stemmer");
	ptxt->f_line = 200;
	ptxt->f_column = 14;
	mprintf(lcd_putchar,"Touch the target to");
	ptxt->f_line = 218;
	ptxt->f_column = 4;
	mprintf(lcd_putchar,"calibrate the Touch Screen");
	ptxt->f_size=3;
	lcd_dot(CAL_CX1, CAL_CY1);	/* Draw first target */
	lcd_circle(CAL_CX1, CAL_CY1, 8);
	draw_bmp(104,140,pbigguy_img);
	ptxt->f_size=1;
	lcd_show_frame();
	k2 = 60; iz = 0;
	state = INITIAL;
	U1puts("\r\nSokoban\r\n"
	"Press Enter to continue (without calibrating touch-screen)\r\n");
	
	while (1) {
		if(U1available()) {
			ix = U1getchar();
			if(ix == '\r' || ix == '\n' || ix == ' ') {
				state = CALIBRA2;
			}
		}
		if(touch_get_size() > 0) {
			touch_read_raw_xy( &ix, &iy, &iz);
			if(state == INITIAL) state = CALIBRA1;
#ifdef DEBUG
			mprintf(U1putchar,"(%d,%d)\r\n", ix,iy);
#endif
			k2 = 600;
		} else if(k2) k2--;
		if(!k2 && state == CALIBRA1) {	/* Register first calibration point */
			tx1 = ix;
			ty1 = iy;
			state = CALIBRA2;
			k2=600000;
			ptxt->f_size=3;
			/* Erase first target */
			lcd_rectangle(CAL_CX1-10, CAL_CY1-10, 
				      CAL_CX1 + 10, CAL_CY1 + 10, ptxt->f_bgcolor);
			lcd_show_rect(CAL_CX1-10, CAL_CY1-10, CAL_CX1 + 10, CAL_CY1 + 10);
			/* Draw second target */
			lcd_dot(CAL_CX2, CAL_CY2);
			lcd_circle(CAL_CX2, CAL_CY2, 8);
			lcd_show_rect(	CAL_CX2 - 10, CAL_CY2 - 10,
					CAL_CX2 + 10, CAL_CY2 + 10);
			ptxt->f_size=1;

		}
		if(!k2 && state == CALIBRA2) {	/* Register second calibration point */
			tx2 = ix;
			ty2 = iy;
			U1putchar('#');
			/* If points are much diferent from expected (likely wrong), use fixed values */
			if(tx1 < 2800 || tx1 > 3500) tx1 = 3150;
			if(ty1 > 1000 || ty1 < 400 ) ty1 = 800;
			if(tx2 > 1000 || tx2 < 400 ) tx2 = 700;
			if(ty1 > 3800 || ty2 < 3000) ty2 = 3461;
			k2=80000;
#ifdef _DEBUG
			mprintf(U1putchar, "x1=%-4d y1=%d\r\nx2=%-4d y2=%d\r\n", tx1,ty1,tx2,ty2);
#endif
			/* Compute parameters of linear mapping */
			multix = ((CAL_CX2 - CAL_CX1) << 12)/(tx2 - tx1);
			multiy = ((CAL_CY2 - CAL_CY1) << 12)/(ty2 - ty1);
			somax = CAL_CX1 - ((tx1 * multix) >> 12);
			somay = CAL_CY1 - ((ty1 * multiy) >> 12);
			state = DONE;
			lcd_rectangle(0,0, LCD_WIDTH-1, LCD_HIGHT - 1, lcd_cor565(0xff,0xff,0xcf));
			ptxt->f_size = 1;
			ptxt->f_line = 10;
			ptxt->f_column = 4;
			lcd_show_frame();
			break;
		}
		/* Dummy loop to spend some time */
		for(ee=2000; ee--; ) __asm__("nop");
	}
U1puts(	"Use arrow keys to move and to push boxes.\r\n");
return 0;
}

/***************************************************
 * Read touch-screen in LCD coordenates
 * Get x, y, z parameters by reference
 * ************************************************/
int touch_read_xy(int *x, int *y, int *z)
{
	int ix, iy, n;
	n = touch_read_raw_xy( &ix, &iy, z);
	/* Coordenate mapping to LCD */
	*x = ((multix * ix) >> 12) + somax;
	*y = ((multiy * iy) >> 12) + somay;
	if(*x < 0) *x = 0;
	if(*y < 0) *y = 0;
	return n;
}
