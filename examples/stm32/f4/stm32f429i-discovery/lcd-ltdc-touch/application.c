/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2016 Oliver Meier <h2obrain@gmail.com>
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

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <stdio.h>
#include <stdbool.h>
#include "clock.h"
#include "sdram.h"
#include "lcd_ili9341.h"

#include "touchscreen_controller_stmpe811.h"
#include "fonts/Tamsyn5x9r_9.h"
#include "fonts/Tamsyn5x9b_9.h"
#include "vector_gfx/vector_gfx.h"
#include "application_balls.h"

/**
 * Interrupts
 */

/* Display SPI interrupt */
void spi5_isr()
{
	if (ILI9341_SPI_IS_SELECTED()) {
		ili9341_spi5_isr();
	} else {
		spi_disable_tx_buffer_empty_interrupt(SPI5);
		(void)SPI_DR(SPI5);
	}
}

/* Touchscreen interrupt */
void
exti15_10_isr() {
	exti_reset_request(EXTI15);
	stmpe811_handle_interrupt();
}

/* Blue button interrupt */
typedef enum {
	DEMO_MODE_ALL,
	DEMO_MODE_FLOODFILL4,
	DEMO_MODE_BEZIER,
	DEMO_MODE_BEZIER_INTERACTIVE,
	DEMO_MODE_BALLS,
} demo_mode_t;
demo_mode_t new_demo_mode = DEMO_MODE_ALL;

void
exti0_isr()
{
	exti_reset_request(EXTI0);
	new_demo_mode = (new_demo_mode+1) % (DEMO_MODE_BALLS+1);
}




/**
 * update touchscreen infos
 */
stmpe811_touch_t touch_data;
stmpe811_drag_data_t drag_data;
point2d_t touch_point;
point2d_t drag_distance;

static
void
update_touchscreen_data(void)
{
	touch_data    = (stmpe811_touch_t){0};
	drag_data     = (stmpe811_drag_data_t){0};
	touch_point   = (point2d_t) {-1, -1};
	drag_distance = (point2d_t) { 0,  0};

	touch_data = stmpe811_get_touch_data();
	if (touch_data.touched == 1) {
		touch_point = (point2d_t) {
				(vector_flt_t)gfx_width()
				* (touch_data.y - STMPE811_Y_MIN)
				/ (STMPE811_Y_MAX - STMPE811_Y_MIN),
				(vector_flt_t)gfx_height()
				* (touch_data.x-STMPE811_X_MIN)
				/ (STMPE811_X_MAX-STMPE811_X_MIN)
		};
	} else {
		drag_data = stmpe811_get_drag_data();
		if (drag_data.data_is_valid) {
			drag_distance = (point2d_t) {
					(vector_flt_t)gfx_width()
					* drag_data.dy
					/ (STMPE811_Y_MAX-STMPE811_Y_MIN),
					(vector_flt_t)gfx_height()
					* drag_data.dx
					/ (STMPE811_X_MAX-STMPE811_X_MIN)
			};
		} else {
			/* stop drag*/
		}
	}
}
static
void
print_touchscreen_data(int16_t x, int16_t y)
{
	char conv_buf[1024];

	const char *state;
	/*switch (stmpe811.sample_read_state) {*/
	switch (stmpe811.current_touch_state) {
	case STMPE811_TOUCH_STATE__UNTOUCHED:
		state = "untouched";
		break;
	case STMPE811_TOUCH_STATE__TOUCHED:
		state = "touched";
		break;
	case STMPE811_TOUCH_STATE__TOUCHED_WAITING_FOR_TIMEOUT:
		state = "touched waiting";
		break;
	default:
		state = "invalid";
		break;
	}

	sprintf(conv_buf,
			"touch % 8.1f%8.1f\n"
			"drag  % 8.1f%8.1f\n"
			"touch interrupts: %lu\n"
			"state : %s\n"
			"values count : %lu",
			touch_point.x, touch_point.y,
			drag_distance.x, drag_distance.y,
			stmpe811.touch_interrupts,
			state,
			stmpe811.last_values_count
		);

	gfx_puts2(x, y, conv_buf, &font_Tamsyn5x9r_9, GFX_COLOR_WHITE);
}




/**
 * Flood fill 4 and offscreen rendering demonstration
 */
#define FROWNY_WIDTH  101
#define FROWNY_HEIGHT 121
uint16_t frowny[FROWNY_WIDTH * FROWNY_HEIGHT];
static void init_floodfill4(void)
{
    /* init flood-fill (also offscreen rendering demo) */
	gfx_offscreen_rendering_begin(frowny, FROWNY_WIDTH, FROWNY_HEIGHT);

	gfx_fill_screen(ILI9341_LAYER2_COLOR_KEY);

    /*gfx_draw_rect(0,0,100,100, GFX_COLOR_GREEN2);*/
	/* Frowny */
	gfx_draw_circle(50, 50, 50, GFX_COLOR_GREEN2);
	gfx_draw_circle(30, 30, 6, GFX_COLOR_GREEN2);
	gfx_draw_circle(70, 30, 6, GFX_COLOR_GREEN2);
	gfx_draw_line(30, 80, 40, 70, GFX_COLOR_GREEN2);
	gfx_draw_line(40, 70, 50, 80, GFX_COLOR_GREEN2);
	gfx_draw_line(50, 80, 60, 70, GFX_COLOR_GREEN2);
	gfx_draw_line(60, 70, 70, 80, GFX_COLOR_GREEN2);
	/* Goatee */
	gfx_draw_vline(50, 80, 20, GFX_COLOR_GREEN2);
	/* 4 Markers */
	gfx_draw_vline(70,  1, 15, GFX_COLOR_GREEN2);
	gfx_draw_vline(70, 90, 15, GFX_COLOR_GREEN2);
	gfx_draw_vline(30,  1, 15, GFX_COLOR_GREEN2);
	gfx_draw_vline(30, 90, 15, GFX_COLOR_GREEN2);
	/* Scars */
	gfx_draw_line(20, 10, 70, 70, GFX_COLOR_GREEN2);
	gfx_draw_line(10, 75, 90, 30, GFX_COLOR_GREEN2);
	/* Fly:) */
	gfx_draw_rect(10, 100, 80, 21,  GFX_COLOR_GREEN2);
	gfx_draw_line(13, 105, 87, 117, GFX_COLOR_GREEN2);
	gfx_draw_line(20, 117, 70, 100, GFX_COLOR_GREEN2);
	/* Worst case dots
	for (int16_t px=1; px<100; px+=4) {
		for (int16_t py=1; py<120; py+=2) {
			gfx_draw_pixel(px, py, GFX_COLOR_GREEN2);
			gfx_draw_pixel(px+2, py+1, GFX_COLOR_GREEN2);
		}
	}
	*/
	/* Head-Fly connection */
	gfx_draw_vline(50, 99, 3, ILI9341_LAYER2_COLOR_KEY);

	/* resume normal rendering */
	gfx_offscreen_rendering_end();
}
static void draw_floodfill4(demo_mode_t demo_mode)
{
	int16_t x, y, px,py;
	static int16_t scan_x = -1, scan_y = -1;

	switch (demo_mode) {
	case DEMO_MODE_ALL:
		x = 10;
		y = 70;
		break;
	case DEMO_MODE_FLOODFILL4:
		x = (gfx_width()-FROWNY_WIDTH)/2;
		y = 70;
		break;
	default:
		return;
	}

	gfx_set_clipping_area(
			x-20, y-20,
			x+FROWNY_WIDTH+20, y+FROWNY_HEIGHT+20
		);
	/* copy buffered frowny */
	gfx_draw_raw_rbg565_buffer(x, y, FROWNY_WIDTH, FROWNY_HEIGHT, frowny);
	/* additional stuff */
	if (demo_mode == DEMO_MODE_FLOODFILL4) {
		/* draw barriers */
		if (scan_y == FROWNY_HEIGHT) {
			scan_y = -1;
		} else
		if (scan_y >= 0) {
			scan_y++;
		} else
		if (scan_x == FROWNY_WIDTH) {
			scan_x = -1;
			scan_y++;
		} else {
			scan_x++;
		}
		if (scan_x >= 0) {
			gfx_draw_vline(
					x+scan_x, y,
					FROWNY_HEIGHT, GFX_COLOR_BLUE2
				);
		}
		if (scan_y >= 0) {
			gfx_draw_hline(
					x, y+scan_y,
					FROWNY_WIDTH , GFX_COLOR_BLUE2
				);
		}

		/* Worst case dots */
		for (px = x+1; px < x+100; px += 4) {
			for (py = y+1; py < y+120; py += 2) {
				gfx_draw_pixel(px, py, GFX_COLOR_GREEN2);
				gfx_draw_pixel(px+2, py+1, GFX_COLOR_GREEN2);
			}
		}
	}
	/* Flood fill */
	uint8_t fill_segment_buf[8*2048];
	fill_segment_queue_statistics_t stats =
		gfx_flood_fill4(
				x+40, y+50,
				ILI9341_LAYER2_COLOR_KEY, GFX_COLOR_RED,
				fill_segment_buf, sizeof(fill_segment_buf)
			);
	gfx_set_clipping_area_max();
	/* Print statistics */
	char buf[1024];
	snprintf(buf, 1023,
			"flood_fill4 test:\n"
			"%5d segments stored (max)\n"
			"%5d segments stored (total)\n"
			"%5d buffer overflows",
			stats.count_max,
			stats.count_total,
			stats.overflows);
	buf[1023] = 0;
	gfx_puts2(10, 198, buf, &font_Tamsyn5x9r_9 , GFX_COLOR_WHITE);
}


/**
 * Bezier demonstration
 */
uint16_t color = GFX_COLOR_BLACK;
static void draw_segment(point2d_t p1, point2d_t p2)
{
	gfx_draw_line(
			(int16_t)p1.x, (int16_t)p1.y,
			(int16_t)p2.x, (int16_t)p2.y,
			color
		);
}
static void draw_point_list(point2d_t *points, size_t points_count, uint16_t _color)
{
	unsigned int i;
	color = _color;
	for (i = 0; i < points_count-1; i++) {
		draw_segment(points[i], points[i+1]);
	}
}
point2d_t pentagram[] = {
		{  0.00000,  1.00000 },
		{ -0.58779, -0.80902 },
		{  0.95106,  0.30902 },
		{ -0.95106,  0.30902 },
		{  0.58779, -0.80902 },

		{  0.00000,  1.00000 },
		{ -0.58779, -0.80902 },
		{  0.95106,  0.30902 },
		{ -0.95106,  0.30902 },
		{  0.58779, -0.80902 },

		{  0.00000,  1.00000 },
		{ -0.58779, -0.80902 },
		{  0.95106,  0.30902 },
		{ -0.95106,  0.30902 },
		{  0.58779, -0.80902 },

		{  0.00000,  1.00000 },
};
uint32_t num_points;
uint32_t num_ipoints;
static void init_bezier(void)
{
	num_points  = sizeof(pentagram)/sizeof(pentagram[0]);
	num_ipoints = bezier_calculate_int_points_length(num_points);
}
#define TENSION_MIN 0.15f
#define TENSION_MAX 3.0f
static void draw_bezier(demo_mode_t demo_mode)
{
	switch (demo_mode) {
	case DEMO_MODE_ALL:
	case DEMO_MODE_BEZIER:
		break;
	default:
		return;
	}

	static vector_flt_t tension = 1.0f;
	static vector_flt_t tension_change = 1.1f;
	static vector_flt_t rot = 0.0f;
	rot += 0.01f;
	uint32_t i;

	/* transform pentagram */
	point2d_t pentagram_[num_points];

	vector_flt_t x, y;
	x = gfx_width() / 2       + 30 * vector_flt_sin(rot * 3);
	y = 20 + gfx_height() / 2 + 30 * vector_flt_cos(rot * 3);

	gfx_set_clipping_area(1, 41, gfx_width()-1, gfx_height()-1);


	mat3x3_t mat;

	switch (demo_mode) {
	case DEMO_MODE_ALL:
		break;
	case DEMO_MODE_BEZIER:
		/* shear x */
		mat =                  mat3x3_translate(40, 100);
		mat = mat3x3_mult(mat, mat3x3_scale(20, 20));
		mat = mat3x3_mult(mat, mat3x3_shear_x(rot*2));
		mat3x3_affine_transform(
				mat,
				pentagram_, pentagram, num_points
			);
		draw_point_list(
				pentagram_, num_points,
				GFX_COLOR_GREEN
			);
		/* shear y */
		mat =                  mat3x3_translate(280, 100);
		mat = mat3x3_mult(mat, mat3x3_scale(20, 20));
		mat = mat3x3_mult(mat, mat3x3_shear_y(rot*2));
		mat3x3_affine_transform(
				mat,
				pentagram_, pentagram, num_points
			);
		draw_point_list(
				pentagram_, num_points,
				GFX_COLOR_GREEN
			);
		/* scale */
		mat =                  mat3x3_translate(280, 190);
		mat = mat3x3_mult(mat, mat3x3_scale(15/tension, 15*tension));
		mat3x3_affine_transform(
				mat,
				pentagram_, pentagram, num_points
			);
		draw_point_list(
				pentagram_, num_points,
				GFX_COLOR_GREEN
			);
		/* scale */
		mat =                  mat3x3_translate(40, 190);
		mat = mat3x3_mult(mat, mat3x3_scale(25, 25));
		mat = mat3x3_mult(mat, mat3x3_rotate(-rot*6));
		mat3x3_affine_transform(
				mat,
				pentagram_, pentagram, num_points
			);
		draw_point_list(
				pentagram_, num_points,
				GFX_COLOR_GREEN
			);

		break;
	default:
		return;
	}



	mat =                  mat3x3_translate(x, y);
	mat = mat3x3_mult(mat, mat3x3_scale(20, 20));
	mat = mat3x3_mult(mat, mat3x3_rotate(rot));
	mat3x3_affine_transform(
			mat,
			pentagram_, pentagram, num_points
		);
	/* draw original form */
	draw_point_list(pentagram_, num_points, GFX_COLOR_GREEN);

	/* change tension, then redraw bezier curve.. */
	color = GFX_COLOR_BLUE;
	tension *= tension_change;
	if (tension <= TENSION_MIN) {
		tension  = TENSION_MIN;
		tension_change = 1/tension_change;
	} else
	if (tension >= TENSION_MAX) {
		tension  = TENSION_MAX;
		tension_change = 1/tension_change;
	}
	point2d_t pi1[num_ipoints];
	bezier_cubic(pi1, pentagram_, num_points, 0.0001f, tension);
	for (i = 0; i < num_ipoints-1; i += 3) {
		bezier_draw_cubic(
				draw_segment,
				15,
				pi1[i],
				pi1[i+1], pi1[i+2],
				pi1[i+3]
			);
	}
	gfx_set_clipping_area_max();
}

/**
 * Bezier interactive demo
 */

static void draw_bezier_interactive(demo_mode_t demo_mode)
{
	switch (demo_mode) {
	case DEMO_MODE_ALL:
		print_touchscreen_data(200, 190);
		return;

	case DEMO_MODE_BEZIER_INTERACTIVE:
		print_touchscreen_data(10, 190);
		break;
	default:
		return;
	}


	uint32_t i;
	char buf[2];
	buf[1] = 0;

	int16_t point_radius = 15;

	/* small demo curve */
	static point2d_t curve_points[4] = {
			{ 100,  80},
			{  50, 180},
			{ 270, 180},
			{ 220,  80},
	};
	static point2d_t *selected_point;

	if ((touch_point.x != -1.0f) || (touch_point.y != -1.0f)) {
		/* find nearest point */
		vector_flt_t min_dist = point2d_dist(curve_points[0], touch_point);
		selected_point = &curve_points[0];
		for (i = 1; i < 4; i++) {
			vector_flt_t dist = point2d_dist(curve_points[i], touch_point);
			if (min_dist > dist) {
				min_dist = dist;
				selected_point = &curve_points[i];
			}
		}
		if (min_dist > point_radius*1.5f) {
			selected_point = NULL;
		}
	}
	if ((selected_point != NULL)
	 && ((drag_distance.x != 0.0f) || (drag_distance.y != 0.0f))
	) {
		selected_point->x += drag_distance.x;
		selected_point->y += drag_distance.y;
	} else
	if (stmpe811.current_touch_state == STMPE811_TOUCH_STATE__UNTOUCHED) {
		selected_point = NULL;
	}


	/* draw bezier */
	color = GFX_COLOR_RED;
	bezier_draw_cubic(
			draw_segment,
			20,
			curve_points[0],
			curve_points[1], curve_points[2],
			curve_points[3]
		);
	for (i = 0; i < 4; i++) {
		gfx_fill_circle(
				(int16_t)(curve_points[i].x),
				(int16_t)(curve_points[i].y),
				(int16_t) point_radius,
				GFX_COLOR_RED
			);
		buf[0] = 48 + (char)(i%10);
		gfx_set_font_scale(2);
		gfx_puts2(
				(int16_t)curve_points[i].x
				- font_Tamsyn5x9b_9.charwidth
				 *gfx_get_font_scale()/2+1,
				(int16_t)curve_points[i].y
				- font_Tamsyn5x9b_9.lineheight
				 *gfx_get_font_scale()/2,
				buf,
				&font_Tamsyn5x9b_9,
				GFX_COLOR_WHITE
			);
	}

}


/**
 * Ball simulation
 */
balls_t ball_simulation;
ball_t  balls[100];
uint64_t ball_timeout;
static void init_balls(void)
{
	ball_setup(
			&ball_simulation,
			mtime(),
			/* the walls are changed in the
			 * draw_background function!     */
			(walls_t) {
				.x1 = 201,
				.y1 = 61,
				.x2 = gfx_width()-4,
				.y2 = 179,
				.bg_color = GFX_COLOR_GREY2,
				.fg_color = GFX_COLOR_WHITE
			},
			balls, sizeof(balls)/sizeof(ball_t)
		);
    /* create some balls */
	ball_create(&ball_simulation,  5,  5, (point2d_t){ 100,  56 }, (point2d_t){  30, -20 }, GFX_COLOR_BLUE);
	ball_create(&ball_simulation,  6,  6, (point2d_t){ 130,  60 }, (point2d_t){  10, -50 }, GFX_COLOR_BLUE2);
	ball_create(&ball_simulation,  7,  7, (point2d_t){ 190, 140 }, (point2d_t){ -30,  20 }, GFX_COLOR_ORANGE);
	ball_create(&ball_simulation,  4,  4, (point2d_t){ 100, 190 }, (point2d_t){  10, -30 }, GFX_COLOR_YELLOW);
	ball_create(&ball_simulation,  7, 10, (point2d_t){  20, 200 }, (point2d_t){  20,  10 }, GFX_COLOR_GREEN2);
	ball_create(&ball_simulation,  8, 15, (point2d_t){  70, 120 }, (point2d_t){ -20,  30 }, GFX_COLOR_BLACK);
	ball_create(&ball_simulation,  5,  1, (point2d_t){ 180,  90 }, (point2d_t){  40,  10 }, GFX_COLOR_CYAN);
	ball_create(&ball_simulation,  5,  1, (point2d_t){  75, 200 }, (point2d_t){ -20,  20 }, GFX_COLOR_MAGENTA);
	ball_create(&ball_simulation,  5,  1, (point2d_t){ 250, 210 }, (point2d_t){  20, -30 }, GFX_COLOR_GREEN);

	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  10, -20 }, GFX_COLOR_BLACK);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  50, -20 }, GFX_COLOR_DARKGREY);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  20, -20 }, GFX_COLOR_GREY);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  50, -20 }, GFX_COLOR_BLUE);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  20, -20 }, GFX_COLOR_BLUE2);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  40, -20 }, GFX_COLOR_RED);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  50, -20 }, GFX_COLOR_MAGENTA);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  10, -20 }, GFX_COLOR_GREEN);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  50, -20 }, GFX_COLOR_GREEN2);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  20, -20 }, GFX_COLOR_CYAN);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  50, -20 }, GFX_COLOR_YELLOW);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  10, -20 }, GFX_COLOR_ORANGE);
	ball_create(&ball_simulation,  3,  1, (point2d_t){ 100,  56 }, (point2d_t){  50, -20 }, GFX_COLOR_BROWN);

	ball_timeout = mtime() + 5000;
}
static void draw_balls(demo_mode_t demo_mode)
{
	uint64_t ctime = mtime();
	switch (demo_mode) {
	case DEMO_MODE_ALL:
	case DEMO_MODE_BALLS:
		break;
	default:
		/* this results in a lot of computation after a while */
		ball_simulation.time_ms_0 = ctime;
		return;
	}
	/* generate a fast heavy ball to get things moving.. */
	if (ball_timeout < ctime) {
		ball_timeout = UINT64_MAX;
		ball_create(
				&ball_simulation,
				6, 30,
				(point2d_t){ 0, 0 },
				(point2d_t){ 150, 0 },
				GFX_COLOR_RED
			);
		/* puts this ball outside the area.. */
		ball_simulation.balls[ball_simulation.balls_count-1].pos =
				(point2d_t) {10, gfx_height()/2};
	}
	/* move balls */
	ball_move(&ball_simulation, ctime);
	/* draw balls */
	ball_draw(&ball_simulation);
}


/**
 * Re-/draw background
 */
static void draw_background(demo_mode_t demo_mode)
{
	ili9341_set_layer1();

	gfx_fill_screen(GFX_COLOR_BLACK);

	gfx_draw_rect(0, 0, gfx_width()  , 40  , GFX_COLOR_DARKGREY);
	gfx_fill_rect(1, 1, gfx_width()-2, 40-2,
						((0x11 << 11) | (0x11 << 5) | 0x11));
	/*					ltdc_get_rgb565_from_rgb888(0x111111));*/

	gfx_set_font_scale(3);
	gfx_puts2(10, 10, "äLTDC Demo", &font_Tamsyn5x9b_9 , GFX_COLOR_WHITE);
	gfx_set_font_scale(1);
	gfx_set_font(&font_Tamsyn5x9r_9);
	gfx_set_text_color(GFX_COLOR_BLUE2);
	gfx_puts3(
			gfx_width()-10, 14,
			"Press the blue button to\nchange the demonstrations",
			GFX_ALIGNMENT_RIGHT
		);

	/*  */
	/* draw the stage (draw_plane) */
	if (demo_mode == DEMO_MODE_ALL) {
		ball_simulation.walls = (walls_t) {
			.x1 = 201,
			.y1 = 61,
			.x2 = gfx_width()-4,
			.y2 = 179,
			.bg_color = GFX_COLOR_GREY2, .fg_color = GFX_COLOR_WHITE
		};
		ball_draw_walls(&ball_simulation);
	} else
	if (demo_mode == DEMO_MODE_BALLS) {
		ball_simulation.walls = (walls_t) {
			.x1 = 1,
			.y1 = 51,
			.x2 = gfx_width()-1,
			.y2 = gfx_height()-1,
			.bg_color = GFX_COLOR_GREY2, .fg_color = GFX_COLOR_WHITE
		};
		ball_draw_walls(&ball_simulation);
	}

	/* flip background buffer */
	ili9341_flip_layer1_buffer();
}

/**
 * Main loop
 */

#define DISPLAY_TIMEOUT   33 /* ~30fps */
int main(void)
{
	/* init timers. */
	clock_setup();

	/* setup blue button */
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
	exti_select_source(EXTI0, GPIOA);
	exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
	exti_enable_request(EXTI0);
	nvic_enable_irq(NVIC_EXTI0_IRQ);

	/* set up SDRAM. */
	sdram_init();

	/* init LTDC (gfx
	 * cm3 and spi are also initialized by this function call) */
	ili9341_init(
		(uint16_t *[]){
			(uint16_t *)(
				SDRAM_BASE_ADDRESS + 0*ILI9341_SURFACE_SIZE
			),
			(uint16_t *)(
				SDRAM_BASE_ADDRESS + 1*ILI9341_SURFACE_SIZE
			)
		},
		(uint16_t *[]){
			(uint16_t *)(
				SDRAM_BASE_ADDRESS + 2*ILI9341_SURFACE_SIZE
			),
			(uint16_t *)(
				SDRAM_BASE_ADDRESS + 3*ILI9341_SURFACE_SIZE
			)
		}

	);


	/**
	 * setup stmpe811 touchscreen controller (via i2c)
	 */
	stmpe811_setup_hardware();
	stmpe811_setup();
	/* Temperature readings make no sense!
	stmpe811_start_temp_measurements();
	msleep(100);
	for (i=0; i<100000; i++) {
		uint16_t temp;
		msleep(11);
		temp = stmpe811_read_temp_sample();
		// do something with temp
	}
	stmpe811_stop_temp_measuements();
	*/
	stmpe811_start_tsc();
	stmpe811_enable_interrupts();


	/*
	 * Application start..
	 */

	/* rotate LCD for 90 degrees */
	gfx_rotate(GFX_ROTATION_270_DEGREES);

	/* set background color */
	ltdc_set_background_color(0, 0, 0);

	/* clear unused layers */
	ili9341_set_layer1();
	gfx_fill_screen(GFX_COLOR_BLACK);
	ili9341_flip_layer1_buffer();

	ili9341_set_layer2();

	/* color key sets alpha to 0 (aka clear screen) */
	gfx_fill_screen(ILI9341_LAYER2_COLOR_KEY);
	ili9341_flip_layer2_buffer();
	gfx_fill_screen(ILI9341_LAYER2_COLOR_KEY);
	ili9341_flip_layer2_buffer();

	ltdc_reload(LTDC_SRCR_RELOAD_VBR);
	while (LTDC_SRCR_IS_RELOADING());

	/* draw background */
	demo_mode_t demo_mode = new_demo_mode;
	draw_background(demo_mode);

	/* init floodfill4 demo */
	init_floodfill4();

	/* init/draw bezier */
	init_bezier();

	/* init balls demo */
	init_balls();


	ltdc_reload(LTDC_SRCR_RELOAD_VBR);

	while (1) {
		uint64_t ctime   = mtime();
		static uint64_t draw_timeout = 1;
		static char fps_s[7] = "   fps";

		if (!LTDC_SRCR_IS_RELOADING() && (draw_timeout <= ctime)) {
			if (demo_mode != new_demo_mode) {
				demo_mode = new_demo_mode;
				draw_background(demo_mode);
			}

			/* calculate fps */
			uint32_t fps;
			fps = 1000 / (ctime-draw_timeout+DISPLAY_TIMEOUT);
			/* set next timeout */
			draw_timeout = ctime+DISPLAY_TIMEOUT;

			/* select layer to draw on */
			ili9341_set_layer2();
			/* clear the whole screen */
			gfx_fill_screen(ILI9341_LAYER2_COLOR_KEY);

			/**
			 * Get touch infos
			 */
			update_touchscreen_data();

			/**
			 * Flood fill test
			 */
			draw_floodfill4(demo_mode);

			/**
			 * Bezier test
			 */
			draw_bezier(demo_mode);

			/**
			 * Bezier interactive
			 */
			draw_bezier_interactive(demo_mode);

			/**
			 * Ball stuff
			 */
			draw_balls(demo_mode);


			/* draw fps */
			gfx_set_font_scale(1);
			sprintf(fps_s, "%lu fps", fps);
			gfx_puts2(
					10, 55,
					fps_s,
					&font_Tamsyn5x9b_9,
					GFX_COLOR_WHITE
				);

			/* swap the double buffer */
			ili9341_flip_layer2_buffer();

			/* update dislay */
			ltdc_reload(LTDC_SRCR_RELOAD_VBR);
		}
	}
}


