/**************************************************************************
 * Game of Sokoban for kit STM32F429 Discovery using gcc and libopencm3
 * (c) Marcos Augusto Stemmer
 * Setup UART1 with interrupts and FIFO buffers
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
****************************************************************************/
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include "uart.h"
/* FIFO_SIZE must be an exact power of 2 */
#define FIFO_SIZE_RX 64
#define FIFO_SIZE_TX 64

volatile unsigned char fifo_rx[FIFO_SIZE_RX];
volatile int rx_tail;		/* Place of arrival */
volatile int rx_head;
volatile unsigned char fifo_tx[FIFO_SIZE_TX];
volatile int tx_tail;
volatile int tx_head;
volatile int tx_idle;		/* Not transmitting */

/* Service of USART1 interrupt */
void usart1_isr(void)
{
uint32_t sr;
sr = USART_SR(USART1);
while(sr & USART_SR_RXNE) { /* USART_SR_RXNE Receiver Not Empty */
	fifo_rx[rx_tail] = USART_DR(USART1);
	rx_tail = (rx_tail + 1) & (FIFO_SIZE_RX-1);
	sr = USART_SR(USART1);
	}
if(sr & USART_SR_TC) {	/* USART_SR_TC: Transmission Complete */
	tx_idle = (tx_head == tx_tail);
	if( tx_idle) {
		USART_SR(USART1) &= ~USART_SR_TC;
		}
	else	{
		USART_DR(USART1) = fifo_tx[tx_head];
		tx_head = ((tx_head + 1) & (FIFO_SIZE_TX-1));
		}
	}
}

/* Send one character using UART1 */
void U1putchar(int c)
{
/* Wait to avoid overflow of transmission buffer */
while(((tx_tail - tx_head) & (FIFO_SIZE_TX-1)) > (FIFO_SIZE_TX - 4));
/* Place a character at the end of the line */
fifo_tx[tx_tail] = c;
tx_tail = (tx_tail + 1) & (FIFO_SIZE_TX-1);
/* If idele send a character to start transmitting */
if(tx_idle) {
	c = USART_SR(USART1);	/* Must read SR to clear flags */
	USART_DR(USART1) = fifo_tx[tx_head];
	tx_head = (tx_head + 1) & (FIFO_SIZE_TX-1);
	tx_idle = 0;
	}
}

/* Get a character with echo */
int U1getchare(void)
{
int c;
U1putchar(c = U1getchar());
return c;
}

/* Return the number of characters available for reading */
int U1available(void)
{
return ((rx_tail - rx_head) & (FIFO_SIZE_RX-1));
}

int U1getchar(void)
{
int c;
while ( !U1available());
c = fifo_rx[rx_head];
rx_head = (rx_head + 1) & (FIFO_SIZE_RX-1);
return c & 0xff;
}

/* Setup USART1 with interrupts */
void usart_setup(void)
{
rcc_periph_clock_enable(RCC_GPIOA);
 /* GPIOA-9 = USART1-TX;   GPIOA-10 = USART1-RX; */
gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
 /* In STM32F429i USART1 is the alternate function 7 of pins 9 and 10 of GPIOA */
gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);
rcc_periph_clock_enable(RCC_USART1);
usart_set_baudrate(USART1, 19200);
usart_set_databits(USART1, 8);
usart_set_stopbits(USART1, USART_STOPBITS_1);
usart_set_mode(USART1, USART_MODE_TX_RX);
usart_set_parity(USART1, USART_PARITY_NONE);
usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
usart_enable(USART1);
/* Enable USART1 interrupt in NVIC */
nvic_enable_irq(NVIC_USART1_IRQ);
/* Intialize FIFO indexes */
rx_head = rx_tail = 0;
tx_head = tx_tail = 0;
tx_idle = 1;
/* Enable RXNEIE and TCIE of USART1 */
USART_CR1(USART1) |= USART_CR1_RXNEIE | USART_CR1_TCIE;
}

/* Send a message */
void U1puts(char *txt)
{
while(*txt) U1putchar(*txt++);
}

/* Get string */
void U1gets(char *txt, int nmax)
{
int k;	/* Number of stored charactes */
int c;
k=0;
do	{
	c=U1getchar();
	if(c==0x7f) c=8;	/* In Linux backspace is 0x7f */
	U1putchar(c);
	/* If backspace remove one character */
	if(c==8) {
		if(k)	{ 
			k--; U1putchar(' '); 
			U1putchar(8); 
			}
		}
	else if(k<nmax) txt[k++]=c;
	} while(c!='\n' && c!='\r');
txt[k-1]='\0';
}

/* Read a number */
int U1getnum(char *prompt, unsigned base)
{
char buf[32];
U1puts(prompt);
U1gets(buf,32);
return xatoi(buf, base);
}

/* Convert string to integer */
unsigned xatoi(char *str, unsigned base)
{
unsigned x,d, s;
x=0; s=0;
while((d = *str++) < '0') s |= (d=='-');
while(d >= '0') {;
	if(d >= 'a') d -= 'a' - 'A';
	d -= (d > '9')? 'A'-10: '0'; 
	if(d >= base) break;
	x = x*base + d;
	d = *str++;
	};
return s? -x: x;
}
