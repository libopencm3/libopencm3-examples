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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/flash.h>

#define USART_ECHO_EN 1
#define SEND_BUFFER_SIZE 256
#define FLASH_OPERATION_ADDRESS ((uint32_t)0x0800f000)
#define FLASH_PAGE_NUM_MAX 127
#define FLASH_PAGE_SIZE 0x800
#define FLASH_WRONG_DATA_WRITTEN 0x80
#define RESULT_OK 0

/*hardware initialization*/
static void init_system(void);
static void init_usart(void);
/*usart operations*/
static void usart_send_string(uint32_t usart, uint8_t *string, uint16_t str_size);
static void usart_get_string(uint32_t usart, uint8_t *string, uint16_t str_max_size);
/*flash operations*/
static uint32_t flash_program_data(uint32_t start_address, uint8_t *input_data, uint16_t num_elements);
static void flash_read_data(uint32_t start_address, uint16_t num_elements, uint8_t *output_data);
/*local functions to work with strings*/
static void local_ltoa_hex(uint32_t value, uint8_t *out_string);

int main(void)
{
	uint32_t result = 0;
	uint8_t str_send[SEND_BUFFER_SIZE], str_verify[SEND_BUFFER_SIZE];

	init_system();

	while(1)
	{
		usart_send_string(USART1, (uint8_t*)"Please enter string to write into Flash memory:\n\r", SEND_BUFFER_SIZE);
		usart_get_string(USART1, str_send, SEND_BUFFER_SIZE);
		result = flash_program_data(FLASH_OPERATION_ADDRESS, str_send, SEND_BUFFER_SIZE);

		switch(result)
		{
		case RESULT_OK: /*everything ok*/
			usart_send_string(USART1, (uint8_t*)"Verification of written data: ", SEND_BUFFER_SIZE);
			flash_read_data(FLASH_OPERATION_ADDRESS, SEND_BUFFER_SIZE, str_verify);
			usart_send_string(USART1, str_verify, SEND_BUFFER_SIZE);
			break;
		case FLASH_WRONG_DATA_WRITTEN: /*data read from Flash is different than written data*/
			usart_send_string(USART1, (uint8_t*)"Wrong data written into flash memory", SEND_BUFFER_SIZE);
			break;
		default: /*wrong flags' values in Flash Status Register (FLASH_SR)*/
			usart_send_string(USART1, (uint8_t*)"Wrong value of FLASH_SR: ", SEND_BUFFER_SIZE);
			local_ltoa_hex(result, str_send);
			usart_send_string(USART1, str_send, SEND_BUFFER_SIZE);
			break;
		}
		/*send end_of_line*/
		usart_send_string(USART1, (uint8_t*)"\r\n", 3);
	}
	return 0;
}

static void init_system(void)
{
	/* setup SYSCLK to work with 64Mhz HSI */
	rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_HSI_64MHZ]);
	init_usart();
}

static void init_usart(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_USART1);

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

static void usart_send_string(uint32_t usart, uint8_t *string, uint16_t str_size)
{
	uint16_t iter = 0;
	do
	{
		usart_send_blocking(usart, string[iter++]);
	}while(string[iter] != 0 && iter < str_size);
}

static void usart_get_string(uint32_t usart, uint8_t *out_string, uint16_t str_max_size)
{
	uint8_t sign = 0;
	uint16_t iter = 0;

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
			usart_send_string(USART1, (uint8_t*)"\r\n", 3);
			break;
		}
	}
}

static uint32_t flash_program_data(uint32_t start_address, uint8_t *input_data, uint16_t num_elements)
{
	uint16_t iter;
	uint32_t current_address = start_address;
	uint32_t page_address = start_address;
	uint32_t flash_status = 0;

	/*check if start_address is in proper range*/
	if((start_address - FLASH_BASE) >= (FLASH_PAGE_SIZE * (FLASH_PAGE_NUM_MAX+1)))
		return 1;

	/*calculate current page address*/
	if(start_address % FLASH_PAGE_SIZE)
		page_address -= (start_address % FLASH_PAGE_SIZE);

	flash_unlock();

	/*Erasing page*/
	flash_erase_page(page_address);
	flash_status = flash_get_status_flags();
	if(flash_status != FLASH_SR_EOP)
		return flash_status;

	/*programming flash memory*/
	for(iter=0; iter<num_elements; iter += 4)
	{
		/*programming word data*/
		flash_program_word(current_address+iter, *((uint32_t*)(input_data + iter)));
		flash_status = flash_get_status_flags();
		if(flash_status != FLASH_SR_EOP)
			return flash_status;

		/*verify if correct data is programmed*/
		if(*((uint32_t*)(current_address+iter)) != *((uint32_t*)(input_data + iter)))
			return FLASH_WRONG_DATA_WRITTEN;
	}

	return 0;
}

static void flash_read_data(uint32_t start_address, uint16_t num_elements, uint8_t *output_data)
{
	uint16_t iter;
	uint32_t *memory_ptr= (uint32_t*)start_address;

	for(iter=0; iter<num_elements/4; iter++)
	{
		*(uint32_t*)output_data = *(memory_ptr + iter);
		output_data += 4;
	}
}

static void local_ltoa_hex(uint32_t value, uint8_t *out_string)
{
	uint8_t iter;

	/*end of string*/
	out_string += 8;
	*(out_string--) = 0;

	for(iter=0; iter<8; iter++)
	{
		*(out_string--) = (((value&0xf) > 0x9) ? (0x40 + ((value&0xf) - 0x9)) : (0x30 | (value&0xf)));
		value >>= 4;
	}
}
