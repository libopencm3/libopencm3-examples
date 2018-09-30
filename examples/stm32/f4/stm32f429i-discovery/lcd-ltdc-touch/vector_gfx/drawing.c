/*
 * drawing.c
 *
 *  Created on: 30 Jul 2017
 *      Author: h2obrain
 */

#include "drawing.h"
#include "../gfx_locm3.h"

/* Xiaolin Wu's line algorithm - Thanks Wikipedia! */
#define darken_color(c,col) ( \
		((uint16_t)vector_flt_round((col&0xf800)*c)&0xf800) \
	  | ((uint16_t)vector_flt_round((col&0x07e0)*c)&0x07e0) \
	  | ((uint16_t)vector_flt_round((col&0x001f)*c)&0x001f) \
	)
#define plot(x,y,c,col) gfx_draw_pixel(x,y,darken_color(c,col));
#define fpart(x)  vector_flt_mod(x,NULL)
#define rfpart(x) (1-fpart(x))

void draw_antialised_line(segment2d_t s, uint16_t col) {
	bool steep = vector_flt_abs(s.p2.y - s.p1.y) > vector_flt_abs(s.p2.x - s.p1.x);

	if (steep) {
		vector_flt_swap(s.p1.x, s.p1.y);
		vector_flt_swap(s.p2.x, s.p2.y);
	}
	if (s.p1.x > s.p2.x) {
		vector_flt_swap(s.p1.x, s.p2.x);
		vector_flt_swap(s.p1.y, s.p2.y);
	}

	point2d_t d = point2d_sub_ts(s.p2,s.p1);
	vector_flt_t gradient = d.y / d.x;
	if (d.x == 0) {
		gradient = 1;
	}

	// handle first endpoint
	vector_flt_t xend = vector_flt_round(s.p1.x);
	vector_flt_t yend = s.p1.y + gradient * (xend - s.p1.x);
	vector_flt_t xgap = rfpart(s.p1.x + 0.5);
	int16_t xpxl1 = xend; // this will be used in the main loop
	int16_t ypxl1 = vector_flt_floor(yend);
	if (steep) {
		plot(ypxl1,   xpxl1, rfpart(yend) * xgap, col);
		plot(ypxl1+1, xpxl1,  fpart(yend) * xgap, col);
	} else {
		plot(xpxl1, ypxl1  , rfpart(yend) * xgap, col);
		plot(xpxl1, ypxl1+1,  fpart(yend) * xgap, col);
	}
	vector_flt_t intery = yend + gradient; // first y-intersection for the main loop

	// handle second endpoint
	xend = vector_flt_round(s.p2.x);
	yend = s.p2.y + gradient * (xend - s.p2.x);
	xgap = fpart(s.p2.x + 0.5);
	int16_t xpxl2 = xend; //this will be used in the main loop
	int16_t ypxl2 = vector_flt_floor(yend);
	if (steep) {
		plot(ypxl2  , xpxl2, rfpart(yend) * xgap, col);
		plot(ypxl2+1, xpxl2,  fpart(yend) * xgap, col);
	} else {
		plot(xpxl2, ypxl2,  rfpart(yend) * xgap, col);
		plot(xpxl2, ypxl2+1, fpart(yend) * xgap, col);
	}

	// main loop
	if (steep) {
		for (int16_t x = xpxl1 + 1; x < xpxl2; x++) {
			plot(vector_flt_floor(intery)  , x, rfpart(intery), col);
			plot(vector_flt_floor(intery)+1, x,  fpart(intery), col);
			intery = intery + gradient;
		}
	} else {
		for (int16_t x = xpxl1 + 1; x < xpxl2; x++) {
			plot(x, vector_flt_floor(intery),  rfpart(intery), col);
			plot(x, vector_flt_floor(intery)+1, fpart(intery), col);
			intery = intery + gradient;
		}
	}
}

#undef darken_color
#undef plot
#undef fpart
#undef rfpart



/* code borrowed from http://kt8216.unixcab.org/murphy/index.html (thick.c) */

/***********************************************************************
 *                                                                     *
 *                            X BASED LINES                            *
 *                                                                     *
 ***********************************************************************/

static
void x_perpendicular(
			uint16_t color,
			int16_t x0,int16_t y0,int16_t dx,int16_t dy,int16_t xstep, int16_t ystep,
			int16_t einit,int16_t w_left, int16_t w_right,int16_t winit
) {
	int16_t x,y,threshold,E_diag,E_square;
	int16_t tk;
	int16_t error;
	int16_t p,q;

	threshold = dx - 2*dy;
	E_diag= -2*dx;
	E_square= 2*dy;
	p=q=0;

	y= y0;
	x= x0;
	error= einit;
	tk= dx+dy-winit;

	while(tk<=w_left)
	{
		 gfx_draw_pixel(x,y, color);
		 if (error>=threshold)
		 {
			 x= x + xstep;
			 error = error + E_diag;
			 tk= tk + 2*dy;
		 }
		 error = error + E_square;
		 y= y + ystep;
		 tk= tk + 2*dx;
		 q++;
	}

	y= y0;
	x= x0;
	error= -einit;
	tk= dx+dy+winit;

	while(tk<=w_right)
	{
		 if (p)
			 gfx_draw_pixel(x,y, color);
		 if (error>threshold)
		 {
			 x= x - xstep;
			 error = error + E_diag;
			 tk= tk + 2*dy;
		 }
		 error = error + E_square;
		 y= y - ystep;
		 tk= tk + 2*dx;
		 p++;
	}

	if (q==0 && p<2) gfx_draw_pixel(x0,y0,color); // we need this for very thin lines
}


static
void x_varthick_line(
			uint16_t color,
			int16_t x0,int16_t y0,int16_t dx,int16_t dy,int16_t xstep, int16_t ystep,
			drawing_varthick_fct_t left, void *argL,
			drawing_varthick_fct_t right,void *argR, int16_t pxstep,int16_t pystep
) {
	int16_t p_error, error, x,y, threshold, E_diag, E_square, length, p;
	int16_t w_left, w_right;
	vector_flt_t D;


	p_error= 0;
	error= 0;
	y= y0;
	x= x0;
	threshold = dx - 2*dy;
	E_diag= -2*dx;
	E_square= 2*dy;
	length = dx+1;
	D= vector_flt_sqrt(dx*dx+dy*dy);

	for(p=0;p<length;p++)
	{
		w_left  = left(argL, p, length)*2*D;
		w_right = right(argR,p, length)*2*D;
		x_perpendicular(color,x,y, dx, dy, pxstep, pystep,
						p_error,w_left,w_right,error);
		if (error>=threshold)
		{
			y= y + ystep;
			error = error + E_diag;
			if (p_error>=threshold)
			{
				x_perpendicular(color,x,y, dx, dy, pxstep, pystep,
								(p_error+E_diag+E_square),
								w_left,w_right,error);
				p_error= p_error + E_diag;
			}
			p_error= p_error + E_square;
		}
		error = error + E_square;
		x= x + xstep;
	}
}

/***********************************************************************
 *  	                                                                 *
 *                            Y BASED LINES                            *
 *                                                                     *
 ***********************************************************************/

static
void y_perpendicular(
			uint16_t color,
			 int16_t x0,int16_t y0,int16_t dx,int16_t dy,int16_t xstep, int16_t ystep,
			 int16_t einit,int16_t w_left, int16_t w_right,int16_t winit
) {
	int16_t x,y,threshold,E_diag,E_square;
	int16_t tk;
	int16_t error;
	int16_t p,q;

	p=q= 0;
	threshold = dy - 2*dx;
	E_diag= -2*dy;
	E_square= 2*dx;

	y= y0;
	x= x0;
	error= -einit;
	tk= dx+dy+winit;

	while(tk<=w_left)
	{
		 gfx_draw_pixel(x,y, color);
		 if (error>threshold)
		 {
			 y= y + ystep;
			 error = error + E_diag;
			 tk= tk + 2*dx;
		 }
		 error = error + E_square;
		 x  = x + xstep;
		 tk = tk + 2*dy;
		 q++;
	}


	y= y0;
	x= x0;
	error= einit;
	tk= dx+dy-winit;

	while(tk<=w_right)
	{
		 if (p)
			 gfx_draw_pixel(x,y, color);
		 if (error>=threshold)
		 {
			 y= y - ystep;
			 error = error + E_diag;
			 tk= tk + 2*dx;
		 }
		 error = error + E_square;
		 x= x - xstep;
		 tk= tk + 2*dy;
		 p++;
	}

	if (q==0 && p<2) gfx_draw_pixel(x0,y0,color); // we need this for very thin lines
}


static
void y_varthick_line(
		uint16_t color,
		int16_t x0,int16_t y0,int16_t dx,int16_t dy,int16_t xstep, int16_t ystep,
		drawing_varthick_fct_t left, void *argL,
		drawing_varthick_fct_t right,void *argR,int16_t pxstep,int16_t pystep
) {
	int16_t p_error, error, x,y, threshold, E_diag, E_square, length, p;
	int16_t w_left, w_right;
	vector_flt_t D;

	p_error= 0;
	error= 0;
	y= y0;
	x= x0;
	threshold = dy - 2*dx;
	E_diag= -2*dy;
	E_square= 2*dx;
	length = dy+1;
	D = vector_flt_sqrt(dx*dx+dy*dy);

	for(p=0;p<length;p++)
	{
		w_left  = left(argL, p, length)*2*D;
		w_right = right(argR, p, length)*2*D;
		y_perpendicular(color,x,y, dx, dy, pxstep, pystep,
						p_error,w_left,w_right,error);
		if (error>=threshold)
		{
			x += xstep;
			error += E_diag;
			if (p_error>=threshold)
			{
				y_perpendicular(color,x,y, dx, dy, pxstep, pystep,
								 p_error+E_diag+E_square,
								 w_left,w_right,error);
				p_error= p_error + E_diag;
			}
			p_error= p_error + E_square;
		}
		error += E_square;
		y += ystep;
	}
}


/***********************************************************************
 * 	                                                                  *
 *                                ENTRY                                *
 *                                                                     *
 ***********************************************************************/

void draw_varthick_line(
		int16_t x0,int16_t y0,int16_t x1, int16_t y1,
		drawing_varthick_fct_t left, void *argL,
		drawing_varthick_fct_t right, void *argR,
		uint16_t color
) {
	int16_t dx,dy,xstep,ystep;
	int16_t pxstep=0, pystep=0;
	int16_t xch; // whether left and right get switched.

	dx= x1-x0;
	dy= y1-y0;
	xstep= ystep= 1;

	if (dx<0) { dx= -dx; xstep= -1; }
	if (dy<0) { dy= -dy; ystep= -1; }

	if (dx==0) xstep= 0;
	if (dy==0) ystep= 0;

	// TODO FIX THIS CASE!!!
	if ((dx==0)&&(dy==0)) return;

	xch= 0;
	switch(xstep + ystep*4)
	{
		case -1 + -1*4 :  pystep= -1; pxstep= 1; xch= 1; break;   // -5
		case -1 +  0*4 :  pystep= -1; pxstep= 0; xch= 1; break;   // -1
		case -1 +  1*4 :  pystep=  1; pxstep= 1; break;   // 3
		case  0 + -1*4 :  pystep=  0; pxstep= -1; break;  // -4
		case  0 +  0*4 :  pystep=  0; pxstep= 0; break;   // 0
		case  0 +  1*4 :  pystep=  0; pxstep= 1; break;   // 4
		case  1 + -1*4 :  pystep= -1; pxstep= -1; break;  // -3
		case  1 +  0*4 :  pystep= -1; pxstep= 0;  break;  // 1
		case  1 +  1*4 :  pystep=  1; pxstep= -1; xch=1; break;  // 5
	}

	if (xch) {
		void *K;
		K= argL; argL= argR; argR= K;
		K= left; left= right; right= K;
	}

	if (dx>dy) {
		x_varthick_line(
				color,x0,y0,dx,dy,xstep,ystep,
				left,argL,right,argR,
				pxstep,pystep
			);
	} else {
		y_varthick_line(
				color,x0,y0,dx,dy,xstep,ystep,
                left,argL,right,argR,
                pxstep,pystep
			);
	}
}
static vector_flt_t const_thickness(vector_flt_t *arg, int16_t P, int16_t length) {
	(void)P;(void)length;
	return *arg;
}
void draw_thick_line(
		int16_t x0,int16_t y0,int16_t x1, int16_t y1,
		vector_flt_t thickness,
		uint16_t color
) {
	thickness /= 2;
	draw_varthick_line(
			x0,y0,x1,y1,
			(drawing_varthick_fct_t )const_thickness,&thickness,
			(drawing_varthick_fct_t )const_thickness,&thickness,
			color
		);
}



