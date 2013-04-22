/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Damian Miller <damian.m.miller@gmail.com>
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

#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/usart.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/flash.h>

#define USART_ECHO_EN 1
#define SEND_BUFFER_SIZE 256
#define FLASH_OPERATION_ADDRESS ((u32)0x0800f000)
#define FLASH_PAGE_NUM_MAX 127
#define FLASH_PAGE_SIZE 0x800
#define FLASH_WRONG_DATA_WRITTEN 0x80
#define RESULT_OK 0

/*hardware initialization*/
static void init_system(void);
static void init_usart(void);
/*usart operations*/
static void usart_send_string(u32 usart, u8 *string, u16 str_size);
static void usart_get_string(u32 usart, u8 *string, u16 str_max_size);
/*flash operations*/
static u32 flash_program_data(u32 start_address, u8 *input_data, u16 num_elements);
static void flash_read_data(u32 start_address, u16 num_elements, u8 *out_string);
/*local functions to work with strings*/
static u16 local_strlen(u8 *string);
static void local_atol_hex(u32 value, u8 *out_string);

int main(void)
{
	u32 result = 0;
	u8 str_send[SEND_BUFFER_SIZE], str_verify[SEND_BUFFER_SIZE];

	init_system();

	while(1)
	{
		usart_send_string(USART1, (u8*)"Please enter string to write into Flash memory:\n\r", local_strlen((u8*)"Please enter string to write into Flash memory:\n\r"));
		usart_get_string(USART1, str_send, SEND_BUFFER_SIZE);
		result = flash_program_data(FLASH_OPERATION_ADDRESS, str_send, local_strlen(str_send));

		switch(result)
		{
		case RESULT_OK: /*everything ok*/
			usart_send_string(USART1, (u8*)"Verification of written data: ", local_strlen((u8*)"Verification of written data: "));
			flash_read_data(FLASH_OPERATION_ADDRESS, SEND_BUFFER_SIZE, str_verify);
			usart_send_string(USART1, str_verify, local_strlen(str_verify));
			break;
		case FLASH_WRONG_DATA_WRITTEN: /*data read from Flash is different than written data*/
			usart_send_string(USART1, (u8*)"Wrong data written into flash memory", SEND_BUFFER_SIZE);
			break;
		default: /*wrong flags' values in Flash Status Register (FLASH_SR)*/
			usart_send_string(USART1, (u8*)"Wrong value of FLASH_SR: ", SEND_BUFFER_SIZE);
			local_atol_hex(result, str_send);
			usart_send_string(USART1, str_send, SEND_BUFFER_SIZE);
			break;
		}

		usart_send_string(USART1, (u8*)"\r\n", local_strlen((u8*)"\r\n"));
	}
	return 0;
}

static void init_system(void)
{
	/* setup SYSCLK to work with 64Mhz HSI */
	rcc_clock_setup_in_hsi_out_64mhz();
	init_usart();
}

static void init_usart(void)
{
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_USART1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN);

	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_enable(USART1);
}

static void usart_send_string(u32 usart, u8 *string, u16 str_size)
{
	u16 iter = 0;
	do
	{
		usart_send_blocking(usart, string[iter++]);
	}while(string[iter] != 0 && iter < str_size);
}

static void usart_get_string(u32 usart, u8 *out_string, u16 str_max_size)
{
	u8 sign = 0;
	u16 iter = 0;

	while(iter < str_max_size)
	{
		sign = usart_recv_blocking(usart);

#if USART_ECHO_EN == 1
		usart_send_blocking(usart, sign);
#endif

		if(sign != '\n' && sign != '\r')
			out_string[iter++] = sign;
		else
		{
			out_string[iter] = 0;
			usart_send_string(USART1, (u8*)"\r\n", 3);
			break;
		}
	}
}

static u32 flash_program_data(u32 start_address, u8 *input_data, u16 num_elements)
{
	u16 iter;
	u32 current_address = start_address;
	u32 page_address = start_address;
	u32 flash_status = 0;

	if((start_address - FLASH_BASE) >= (FLASH_PAGE_SIZE * (FLASH_PAGE_NUM_MAX+1)))
		return 1;


	if(start_address % FLASH_PAGE_SIZE)
		page_address -= (start_address % FLASH_PAGE_SIZE);

	flash_unlock();

	flash_erase_page(page_address);
	flash_status = flash_get_status_flags();
	if(flash_status != FLASH_SR_EOP)
		return flash_status;

	for(iter=0; iter<num_elements; iter += 4)
	{
		flash_program_word(current_address+iter, *((u32*)(input_data + iter)));
		flash_status = flash_get_status_flags();
		if(flash_status != FLASH_SR_EOP)
			return flash_status;

		if(*((u32*)(current_address+iter)) != *((u32*)(input_data + iter)))
			return FLASH_WRONG_DATA_WRITTEN;
	}

	return 0;
}

static void flash_read_data(u32 start_address, u16 num_elements, u8 *out_string)
{
	u16 iter;
	u32 *memory_ptr= (u32*)start_address;

	for(iter=0; iter<num_elements/4; iter++)
	{
		*(u32*)out_string = *(memory_ptr + iter);
		out_string += 4;
	}
}

static u16 local_strlen(u8 *string)
{
	u16 iter = 0;

	while(string[iter++] != 0);

	return iter;
}

static void local_atol_hex(u32 value, u8 *out_string)
{
	u8 iter;

	/*end of string*/
	out_string += 8;
	*(out_string--) = 0;

	for(iter=0; iter<8; iter++)
	{
		*(out_string--) = (((value&0xf) > 0x9) ? (0x40 + ((value&0xf) - 0x9)) : (0x30 | (value&0xf)));
		value >>= 4;
	}
}
