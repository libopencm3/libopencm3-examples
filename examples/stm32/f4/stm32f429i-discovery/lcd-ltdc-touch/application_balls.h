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
 * This code is based on http://vobarian.com/bouncescope/
 *
 */


#include <stdint.h>
#include <stdbool.h>

#include "vector_gfx/vector.h"

typedef struct {
	vector_flt_t x1, y1, x2, y2;
	uint16_t bg_color, fg_color;
} walls_t;
typedef struct {
	vector_flt_t radius, mass;
	point2d_t pos;
	point2d_t vel;
	uint16_t color;
} ball_t;
typedef struct {
	uint64_t time_ms_0;
	walls_t walls;
	ball_t *balls;
	size_t balls_size;
	size_t balls_count;
} balls_t;

static inline
void
ball_setup(
		balls_t *self,
		uint64_t time_ms_0,
		walls_t walls,
		ball_t balls[], size_t balls_size
) {
	self->time_ms_0 = time_ms_0;
	self->walls = walls;
	self->balls = balls;
	self->balls_size  = balls_size;
	self->balls_count = 0;
}

static inline
bool
ball_hittest(ball_t *b1, ball_t *b2) {
	return point2d_dist(b1->pos, b2->pos) < (b1->radius + b2->radius + 10);
}

static inline
void
ball_create(
		balls_t *self,
		vector_flt_t radius,
		vector_flt_t mass,
		point2d_t pos,
		point2d_t vel,
		uint16_t color
) {
	if (self->balls_count >= self->balls_size) {
		return;
	}

	ball_t ball;
	ball = (ball_t) {
		.radius = radius,
		.mass   = mass,
		.pos    = pos,
		.vel    = vel,
		.color  = color,
	};


	/* try to fit the ball.. */
	int16_t x = pos.x;
	int16_t y = pos.y;

	int16_t x_range = self->walls.x2 - self->walls.x1 - 2*radius;
	int16_t y_range = self->walls.y2 - self->walls.y1 - 2*radius;

	int16_t x_move = 0;
	int16_t y_move = 0;



	bool hit = true;
	while (hit) {
		/* fit ball into walls */
		ball.pos.x = (vector_flt_t)((x+x_move) % x_range);
		ball.pos.y = (vector_flt_t)((y+y_move) % y_range);

		ball.pos.x += self->walls.x1 + radius;
		ball.pos.y += self->walls.y1 + radius;

		size_t l  = self->balls_count;
		ball_t *b = self->balls;

		hit = false;
		while (l) {
			if ((ball_hittest(&ball, b))) {
				hit = true;
				if (x_move < x_range) {
					x_move += 1;
				} else
				if (y_move < y_range) {
					x_move =  0;
					y_move += 1;
				} else {
					/* failed to fit ball into walls! */
					hit = false;
				}
				break;
			}
			l--;
			b++;
		}
	}

	self->balls[self->balls_count++] = ball;
}


typedef enum {
	COLL__NONE,
	COLL__BALL,
	COLL__WALL_X1,
	COLL__WALL_X2,
	COLL__WALL_Y1,
	COLL__WALL_Y2
} collision_type_t;
typedef struct {
	collision_type_t type;
	vector_flt_t time_to;
} collision_t;

static inline
collision_t
ball_to_ball_collision_time(ball_t *b1, ball_t *b2) {
	point2d_t vdiff = point2d_sub_ts(b2->vel, b1->vel);
	point2d_t pdiff = point2d_sub_ts(b2->pos, b1->pos);
	vector_flt_t r = b1->radius + b2->radius;

	/* Compute parts of quadratic formula */
	/* a = (v2x - v1x) ^ 2 + (v2y - v1y) ^ 2 */
	vector_flt_t a = point2d_dot(vdiff, vdiff);
	/* b = 2 * ((x20 - x10) * (v2x - v1x) + (y20 - y10) * (v2y - v1y)) */
	vector_flt_t b = 2 * point2d_dot(vdiff, pdiff);
	/* c = (x20 - x10) ^ 2 + (y20 - y10) ^ 2 - (r1 + r2) ^ 2 */
	vector_flt_t c = point2d_dot(pdiff, pdiff) - r*r;

	/* Determinant = b^2 - 4ac */
	vector_flt_t det = b*b - 4 * a * c;

	if (a != 0.) { /* If a == 0 then v2x==v1x and v2y==v1y and
					* there will be no collision */
		/* Quadratic formula. t = time to collision */
		vector_flt_t t = (-b - vector_flt_sqrt(det)) / (2. * a);
		if (t >= 0.) {
			return (collision_t) {
				.type = COLL__BALL,
				.time_to = t
			};
		}
	}
	return (collision_t) { .type = COLL__NONE };
}

static inline
collision_t
ball_to_wall_collision_time(walls_t *walls, ball_t *b) {
	collision_t collision = { .type = COLL__NONE };

	/* Check for collision with wall X1 */
	if (b->vel.x < 0.) {
		vector_flt_t t = (b->radius - b->pos.x + walls->x1) / b->vel.x;
		/*if (t >= 0.) {*/ /* If t < 0 then ball is headed away
							* from wall, we ignore this case to
							* keep the ball inside the walls..
							*/
		collision = (collision_t) {
			.type = COLL__WALL_X1,
					.time_to = t
		};
		/*}*/
	}

	/* Check for collision with wall Y1 */
	if (b->vel.y < 0.) {
		vector_flt_t t = (b->radius - b->pos.y + walls->y1) / b->vel.y;
		/*if (t >= 0.) {*/
		if (collision.type == COLL__NONE || (t < collision.time_to)) {
			collision = (collision_t) {
				.type = COLL__WALL_Y1,
				.time_to = t
			};
		}
		/*}*/
	}

	/* Check for collision with wall X2 */
	if (b->vel.x > 0.) {
		vector_flt_t t = (walls->x2 - b->radius - b->pos.x) / b->vel.x;
		/*if (t >= 0.) {*/
		if (collision.type == COLL__NONE || (t < collision.time_to)) {
			collision = (collision_t) {
				.type = COLL__WALL_X2,
				.time_to = t
			};
		}
		/*}*/
	}

	/* Check for collision with wall Y2 */
	if (b->vel.y > 0.) {
		vector_flt_t t = (walls->y2 - b->radius - b->pos.y) / b->vel.y;
		/*if (t >= 0.) {*/
		if (collision.type == COLL__NONE || (t < collision.time_to)) {
			collision = (collision_t) {
				.type = COLL__WALL_Y2,
				.time_to = t
			};
		}
		/*}*/
	}

	return collision;
}

static inline
void
ball_to_ball_elastic_collision(ball_t *b1, ball_t *b2) {
	/* Avoid division by zero below in computing new normal velocities
	 * Doing a collision where both balls have no mass makes no sense anyway
	 */
	if (b1->mass == 0. && b2->mass == 0.) {
		return;
	}

	/* Compute unit normal and unit tangent vectors */

	/* normal vector - a vector normal to the collision surface */
	point2d_t v_n  = point2d_sub_ts(b2->pos, b1->pos);
	/* unit normal vector */
	point2d_t v_un = point2d_unit(v_n);
	/* unit tangent vector */
	point2d_t v_ut = { -v_un.y, v_un.x };

	/* Compute scalar projections of velocities onto v_un and v_ut */
	vector_flt_t v1n = point2d_dot(v_un, b1->vel); /* Dot product */
	vector_flt_t v1t = point2d_dot(v_ut, b1->vel);
	vector_flt_t v2n = point2d_dot(v_un, b2->vel);
	vector_flt_t v2t = point2d_dot(v_ut, b2->vel);

	/* Compute new tangential velocities */
	/* Note: in reality, the tangential velocities
	 *       do not change after the collision */
	vector_flt_t v1t_prime = v1t;
	vector_flt_t v2t_prime = v2t;

	/* Compute new normal velocities using one-dimensional elastic
	 * collision equations in the normal direction
	 *
	 * Division by zero avoided. See early return above.
	 */
	vector_flt_t v1n_prime =
			(v1n * (b1->mass - b2->mass) + 2. * b2->mass * v2n)
			/ (b1->mass + b2->mass);
	vector_flt_t v2n_prime =
			(v2n * (b2->mass - b1->mass) + 2. * b1->mass * v1n)
			/ (b1->mass + b2->mass);

	/* Compute new normal and tangential velocity vectors
	 * (Multiplication by a scalar)
	 */
	point2d_t v_v1nPrime = point2d_mul_t(v_un, v1n_prime);
	point2d_t v_v1tPrime = point2d_mul_t(v_ut, v1t_prime);
	point2d_t v_v2nPrime = point2d_mul_t(v_un, v2n_prime);
	point2d_t v_v2tPrime = point2d_mul_t(v_ut, v2t_prime);

	/* Set new velocities in x and y coordinates */
	b1->vel = (point2d_t) {
		v_v1nPrime.x + v_v1tPrime.x, v_v1nPrime.y + v_v1tPrime.y
	};
	b2->vel = (point2d_t) {
		v_v2nPrime.x + v_v2tPrime.x, v_v2nPrime.y + v_v2tPrime.y
	};
}

static inline
void
ball_to_wall_elastic_collision(ball_t *b, collision_type_t type) {
	switch (type) {
	case COLL__WALL_X1:
		b->vel.x =  vector_flt_abs(b->vel.x);
		break;
	case COLL__WALL_Y1:
		b->vel.y =  vector_flt_abs(b->vel.y);
		break;
	case COLL__WALL_X2:
		b->vel.x = -vector_flt_abs(b->vel.x);
		break;
	case COLL__WALL_Y2:
		b->vel.y = -vector_flt_abs(b->vel.y);
		break;

	default:
		/* invalid! */
		break;
	}
}

static inline
void
ball_advance_positions(ball_t *balls, size_t balls_count, vector_flt_t dt) {
	/* move all balls */
	while (balls_count) {
		balls->pos = point2d_add_ts(
				balls->pos, point2d_mul_t(balls->vel, dt)
			);
		balls_count--;
		balls++;
	}
}

static inline
collision_t
ball_find_earliest_collision(balls_t *self, ball_t **b1c, ball_t **b2c) {
	if (self->balls_count == 0) {
		return (collision_t) { .type = COLL__NONE };
	}


	/* Compare each pair of balls. Index i runs from the first
	 * ball up through the second-to-last ball. For each value of
	 * i, index j runs from the ball after i up through the last ball.
	 */

	collision_t earliest_collision = { .type = COLL__NONE };

	ball_t *b1, *b2;
	size_t l, ll;
	b1 = self->balls;
	l  = self->balls_count;
	while (1) {
		collision_t c;

		/* check for wall collisions */
		c = ball_to_wall_collision_time(&self->walls, b1);
		if (c.type != COLL__NONE) {
			if (
				(earliest_collision.type == COLL__NONE)
			 || (c.time_to < earliest_collision.time_to)
			) {
				earliest_collision = c;
				*b1c = b1;
				*b2c = NULL;
			}
		}

		l--;
		if (l == 0) {
			break;
		}


		/* check for ball to ball collisions */
		ll = l;
		b2 = b1+1;
		while (ll) {
			c = ball_to_ball_collision_time(b1, b2);
			if (c.type == COLL__BALL) {
				if (
					(earliest_collision.type == COLL__NONE)
				 || (c.time_to < earliest_collision.time_to)
				) {
					earliest_collision = c;
					*b1c = b1;
					*b2c = b2;
				}
			}
			b2++;
			ll--;
		}
		b1++;
	}

	return earliest_collision;
}

static inline
void
ball_move(balls_t *self, uint64_t time_ms) {
	vector_flt_t dt = (vector_flt_t)(time_ms-self->time_ms_0)/1000.;
	self->time_ms_0 = time_ms;

	collision_t last_collision = { .type = COLL__NONE };

	size_t max_collisions = 1000;
	while (--max_collisions) {
		/* Find earliest collision */
		ball_t *b1c = NULL, *b2c = NULL;
		collision_t c = ball_find_earliest_collision(self, &b1c, &b2c);

		/* If no collisions, break */
		if (c.time_to == COLL__NONE) {
			break;
		}

		/* Is collision within the time frame?
		 * Note: condition is tElapsed + timeToCollision strictly < dt,
		 *       not <=, because if the two were exactly equal, we would
		 *       perform the velocity adjustment for collision but not
		 *       move the balls any more, so the collision could be
		 *       detected again on the next call to advanceSim().
		 */
		if (c.time_to < dt) {
			/* Collision is within time frame
			 * Advance balls to point of collision
			 */
			ball_advance_positions(
					self->balls, self->balls_count,
					c.time_to);
			/* Collision is now occuring.
			 * Do collision calculation
			 */
			if (c.type == COLL__BALL) {
				ball_to_ball_elastic_collision(b1c, b2c);
			} else {
				ball_to_wall_elastic_collision(b1c, c.type);
			}
			/* Move time counter forward */
			dt -= c.time_to;
		} else {
			/* Break if collision is not within this frame */
			break;
		}

		/* Update lastCollision */
		last_collision = c;
	}

	(void)last_collision;

	/* Advance ball positions further if necessary after any collisions
	 * to complete the time frame
	 */
	ball_advance_positions(self->balls, self->balls_count, dt);
}

static inline
void
ball_draw_walls(balls_t *self) {

	/* draw walls and background */
	gfx_draw_rect(
			(int16_t)(self->walls.x1 - 1),
			(int16_t)(self->walls.y1 - 1),
			(int16_t)(self->walls.x2 - self->walls.x1 + 2),
			(int16_t)(self->walls.y2 - self->walls.y1 + 2),
			self->walls.fg_color
		);
	gfx_fill_rect(
			(int16_t)(self->walls.x1),
			(int16_t)(self->walls.y1),
			(int16_t)(self->walls.x2 - self->walls.x1),
			(int16_t)(self->walls.y2 - self->walls.y1),
			self->walls.bg_color
		);

}
static inline
void
ball_draw(balls_t *self) {
	/* draw balls */

	char buf[2] = " ";
	uint32_t b_id = 0;
	gfx_set_font_scale(1);

	ball_t *b = self->balls;
	size_t  l = self->balls_count;
	while (l--) {
		gfx_fill_circle(
				(int16_t)(b->pos.x),
				(int16_t)(b->pos.y),
				(int16_t)(b->radius),
				b->color
			);
		buf[0] = 48+b_id++%10;
		gfx_puts2(
				(int16_t)b->pos.x
				- font_Tamsyn5x9b_9.charwidth
				  * gfx_get_font_scale() / 2
				+ 1,
				(int16_t)b->pos.y
				- font_Tamsyn5x9b_9.lineheight
				  * gfx_get_font_scale() / 2,
				buf,
				&font_Tamsyn5x9b_9,
				GFX_COLOR_WHITE
			);
		b++;
	}
}
