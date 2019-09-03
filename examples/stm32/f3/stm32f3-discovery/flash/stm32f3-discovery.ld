/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2019 Roel Gerrits <roel@roelgerrits.nl>
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

/* Linker script for ST STM32F3DISCOVERY (STM32F303VC, 256K flash,
 * 48K RAM (Of which 8kB are CCM RAM and 40KB are SRAM). */

/* Define memory regions. */
MEMORY
{
	/* NOTE: we decrease the length of the rom section with 2K
	 * as to make space for our custom flash section! */
	rom (rx) : ORIGIN = 0x08000000, LENGTH = 254K 
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 40K

    /* reserve the last page of memory for permanent storage functionality */
    my_flash (rw) : ORIGIN = 0x08000000 + 254K, LENGTH = 2K
}



/*
 * Define a custom flash section.
 * This is done for the single purpose of obtaining a
 * variable (_my_flash_start) that is located at the start of
 * the my_flash section. This variable can then be used in code
 * to learn the the start address of that flash section,
 *
 * &_my_flash_start will be equal to 0x803f800 in this case.
*/
SECTIONS
{
    .my_flash :
    {
        _my_flash_start = .;
    } > my_flash
}


/* Include the common ld script. */
INCLUDE cortex-m-generic.ld

