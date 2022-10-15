/*************************************************
*                      ASPIC                     *
*************************************************/

/* Copyright (c) University of Cambridge 1991 - 2022 */
/* Created: February 1991 */
/* Last modified: October 2022 */

/* This module contains outputting functions that are not specific to the
output format. */


#include "aspic.h"




/*************************************************
*           Compare two colours                  *
*************************************************/

BOOL
samecolour(colour a, colour b)
{
return a.red == b.red && a.green == b.green && a.blue == b.blue;
}



/*************************************************
*         Find a line depth for a string         *
*************************************************/

/* The line depth is the greater of the value set in the item from the
environment when it was read, and the size of the font for the string.

Arguments:
  p         the item to which the string is attached
  s         the string

Returns:    the line depth
*/

int
find_linedepth(item *p, stringchain *s)
{
int fontsize = 12000;    /* default */
for (bindfont *b = font_base; b != NULL; b = b->next)
  {
  if (b->number == s->font)
    {
    fontsize = b->size;
    break;
    }
  }
return (fontsize > p->linedepth)? fontsize: p->linedepth;
}



/*************************************************
*         Find a font depth for a string         *
*************************************************/

/* The font depth is the greater of the value set in the item from the
environment when it was read, and half the size of the font for the string.

Arguments:
  p         the item to which the string is attached
  s         the string

Returns:    the font depth
*/

int
find_fontdepth(item *p, stringchain *s)
{
int fontdepth = 6000;    /* default */
for (bindfont *b = font_base; b != NULL; b = b->next)
  {
  if (b->number == s->font)
    {
    fontdepth = b->size/2;
    break;
    }
  }

return (fontdepth > p->fontdepth)? fontdepth : p->fontdepth;
}



/*************************************************
*          Determine the bounding box            *
*************************************************/

/* Internal function to update the overall bounding box from the bounding box
of an individual item.

Arguments:
  box        the box to updata
  x, y       bottom left of item box
  w, d       width and depth of item box

Returns:     nothing
*/

static void
setbbox(int *box, int x, int y, int w, int d)
{
int xx = x + w;
int yy = y + d;

if (x > xx) { int temp = x; x = xx; xx = temp; }
if (y > yy) { int temp = y; y = yy; yy = temp; }

if (x  < box[0]) box[0] = x;
if (y  < box[1]) box[1] = y;
if (xx > box[2]) box[2] = xx;
if (yy > box[3]) box[3] = yy;
}


/* This is the function that is called from outside.

Argument:   pointer to a vector of 4 units
Returns:    nothing
*/

void
find_bbox(int *box)
{
item *p;
int x, y;

box[0] = box[1] = INT_MAX;
box[2] = box[3] = INT_MIN;

/* Scan all the items, ignoring those that are invisible, unless they have
attached strings or a set shapefilled value. */

for (p = main_item_base; p != NULL; p = p->next)
  {
  if (p->style == is_invi &&
      p->strings == NULL &&
      samecolour(p->shapefilled, unfilled))
    continue;

  switch (p->type)
    {
    case i_arc:
      {
      item_arc *pp = (item_arc *)p;
      int bx, by, cx, cy;
      double a1 = pp->angle1;
      double a2 = pp->angle2;

      if (pp->cw) { double temp = a1; a1 = a2; a2 = temp; }

      /* Arrange that a1 <= a2 and 0 <= a2 <= 2*pi and -2*pi <= a1 <= 2*pi */

      while (a1 > a2) a2 += 2.0*pi;
      while (a2 > 2.0*pi) { a1 -= 2.0*pi; a2 -= 2.0*pi; }
      while (a2 < 0) { a1 += 2.0*pi; a2 += 2.0*pi; }

      bx = (int )((double)(pp->radius) * cos(a1));
      by = (int )((double)(pp->radius) * sin(a1));
      cx = (int )((double)(pp->radius) * cos(a2));
      cy = (int )((double)(pp->radius) * sin(a2));

      if (a1 < 0)   /* We know a2 must be > 0 */
	{
	if (bx < cx) cx = bx;
	bx = pp->radius;
	}

      if ((a1 < -pi) || (a1 < pi && a2 > pi))
	{
	if (cx > bx) bx = cx;
	cx = -(pp->radius);
	}

      if ((a1 < -1.5*pi) || (a1 < 0.5*pi && a2 > 0.5*pi))
	{
	if (by < cy) cy = by;
	by = pp->radius;
	}

      if ((a1 < -0.5*pi) || (a1 < 1.5*pi && a2 > 1.5*pi))
	{
	if (cy > by) by = cy;
	cy = -(pp->radius);
	}

      setbbox(box, pp->x + bx, pp->y + by, cx - bx, cy - by);
      }
    break;

    case i_curve:
      {
      item_curve *pp = (item_curve *)p;
      int th = pp->thickness;
      for (double t = 0.0; t <= 1.0; t += 0.1)
        {
        int x, y;
        find_curvepos(pp, t, &x, &y);
        if (x - th < box[0]) box[0] = x - th;
        if (y - th < box[1]) box[1] = y - th;
        if (x + th > box[2]) box[2] = x + th;
        if (y + th > box[3]) box[3] = y + th;
        }
      }
    break;

    case i_box:
      {
      item_box *pp = (item_box *)p;
      int bx = pp->x - pp->width/2 - pp->thickness/2;
      int by = pp->y - pp->depth/2 - pp->thickness/2;
      setbbox(box, bx, by, pp->width + pp->thickness,
        pp->depth + pp->thickness);
      }
    break;

    case i_line:
      {
      item_line *pp = (item_line *)p;
      int x = pp->x;
      int y = pp->y;
      int width = pp->width;
      int depth = pp->depth;
      int t = pp->thickness;

      if (width == 0) { x -= t/2; width = t; }             /* Vertical line */
      if (depth == 0) { y -= t/2; depth = t; }             /* Horizontal line */

      /* For horizontal or vertical arrows, we include the head because that is
      simple to do. In practice, the arrow head will rarely affect the bounding
      box. */

      if (pp->arrow_start || pp->arrow_end)
	{
	int ww = pp->arrow_y;
	if (width == 0) { x -= ww/2; width = ww; }
	else if (depth == 0) { y -= ww/2; depth = ww; }
	}

      setbbox(box, x, y, width, depth);
      }
    break;

    /* All strings are handled below, so there's nothing to do for i_text. */

    case i_text:
    break;
    }

  /* Make an attempt to allow for strings that might stick out of the boundary
  of the existing bbox. We don't know the actual width of strings. A reasonable
  guess is to use half the font size for each character width. This is
  over-generous for long lower-case Roman strings, and not enough for all upper
  case, but it's better than nothing. */

  if (p->strings != NULL)
    {
    stringchain *s = p->strings;
    stringpos(p, &x, &y);      /* Find the position for the strings */

    for (;;)
      {
      bindfont *f;
      int len = s->chcount;
      int depth = p->fontdepth;
      int bx, by, bw, bd;

      for (f = font_base; f != NULL; f = f->next)
        { if (f->number == s->font) break; }

      if (f != NULL)
        {
        len *= f->size/2;
        if (depth < f->size/2) depth = f->size/2;
        }
      /* LCOV_EXCL_START */
      else len *= 6000;   /* Unbound font (should not occur) */
      /* LCOV_EXCL_STOP */

      /* The "depth" value isn't enough to get the top of the bounding box
      totally clear of the text, so extend it a bit. Lower the bottom to allow
      for descenders. */

      bx = x - ((s->justify == just_centre)? len/2 :
                (s->justify == just_right)? len : 0);
      by = y - depth/2;
      bw = len;
      bd = 2*depth;

      /* Adjust for rotation if necessary. The centre of the rotation is at
      (x,y). */

      if (s->rotate != 0)
        {
        double rsin = sin(s->rrotate);
        double rcos = cos(s->rrotate);

        int nbx = (int)((double)(bx - x) * rcos - (double)(by - y) * rsin);
        int nby = (int)((double)(by - y) * rcos + (double)(bx - x) * rsin);

        int nbw = (int)((double)bd * rsin + (double)bw * rcos);
        int nbd = (int)((double)bw * rsin + (double)bd * rcos);

        bx = nbx - (int)((double)bd * rsin);
        by = nby;

        bw = nbw;
        bd = nbd;
        }

      /* Add to the overall bbox */

      setbbox(box, bx, by, bw, bd);

      /* Move on to the next string; if we are not done, move down by its
      depth, where "down" may be in any direction for a rotated string. */

      s = s->next;
      if (s == NULL) break;

      depth = find_linedepth(p, s);
      if (s->rotate == 0) y -= depth; else
        {
        y -= (int)((double)depth * cos(s->rrotate));
        x += (int)((double)depth * sin(s->rrotate));
        }
      }
    }
  }

/* Adjust the values if a frame is required */

if (drawbbox != NULL)
  {
  box[0] -= drawbboxoffset;
  box[1] -= drawbboxoffset;
  box[2] += drawbboxoffset;
  box[3] += drawbboxoffset;
  }
}



/*************************************************
*           Round dimension to resolution        *
*************************************************/

/* The resolution can be changed per file, and defaults differently for
different output formats.

Arguments:
  value       the dimension to be rounded

Returns:      the rounded dimension
*/

int
rnd(int value)
{
int sign = (value < 0)? (-1) : (+1);
div_t split = div(abs(value), resolution);
if (split.rem > resolution/2) split.quot++;
return split.quot * resolution * sign;
}



/*************************************************
*           Coordinate to fixed point string     *
*************************************************/

/* This function converts a dimension into a fixed-point string. A large
circular buffer is used so that several results can be in existence at once
(e.g. several values in a single fprintf() call).

Arguments:
 x             the dimension

Returns:       pointer to a string
*/

static uschar fixed_buffer[200];
static int   fixed_ptr = 0;

uschar *
fixed(int x)
{
uschar *p = fixed_buffer + fixed_ptr;
int n = 0;
if (x < 0) { *p = '-'; x = -x; n++; }
n = sprintf(CS(p + n), "%d", x/1000);
n = (int)(x%1000);
if (n != 0)
  {
  int m = (int)Ustrlen(p);
  m += sprintf(CS(p + m), ".%03d", n);
  while (p[m-1] == '0') p[--m] = 0;       /* Remove trailing zeroes */
  }
fixed_ptr += 20;
if (fixed_ptr >= 200) fixed_ptr = 0;
return p;
}



/*************************************************
*            Draw an elliptical arc                 *
*************************************************/

/* Called to draw an arc < 2.0*pi from current position. For anticlockwise
arcs, angle2 > angle2; and for clockwise arcs, angle2 < angle1. The function to
do the actual output is an argument.

Arguments:
  radius1       x "radius"
  radius2       y "radius"
  angle1        start angle
  angle2        end angle
  rbezier       callback function do do the output

Returns:        nothing
*/

void
smallarc(int radius1, int radius2, double angle1, double angle2,
  void (*rbezier)(int , int , int , int , int , int ))
{
double r1 = (double) radius1;
double r2 = (double) radius2;
double x0, x1, x2, x3, xp, xq;
double y0, y1, y2, y3, yp, yq;
double arp, arq;
double bx, cx, by, cy;

arp = 0.6667*angle1 + 0.3333*angle2;
arq = 0.3333*angle1 + 0.6667*angle2;

x0 = r1 * cos(angle1);
y0 = r2 * sin(angle1);

x3 = r1 * cos(angle2);
y3 = r2 * sin(angle2);

xp = r1 * cos(arp);
yp = r2 * sin(arp);

xq = r1 * cos(arq);
yq = r2 * sin(arq);

cx = (18.0*xp - 9.0*xq + 2.0*x3 - 11.0*x0)/2.0;
bx = (-45.0*xp + 36.0*xq - 9.0*x3 + 18.0*x0)/2.0;

x1 = x0 + cx/3.0;
x2 = x1 + (cx+bx)/3.0;

cy = (18.0*yp - 9.0*yq + 2.0*y3 - 11.0*y0)/2.0;
by = (-45.0*yp + 36.0*yq - 9.0*y3 + 18.0*y0)/2.0;

y1 = y0 + cy/3.0;
y2 = y1 + (cy+by)/3.0;

rbezier((int )(x1-x0), (int )(y1-y0), (int )(x2-x0), (int )(y2-y0), (int )(x3-x0), (int )(y3-y0));
}



/*************************************************
*        Find position for strings               *
*************************************************/

/* The position depends on the item type to which the strings are attached.

Arguments:
  p             the item
  xx            where to return the x coordinate
  yy            where to return the y coordinate
*/

void
stringpos(item *p, int *xx, int *yy)
{
stringchain *s, *ss;
int x = p->x;
int y = p->y;
int n = 0;
int i;

/* Find the number of strings */

for (s = p->strings; s != NULL; s = s->next) n++;

/* Find the "middle" string */

for (s = p->strings, i = 1; i < (n + 1)/2; s = s->next, i++);

/* Sort out the position for strings on a line */

if (p->type == i_line)
  {
  item_line *pp = (item_line *)p;
  x += pp->width/2;

  /* Horizontal line */

  if (pp->depth == 0)
    y += (n == 1)? 2000 : (find_linedepth(p, s)/2 - find_fontdepth(p, s)/2);

  /* Non-horizontal line */

  else
    {
    x += 3000;         /* Move a little bit away */
    y += pp->depth/2 - find_fontdepth(p, s)/2;
    if ((n & 1) == 0) y += find_linedepth(p, s)/2;
    }
  }

/* Sort out the position for strings on an arc */

else if (p->type == i_arc)
  {
  item_arc *arc = (item_arc *)p;
  double radius = (double)(arc->radius);
  double angle = (arc->angle1 + arc->angle2)/2.0;

  if (arc->angle1 > arc->angle2) angle += pi;
  if (arc->cw) angle += pi;

  x += (int )(radius * cos(angle)) + 6000;  /* Move a little bit away */
  y += (int )(radius * sin(angle));
  angle = fabs(angle);

  if (angle > 3.0*pi/8.0 && angle < 5.0*pi/8.0)
    y += find_fontdepth(p, s)/2 + 2000;
  if ((n & 1) != 0) y -= find_fontdepth(p, s)/2;
  }

/* Sort out the position for string on a curve. The default x,y fields contain
the midpoint of the base line. I haven't come up with any clever adjustments
that depend on the curve's shape. */

else if (p->type == i_curve)
  {
  /* Do nothing */
  }

/* Sort out the position for strings in a closed shape */

else
  {
  y -= find_fontdepth(p, s)/2;
  if ((n & 1) == 0) y += find_linedepth(p, s)/2;
  }

/* The y coordinate is now set for the "middle" string. If there are other
strings above it, we must move the y coordinate upwards. */

for (ss = p->strings; ss != s; ss = ss->next)
  y += find_linedepth(p, ss->next);

/* Return the coordinates */

*xx = x;
*yy = y;
}

/* End of write.c */
