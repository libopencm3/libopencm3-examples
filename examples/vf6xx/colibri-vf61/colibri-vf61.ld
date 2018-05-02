/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2014 Stefan Agner <stefan@agner.ch>
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

/* Linker script for Colibri VF61 (Vybrid VF610, use sysRAM0, 512KiB). */

/* Define memory regions. */
MEMORY
{
	/*
	 * Note: 0x1f000000 is an alias for code bus to the same memory
	 * located at 0x3f000000, hence start with the data section
	 * at an offset of 256KiB. One can define this section lenghts
	 * freely
	 */
	pc_ram (rwx) : ORIGIN = 0x1f000000, LENGTH = 256K
	ps_ram (rwx) : ORIGIN = 0x3f040000, LENGTH = 256K
}

/* Include the common ld script. */
INCLUDE vf6xx/libopencm3_vf6xx.ld

