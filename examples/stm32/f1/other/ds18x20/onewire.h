#ifndef ONEWIRE_H_
#define ONEWIRE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <libopencm3/stm32/gpio.h>
#include <stdint.h>
#include "delay.h"

/*******************************************/
/* Hardware connection                     */
/*******************************************/

#define OW_PINN 6
#define OW_PORT GPIOC
#define OW_CONF_DELAYOFFSET 0

#define OW_PIN  (1 << OW_PINN)
#if (OW_PINN <= 7)
	#define OW_CR	GPIO_CRL(OW_PORT)
	#define OW_CRSHIFT	(OW_PINN*4)
#else
	#define OW_CR	GPIO_CRH(OW_PORT)
	#define OW_CRSHIFT	((OW_PINN-8)*4)
#endif

// Recovery time (T_Rec) minimum 1usec - increase for long lines
// 5 usecs is a value given in some Maxim AppNotes
// 30 usecs seem to be reliable for longer lines
//#define OW_RECOVERY_TIME	30 /* usec */
#define OW_RECOVERY_TIME	5 /* usec */

// Use STM32's internal pull-up resistor instead of external 4.7k resistor.
// Based on information from Sascha Schade. Experimental but worked in tests
// with one DS18B20 and one DS18S20 on a rather short bus (60cm), where both
// sensores have been parasite-powered.
#define OW_USE_INTERNAL_PULLUP     1  /* 0=external, 1=internal */

/*******************************************/


#define OW_MATCH_ROM    0x55
#define OW_SKIP_ROM     0xCC
#define OW_SEARCH_ROM   0xF0

#define OW_SEARCH_FIRST 0xFF        // start new search
#define OW_PRESENCE_ERR 0xFF
#define OW_DATA_ERR     0xFE
#define OW_LAST_DEVICE  0x00        // last device found

// rom-code size including CRC
#define OW_ROMCODE_SIZE 8

uint8_t ow_reset(void);

uint8_t ow_bit_io( uint8_t b );
uint8_t ow_byte_wr( uint8_t b );
uint8_t ow_byte_rd( void );

uint8_t ow_rom_search( uint8_t diff, uint8_t *id );

void ow_command( uint8_t command, uint8_t *id );
void ow_command_with_parasite_enable( uint8_t command, uint8_t *id );

uint8_t ow_input_pin_state( void );

#ifdef __cplusplus
}
#endif

#endif
