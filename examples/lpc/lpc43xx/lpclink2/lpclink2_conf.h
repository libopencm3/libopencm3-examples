/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2012 Benjamin Vernoux <titanmkd@gmail.com>
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

#ifndef __LPCLINKII_CONF_H
#define __LPCLINKII_CONF_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <libopencm3/lpc43xx/scu.h>

/*
 * LPCLinkII SCU PinMux
 */

/* GPIO Output PinMux */
#define SCU_PINMUX_LED1     (P1_1) /* GPIO0[8] on P1_1 */

/* GPIO Input PinMux */
#define SCU_PINMUX_BOOT1    (P1_2) /* GPIO0[9] on P1_2 */
#define SCU_PINMUX_BOOT2    (P2_8) /* GPIO5[7] on P2_8 */
#define SCU_PINMUX_BOOT3    (P2_9) /* GPIO1[10] on P2_9 */

/* SSP1 Peripheral PinMux */
#define SCU_SSP1_MISO       (P1_3) /* P1_3 */
#define SCU_SSP1_MOSI       (P1_4) /* P1_4 */
#define SCU_SSP1_SCK        (P1_19) /* P1_19 */
#define SCU_SSP1_SSEL       (P1_20) /* P1_20 */

/* TODO add other Pins */

/*
 * LPCLinkII GPIO Pin
 */
/* GPIO Output */
#define PIN_LED1    (BIT8) /* GPIO0[8] on P1_1 */
#define PORT_LED1   (GPIO0) /* PORT for LED1 */

/* GPIO Input */
#define PIN_BOOT1   (BIT9)  /* GPIO0[9] on P1_2 */
#define PIN_BOOT2   (BIT7)  /* GPIO5[7] on P2_8 */
#define PIN_BOOT3   (BIT10) /* GPIO1[10] on P2_9 */

/* Read GPIO Pin */
#define BOOT1_STATE ( (GPIO0_PIN & PIN_BOOT1)==PIN_BOOT1 )
#define BOOT2_STATE ( (GPIO5_PIN & PIN_BOOT2)==PIN_BOOT2 )
#define BOOT3_STATE ( (GPIO1_PIN & PIN_BOOT3)==PIN_BOOT3 )

/* TODO add other Pins */

#ifdef __cplusplus
}
#endif

#endif
