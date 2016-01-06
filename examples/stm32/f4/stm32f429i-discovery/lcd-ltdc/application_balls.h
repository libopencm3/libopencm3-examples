#include <stdint.h>

typedef struct {
	float x,y,w,h;
	uint16_t color;
} plane_t;
typedef struct {
	float x,y,w,h,sh,sv;
	uint16_t color;
} ball_t;

plane_t draw_plane;
ball_t balls[10];

static inline
void
ball_create(ball_t *b, float x, float y, float w, float h, float sh, float sv, uint16_t color) {
	b->x=x;
	b->y=y;
	b->w=w;
	b->h=h;
	b->sh=sh;
	b->sv=sv;
	b->color=color;
}
static inline
bool
ball_hittest(ball_t *b1, ball_t *b2) {
	 return (((b1->x + b1->w >= b2->x) && (b1->x <= b2->x + b2->w)) && ((b1->y + b1->h >= b2->y) && (b1->y <= b2->y + b2->h)));
}
static inline
void
ball_move(ball_t *bs, size_t bs_len) {
	size_t l,ll;
	ball_t *b, *bsp;
	
	/* change x */
	b = bs;
	l = bs_len;
	while (l--) {
		b->x += b->sh;
		b++;
	}
	
	/* hittest x */
	bsp = bs;
	l   = bs_len;
	while (l--) {
		ball_t *bb;
		
		b = bsp++;
		if ((b->x < 0) || ((b->x+b->w) > draw_plane.w)) { // hittest borders
			b->sh = -b->sh;
			b->x += b->sh;
		} else {
			ll = l;
			bb = bsp;
			while (ll--) {
				if (ball_hittest(b,bb)) {
					float s;
					s       = b->sh - bb->sh;
					b->x   -= b->sh;
					bb->x  -= bb->sh;
					bb->sh += s;
					b->sh  -= s;
				}
				bb++;
			}
		}
	}


	/* change y */
	b = bs;
	l = bs_len;
	while (l--) {
		b->y += b->sv;
		b++;
	}
	
	/* hittest y */
	bsp = bs;
	l   = bs_len;
	while (l--) {
		ball_t *bb;
		
		b = bsp++;
		if ((b->y < 0) || ((b->y+b->h) > draw_plane.h)) { // hittest borders
			b->sv = -b->sv;
			b->y += b->sv;
		} else {
			ll = l;
			bb = bsp;
			while (ll--) {
				if (ball_hittest(b,bb)) {
					float s;
					s       = b->sv - bb->sv;
					b->y   -= b->sv;
					bb->y  -= bb->sv;
					bb->sv += s;
					b->sv  -= s;
				}
				bb++;
			}
		}
	}
}
static inline
void
ball_draw(ball_t *bs, size_t bs_len) {
	char buf[2] = " ";
	uint32_t b_id = 0;
	gfx_set_font_scale(2);
	while (bs_len--) {
		gfx_fill_round_rect(
			(int16_t)(draw_plane.x + bs->x),
			(int16_t)(draw_plane.y + bs->y),
			(int16_t)bs->w,
			(int16_t)bs->h,
			5,
			bs->color
		);
		buf[0]=48+b_id++%10;
		gfx_puts2(
			(int16_t)(draw_plane.x + bs->x + bs->w/2) - font_Tamsyn5x9b_9.charwidth/2*gfx_get_font_scale(),
			(int16_t)(draw_plane.y + bs->y + bs->h/2) - font_Tamsyn5x9b_9.lineheight/2*gfx_get_font_scale(),
			buf,
			&font_Tamsyn5x9b_9 ,
			GFX_COLOR_WHITE);
		bs++;
	}
}
