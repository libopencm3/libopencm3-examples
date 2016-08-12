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
 *
 * Warning, those functions are probably not correct!
 *
 */

#ifndef MATRIX_H_
#define MATRIX_H_

#include "vector_gfx.h"

typedef struct {
	vector_flt_t a, b, c;
	vector_flt_t p, q, r;
	vector_flt_t u, v, w;
} mat3x3_t;
static inline
mat3x3_t
mat3x3_empty(void) {
	return (mat3x3_t) {
		.a = 1, .b = 0, .c = 0,
		.p = 0, .q = 1, .r = 0,
		.u = 0, .v = 0, .w = 1
	};
}
static inline
mat3x3_t
mat3x3_translate(vector_flt_t x, vector_flt_t y) {
	return (mat3x3_t) {
		.a = 1, .b = 0, .c = x,
		.p = 0, .q = 1, .r = y,
		.u = 0, .v = 0, .w = 1
	};
}
static inline
mat3x3_t
mat3x3_scale(vector_flt_t w, vector_flt_t h) {
	return (mat3x3_t) {
		.a = w, .b = 0, .c = 0,
		.p = 0, .q = h, .r = 0,
		.u = 0, .v = 0, .w = 1
	};
}
static inline
mat3x3_t
mat3x3_rotate(vector_flt_t angle) {
	vector_flt_t s, c;
	s = vector_flt_sin(angle);
	c = vector_flt_cos(angle);
	return (mat3x3_t) {
		.a =  c, .b = s, .c = 0,
		.p = -s, .q = c, .r = 0,
		.u =  0, .v = 0, .w = 1
	};
}
static inline
mat3x3_t
mat3x3_shear_x(vector_flt_t angle) {
	return (mat3x3_t) {
		.a = 1, .b = vector_flt_tan(angle), .c = 0,
		.p = 0, .q = 1, .r = 0,
		.u = 0, .v = 0, .w = 1
	};
}
static inline
mat3x3_t
mat3x3_shear_y(vector_flt_t angle) {
	return (mat3x3_t) {
		.a = 1, .b = 0, .c = 0,
		.p = vector_flt_tan(angle), .q = 1, .r = 0,
		.u = 0, .v = 0, .w = 1
	};
}
static inline
mat3x3_t
mat3x3_reflect_xy(void) {
	return (mat3x3_t) {
		.a = -1, .b =  0, .c = 0,
		.p =  0, .q = -1, .r = 0,
		.u =  0, .v =  0, .w = 1
	};
}
static inline
mat3x3_t
mat3x3_reflect_x(void) {
	return (mat3x3_t) {
		.a =  1, .b =  0, .c = 0,
		.p =  0, .q = -1, .r = 0,
		.u =  0, .v =  0, .w = 1
	};
}
static inline
mat3x3_t
mat3x3_reflect_y(void) {
	return (mat3x3_t) {
		.a = -1, .b =  0, .c = 0,
		.p =  0, .q =  1, .r = 0,
		.u =  0, .v =  0, .w = 1
	};
}

static inline
mat3x3_t
mat3x3_mult(mat3x3_t A, mat3x3_t B) {
	return (mat3x3_t) {
		.a = A.a*B.a+A.b*B.p+A.c*B.u,
		.b = A.a*B.b+A.b*B.q+A.c*B.v,
		.c = A.a*B.c+A.b*B.r+A.c*B.w,

		.p = A.p*B.a+A.q*B.p+A.r*B.u,
		.q = A.p*B.b+A.q*B.q+A.r*B.v,
		.r = A.p*B.c+A.q*B.r+A.r*B.w,

		.u = A.u*B.a+A.v*B.p+A.w*B.u,
		.v = A.u*B.b+A.v*B.q+A.w*B.v,
		.w = A.u*B.c+A.v*B.r+A.w*B.w
	};
}

static inline
void
mat3x3_affine_transform(
		mat3x3_t mat,
		point2d_t *points_dest, point2d_t *points_src,
		size_t points_count
) {
	while (points_count--) {
		/* Copy the source point, this allows for src and dest to be
		 * the same array
		 */
		point2d_t p = *points_src++;

		points_dest->x = mat.a * p.x + mat.b * p.y + mat.c; /* p.z; */
		points_dest->y = mat.p * p.x + mat.q * p.y + mat.r; /* p.z; */
		/* points->z = mat.u * p.x + mat.v * p.y + mat.w * p.z; */
		points_dest++;
	}
}


#endif /* MATRIX_H_ */
