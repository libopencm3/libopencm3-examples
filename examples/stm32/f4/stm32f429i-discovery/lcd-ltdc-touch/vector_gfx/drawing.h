/*
 * drawing.h
 *
 *  Created on: 29 Jul 2017
 *	  Author: h2obrain
 */

#ifndef VECTOR_GFX_DRAWING_H_
#define VECTOR_GFX_DRAWING_H_

#include "vector_gfx.h"

void draw_antialised_line(segment2d_t s, uint16_t col);


void draw_thick_line(
		int16_t x0,int16_t y0,int16_t x1, int16_t y1,
		vector_flt_t thickness,
		uint16_t color
	);
typedef vector_flt_t (*drawing_varthick_fct_t)(void *, int16_t, int16_t);
void draw_varthick_line(
		int16_t x0,int16_t y0,int16_t x1, int16_t y1,
		drawing_varthick_fct_t left, void *argL,
		drawing_varthick_fct_t right, void *argR,
		uint16_t color
	);
#endif /* VECTOR_GFX_DRAWING_H_ */
