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
 *
 */

#ifndef BEZIER_H_
#define BEZIER_H_

#include "vector_gfx.h"

typedef struct {
	point2d_t p;
	vector_flt_t a, b;
	vector_flt_t beta;
	vector_flt_t dist;
} bpoint_t;

static inline
uint32_t
bezier_calculate_int_points_length(uint32_t points_length) {
	return (points_length-2)*3+4;
}

typedef void(*h2bez_draw_t) (point2d_t p1, point2d_t p2);

/**
 * Interpolate the points to create a smooth curve
 * for every point which is not the start or end point
 * first look at the slope between the next and previous points
 * Then place the handles at 1/tension the way from the point
 * in the direction of the slope
 *
 * fills int_points array of point2d_t with the length (points_length-2)*3+4
 * in the form :
 *   [ p_start_end,p_control_1,p_control_2,p_start_end, p_control_1, ... ]
 *
 * @param int_points
 * @param points
 * @param points_length
 * @param overshoot
 * @param tension eg. 3
 * @return
 */

void
bezier_cubic(
		point2d_t *int_points,
		point2d_t *points,
		uint32_t points_length,
		vector_flt_t overshoot,
		vector_flt_t tension
	);

/**
 * Interpolate the points to create a smooth curve
 * for every point which is not the start or end point
 * first look at the slope between the next and previous points
 * Then place the handles at 1/tension the way from the point in
 * the direction of the slope
 *
 * Given that all the points are roughly equidistant from the next
 * (saves some computation time)
 *
 * fills int_points array of point2d_t with the length
 * (points_length-2)*3+4 in the form
 *  [ p_start_end,p_control_1,p_control_2,p_start_end, p_control_1, ... ]
 *
 * @param int_points buffer with the size of (points_length-2)*3+4 or h2_bezier_calculate_int_points_length
 * @param points
 * @param points_length
 * @param overshoot
 * @param tension eg. 3
 * @return
 */

void
bezier_cubic_symmetric(
		point2d_t *int_points,
		point2d_t *points,
		uint32_t points_length,
		vector_flt_t overshoot,
		vector_flt_t tension
	);

/* TODO resolution x/y instead of num_segments */

void
bezier_draw_cubic(
		h2bez_draw_t draw,
		uint32_t num_segments,
		point2d_t p0, point2d_t p1, point2d_t p2, point2d_t p3
	);


void
bezier_draw_cubic2(
		h2bez_draw_t draw,
		uint32_t num_segments,
		point2d_t p0, point2d_t p1, point2d_t p2, point2d_t p3
	);


#endif /* BEZIER_H_ */
