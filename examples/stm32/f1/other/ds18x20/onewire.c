/*
Access Dallas 1-Wire Devices
Copyright (C) Peter Dannegger <danni@specs.de>
Copyright (C) 2011 Martin Thomas <eversmith@heizung-thomas.de>

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

#include "onewire.h"

#define OW_GET_IN()   ( GPIO_IDR(OW_PORT) & OW_PIN )
#define OW_OUT_LOW()  ( GPIO_BRR(OW_PORT) = OW_PIN )
#define OW_OUT_HIGH() ( GPIO_BSRR(OW_PORT) = OW_PIN )

#if (OW_USE_INTERNAL_PULLUP)
	#define OW_DIR_IN()   ( \
		OW_CR = (OW_CR & ~(0xf << OW_CRSHIFT)) | \
		 (((GPIO_CNF_INPUT_PULL_UPDOWN << 2) | GPIO_MODE_INPUT) << OW_CRSHIFT) )
#else
	#define OW_DIR_IN()   ( \
		OW_CR = (OW_CR & ~(0xf << OW_CRSHIFT)) | \
		 (((GPIO_CNF_INPUT_FLOAT << 2) | GPIO_MODE_INPUT) << OW_CRSHIFT) )
#endif

#define OW_DIR_OUT()  (	OW_CR = (OW_CR & ~(0xf << OW_CRSHIFT)) | \
		 (((GPIO_CNF_OUTPUT_PUSHPULL << 2) | GPIO_MODE_OUTPUT_50_MHZ) << OW_CRSHIFT) )


uint8_t ow_input_pin_state()
{
	return !!OW_GET_IN();
}

static void ow_parasite_enable(void)
{
	OW_OUT_HIGH();
	OW_DIR_OUT();
}

#if 0
static void ow_parasite_disable(void)
{
	OW_DIR_IN();
}
#endif

uint8_t ow_reset(void)
{
	uint8_t err;

	OW_OUT_LOW();
	OW_DIR_OUT();		 // pull OW-Pin low for 480us
	delay_us(480);

	// set Pin as input - wait for clients to pull low
	OW_DIR_IN(); // input
#if OW_USE_INTERNAL_PULLUP
	OW_OUT_HIGH();
#endif
	delay_us(64);	    // was 66
	err = OW_GET_IN();   // no presence detect
			     // if err!=0: nobody pulled to low, still high

	// after a delay the clients should release the line
	// and input-pin gets back to high by pull-up-resistor
	delay_us(480 - 64);	  // was 480-66
	if( OW_GET_IN() == 0 ) {
		err = 1;	     // short circuit, expected low but got high
	}

	return err;
}


/* Timing issue when using runtime-bus-selection (!OW_ONE_BUS):
   The master should sample at the end of the 15-slot after initiating
   the read-time-slot. The variable bus-settings need more
   cycles than the constant ones so the delays had to be shortened
   to achive a 15uS overall delay
   Setting/clearing a bit in I/O Register needs 1 cyle in OW_ONE_BUS
   but around 14 cyles in configureable bus (us-Delay is 4 cyles per uS) */
static uint8_t ow_bit_io_intern( uint8_t b, uint8_t with_parasite_enable )
{
#if OW_USE_INTERNAL_PULLUP
	OW_OUT_LOW();
#endif
	OW_DIR_OUT();	 // drive bus low
	delay_us(2);	// T_INT > 1usec accoding to timing-diagramm
	if ( b ) {
		OW_DIR_IN(); // to write "1" release bus, resistor pulls high
#if OW_USE_INTERNAL_PULLUP
		OW_OUT_HIGH();
#endif
	}

	// "Output data from the DS18B20 is valid for 15usec after the falling
	// edge that initiated the read time slot. Therefore, the master must
	// release the bus and then sample the bus state within 15ussec from
	// the start of the slot."
	delay_us(15-2-OW_CONF_DELAYOFFSET);

	if( OW_GET_IN() == 0 ) {
		b = 0;	// sample at end of read-timeslot
	}

	delay_us(60-15-2+OW_CONF_DELAYOFFSET);
#if OW_USE_INTERNAL_PULLUP
	OW_OUT_HIGH();
#endif
	OW_DIR_IN();

	if ( with_parasite_enable ) {
		ow_parasite_enable();
	}

	delay_us(OW_RECOVERY_TIME); // may be increased for longer wires

	return b;
}

uint8_t ow_bit_io( uint8_t b )
{
	return ow_bit_io_intern( b & 1, 0 );
}

uint8_t ow_byte_wr( uint8_t b )
{
	uint8_t i = 8, j;

	do {
		j = ow_bit_io( b & 1 );
		b >>= 1;
		if( j ) {
			b |= 0x80;
		}
	} while( --i );

	return b;
}

uint8_t ow_byte_wr_with_parasite_enable( uint8_t b )
{
	uint8_t i = 8, j;

	do {
		if ( i != 1 ) {
			j = ow_bit_io_intern( b & 1, 0 );
		} else {
			j = ow_bit_io_intern( b & 1, 1 );
		}
		b >>= 1;
		if( j ) {
			b |= 0x80;
		}
	} while( --i );

	return b;
}


uint8_t ow_byte_rd( void )
{
	// read by sending only "1"s, so bus gets released
	// after the init low-pulse in every slot
	return ow_byte_wr( 0xFF );
}


uint8_t ow_rom_search( uint8_t diff, uint8_t *id )
{
	uint8_t i, j, next_diff;
	uint8_t b;

	if( ow_reset() ) {
		return OW_PRESENCE_ERR;		// error, no device found <--- early exit!
	}

	ow_byte_wr( OW_SEARCH_ROM );	    // ROM search command
	next_diff = OW_LAST_DEVICE;	    // unchanged on last device

	i = OW_ROMCODE_SIZE * 8;	    // 8 bytes

	do {
		j = 8;				// 8 bits
		do {
			b = ow_bit_io( 1 );	    // read bit
			if( ow_bit_io( 1 ) ) {	    // read complement bit
				if( b ) {		// 0b11
					return OW_DATA_ERR; // data error <--- early exit!
				}
			}
			else {
				if( !b ) {		// 0b00 = 2 devices
					if( diff > i || ((*id & 1) && diff != i) ) {
						b = 1;		// now 1
						next_diff = i;	// next pass 0
					}
				}
			}
			ow_bit_io( b );		    // write bit
			*id >>= 1;
			if( b ) {
				*id |= 0x80;		// store bit
			}

			i--;

		} while( --j );

		id++;				// next byte

	} while( i );

	return next_diff;		    // to continue search
}


static void ow_command_intern( uint8_t command, uint8_t *id, uint8_t with_parasite_enable )
{
	uint8_t i;

	ow_reset();

	if( id ) {
		ow_byte_wr( OW_MATCH_ROM );	// to a single device
		i = OW_ROMCODE_SIZE;
		do {
			ow_byte_wr( *id );
			id++;
		} while( --i );
	}
	else {
		ow_byte_wr( OW_SKIP_ROM );	// to all devices
	}

	if ( with_parasite_enable  ) {
		ow_byte_wr_with_parasite_enable( command );
	} else {
		ow_byte_wr( command );
	}
}

void ow_command( uint8_t command, uint8_t *id )
{
	ow_command_intern( command, id, 0);
}

void ow_command_with_parasite_enable( uint8_t command, uint8_t *id )
{
	ow_command_intern( command, id, 1 );
}
