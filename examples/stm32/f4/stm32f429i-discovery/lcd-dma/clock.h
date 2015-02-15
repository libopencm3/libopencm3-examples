/*
 * This include file describes the functions exported by clock.c
 */
#ifndef __CLOCK_H
#define __CLOCK_H

#include <stdint.h>

/*
 * Definitions for functions being abstracted out
 */
void milli_sleep(uint32_t);
uint32_t mtime(void);
void clock_setup(void);

#endif /* generic header protector */

