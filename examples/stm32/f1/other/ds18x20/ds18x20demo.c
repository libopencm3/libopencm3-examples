/*
 * Copyright (C) 2011 Martin Thomas <eversmith@heizung-thomas.de>
 * Copyright (C) 2013 Paul Fertser <fercerpav@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * (1) Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * (2) Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * (3)The name of the author may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#include "onewire.h"
#include "ds18x20.h"
#include "delay.h"

#include <stdlib.h>
#include <stdio.h>

#define MAXSENSORS 10

volatile size_t systick_counter;

uint8_t gSensorIDs[MAXSENSORS][OW_ROMCODE_SIZE];

static void gpio_setup(void)
{
	/* Enable GPIOC clock. */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);

	/* Use PC7 as an additional pullup. This allows to connect
	   many sensors if you place a jumper between PC6 and PC7
	   without an external 4.7k pullup.
	*/
	gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO7);
	gpio_set(GPIOC, GPIO7);
}

void sys_tick_handler(void)
{
	if (systick_counter)
		systick_counter--;
}

/* Assume systick fires ever 3us */
void delay_us(size_t n)
{
	n = (n + 2)/3;
	systick_counter = n;
	systick_counter_enable();
	while (systick_counter)
		;
	systick_counter_disable();
}

static uint8_t search_sensors(void)
{
	uint8_t i;
	uint8_t id[OW_ROMCODE_SIZE];
	uint8_t diff, nSensors;

	puts( "Scanning bus for DS18x20");

	ow_reset();

	nSensors = 0;

	diff = OW_SEARCH_FIRST;
	while ( diff != OW_LAST_DEVICE && nSensors < MAXSENSORS ) {
		DS18X20_find_sensor( &diff, &id[0] );

		if( diff == OW_PRESENCE_ERR ) {
			puts( "No sensors found" );
			break;
		}

		if( diff == OW_DATA_ERR ) {
			puts( "Bus Error" );
			break;
		}

		for ( i=0; i < OW_ROMCODE_SIZE; i++ )
			gSensorIDs[nSensors][i] = id[i];

		nSensors++;
	}

	return nSensors;
}

static void put_temp(int16_t decicelsius)
{
	char s[10];

	DS18X20_format_from_decicelsius( decicelsius, s, 10 );
	printf("%d deci°C, %s °C\n", decicelsius, s);
}


#if DS18X20_MAX_RESOLUTION

static void put_temp_maxres(int32_t tval)
{
	char s[10];

	DS18X20_format_from_maxres( tval, s, 10 );
	printf("%ld °Ce-4, %s °C\n", tval, s);
}

#endif /* DS18X20_MAX_RESOLUTION */


#if DS18X20_EEPROMSUPPORT

static void th_tl_dump(uint8_t *sp)
{
	DS18X20_read_scratchpad( &gSensorIDs[0][0], sp, DS18X20_SP_SIZE );
	printf( "TH/TL in scratchpad of sensor 1 now : %d / %d\n", sp[DS18X20_TH_REG],
		sp[DS18X20_TL_REG] );
}

static void eeprom_test(void)
{
	uint8_t sp[DS18X20_SP_SIZE], th, tl;

	puts( "DS18x20 EEPROM support test for the first sensor" );
	// DS18X20_eeprom_to_scratchpad(&gSensorIDs[0][0]); // already done at power-on
	th_tl_dump( sp );
	th = sp[DS18X20_TH_REG];
	tl = sp[DS18X20_TL_REG];
	tl++;
	th++;
	DS18X20_write_scratchpad( &gSensorIDs[0][0], th, tl, DS18B20_12_BIT );
	puts( "TH+1 and TL+1 written to scratchpad" );
	th_tl_dump( sp );
	DS18X20_scratchpad_to_eeprom( DS18X20_POWER_PARASITE, &gSensorIDs[0][0] );
	puts( "scratchpad copied to DS18x20 EEPROM" );
	DS18X20_write_scratchpad( &gSensorIDs[0][0], 0, 0, DS18B20_12_BIT );
	puts( "TH and TL in scratchpad set to 0" );
	th_tl_dump( sp );
	DS18X20_eeprom_to_scratchpad(&gSensorIDs[0][0]);
	puts( "DS18x20 EEPROM copied back to scratchpad" );
	DS18X20_read_scratchpad( &gSensorIDs[0][0], sp, DS18X20_SP_SIZE );
	if ( ( th == sp[DS18X20_TH_REG] ) && ( tl == sp[DS18X20_TL_REG] ) ) {
		puts( "TH and TL verified" );
	} else {
		puts( "verify failed" );
	}
	th_tl_dump( sp );
	puts( "Restoring TH and TL, writing EEPROM, copying back to scratchpad");
	DS18X20_write_scratchpad( &gSensorIDs[0][0], --th, --tl, DS18B20_12_BIT );
	DS18X20_scratchpad_to_eeprom( DS18X20_POWER_PARASITE, &gSensorIDs[0][0] );
	DS18X20_eeprom_to_scratchpad(&gSensorIDs[0][0]);
	th_tl_dump( sp );
	putchar('\n');
}
#endif /* DS18X20_EEPROMSUPPORT */


int main( void )
{
	void initialise_monitor_handles(void);
	initialise_monitor_handles();

	rcc_clock_setup_in_hse_8mhz_out_24mhz();
	gpio_setup();

	/* 24MHz / 8 => 3000000 counts per second */
	systick_set_clocksource(STK_CTRL_CLKSOURCE_AHB_DIV8);

	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(systick_get_calib()/1000 - 1);

	systick_interrupt_enable();

	uint8_t nSensors, i;
	int16_t decicelsius;
	uint8_t error;

	puts( "DS18x20 1-Wire demo by Martin Thomas" );
	puts( "------------------------------------\n" );

	nSensors = search_sensors();
	printf( "%d DS18x20 sensor(s) available:\n", (int)nSensors );

	for ( i = 0; i < nSensors; i++ ) {
#if DS18X20_VERBOSE
		printf("    #%-2d- ", (int)i + 1);
		DS18X20_show_id( &gSensorIDs[i][0], OW_ROMCODE_SIZE );
#endif
		printf("    #%d is a ", (int)i+1);
		if ( gSensorIDs[i][0] == DS18S20_FAMILY_CODE ) {
			fputs( "DS18S20/DS1820", stdout );
		} else if ( gSensorIDs[i][0] == DS1822_FAMILY_CODE ) {
			fputs( "DS1822", stdout );
		}
		else {
			fputs( "DS18B20", stdout );
		}
		fputs( " which is ", stdout );
		if ( DS18X20_get_power_status( &gSensorIDs[i][0] ) == DS18X20_POWER_PARASITE ) {
			fputs( "parasite", stdout );
		} else {
			fputs( "externally", stdout );
		}
		puts( " powered" );
	}
	putchar('\n');

#if DS18X20_EEPROMSUPPORT
	if ( nSensors > 0 ) {
		eeprom_test();
	}
#endif

	if ( nSensors == 1 ) {
		puts( "There is only one sensor, running "
			     "demo of DS18X20_read_decicelsius_single():" );
		i = gSensorIDs[0][0]; // family-code for conversion-routine
		DS18X20_start_meas( DS18X20_POWER_PARASITE, NULL );
		delay_ms( DS18B20_TCONV_12BIT );
		DS18X20_read_decicelsius_single( i, &decicelsius );
		put_temp( decicelsius );
		putchar( '\n' );
	}


	for(;;) {   // main loop

		error = 0;

		if ( nSensors == 0 ) {
			error++;
		}

		puts( "Convert_T and Read sensor by sensor (reverse order):" );
		for ( i = nSensors; i > 0; i-- ) {
			if ( DS18X20_start_meas( DS18X20_POWER_PARASITE,
				&gSensorIDs[i-1][0] ) == DS18X20_OK ) {
				delay_ms( DS18B20_TCONV_12BIT );
				printf( "Sensor #%d = ", (int) i );
				if ( DS18X20_read_decicelsius( &gSensorIDs[i-1][0], &decicelsius)
				     == DS18X20_OK ) {
					put_temp( decicelsius );
				} else {
					fputs( "CRC Error (lost connection?)", stdout );
					error++;
				}
			}
			else {
				puts( "Start meas. failed (short circuit?)" );
				error++;
			}
		}

		putchar( '\n' );
		puts( "Convert_T for all sensors and Read sensor by sensor:" );
		if ( DS18X20_start_meas( DS18X20_POWER_PARASITE, NULL )
			== DS18X20_OK) {
			delay_ms( DS18B20_TCONV_12BIT );
			for ( i = 0; i < nSensors; i++ ) {
				printf( "Sensor #%d = ", (int)i + 1 );
				if ( DS18X20_read_decicelsius( &gSensorIDs[i][0], &decicelsius )
				     == DS18X20_OK ) {
					put_temp( decicelsius );
				}
				else {
					fputs( "CRC Error (lost connection?)", stdout );
					error++;
				}
			}
			putchar( '\n' );
#if DS18X20_MAX_RESOLUTION
			int32_t temp_eminus4;
			puts( "Maximum resolution data:");
			for ( i = 0; i < nSensors; i++ ) {
				printf( "Sensor #%d = ", i+1 );
				if ( DS18X20_read_maxres( &gSensorIDs[i][0], &temp_eminus4 )
				     == DS18X20_OK ) {
					put_temp_maxres( temp_eminus4 );
				}
				else {
					fputs( "CRC Error (lost connection?)", stdout );
					error++;
				}
			}
#endif
		}
		else {
			puts( "Start meas. failed (short circuit?)" );
			error++;
		}


#if DS18X20_VERBOSE
		// all devices:
		puts( "\nVerbose output:" );
		DS18X20_start_meas( DS18X20_POWER_PARASITE, NULL );
		delay_ms( DS18B20_TCONV_12BIT );
		DS18X20_read_meas_all_verbose();
#endif

		if ( error ) {
			puts( "*** problems - rescanning bus ...\n" );
			nSensors = search_sensors();
			printf( "%d DS18X20 sensor(s) available\n", (int)nSensors);
			error = 0;
		}

		delay_ms(3000);
	}
	return 0;
}
