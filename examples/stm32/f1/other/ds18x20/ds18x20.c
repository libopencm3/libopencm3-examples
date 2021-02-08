/*
DS18X20-Functions via One-Wire-Bus

Martin Thomas <eversmith@heizung-thomas.de>
http://www.siwawi.arubi.uni-kl.de/avr-projects

Copyright (C) 2011 Martin Thomas <eversmith@heizung-thomas.de>
Partly based on code from Peter Dannegger and others.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

(1) Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

(2) Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided
with the distribution.

(3)The name of the author may not be used to endorse or promote
products derived from this software without specific prior written
permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "ds18x20.h"
#include "onewire.h"
#include "crc8.h"

/*----------- start of "debug-functions" ---------------*/

#if DS18X20_VERBOSE
#if (!DS18X20_DECICELSIUS)
#error "DS18X20_DECICELSIUS must be enabled for verbose-mode"
#endif

/* functions for debugging-output - undef DS18X20_VERBOSE in .h
   if you run out of program-memory */
#include <string.h>

static int16_t DS18X20_raw_to_decicelsius( uint8_t fc, uint8_t sp[] );

/*************************************************************************
Function: puthex_nibble()
Purpose:  prints lower nibble as ASCII-hex
Input:	  byte value
Returns:  none
**************************************************************************/
static void puthex_nibble(const unsigned char b)
{
	unsigned char  c = b & 0x0f;
	if ( c > 9 ) {
		c += 'A'-10;
	}
	else {
		c += '0';
	}
	putchar(c);
}

/*************************************************************************
Function: puthex_byte()
Purpose:  prints upper and lower nibble as ASCII-hex
Input:	  byte value
Returns:  none
**************************************************************************/
static void puthex_byte( const unsigned char  b )
{
	puthex_nibble( b >> 4 );
	puthex_nibble( b );
}


void DS18X20_show_id( uint8_t *id, size_t n )
{
	size_t i;

	for( i = 0; i < n; i++ ) {
		if ( i == 0 ) { fputs( "FC: ", stdout ); }
		else if ( i == n-1 ) { fputs( " CRC: ", stdout ); }
		if ( i == 1 ) { fputs( "SN: ", stdout ); }
		puthex_byte(id[i]);
		putchar(' ');
		if ( i == 0 ) {
			if ( id[0] == DS18S20_FAMILY_CODE ) { fputs ("(18S) ", stdout); }
			else if ( id[0] == DS18B20_FAMILY_CODE ) { fputs ("(18B) ", stdout); }
			else if ( id[0] == DS1822_FAMILY_CODE ) { fputs ("(22) ", stdout); }
			else { fputs ("( ? ) ", stdout); }
		}
	}
	if ( crc8( id, OW_ROMCODE_SIZE) )
		puts( "(FAIL)" );
	else
		puts( "(OK)" );
}

static void show_sp( uint8_t *sp, size_t n )
{
	size_t i;

	fputs("SP: ", stdout);
	for( i = 0; i < n; i++ ) {
		if ( i == n-1 ) { fputs(" CRC: ", stdout); }
		puthex_byte(sp[i]);
		putchar(' ');
	}
}

/*
   convert raw value from DS18x20 to Celsius
   input is:
   - familycode fc (0x10/0x28 see header)
   - scratchpad-buffer
   output is:
   - cel full celsius
   - fractions of celsius in millicelsius*(10^-1)/625 (the 4 LS-Bits)
   - subzero =0 positiv / 1 negativ
   always returns  DS18X20_OK
*/
static uint8_t DS18X20_meas_to_cel( uint8_t fc, uint8_t *sp,
	uint8_t* subzero, uint8_t* cel, uint8_t* cel_frac_bits)
{
	uint16_t meas;
	uint8_t	 i;

	meas = sp[0];  // LSB
	meas |= ( (uint16_t)sp[1] ) << 8; // MSB

	//  only work on 12bit-base
	if( fc == DS18S20_FAMILY_CODE ) { // 9 -> 12 bit if 18S20
		/* Extended res. measurements for DS18S20 contributed by Carsten Foss */
		meas &= (uint16_t) 0xfffe;    // Discard LSB, needed for later extended precicion calc
		meas <<= 3;		      // Convert to 12-bit, now degrees are in 1/16 degrees units
		meas += ( 16 - sp[6] ) - 4;   // Add the compensation and remember to subtract 0.25 degree (4/16)
	}

	// check for negative
	if ( meas & 0x8000 )  {
		*subzero=1;	 // mark negative
		meas ^= 0xffff;	 // convert to positive => (twos complement)++
		meas++;
	}
	else {
		*subzero=0;
	}

	// clear undefined bits for B != 12bit
	if ( fc == DS18B20_FAMILY_CODE || fc == DS1822_FAMILY_CODE ) {
		i = sp[DS18B20_CONF_REG];
		if ( (i & DS18B20_12_BIT) == DS18B20_12_BIT ) { ; }
		else if ( (i & DS18B20_11_BIT) == DS18B20_11_BIT ) {
			meas &= ~(DS18B20_11_BIT_UNDF);
		} else if ( (i & DS18B20_10_BIT) == DS18B20_10_BIT ) {
			meas &= ~(DS18B20_10_BIT_UNDF);
		} else { // if ( (i & DS18B20_9_BIT) == DS18B20_9_BIT ) {
			meas &= ~(DS18B20_9_BIT_UNDF);
		}
	}

	*cel  = (uint8_t)(meas >> 4);
	*cel_frac_bits = (uint8_t)(meas & 0x000F);

	return DS18X20_OK;
}

static void DS18X20_put_temp(const uint8_t subzero,
	const uint8_t cel, const uint8_t cel_frac_bits)
{
	printf("%c%d.%04u°C", (subzero)?'-':'+', (int)cel,
			cel_frac_bits*DS18X20_FRACCONV);
}

/* verbose output rom-search follows read-scratchpad in one loop */
uint8_t DS18X20_read_meas_all_verbose( void )
{
	uint8_t id[OW_ROMCODE_SIZE], sp[DS18X20_SP_SIZE], diff;
	uint8_t i;
	uint16_t meas;
	int16_t decicelsius;
	char s[10];
	uint8_t subzero, cel, cel_frac_bits;

	for( diff = OW_SEARCH_FIRST; diff != OW_LAST_DEVICE; )
	{
		diff = ow_rom_search( diff, &id[0] );

		if( diff == OW_PRESENCE_ERR ) {
			puts( "No sensors found" );
			return OW_PRESENCE_ERR; // <--- early exit!
		}

		if( diff == OW_DATA_ERR ) {
			puts( "Bus error" );
			return OW_DATA_ERR;	// <--- early exit!
		}

		DS18X20_show_id( id, OW_ROMCODE_SIZE );

		if( id[0] == DS18B20_FAMILY_CODE || id[0] == DS18S20_FAMILY_CODE ||
		    id[0] == DS1822_FAMILY_CODE ) {
			// temperature sensor

			ow_byte_wr( DS18X20_READ );	      // read command

			for ( i=0 ; i< DS18X20_SP_SIZE; i++ ) {
				sp[i]=ow_byte_rd();
			}

			show_sp( sp, DS18X20_SP_SIZE );

			if ( crc8( &sp[0], DS18X20_SP_SIZE ) ) {
				puts( "(FAIL)" );
			} else {
				puts( "(OK)" );
			}

			meas = sp[0]; // LSB Temp. from Scrachpad-Data
			meas |= (uint16_t) (sp[1] << 8); // MSB

			fputs( "T_raw = 0x", stdout );
			puthex_byte( (uint8_t)(meas >> 8) );
			puthex_byte( (uint8_t)meas );
			putchar( ' ' );

			if( id[0] == DS18S20_FAMILY_CODE ) { // 18S20
				fputs( "(S20/09) ", stdout );
			}
			else if ( id[0] == DS18B20_FAMILY_CODE ||
				  id[0] == DS1822_FAMILY_CODE ) { // 18B20 or 1822
				i=sp[DS18B20_CONF_REG];
				if ( (i & DS18B20_12_BIT) == DS18B20_12_BIT ) {
					fputs( "(B20/12) ", stdout );
				}
				else if ( (i & DS18B20_11_BIT) == DS18B20_11_BIT ) {
					fputs( "(B20/11) ", stdout );
				}
				else if ( (i & DS18B20_10_BIT) == DS18B20_10_BIT ) {
					fputs( "(B20/10) ", stdout );
				}
				else { // if ( (i & DS18B20_9_BIT) == DS18B20_9_BIT ) {
					fputs( "(B20/09) ", stdout );
				}
			}

			DS18X20_meas_to_cel( id[0], sp, &subzero, &cel, &cel_frac_bits );
			DS18X20_put_temp( subzero, cel, cel_frac_bits );

			decicelsius = DS18X20_raw_to_decicelsius( id[0], sp );
			if ( decicelsius == DS18X20_INVALID_DECICELSIUS ) {
				puts("* INVALID *");
			} else {
				DS18X20_format_from_decicelsius( decicelsius, s, 10 );
				printf(" conv: %d deci°C fmt: %s °C", decicelsius, s);
			}
			putchar( '\n' );
			putchar( '\n' );
		} // if meas-sensor
	} // loop all sensors

	putchar('\n');

	return DS18X20_OK;
}

#endif /* DS18X20_VERBOSE */

#if DS18X20_VERBOSE
#define puts_verbose(s__) puts(s__)
#else
#define puts_verbose(s__)
#endif


/*----------- end of "debug-functions" ---------------*/


/* find DS18X20 Sensors on 1-Wire-Bus
   input/ouput: diff is the result of the last rom-search
		*diff = OW_SEARCH_FIRST for first call
   output: id is the rom-code of the sensor found */
uint8_t DS18X20_find_sensor( uint8_t *diff, uint8_t id[] )
{
	uint8_t go;
	uint8_t ret;

	ret = DS18X20_OK;
	go = 1;
	do {
		*diff = ow_rom_search( *diff, &id[0] );
		if ( *diff == OW_PRESENCE_ERR || *diff == OW_DATA_ERR ||
		     *diff == OW_LAST_DEVICE ) {
			go  = 0;
			ret = DS18X20_ERROR;
		} else {
			if ( id[0] == DS18B20_FAMILY_CODE || id[0] == DS18S20_FAMILY_CODE ||
			     id[0] == DS1822_FAMILY_CODE ) {
				go = 0;
			}
		}
	} while (go);

	return ret;
}

/* get power status of DS18x20
   input:   id = rom_code
   returns: DS18X20_POWER_EXTERN or DS18X20_POWER_PARASITE */
uint8_t DS18X20_get_power_status( uint8_t id[] )
{
	uint8_t pstat;

	ow_reset();
	ow_command( DS18X20_READ_POWER_SUPPLY, id );
	pstat = ow_bit_io( 1 );
	ow_reset();
	return ( pstat ) ? DS18X20_POWER_EXTERN : DS18X20_POWER_PARASITE;
}

/* start measurement (CONVERT_T) for all sensors if input id==NULL
   or for single sensor where id is the rom-code */
uint8_t DS18X20_start_meas( uint8_t with_power_extern, uint8_t id[])
{
	uint8_t ret;

	ow_reset();
	if( ow_input_pin_state() ) { // only send if bus is "idle" = high
		if ( with_power_extern != DS18X20_POWER_EXTERN ) {
			ow_command_with_parasite_enable( DS18X20_CONVERT_T, id );
			/* not longer needed: ow_parasite_enable(); */
		} else {
			ow_command( DS18X20_CONVERT_T, id );
		}
		ret = DS18X20_OK;
	}
	else {
		puts_verbose( "DS18X20_start_meas: Short Circuit!\r" );
		ret = DS18X20_START_FAIL;
	}

	return ret;
}

// returns 1 if conversion is in progress, 0 if finished
// not available when parasite powered.
uint8_t DS18X20_conversion_in_progress(void)
{
	return ow_bit_io( 1 ) ? DS18X20_CONVERSION_DONE : DS18X20_CONVERTING;
}

static uint8_t read_scratchpad( uint8_t id[], uint8_t sp[], uint8_t n )
{
	uint8_t i;
	uint8_t ret;

	ow_command( DS18X20_READ, id );
	for ( i = 0; i < n; i++ ) {
		sp[i] = ow_byte_rd();
	}
	if ( crc8( &sp[0], DS18X20_SP_SIZE ) ) {
		ret = DS18X20_ERROR_CRC;
	} else {
		ret = DS18X20_OK;
	}

	return ret;
}


#if DS18X20_DECICELSIUS

/* convert scratchpad data to physical value in unit decicelsius */
static int16_t DS18X20_raw_to_decicelsius( uint8_t familycode, uint8_t sp[] )
{
	uint16_t measure;
	uint8_t	 negative;
	int16_t	 decicelsius;
	uint16_t fract;

	measure = sp[0] | (sp[1] << 8);
	//measure = 0xFF5E; // test -10.125
	//measure = 0xFE6F; // test -25.0625

	if( familycode == DS18S20_FAMILY_CODE ) {   // 9 -> 12 bit if 18S20
		/* Extended measurements for DS18S20 contributed by Carsten Foss */
		measure &= (uint16_t)0xfffe;   // Discard LSB, needed for later extended precicion calc
		measure <<= 3;		       // Convert to 12-bit, now degrees are in 1/16 degrees units
		measure += (16 - sp[6]) - 4;   // Add the compensation and remember to subtract 0.25 degree (4/16)
	}

	// check for negative
	if ( measure & 0x8000 )	 {
		negative = 1;	    // mark negative
		measure ^= 0xffff;  // convert to positive => (twos complement)++
		measure++;
	}
	else {
		negative = 0;
	}

	// clear undefined bits for DS18B20 != 12bit resolution
	if ( familycode == DS18B20_FAMILY_CODE || familycode == DS1822_FAMILY_CODE ) {
		switch( sp[DS18B20_CONF_REG] & DS18B20_RES_MASK ) {
		case DS18B20_9_BIT:
			measure &= ~(DS18B20_9_BIT_UNDF);
			break;
		case DS18B20_10_BIT:
			measure &= ~(DS18B20_10_BIT_UNDF);
			break;
		case DS18B20_11_BIT:
			measure &= ~(DS18B20_11_BIT_UNDF);
			break;
		default:
			// 12 bit - all bits valid
			break;
		}
	}

	decicelsius = (measure >> 4);
	decicelsius *= 10;

	// decicelsius += ((measure & 0x000F) * 640 + 512) / 1024;
	// 625/1000 = 640/1024
	fract = ( measure & 0x000F ) * 640;
	fract += 512;
	fract /= 1024;
	decicelsius += fract;

	if ( negative ) {
		decicelsius = -decicelsius;
	}

	if ( /* decicelsius == 850 || */ decicelsius < -550 || decicelsius > 1250 ) {
		return DS18X20_INVALID_DECICELSIUS;
	} else {
		return decicelsius;
	}
}

/* format decicelsius-value into string, itoa method inspired
   by code from Chris Takahashi for the MSP430 libc, BSD-license
   modifications mthomas: variable-types, fixed radix 10, use div(),
   insert decimal-point */
uint8_t DS18X20_format_from_decicelsius( int16_t decicelsius, char str[], uint8_t n)
{
	uint8_t sign = 0;
	char temp[7];
	int8_t temp_loc = 0;
	uint8_t str_loc = 0;
	div_t dt;
	uint8_t ret;

	// range from -550:-55.0°C to 1250:+125.0°C -> min. 6+1 chars
	if ( n >= (6+1) && decicelsius > -1000 && decicelsius < 10000 ) {

		if ( decicelsius < 0) {
			sign = 1;
			decicelsius = -decicelsius;
		}

		// construct a backward string of the number.
		do {
			dt = div(decicelsius,10);
			temp[temp_loc++] = dt.rem + '0';
			decicelsius = dt.quot;
		} while ( decicelsius > 0 );

		if ( sign ) {
			temp[temp_loc] = '-';
		} else {
			///temp_loc--;
			temp[temp_loc] = '+';
		}

		// reverse the string.into the output
		while ( temp_loc >=0 ) {
			str[str_loc++] = temp[(uint8_t)temp_loc--];
			if ( temp_loc == 0 ) {
				str[str_loc++] = DS18X20_DECIMAL_CHAR;
			}
		}
		str[str_loc] = '\0';

		ret = DS18X20_OK;
	} else {
		ret = DS18X20_ERROR;
	}

	return ret;
}

/* reads temperature (scratchpad) of sensor with rom-code id
   output: decicelsius
   returns DS18X20_OK on success */
uint8_t DS18X20_read_decicelsius( uint8_t id[], int16_t *decicelsius )
{
	uint8_t sp[DS18X20_SP_SIZE];
	uint8_t ret;

	ow_reset();
	ret = read_scratchpad( id, sp, DS18X20_SP_SIZE );
	if ( ret == DS18X20_OK ) {
		*decicelsius = DS18X20_raw_to_decicelsius( id[0], sp );
	}
	return ret;
}

/* reads temperature (scratchpad) of sensor without id (single sensor)
   output: decicelsius
   returns DS18X20_OK on success */
uint8_t DS18X20_read_decicelsius_single( uint8_t familycode, int16_t *decicelsius )
{
	uint8_t sp[DS18X20_SP_SIZE];
	uint8_t ret;

	ret = read_scratchpad( NULL, sp, DS18X20_SP_SIZE );
	if ( ret == DS18X20_OK ) {
		*decicelsius = DS18X20_raw_to_decicelsius( familycode, sp );
	}
	return ret;
}

#endif /* DS18X20_DECICELSIUS */


#if DS18X20_MAX_RESOLUTION

static int32_t DS18X20_raw_to_maxres( uint8_t familycode, uint8_t sp[] )
{
	uint16_t measure;
	uint8_t	 negative;
	int32_t	 temperaturevalue;

	measure = sp[0] | (sp[1] << 8);
	//measure = 0xFF5E; // test -10.125
	//measure = 0xFE6F; // test -25.0625

	if( familycode == DS18S20_FAMILY_CODE ) {   // 9 -> 12 bit if 18S20
		/* Extended measurements for DS18S20 contributed by Carsten Foss */
		measure &= (uint16_t)0xfffe;   // Discard LSB, needed for later extended precicion calc
		measure <<= 3;		       // Convert to 12-bit, now degrees are in 1/16 degrees units
		measure += ( 16 - sp[6] ) - 4; // Add the compensation and remember to subtract 0.25 degree (4/16)
	}

	// check for negative
	if ( measure & 0x8000 )	 {
		negative = 1;	    // mark negative
		measure ^= 0xffff;  // convert to positive => (twos complement)++
		measure++;
	}
	else {
		negative = 0;
	}

	// clear undefined bits for DS18B20 != 12bit resolution
	if ( familycode == DS18B20_FAMILY_CODE || familycode == DS1822_FAMILY_CODE ) {
		switch( sp[DS18B20_CONF_REG] & DS18B20_RES_MASK ) {
		case DS18B20_9_BIT:
			measure &= ~(DS18B20_9_BIT_UNDF);
			break;
		case DS18B20_10_BIT:
			measure &= ~(DS18B20_10_BIT_UNDF);
			break;
		case DS18B20_11_BIT:
			measure &= ~(DS18B20_11_BIT_UNDF);
			break;
		default:
			// 12 bit - all bits valid
			break;
		}
	}

	temperaturevalue  = (measure >> 4);
	temperaturevalue *= 10000;
	temperaturevalue +=( measure & 0x000F ) * DS18X20_FRACCONV;

	if ( negative ) {
		temperaturevalue = -temperaturevalue;
	}

	return temperaturevalue;
}

uint8_t DS18X20_read_maxres( uint8_t id[], int32_t *temperaturevalue )
{
	uint8_t sp[DS18X20_SP_SIZE];
	uint8_t ret;

	ow_reset();
	ret = read_scratchpad( id, sp, DS18X20_SP_SIZE );
	if ( ret == DS18X20_OK ) {
		*temperaturevalue = DS18X20_raw_to_maxres( id[0], sp );
	}
	return ret;
}

uint8_t DS18X20_read_maxres_single( uint8_t familycode, int32_t *temperaturevalue )
{
	uint8_t sp[DS18X20_SP_SIZE];
	uint8_t ret;

	ret = read_scratchpad( NULL, sp, DS18X20_SP_SIZE );
	if ( ret == DS18X20_OK ) {
		*temperaturevalue = DS18X20_raw_to_maxres( familycode, sp );
	}
	return ret;

}

uint8_t DS18X20_format_from_maxres( int32_t temperaturevalue, char str[], uint8_t n)
{
	uint8_t sign = 0;
	char temp[10];
	int8_t temp_loc = 0;
	uint8_t str_loc = 0;
	ldiv_t ldt;
	uint8_t ret;

	// range from -550000:-55.0000°C to 1250000:+125.0000°C -> min. 9+1 chars
	if ( n >= (9+1) && temperaturevalue > -1000000L && temperaturevalue < 10000000L ) {

		if ( temperaturevalue < 0) {
			sign = 1;
			temperaturevalue = -temperaturevalue;
		}

		do {
			ldt = ldiv( temperaturevalue, 10 );
			temp[temp_loc++] = ldt.rem + '0';
			temperaturevalue = ldt.quot;
		} while ( temperaturevalue > 0 );

		// mk 20110209
		if ((temp_loc < 4)&&(temp_loc > 1)) {
			temp[temp_loc++] = '0';
		} // mk end

		if ( sign ) {
			temp[temp_loc] = '-';
		} else {
			temp[temp_loc] = '+';
		}

		while ( temp_loc >= 0 ) {
			str[str_loc++] = temp[(uint8_t)temp_loc--];
			if ( temp_loc == 3 ) {
				str[str_loc++] = DS18X20_DECIMAL_CHAR;
			}
		}
		str[str_loc] = '\0';

		ret = DS18X20_OK;
	} else {
		ret = DS18X20_ERROR;
	}

	return ret;
}

#endif /* DS18X20_MAX_RESOLUTION */


#if DS18X20_EEPROMSUPPORT

uint8_t DS18X20_write_scratchpad( uint8_t id[],
	uint8_t th, uint8_t tl, uint8_t conf)
{
	uint8_t ret;

	ow_reset();
	if( ow_input_pin_state() ) { // only send if bus is "idle" = high
		ow_command( DS18X20_WRITE_SCRATCHPAD, id );
		ow_byte_wr( th );
		ow_byte_wr( tl );
		if ( id[0] == DS18B20_FAMILY_CODE || id[0] == DS1822_FAMILY_CODE ) {
			ow_byte_wr( conf ); // config only available on DS18B20 and DS1822
		}
		ret = DS18X20_OK;
	}
	else {
		puts_verbose( "DS18X20_write_scratchpad: Short Circuit!\r" );
		ret = DS18X20_ERROR;
	}

	return ret;
}

uint8_t DS18X20_read_scratchpad( uint8_t id[], uint8_t sp[], uint8_t n )
{
	uint8_t ret;

	ow_reset();
	if( ow_input_pin_state() ) { // only send if bus is "idle" = high
		ret = read_scratchpad( id, sp, n );
	}
	else {
		puts_verbose( "DS18X20_read_scratchpad: Short Circuit!\r" );
		ret = DS18X20_ERROR;
	}

	return ret;
}

uint8_t DS18X20_scratchpad_to_eeprom( uint8_t with_power_extern,
	uint8_t id[] )
{
	uint8_t ret;

	ow_reset();
	if( ow_input_pin_state() ) { // only send if bus is "idle" = high
		if ( with_power_extern != DS18X20_POWER_EXTERN ) {
			ow_command_with_parasite_enable( DS18X20_COPY_SCRATCHPAD, id );
			/* not longer needed: ow_parasite_enable(); */
		} else {
			ow_command( DS18X20_COPY_SCRATCHPAD, id );
		}
		delay_ms(DS18X20_COPYSP_DELAY); // wait for 10 ms
		ret = DS18X20_OK;
	}
	else {
		puts_verbose( "DS18X20_copy_scratchpad: Short Circuit!\r" );
		ret = DS18X20_START_FAIL;
	}

	return ret;
}

uint8_t DS18X20_eeprom_to_scratchpad( uint8_t id[] )
{
	uint8_t ret;
	uint8_t retry_count=255;

	ow_reset();
	if( ow_input_pin_state() ) { // only send if bus is "idle" = high
		ow_command( DS18X20_RECALL_E2, id );
		while( retry_count-- && !( ow_bit_io( 1 ) ) ) {
			;
		}
		if ( retry_count ) {
			ret = DS18X20_OK;
		} else {
			puts_verbose( "DS18X20_recall_E2: timeout!\r" );
			ret = DS18X20_ERROR;
		}
	}
	else {
		puts_verbose( "DS18X20_recall_E2: Short Circuit!\r" );
		ret = DS18X20_ERROR;
	}

	return ret;
}

#endif /* DS18X20_EEPROMSUPPORT */
