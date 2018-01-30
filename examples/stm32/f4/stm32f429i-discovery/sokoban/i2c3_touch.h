#ifndef _I2C3_TOUCH_INCLUDE
#define _I2C3_TOUCH_INCLUDE
/* Funcoes de uso interno da biblioteca do touch-screen */
void i2c3_init(void);
int i2c3_write_reg8(uint8_t reg, uint8_t val);
int i2c3_read_reg8(uint8_t reg);
int i2c3_read_reg16(uint8_t reg);
int touch_init(void);
int touch_read_raw_xy(int *x, int *y, int *z);

/* Apenas as tres funcoes a seguir sao necessarias no prograna aplicativo */
int touch_get_size(void);
int touch_calibra(struct txtinfo *ptxt);
int touch_read_xy(int *x, int *y, int *z);
#endif
