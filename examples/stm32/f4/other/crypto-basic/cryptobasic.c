/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
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
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/crypto.h>

static void clock_setup(void)
{
	/* Enable GPIOD clock for LED & USARTs. */
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable clocks for USART2. */
	rcc_periph_clock_enable(RCC_USART2);

	/* Enable clocks for CRYP. */
	rcc_periph_clock_enable(RCC_CRYP);
}

static void usart_setup(void)
{
	/* Setup USART2 parameters. */
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);
}

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO12 on GPIO port D for LED. */
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);

	/* Setup GPIO pins for USART2 transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);

	/* Setup USART2 TX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2);
}

static uint64_t key[4] = {0x11223344, 0x44556677, 0x77889900, 0x99005522};
static uint64_t iv[4] = {0x01020304, 0x02030405, 0x09080706, 0x55245711};

static uint8_t plaintext[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t ciphertext[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t plaintext2[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

int main(void)
{
	int i;
	int iserr = 1;

	clock_setup();
	gpio_setup();
	usart_setup();

	/* Blink the LED (PD12) on the board with every transmitted byte. */
	while (1) {
		gpio_toggle(GPIOD, GPIO12);	/* LED on/off */

		/* encode the plaintext message into ciphertext */

		crypto_set_key(CRYPTO_KEY_128BIT, key);
		crypto_set_iv(iv); /* only in CBC or CTR mode */
		crypto_set_datatype(CRYPTO_DATA_32BIT);
		crypto_set_algorithm(ENCRYPT_DES_ECB);

		crypto_start();
		crypto_process_block((uint32_t *)plaintext,
				     (uint32_t *)ciphertext,
				      8 / sizeof(uint32_t));
		crypto_stop();

		/* decode the previously encoded message
		   from ciphertext to plaintext2 */

		crypto_set_key(CRYPTO_KEY_128BIT, key);
		crypto_set_iv(iv); /* only in CBC or CTR mode */
		crypto_set_datatype(CRYPTO_DATA_32BIT);
		crypto_set_algorithm(DECRYPT_DES_ECB);

		crypto_start();
		crypto_process_block((uint32_t *)ciphertext,
				     (uint32_t *)plaintext2,
				      8 / sizeof(uint32_t));
		crypto_stop();

		/* compare the two plaintexts if they are same */
		iserr = 0;
		for (i = 0; i < 8; i++) {
			if (plaintext[i] != plaintext2[i]) {
				iserr = true;
			}
		}

		if (iserr) {
			gpio_toggle(GPIOD, GPIO12); /* something went wrong. */
		}
	}

	return 0;
}
