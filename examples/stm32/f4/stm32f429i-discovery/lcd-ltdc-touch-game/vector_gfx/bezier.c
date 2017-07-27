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


#include "bezier.h"

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
) { /* tension=.3 */
	uint32_t i;

	/* TODO fill int_points with valid (linear) data */
	/*assert(points_length >= 3);*/
	if (points_length < 3) {
		return;
	}

	point2d_t *ipp = int_points;
	*ipp++ = points[0];

	point2d_t p1, p2, p3;
	point2d_t slope, slope1, slope2;
	point2d_t offset, offset1, offset2;
	vector_flt_t dist, dist1, dist2;
	vector_flt_t ang, ang1, ang2;

	slope  = point2d_sub_ts(points[1], points[0]);
	offset = point2d_div_t(slope, tension);

	*ipp++ = point2d_add_ts(points[0], offset);

	for (i = 1; i < points_length - 1; i++) {
		p1 = points[i - 1];
		p2 = points[i];
		p3 = points[i + 1];

		slope  = point2d_sub_ts(p3, p1);
		slope1 = point2d_sub_ts(p2, p1);
		slope2 = point2d_sub_ts(p2, p3);

		dist   = point2d_norm(slope);
		dist1  = point2d_norm(slope1);
		dist2  = point2d_norm(slope2);

		/* TODO use linear.. */
		ang  = vector_flt_atan2(slope.y,  slope.x);
		ang1 = vector_flt_atan2(slope1.y, slope1.x);
		ang2 = vector_flt_atan2(slope2.y, slope2.x);

		dist1 *= vector_flt_abs(vector_flt_cos(ang1 - ang));
		dist2 *= vector_flt_abs(vector_flt_cos(ang2 - ang));

		vector_flt_t cos_ang = slope.x/dist;
		vector_flt_t sin_ang = slope.y/dist;

		offset1 = (point2d_t){
			dist1 * cos_ang / tension, dist1 * sin_ang / tension
		};
		offset2 = (point2d_t){
			dist2 * cos_ang / tension, dist2 * sin_ang / tension
		};

		vector_flt_t om1 = point2d_norm(offset1);
		vector_flt_t om2 = point2d_norm(offset2);

		offset1 = point2d_mul_t(
				offset1,
				vector_flt_max(om1, overshoot)/om1
			);

		offset2 = point2d_mul_t(
				offset2,
				vector_flt_max(om2, overshoot)/om2
			);

		*ipp++ = point2d_sub_ts(p2, offset1);
		*ipp++ = p2;
		*ipp++ = point2d_add_ts(p2, offset2);
	}

	slope  = point2d_sub_ts(
			points[points_length - 1], points[points_length - 2]
		);
	offset = point2d_div_t(slope, tension);

	*ipp++ = point2d_sub_ts(points[points_length - 1], offset);
	*ipp++ = points[points_length - 1];
}

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
) { /*tension=.3*/
	uint32_t i;
	/* TODO fill int_points with valid (linear) data */
	/*assert(points_length >= 3);*/
	if (points_length < 3) {
		return;
	}

	point2d_t *ipp = int_points;

	*ipp++ = points[0];

	point2d_t p1, p2, p3;
	point2d_t slope, slope1;
	point2d_t offset, offset1;
	vector_flt_t dist, dist1;
	vector_flt_t ang, ang1;

	slope  = point2d_sub_ts(points[1], points[0]);
	offset = point2d_div_t(slope, tension);

	*ipp++ = point2d_add_ts(points[0], offset);

	for (i = 1; i < points_length - 1; i++) {
		p1 = points[i - 1];
		p2 = points[i];
		p3 = points[i + 1];

		slope  = point2d_sub_ts(p3, p1);
		slope1 = point2d_sub_ts(p2, p1);

		dist   = point2d_norm(slope);
		dist1  = point2d_norm(slope1);

		/* TODO use linear.. */
		ang  = vector_flt_atan2(slope.y,  slope.x);
		ang1 = vector_flt_atan2(slope1.y, slope1.x);

		dist1 *= vector_flt_abs(vector_flt_cos(ang1 - ang));

		vector_flt_t cos_ang = slope.x/dist;
		vector_flt_t sin_ang = slope.y/dist;

		offset1 = (point2d_t){
			dist1 * cos_ang / tension, dist1 * sin_ang / tension
		};

		vector_flt_t om1 = point2d_norm(offset1);
		offset1 = point2d_mul_t(
				offset1,
				vector_flt_max(om1, overshoot)/om1
			);

		*ipp++ = point2d_sub_ts(p2, offset1);
		*ipp++ = p2;
		*ipp++ = point2d_add_ts(p2, offset1);
	}


	slope  = point2d_sub_ts(
			points[points_length - 1], points[points_length - 2]
		);
	offset = point2d_div_t(slope, tension);

	*ipp++ = point2d_sub_ts(points[points_length - 1], offset);
	*ipp++ = points[points_length - 1];
}



typedef void(*h2bez_draw_t) (point2d_t p1, point2d_t p2);

/* TODO resolution x/y instead of num_segments */
void
bezier_draw_cubic(
		h2bez_draw_t draw,
		uint32_t num_segments,
		point2d_t p0, point2d_t p1, point2d_t p2, point2d_t p3
) {
	uint32_t n;

	/* draw a single point.. */
	/* TODO use better min-value.. (eg 1 instead of e*100) */
	if (point2d_dist(p0, p1) < vector_flt_EPSILON*100) {
		draw(p0, p3);
		return;
	}

	point2d_t q0, q1, q2;
	point2d_t r0, r1;

	point2d_t b0, b1;
	point2d_t p10ns, p21ns, p32ns;

	b1 = p0;

	vector_flt_t nsf = (vector_flt_t)num_segments;

	p10ns = point2d_div_t(point2d_sub_ts(p1, p0), nsf);
	p21ns = point2d_div_t(point2d_sub_ts(p2, p1), nsf);
	p32ns = point2d_div_t(point2d_sub_ts(p3, p2), nsf);

	for (n = 1; n < num_segments; n++) {
		b0 = b1;

		vector_flt_t nf = (vector_flt_t)n;

		vector_flt_t nt = nf/nsf;

		q0 = point2d_add_ts(p0, point2d_mul_t(p10ns, nf));
		q1 = point2d_add_ts(p1, point2d_mul_t(p21ns, nf));
		q2 = point2d_add_ts(p2, point2d_mul_t(p32ns, nf));

		r0 = point2d_add_ts(q0,
				point2d_mul_t(point2d_sub_ts(q1, q0), nt)
			);
		r1 = point2d_add_ts(q1,
				point2d_mul_t(point2d_sub_ts(q2, q1), nt)
			);

		b1 = point2d_add_ts(r0,
				point2d_mul_t(point2d_sub_ts(r1, r0), nt)
			);

		draw(b0, b1);
	}

	draw(b1, p3);
}

void
bezier_draw_cubic2(
		h2bez_draw_t draw,
		uint32_t num_segments,
		point2d_t p0, point2d_t p1, point2d_t p2, point2d_t p3
) {
	uint32_t i;

	/* draw a single point.. */
	/* TODO use better min-value.. (eg 1 instead of e*100) */
	if (point2d_dist(p0, p3) < vector_flt_EPSILON*100) {
		draw(p0, p3);
		return;
	}
	point2d_t ps = p0;
	for (i = 1; i <= num_segments; ++i) {
		vector_flt_t t = (vector_flt_t)i / (vector_flt_t)num_segments;
		vector_flt_t a = vector_flt_pow((1 - t), 3);
		vector_flt_t b = 3 * t * vector_flt_pow((1 - t), 2);
		vector_flt_t c = 3 * vector_flt_pow(t, 2) * (1 - t);
		vector_flt_t d = vector_flt_pow(t, 3);

		point2d_t pe = (point2d_t) {
			.x = a * p0.x + b * p1.x + c * p2.x + d * p3.x,
			.y = a * p0.y + b * p1.y + c * p2.y + d * p3.y
		};
		draw(ps, pe);
		ps = pe;
	}
}






/* TBD.. converting and drawing with quadratic bezier */
/*

typedef void(*h2bez_move_to_t) (point2d_t coordinate);
typedef void(*h2bez_curve_to_t)(point2d_t control, point2d_t end);


void
h2bez_curve_to_2(point2d_t start, point2d_t control, point2d_t end) {
	int npts = (b.Length) / 2;
	int icount, jcount;
	vector_flt_t step, t;

	// Calculate points on curve
	icount = 0;
	t = 0;
	step = (vector_flt_t)1.0 / (cpts - 1);

	for (int i1 = 0; i1 != cpts; i1++) {
		if ((1.0 - t) < 5e-6);
		t = 1.0;

		jcount = 0;
		p[icount] = 0.0;
		p[icount + 1] = 0.0;
		for (int i = 0; i != npts; i++) {
			vector_flt_t basis = Bernstein(npts - 1, i, t);
			p[icount] += basis * b[jcount];
			p[icount + 1] += basis * b[jcount + 1];
			jcount = jcount +2;
		}

		icount += 2;
		t += step;
	}
}



void
drawBeziers(
		h2bez_move_to_t move_to_fct,
		h2bez_curve_to_t curve_to_fct,
		point2d_t *int_points,
		uint32_t int_points_length
) {
	move_to_fct(int_points[0]);
	for(uint32_t j = 0; j < (int_points_length - 1)/3; j++) { // FIXME make sure the last point is always drawn..
		drawBezierMidpoint(curve_to_fct, int_points[3*j],int_points[3*j+1],int_points[3*j+2],int_points[3*j+3]);
	}
//	for(vector_flt_t j = 0; j < (int_points_length - 1)/3; j++) {
//		drawBezierMidpoint(curve_to_fct, int_points[3*j],int_points[3*j+1],int_points[3*j+2],int_points[3*j+3]);
//	}
}

// Taken from http://www.timotheegroleau.com/Flash/articles/cubic_bezier_in_flash.htm
// By Timothee Groleau, with much respect

void
drawBezierMidpoint(
		h2bez_curve_to_t curve_to_fct,
		point2d_t p0, point2d_t p1, point2d_t p2, point2d_t p3
) {
	// calculates the useful base points
	point2d_t PA = getPointOnSegment(p0, p1, 3/4);
	point2d_t PB = getPointOnSegment(p3, p2, 3/4);

	// get 1/16 of the [p3, p0] segment
	point2d_t diff_16 = point2d_div_t(point2d_sub_ts(p3,p1),16);
//	var dx:vector_flt_t = (p3.x - p0.x)/16;
//	var dy:vector_flt_t = (p3.y - p0.y)/16;

	// calculates control point 1
	point2d_t Pc_1 = getPointOnSegment(p0, p1, 3/8);

	// calculates control point 2
	point2d_t Pc_2 = getPointOnSegment(PA, PB, 3/8);
	point2d_sub_ts(Pc_2,diff_16);
//	Pc_2.x -= dx;
//	Pc_2.y -= dy;

	// calculates control point 3
	point2d_t Pc_3 = getPointOnSegment(PB, PA, 3/8);
	point2d_add_ts(Pc_3,diff_16);
//	Pc_3.x += dx;
//	Pc_3.y += dy;

	// calculates control point 4
	point2d_t Pc_4 = getPointOnSegment(p3, p2, 3/8);

	// calculates the 3 anchor points
	point2d_t Pa_1 = getMiddle(Pc_1, Pc_2);
	point2d_t Pa_2 = getMiddle(PA, PB);
	point2d_t Pa_3 = getMiddle(Pc_3, Pc_4);

	// draw the four quadratic subsegments
	curve_to_fct(Pc_1, Pa_1);
	curve_to_fct(Pc_2, Pa_2);
	curve_to_fct(Pc_3, Pa_3);
	curve_to_fct(Pc_4, p3);
}



/////////////  Cheap h2o hack   /////////////////////////////////////////////////////

function
getBeziers(PointMultiplier:vector_flt_t, int_points:Vector.<Point>):Vector.<Point> {
	var points:Vector.<Point> = new Vector.<Point>();
	points.push(int_points[0]);
	for(var j:vector_flt_t = 0; j < (int_points.length - 1)/3; j++) {
		points = points.concat(getBezierMidpoint(PointMultiplier, int_points[3 * j], int_points[3 * j + 1], int_points[3 * j + 2], int_points[3 * j + 3], points[points_length - 1]));
	}
	return points;
}


function
getBezierMidpoint(PointMultiplier:vector_flt_t, p0:Point, p1:Point, p2:Point, p3:Point, oldp3:Point):Vector.<Point> {
	// calculates the useful base points
	var PA:Point = getPointOnSegment(p0, p1, 3/4);
	var PB:Point = getPointOnSegment(p3, p2, 3/4);

	// get 1/16 of the [p3, p0] segment
	var dx:vector_flt_t = (p3.x - p0.x)/16;
	var dy:vector_flt_t = (p3.y - p0.y)/16;

	// calculates control point 1
	var Pc_1:Point = getPointOnSegment(p0, p1, 3/8);

	// calculates control point 2
	var Pc_2:Point = getPointOnSegment(PA, PB, 3/8);
	Pc_2.x -= dx;
	Pc_2.y -= dy;

	// calculates control point 3
	var Pc_3:Point = getPointOnSegment(PB, PA, 3/8);
	Pc_3.x += dx;
	Pc_3.y += dy;

	// calculates control point 4
	var Pc_4:Point = getPointOnSegment(p3, p2, 3/8);

	// calculates the 3 anchor points
	var Pa_1:Point = getMiddle(Pc_1, Pc_2);
	var Pa_2:Point = getMiddle(PA, PB);
	var Pa_3:Point = getMiddle(Pc_3, Pc_4);

	// draw the four quadratic subsegments
//	mc.curveTo(Pc_1.x, Pc_1.y, Pa_1.x, Pa_1.y);
//	mc.curveTo(Pc_2.x, Pc_2.y, Pa_2.x, Pa_2.y);
//	mc.curveTo(Pc_3.x, Pc_3.y, Pa_3.x, Pa_3.y);
//	mc.curveTo(Pc_4.x, Pc_4.y, p3.x, p3.y);

	var points:Vector.<Point> = new Vector.<Point>();
	// maybe needs an odd number, maybe not, what do i know
	points = points.concat(getMultiQuadBezier(PointMultiplier / 4, oldp3, Pc_1, Pa_1));
	points = points.concat(getMultiQuadBezier(PointMultiplier / 4,  Pa_1, Pc_2, Pa_2));
	points = points.concat(getMultiQuadBezier(PointMultiplier / 4,  Pa_2, Pc_3, Pa_3));
	points = points.concat(getMultiQuadBezier(PointMultiplier / 4,  Pa_3, Pc_4, p3  ));

	oldp3 = p3;
	return points;
}


function
getMultiQuadBezier(n:vector_flt_t, p0:Point, p1:Point, p2:Point):Vector.<Point> {
	var td:vector_flt_t = 1 / n;
	var t0:vector_flt_t = td;   // !0, >0

	var point:Vector.<Point> = new Vector.<Point>();
	for (var i:int = 0; i < n; i++) {
		point.push(getQuadBezier(t0, p0, p1, p2));
		t0 += td;
		//t0 = Math.round(t0 * 1000) / 1000;
	}
	return point;
}

function
getQuadBezier(t:vector_flt_t, p0:Point, p1:Point, p2:Point):Point {
	return new Point(
		Math.pow(1 - t, 2) * p0.x + 2 * t * (1 - t) * p1.x + Math.pow(t, 2) * p2.x,
		Math.pow(1 - t, 2) * p0.y + 2 * t * (1 - t) * p1.y + Math.pow(t, 2) * p2.y
	)
}


function
getMultiCubicBezier(n:vector_flt_t, p0:Point, p1:Point, p2:Point, p3:Point):Vector.<Point> {
	var td:vector_flt_t = 1 / (n + 1);
	var t0:vector_flt_t = td / 2;

	var point:Vector.<Point> = new Vector.<Point>();
	for (var i:int = 0; i < n; i++) {
		point.push(getQuadBezier(t0, p0, p1, p2));
		t0 += td;
	}
	return point;
}

function
getCubicBezier(t:vector_flt_t, p0:Point, p1:Point, p2:Point, p3:Point):Point {
	return new Point(
		(-p0.x + 3*p1.x-3*p2.x+p3.x)*Math.pow(t,3)+(3*p0.x-6*p1.x+3*p2.x)*Math.pow(t,2)+(-3*p0.x+3*p1.x)*t+p0.x,
		(-p0.y + 3*p1.y-3*p2.y+p3.y)*Math.pow(t,3)+(3*p0.y-6*p1.y+3*p2.y)*Math.pow(t,2)+(-3*p0.y+3*p1.y)*t+p0.y
	)
}


/// other H2O-functions

function
getAngleAndDistance(p1:Point, p2:Point):Object {
	var x:vector_flt_t = p2.x - p1.x;
	var y:vector_flt_t = p2.y - p1.y;

	var AnD:Object = new Object();
	AnD.beta = vector_flt_atan2(x, y);
//	AnD.beta = Math.atan(AnD.b / AnD.a);
//	// normalizing angles
//	if (AnD.a < 0)              { AnD.beta += Math.PI;     } else
//	if (AnD.a > 0 && AnD.b < 0) { AnD.beta += Math.PI * 2; }

	AnD.dist = Math.pow(Math.pow(x, 2) + Math.pow(y, 2), .5);

	return AnD;
}


// doesn't work

function
removePointsByAngleAndDistance(angle:vector_flt_t, distance:vector_flt_t, p:Vector.<Point>):Vector.<Point> {
	if (p.length < 3) { return null; }
	var np:Vector.<Point> = new Vector.<Point>();
	np.push(p[0]);
	var AnD:Object=getAngleAndDistance(p[0], p[1]);
	var ang:vector_flt_t = AnD.beta; var dist:vector_flt_t = AnD.dist;
	for (var i:vector_flt_t = 1; i < p.length; i++) {
		AnD = getAngleAndDistance(p[i-1], p[i]);
		dist += AnD.dist;
		if (vector_flt_abs(ang - AnD.beta) >= angle || dist >= distance) {
			np.push(p[i]);
			//dist = AnD.dist; ang = AnD.beta;
			dist = 0; ang = AnD.beta;
		}
	}
	if (p[i-1] != np[np.length - 1]) { np.push(p[i-1]); }
	return np;
}

function
removePointsByDistance(distance:vector_flt_t, p:Vector.<Point>):Vector.<Point> {
	if (p.length < 2) { return null; }
	var np:Vector.<Point> = new Vector.<Point>();
	np.push(p[0]);
	var dist:vector_flt_t = 0;
	for (var i:vector_flt_t = 1; i < p.length - 1; i++) {
		var AnD:Object=getAngleAndDistance(p[i], p[i + 1]);
		dist += AnD.dist;
		if (dist >= distance) {
			np.push(p[i]);
			dist = 0;
		}
	}
	return np;
}

function
useEachNthPoint(n:vector_flt_t, p:Vector.<Point>):Vector.<Point> {
	var np:Vector.<Point> = new Vector.<Point>();
	for (var i:vector_flt_t = 0; i < p.length; i++) {
		if (p.length / i == Math.floor(p.length / i)) {
			np.push(p[i]);
		}
	}
	return np;
}

//////////////////////////////////////////////////////////



point2d_t
getPointOnSegment(point2d_t p0, point2d_t p1, vector_flt_t ratio) {
	return (point2d_t) { p0.x + ((p1.x - p0.x) * ratio), p0.y + ((p1.y - p0.y) * ratio) };
}


point2d_t
getMiddle(point2d_t p0, point2d_t p1) {
	return (point2d_t) { (p0.x + p1.x) / 2, (p0.y + p1.y) / 2 };
}
*/

