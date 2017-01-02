/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 Kuldeep Singh Dhaka <kuldeepdhaka9@gmail.com>
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

#include <libopencm3/efm32/cmu.h>
#include <libopencm3/efm32/dac.h>

int main(void)
{
	cmu_clock_setup_in_hfxo_out_48mhz();
	CMU_HFPERCLKEN0 = CMU_HFPERCLKEN0_DAC0;
	DAC0_CTRL = DAC_CTRL_REFSEL_VDD | DAC_CTRL_PRESC(6) | DAC_CTRL_OUTMODE_PIN;
	DAC0_CH0CTRL = DAC_CH0CTRL_EN;
	DAC0_CH0DATA = 0x7FF;

	while(1);
	return 1;
}
