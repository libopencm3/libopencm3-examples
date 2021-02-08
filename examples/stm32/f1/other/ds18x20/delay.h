#ifndef _DELAY_H
#define _DELAY_H

#include <stdlib.h>

void delay_us(size_t n);
#define delay_ms(x)	delay_us((x)*1000)

#endif /* _DELAY_H */
