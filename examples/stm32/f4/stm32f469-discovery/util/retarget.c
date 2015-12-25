/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 Chuck McManis <cmcmanis@mcmanis.com>
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

/* retarget.c
 *
 * Stub routines to connect the board's USART3 which is wired into the
 * STLink port, into the C library functions for standard I/O.
 */
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include "../util/clock.h"
#include "../util/console.h"

#ifndef NULL
#define NULL	0	
#endif

#define BUFLEN 127

static void SystemInit(void);

/*
 * These are the functions to define to enable the
 * newlib hooks to implement basic character I/O
 */
int _write (int fd, char *ptr, int len);
int _read (int fd, char *ptr, int len);

/*
 * A 128 byte buffer for getting a string from the
 * console.
 */
static char buf[BUFLEN+1] = {0};
static char *next_char;

/* 
 * Called by libc stdio functions
 */
int 
_write (int fd, char *ptr, int len)
{
	int i = 0;

	/* 
	 * Write "len" of char from "ptr" to file id "fd"
	 * Return number of char written.
	 */
	if (fd > 2) {
		return -1;  // STDOUT, STDIN, STDERR
	}
	if (fd == 2) {
		/* set the text output YELLOW when sending to stderr */
		console_puts("\033[33;40;1m");
	}
	while (*ptr && (i < len)) {
		console_putc(*ptr);
		if (*ptr == '\n') {
			console_putc('\r');
		}
		i++;
		ptr++;
	}
	if (fd == 2) {
		/* return text out to its default state */
		console_puts("\033[0m");
	}
  return i;
}


/*
 * Depending on the implementation, this function can call
 * with a buffer length of 1 to 1024. However it does no
 * editing on console reading. So, the console_gets code 
 * implements a simple line editing input style.
 */
int
_read (int fd, char *ptr, int len)
{
	int	my_len;

	if (fd > 2) {
		return -1;
	}

	/* If not null we've got more characters to return */
	if (next_char == NULL) {
		console_gets(buf, BUFLEN);
		next_char = &buf[0];
	}

	my_len = 0;
	while ((*next_char != 0) && (len > 0)) {
		*ptr++ = *next_char++;
		my_len++;
		len--;
	}
	if (*next_char == 0) {
		next_char = NULL;
	}
	return my_len; // return the length we got
}

/* SystemInit will be called before main 
 * This works because we tell GCC that it is a constructor
 * which means "run this before main is invoked".
 */
__attribute__((constructor))
static void SystemInit()
{
	clock_setup();
	console_setup(115200);
	next_char = NULL;
}
