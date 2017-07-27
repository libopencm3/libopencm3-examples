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

#ifndef VECTOR_GFX_H_
#define VECTOR_GFX_H_

#include <stdbool.h>
#include <inttypes.h>
#include <float.h>
#include <math.h>

#define vector_flt_t         float
#define vector_flt_MIN       FLT_MIN
#define vector_flt_MAX       FLT_MAX
#define vector_flt_EPSILON   FLT_EPSILON
#define vector_flt_MIN_VALUE 0.00000001
#define vector_flt_abs       fabsf
#define vector_flt_sqrt      sqrtf
#define vector_flt_pow       powf
#define vector_flt_sin       sinf
#define vector_flt_cos       cosf
#define vector_flt_tan       tanf
#define vector_flt_atan2     atan2f
#define vector_flt_min       fminf
#define vector_flt_max       fmaxf

#include "vector.h"
#include "matrix.h"
#include "bezier.h"

#endif /* VECTOR_GFX_H_ */
