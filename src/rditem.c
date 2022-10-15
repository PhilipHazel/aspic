/*************************************************
*                      ASPIC                     *
*************************************************/

/* Copyright (c) University of Cambridge 1991 - 2022 */
/* Created: February 1991 */
/* Last modified: October 2022 */

/* This module contains code for reading commands that define shapes. Each gets
added to the item list. */

#include "aspic.h"

/* Define some shorthand */

#define oo offsetof



/*************************************************
*            Tables defining optional args       *
*************************************************/

/* Those specific to arrows (as opposed to lines) come at the start here. */

static arg_item arrow_args[] = {
  { US"back",        opt_bool,  oo(item_line, arrow_start),
                                      oo(item_line, arrow_end) },
  { US"both",        opt_bool,  oo(item_line, arrow_start),   -1 },
  { US"filled",      opt_colgrey, oo(item_line, arrow_filled),  -1 },

  /* From here on they apply to both lines and arrows */

  { US"up",          opt_yline,  oo(item_line, depth), oo(item_line, width) },
  { US"down",        opt_ynline, oo(item_line, depth), oo(item_line, width) },
  { US"left",        opt_xnline, oo(item_line, width), oo(item_line, depth) },
  { US"right",       opt_xline,  oo(item_line, width), oo(item_line, depth) },
  { US"from",        opt_at,     oo(item_line,     x), oo(item_line,     y) },
  { US"to",          opt_at,     oo(item_line,  endx), oo(item_line,  endy) },
  { US"align",       opt_at,     oo(item_line,alignx), oo(item_line,aligny) },
  { US"dashed",      opt_bool,   oo(item_line, dash1),                   -1 },
  { US"thickness",   opt_dim,    oo(item_line, thickness),               -1 },
  { US"colour",      opt_colour, oo(item_line, colour),                  -1 },
  { US"grey",        opt_grey,   oo(item_line, colour),                  -1 },
  { US"shapefilled", opt_colgrey,oo(item_line, shapefilled),             -1 },
  { US"level",       opt_int,    oo(item_line, level),                   -1 },
  { US"", 0, -1, -1 }
};

static arg_item *line_args = arrow_args + 3;

/* The arguments for an ellipse are the same as for a box. */

static arg_item box_args[] = {
  { US"at",          opt_at,   oo(item_box,         x), oo(item_box, y)      },
  { US"join",        opt_join, oo(item_box, joinpoint),
                                                 oo(item_box, pointjoined) },
  { US"width",       opt_dim,  oo(item_box, width),                       -1 },
  { US"depth",       opt_dim,  oo(item_box, depth),                       -1 },
  { US"dashed",      opt_bool, oo(item_box, dash1),                       -1 },
  { US"filled",      opt_colgrey, oo(item_box, shapefilled),              -1 },
  { US"thickness",   opt_dim,  oo(item_box, thickness),                   -1 },
  { US"colour",      opt_colour, oo(item_box, colour),                    -1 },
  { US"grey",        opt_grey, oo(item_box, colour),                      -1 },
  { US"level",       opt_int,  oo(item_box, level),                       -1 },
  { US"", 0, -1, -1 }
};

static arg_item circle_args[] = {
  { US"at",          opt_at,   oo(item_box,         x), oo(item_box, y)      },
  { US"join",        opt_join, oo(item_box, joinpoint),
                                                 oo(item_box, pointjoined) },
  { US"radius",      opt_dim,  oo(item_box, width),                       -1 },
  { US"dashed",      opt_bool, oo(item_box, dash1),                       -1 },
  { US"thickness",   opt_dim,  oo(item_box, thickness),                   -1 },
  { US"colour",      opt_colour, oo(item_box, colour),                    -1 },
  { US"grey",        opt_grey, oo(item_box, colour),                      -1 },
  { US"filled",      opt_colgrey, oo(item_box, shapefilled),              -1 },
  { US"level",       opt_int,  oo(item_box, level),                       -1 },
  { US"", 0, -1, -1 }
};

/* Those specific to arc arrows come at the start here. */

static arg_item arcarrow_args[] = {
  { US"back",        opt_bool,  oo(item_arc, arrow_start),
                                oo(item_arc, arrow_end) },
  { US"both",        opt_bool,  oo(item_arc, arrow_start), -1 },
  { US"filled",      opt_colgrey,  oo(item_line, arrow_filled),  -1 },

  /* From here on they apply to both arcs and arcarrows */

  { US"from",        opt_at,   oo(item_arc, x0), oo(item_arc,       y0) },
  { US"to",          opt_at,   oo(item_arc, x1), oo(item_arc,       y1) },
  { US"clockwise",   opt_bool, oo(item_arc, cw),                     -1 },
  { US"radius",      opt_dim,  oo(item_arc, radius),                 -1 },
  { US"angle",       opt_angle,oo(item_arc, angle),                  -1 },
  { US"depth",       opt_dim,  oo(item_arc, depth),                  -1 },
  { US"via",         opt_at,   oo(item_arc, via_x), oo(item_arc, via_y) },
  { US"dashed",      opt_bool, oo(item_arc, dash1),                  -1 },
  { US"up",          opt_dir,  oo(item_arc, direction),           north },
  { US"down",        opt_dir,  oo(item_arc, direction),           south },
  { US"left",        opt_dir,  oo(item_arc, direction),            west },
  { US"right",       opt_dir,  oo(item_arc, direction),            east },
  { US"thickness",   opt_dim,  oo(item_arc, thickness),              -1 },
  { US"colour",      opt_colour, oo(item_arc, colour),               -1 },
  { US"grey",        opt_grey, oo(item_arc, colour),                 -1 },
  { US"shapefilled", opt_colgrey, oo(item_arc, shapefilled),         -1 },
  { US"level",       opt_int,  oo(item_arc, level),                  -1 },
  { US"", 0, -1, -1 }
};

static arg_item *arc_args = arcarrow_args + 3;

static arg_item curve_args[] = {
  { US"from",        opt_at,   oo(item_curve, x0), oo(item_curve,   y0) },
  { US"to",          opt_at,   oo(item_curve, x1), oo(item_curve,   y1) },
  { US"clockwise",   opt_bool, oo(item_curve, cw),                   -1 },
  { US"wavy",        opt_bool, oo(item_curve, wavy),                 -1 },
  { US"c1",          opt_at,   oo(item_curve, cx1), oo(item_curve, cy1) },
  { US"c2",          opt_at,   oo(item_curve, cx2), oo(item_curve, cy2) },
  { US"cs",          opt_at,   oo(item_curve, cxs), oo(item_curve, cys) },
  { US"dashed",      opt_bool, oo(item_curve, dash1),                -1 },
  { US"thickness",   opt_dim,  oo(item_curve, thickness),            -1 },
  { US"colour",      opt_colour, oo(item_curve, colour),             -1 },
  { US"grey",        opt_grey, oo(item_curve, colour),               -1 },
  { US"shapefilled", opt_colgrey, oo(item_curve, shapefilled),       -1 },
  { US"level",       opt_int,  oo(item_curve, level),                -1 },
  { US"", 0, -1, -1 }
};



/*************************************************
*        Find start for arc or curve             *
*************************************************/

/* Called for arcs or curves when no start is specified. Choose an appropriate
point on the previous item, if it exists. */

static void
find_arcurve_start(int *ax0, int *ay0, int direction)
{
if (baseitem == NULL)
  {
  *ax0 = 0;
  *ay0 = 0;
  }
else switch(baseitem->type)
  {
  case i_arc:
  *ax0 = ((item_arc *)baseitem)->x1;
  *ay0 = ((item_arc *)baseitem)->y1;
  break;

  case i_curve:
  *ax0 = ((item_curve *)baseitem)->x1;
  *ay0 = ((item_curve *)baseitem)->y1;
  break;

  case i_line:
  *ax0 = ((item_line *)baseitem)->x + ((item_line *)baseitem)->width;
  *ay0 = ((item_line *)baseitem)->y + ((item_line *)baseitem)->depth;
  break;

  case i_box:
    {
    item_box *box = (item_box *)baseitem;
    int width2 = box->width/2;
    int depth2 = box->depth/2;

    *ax0 = box->x;
    *ay0 = box->y;

    switch ((direction == unset_dirpos)? env->direction : direction)
      {
      case north:
      *ay0 += depth2;
      break;

      case south:
      *ay0 -= depth2;
      break;

      case east:
      *ax0 += width2;
      break;

      case west:
      *ax0 -= width2;
      break;
      }
    }
  break;
  }
}



/*************************************************
*              The ARC command                   *
*************************************************/

void
c_arc(void)
{
double arrowx = (double)env->arrow_x;
double cwangle = 0.0;
double radius;
double comp = 1.0;
int    icwsign = 1;
item_arc *arc = getstore(sizeof(item_arc));

arc->type = i_arc;
arc->style = item_arg1;
arc->linedepth = env->linedepth;
arc->fontdepth = env->fontdepth;
arc->thickness = env->linethickness;
arc->dash1 = 0;
arc->dash2 = 0;
arc->colour = env->linecolour;
arc->shapefilled = env->shapefilled;
arc->arrow_filled = env->arrowfilled;
arc->arrow_start = FALSE;
arc->arrow_end = item_arg2;
arc->arrow_x = env->arrow_x;
arc->arrow_y = env->arrow_y;
arc->cw = FALSE;
arc->next = NULL;
arc->strings = NULL;
arc->level = env->level;

arc->radius = arc->angle = UNSET;
arc->depth = arc->via_x = arc->via_y = UNSET;
arc->direction = unset_dirpos;
arc->x0 = arc->y0 = arc->x1 = arc->y1 = UNSET;

/* Read optional parameters and sort out dashing */

options((item *)arc, item_arg2? arcarrow_args : arc_args);
if (arc->dash1) { arc->dash1 = env->linedash1; arc->dash2 = env->linedash2; }
if (arc->level > max_level) max_level = arc->level;
if (arc->level < min_level) min_level = arc->level;

/* Convert angle to radians and adjust for clockwise */

if (arc->angle != UNSET) arc->angle2 = (double)(arc->angle) * pi / 180000.0;
if (arc->cw) { cwangle = pi; icwsign = -1; }

/* Now determine the centre and starting angle. If the starting point is
given without an end point, both a radius and an angle may be specified,
but a depth or a via point may not. */

if (arc->x0 != UNSET && arc->x1 == UNSET)
  {
  if (arc->depth != UNSET || arc->via_x != UNSET) error_moan(23);
  if (arc->angle == UNSET) arc->angle2 = 0.5*pi;
  if (arc->radius == UNSET) arc->radius = env->arcradius;
  radius = (double)arc->radius;

  switch ((arc->direction == unset_dirpos)? env->direction : arc->direction)
    {
    case north:
    arc->angle1 = cwangle;
    arc->x = arc->x0 - icwsign * arc->radius;
    arc->y = arc->y0;
    break;

    case south:
    arc->angle1 = cwangle - pi;
    arc->x = arc->x0 + icwsign * arc->radius;
    arc->y = arc->y0;
    break;

    case east:
    arc->angle1 = cwangle - 0.5*pi;
    arc->x = arc->x0;
    arc->y = arc->y0 + icwsign * arc->radius;
    break;

    case west:
    arc->angle1 = cwangle + 0.5*pi;
    arc->x = arc->x0;
    arc->y = arc->y0 - icwsign * arc->radius;
    break;
    }

  /* Convert from relative to absolute angle */

  arc->angle2 = arc->angle1 + (double)icwsign * arc->angle2;

  /* Compute the end point */

  arc->x1 = arc->x + (int )(radius * cos(arc->angle2));
  arc->y1 = arc->y + (int )(radius * sin(arc->angle2));
  }

/* If the end is specified, with or without a starting point, then EITHER
a radius OR an angle may be given, but not both. */

else if (arc->x1 != UNSET)
  {
  int centresign = 1;
  double xx, yy;
  double angle, len1, len2;

  /* If the start is unset, find it from the previous item */

  if (arc->x0 == UNSET)
    find_arcurve_start(&(arc->x0), &(arc->y0), arc->direction);

  /* We now have a start and a finish position. */

  xx = (double)((int )(arc->x1 - arc->x0));
  yy = (double)((int )(arc->y1 - arc->y0));

  angle = atan2(yy, xx);

  len1 = 0.5 * sqrt(xx*xx + yy*yy);

  /* If no constraints, set angle to 90 degrees */

  if (arc->angle == UNSET && arc->radius == UNSET &&
    arc->depth == UNSET && arc->via_x == UNSET)
    {
    arc->angle = 0;       /* just mark "set" */
    arc->angle2 = 0.5*pi;
    }

  /* Allow only one of the four constraints: angle, radius, via, or
  depth. Calculate the radius in other three cases. */

  /* If angle is supplied, we can calculate the radius directly. Complain
  if any others are given.*/

  if (arc->angle != UNSET)
    {
    if (arc->radius != UNSET || arc->depth != UNSET ||
      arc->via_x != UNSET) error_moan(19);
    radius = len1/sin(arc->angle2/2.0);
    if (arc->angle2 > pi) comp = -comp;
    }

  /* If the radius is given, just check that no others are */

  else if (arc->radius != UNSET)
    {
    if (arc->depth != UNSET || arc->via_x != UNSET) error_moan(19);
    radius = (double)(arc->radius);
    }

  /* Via or depth given */

  else
    {
    /* If via is given, compute the depth; otherwise depth is given
    directly. Not all possible positions are valid for via. */

    if (arc->via_x != UNSET)
      {
      double xxx = (double)( (int )(arc->via_x - arc->x0) );
      double yyy = (double)( (int )(arc->via_y - arc->y0) );
      double sss = sin(angle);
      double ccc = cos(angle);
      double zzz = xxx*sss - yyy*ccc;
      double ttt = xxx*ccc + yyy*sss;

      /* Error if depth given too */

      if (arc->depth != UNSET) error_moan(19);

      /* Check for an impossible via point; if found, set a valid
      depth so that things don't bomb. */

      if (fabs(zzz) < 0.001 || (arc->cw && zzz > 0.0) || (!arc->cw && zzz < 0.0))
        {
        error_moan(24);
        arc->depth = (int )len1;
        }

      /* OK to compute depth from via point */

      else
        {
        double beta = atan2(2.0*len1 - ttt, zzz) + atan2(ttt, zzz);
        /* For some reason gcc gives a cast warning if these two statements
        are combined into one. */
        double depth = fabs(len1/tan(0.5*beta));
        arc->depth = (int )depth;
        }
      }

    /* Depth known; compute the radius */

    radius = ((len1 * len1) + ((double)arc->depth * (double)arc->depth)) /
      (double)((int )(2 * arc->depth));

    /* If depth is greater than half the distance between the ends, we want
    to draw an arc with the centre on the other side of the joining line to
    the default. */

    if ((double)arc->depth > len1) centresign = -1;
    }

  /* Set the used radius */

  arc->radius = (int )(radius);

  /* Force minimum radius */

  if (len1 > radius) { radius = len1; arc->radius = (int )radius; }

  len2 = comp * sqrt(radius*radius - len1*len1);

  arc->x = (arc->x0 + arc->x1)/2 - centresign * icwsign * (int )(len2 * sin(angle));
  arc->y = (arc->y0 + arc->y1)/2 + centresign * icwsign * (int )(len2 * cos(angle));

  /* Now we have the centre, we can compute the angles. */

  arc->angle1 = atan2((double)((int )(arc->y0 - arc->y)), (double)((int )(arc->x0 - arc->x)));
  arc->angle2 = atan2((double)((int )(arc->y1 - arc->y)), (double)((int )(arc->x1 - arc->x)));
  }

/* If neither the start nor end point has been set, we position the
arc according to the previous item. It is permitted for the user to
supply both a radius and an angle, but not a depth or via item. */

else
  {
  if (arc->depth != UNSET || arc->via_x != UNSET) error_moan(23);
  if (arc->angle == UNSET) arc->angle2 = 0.5*pi;
  if (arc->radius == UNSET) arc->radius = env->arcradius;
  radius = (double)(arc->radius);

  /* If there is no previous item, put the centre of the arc at the origin. */

  if (baseitem == NULL)
    {
    arc->x = 0;
    arc->y = 0;

    switch(arc->direction)
      {
      case unset_dirpos:
      case north:
      arc->angle1 = cwangle;
      break;

      case south:
      arc->angle1 = cwangle - pi;
      break;

      case east:
      arc->angle1 = cwangle - 0.5*pi;
      break;

      case west:
      arc->angle1 = cwangle + 0.5*pi;
      break;
      }
    }

  else switch (baseitem->type)
    {
    case i_arc:
      {
      item_arc *lastarc = (item_arc *)baseitem;
      if (lastarc->cw) cwangle = (arc->cw)? 0.0 : pi;

      switch (arc->direction)
        {
	case unset_dirpos:

        /* Must find true angle of end of last arc; saved value may have been
        changed to accommodate an arrowhead. */

        arc->angle1 =
	  atan2((double)((int )(lastarc->y1 - lastarc->y)),
	    (double)((int )(lastarc->x1 - lastarc->x))) - cwangle;
        break;

        case north: arc->angle1 = cwangle; break;
        case south: arc->angle1 = cwangle - pi; break;
        case east:  arc->angle1 = cwangle - 0.5*pi; break;
        case west:  arc->angle1 = cwangle + 0.5*pi; break;
        }

      arc->x = lastarc->x1 - (int )(radius * cos(arc->angle1));
      arc->y = lastarc->y1 - (int )(radius * sin(arc->angle1));
      }
     break;

    case i_line:
      {
      double angle;
      item_line *line = (item_line *)baseitem;
      switch (arc->direction)
        {
        case north: angle = 0.5*pi;  break;
        case south: angle = -0.5*pi; break;
        case east:  angle = 0.0; break;
        case west:  angle = pi; break;
        default: angle = atan2((double)(line->depth), (double)(line->width));
        break;
        }
      arc->angle1 = cwangle - (0.5*pi - angle);
      arc->x = line->x + line->width - icwsign * (int)(radius * sin(angle));
      arc->y = line->y + line->depth + icwsign * (int)(radius * cos(angle));
      }
    break;

    case i_curve:
      {
      double angle;
      item_curve *curve = (item_curve *)baseitem;
      switch (arc->direction)
        {
        case north: angle = 0.5*pi;  break;
        case south: angle = -0.5*pi; break;
        case east:  angle = 0.0; break;
        case west:  angle = pi; break;
        default: angle = atan2((double)(curve->y1 - (curve->y0 + curve->cy2)),
          (double)(curve->x1 - (curve->x0 + curve->cx2)));
        break;
        }
      arc->angle1 = cwangle - (0.5*pi - angle);
      arc->x = curve->x1 - icwsign * (int)(radius * sin(angle));
      arc->y = curve->y1 + icwsign * (int)(radius * cos(angle));
      }
    break;

    case i_box:
      {
      item_box *box = (item_box *)baseitem;
      int width2 = box->width/2;
      int depth2 = box->depth/2;

      arc->x = box->x;
      arc->y = box->y;

      switch ((arc->direction == unset_dirpos)? env->direction : arc->direction)
        {
        case north:
        arc->angle1 = cwangle;
        arc->x -= icwsign * arc->radius;
        arc->y += depth2;
        break;

        case south:
        arc->angle1 = pi - cwangle;
        arc->x += icwsign * arc->radius;
        arc->y -= depth2;
        break;

        case east:
        arc->angle1 = cwangle - 0.5*pi;
        arc->x += width2;
        arc->y += icwsign * arc->radius;
        break;

        case west:
        arc->angle1 = cwangle + 0.5*pi;
        arc->x -= width2;
        arc->y -= icwsign * arc->radius;
        break;
        }
      }
    break;

    /* This should never occur because baseitem may only be one of the above
    types. */

    /* LCOV_EXCL_START */
    default:
    arc->angle1 = 0;
    arc->x = baseitem->x;
    arc->y = baseitem->y;
    break;
    /* LCOV_EXCL_STOP */
    }

  /* End angle is relative to the starting angle; convert to absolute */

  arc->angle2 = arc->angle1 + (double)icwsign * arc->angle2;

  /* Compute the actual start and end points */

  arc->x0 = arc->x + (int )(radius * cos(arc->angle1));
  arc->y0 = arc->y + (int )(radius * sin(arc->angle1));
  arc->x1 = arc->x + (int )(radius * cos(arc->angle2));
  arc->y1 = arc->y + (int )(radius * sin(arc->angle2));
  }

/* Adjust if arrow heads are around */

if (arc->arrow_start)
  arc->angle1 += (double)icwsign * 2.0 * asin(arrowx/(2.0*radius));

if (arc->arrow_end)
  arc->angle2 -= (double)icwsign * 2.0 * asin(arrowx/(2.0*radius));

/* Read any associated strings, then connect to chain, updating the last item
and sorting the label. */

readstringchain((item *)arc, just_left);
chain_label((item *)arc);
}



/*************************************************
*             The CURVE command                  *
*************************************************/

void
c_curve(void)
{
item_curve *curve = getstore(sizeof(item_curve));
double f, fm, h, w, angle, len, flen, ylen, dx, dy, cwsign;

curve->next = NULL;
curve->strings = NULL;

curve->type = i_curve;
curve->style = item_arg1;
curve->level = env->level;

curve->linedepth = env->linedepth;
curve->fontdepth = env->fontdepth;
curve->thickness = env->linethickness;
curve->dash1 = 0;
curve->dash2 = 0;
curve->colour = env->linecolour;
curve->shapefilled = env->shapefilled;

curve->cw = 0;
curve->wavy = 0;

curve->x0 = curve->y0 = curve->x1 = curve->y1 = UNSET;
curve->cx1 = curve->cy1 = curve->cx2 = curve->cy2 = 0;
curve->cxs = curve->cys = 0;

/* Read optional parameters and sort out dashing */

options((item *)curve, curve_args);
if (curve->dash1) { curve->dash1 = env->linedash1; curve->dash2 = env->linedash2; }
if (curve->level > max_level) max_level = curve->level;
if (curve->level < min_level) min_level = curve->level;

/* An end point must be specified. */

if (curve->x1 == UNSET)
  {
  error_moan(33);
  return;
  }

/* If the start is unset, find it from the previous item */

if (curve->x0 == UNSET)
  find_arcurve_start(&(curve->x0), &(curve->y0), unset_dirpos);

/* Set the general x, y values to the middle of the base line. */

curve->x = (curve->x0 + curve->x1)/2;
curve->y = (curve->y0 + curve->y1)/2;

/* Add any "cs" adjustment to the individual control point adjustments. */

curve->cx1 += curve->cxs;
curve->cy1 += curve->cys;
curve->cx2 += curve->cxs;
curve->cy2 += curve->cys;

/* Compute the positions of the control points relative to the start of the
curve and put them in the cx/cy fields. */

f = 0.25;    /* Default fraction of joining line for control points */
cwsign = (curve->cw)? -1.0 : +1.0;  /* Clockwise sign */

h = (double)(curve->y1 - curve->y0);
w = (double)(curve->x1 - curve->x0);
len = sqrt(h*h + w*w);

if (len < 0.001)
  {
  error_moan(34, len);
  return;
  }

/* For horizontal or vertical base lines, choose an appropriate angle in order
to get the correct signs for cos and sin. */

if (w == 0) angle = (h > 0)? pi/2.0 : (3.0*pi)/2;
else if (h == 0) angle = (w < 0)? pi : 0;
else angle = atan(h/w);

/* flen is the basic fraction of the baseline length; ylen defaults to the same
length but can be adjusted. */

flen = len * f;
ylen = flen + (double)(curve->cy1);

/* First control point; calculate fraction modified by x adjustment, then the
deltas and the actual position. */

fm = (flen + (double)(curve->cx1)) / len;
dx = ylen * sin(angle) * cwsign;
dy = ylen * cos(angle) * cwsign;

curve->cx1 = (int)(w * fm + dx);
curve->cy1 = (int)(h * fm - dy);

/* Second control point; adjust for wavy, adjust ylen, then as before. */

if (curve->wavy) cwsign = - cwsign;
ylen = flen + (double)(curve->cy2);

fm = (flen + (double)(curve->cx2)) / len;
dx = ylen * sin(angle) * cwsign;
dy = ylen * cos(angle) * cwsign;

curve->cx2 = (int)(w - w * fm + dx);
curve->cy2 = (int)(h - h * fm - dy);

/* Read any associated strings, then connect to chain, updating the last item
and sorting the label. */

readstringchain((item *)curve, just_left);
chain_label((item *)curve);
}



/*************************************************
*       The BOX, CIRCLE, and ELLIPSE commands    *
*************************************************/

/* This common subroutine is called with different arguments for each
variation on the theme.

Arguments:
  boxtype    box_box, box_circle, or box_ellipse
  args       allowed arguments for the type

Returns:     nothing
*/

static void
bce(int boxtype, arg_item *args)
{
int x_corner, y_corner;
int depth2, width2;
item_box *box = getstore(sizeof(item_box));

/* Initialize with default parameters */

box->type = i_box;
box->style = item_arg1;
box->dash1 = 0;
box->linedepth = env->linedepth;
box->fontdepth = env->fontdepth;
box->next = NULL;
box->strings = NULL;
box->boxtype = boxtype;
box->level = env->level;

switch (boxtype)
  {
  case box_box:
  box->width = env->boxwidth;
  box->depth = env->boxdepth;
  box->thickness = env->boxthickness;
  box->colour = env->boxcolour;
  box->shapefilled = env->boxfilled;
  break;

  case box_circle:
  box->width = env->cirradius;
  box->thickness = env->cirthickness;
  box->colour = env->circolour;
  box->shapefilled = env->cirfilled;
  break;

  case box_ellipse:
  box->width = env->ellwidth;
  box->depth = env->elldepth;
  box->thickness = env->ellthickness;
  box->colour = env->ellcolour;
  box->shapefilled = env->ellfilled;
  break;
  }

box->x = box->y = UNSET;
box->joinpoint = unset_dirpos;
box->pointjoined = FALSE;

/* Read optional parameters and compute 1/2 widths and distances to "corners". */

options((item *)box, args);
if (box->level > max_level) max_level = box->level;
if (box->level < min_level) min_level = box->level;

if (boxtype == box_circle) box->width = box->depth = 2 * box->width;

depth2 = box->depth/2;
width2 = box->width/2;

if (boxtype == box_box)
  {
  x_corner = width2;
  y_corner = depth2;
  }
else
  {
  x_corner = (int )((double)(width2)*cos(0.25*pi));
  y_corner = (int )((double)(depth2)*sin(0.25*pi));
  }

/* Sort out dashing parameters */

if (box->dash1) switch (boxtype)
  {
  case box_box:
  box->dash1 = env->boxdash1;
  box->dash2 = env->boxdash2;
  break;

  case box_circle:
  box->dash1 = env->cirdash1;
  box->dash2 = env->cirdash2;
  break;

  case box_ellipse:
  box->dash1 = env->elldash1;
  box->dash2 = env->elldash2;
  break;
  }

/* Set up default position if required */

if (box->x == UNSET)
  {
  if (baseitem == NULL)
    {
    box->x = 0;
    box->y = 0;
    }

  /* This box follows another box */

  else if (baseitem->type == i_box)
    {
    item_box *lastbox = (item_box *)baseitem;
    int lastwidth2 = lastbox->width/2;
    int lastdepth2 = lastbox->depth/2;
    int last_x_corner, last_y_corner;

    if (lastbox->boxtype == box_box)
      {
      last_x_corner = lastwidth2;
      last_y_corner = lastdepth2;
      }
    else
      {
      last_x_corner = (int )((double)(lastwidth2)*cos(0.25*pi));
      last_y_corner = (int )((double)(lastdepth2)*sin(0.25*pi));
      }

    if (box->joinpoint == unset_dirpos) switch (env->direction)
      {
      case north: box->joinpoint = south; break;
      case south: box->joinpoint = north; break;
      case east:  box->joinpoint = west;  break;
      case west:  box->joinpoint = east;  break;
      }

    switch(box->joinpoint)
      {
      case north:
      if (box->pointjoined)
        {
        box->x = joined_xx;
        box->y = joined_yy - y_corner;
        }
      else
        {
        box->x = lastbox->x;
        box->y = lastbox->y - lastdepth2 - depth2;
        }
      break;

      case northeast:
      if (box->pointjoined)
        {
        box->x = joined_xx - x_corner;
        box->y = joined_yy - y_corner;
        }
      else
        {
        box->x = lastbox->x - last_x_corner - x_corner;
        box->y = lastbox->y - last_y_corner - y_corner;
        }
      break;

      case east:
      if (box->pointjoined)
        {
        box->x = joined_xx - x_corner;
        box->y = joined_yy;
        }
      else
        {
        box->x = lastbox->x - lastwidth2 - width2;
        box->y = lastbox->y;
        }
      break;

      case southeast:
      if (box->pointjoined)
        {
        box->x = joined_xx - x_corner;
        box->y = joined_yy + y_corner;
        }
      else
        {
        box->x = lastbox->x - last_x_corner - x_corner;
        box->y = lastbox->y + last_y_corner + y_corner;
        }
      break;

      case south:
      if (box->pointjoined)
        {
        box->x = joined_xx;
        box->y = joined_yy + y_corner;
        }
      else
        {
        box->x = lastbox->x;
        box->y = lastbox->y + lastdepth2 + depth2;
        }
      break;

      case southwest:
      if (box->pointjoined)
        {
        box->x = joined_xx + x_corner;
        box->y = joined_yy + y_corner;
        }
      else
        {
        box->x = lastbox->x + last_x_corner + x_corner;
        box->y = lastbox->y + last_y_corner + y_corner;
        }
      break;

      case west:
      if (box->pointjoined)
        {
        box->x = joined_xx + x_corner;
        box->y = joined_yy;
        }
      else
        {
        box->x = lastbox->x + lastwidth2 + width2;
        box->y = lastbox->y;
        }
      break;

      case northwest:
      if (box->pointjoined)
        {
        box->x = joined_xx + x_corner;
        box->y = joined_yy - y_corner;
        }
      else
        {
        box->x = lastbox->x + last_x_corner + x_corner;
        box->y = lastbox->y - last_y_corner - y_corner;
        }
      break;

      case centre:
      if (box->pointjoined)
        {
        box->x = joined_xx;
        box->y = joined_yy;
        }
      else
        {
        box->x = lastbox->x;
        box->y = lastbox->y;
        }
      break;
      }
    }

  /* This box follows a line, arc, or curve */

  else
    {
    int xoffset = 0, yoffset = 0;
    int x, y, xx, yy;

    if (baseitem->type == i_arc)
      {
      item_arc *lastarc = (item_arc *)baseitem;
      double angle2 = lastarc->angle2;
      xx = lastarc->x1;
      yy = lastarc->y1;
      if (box->joinpoint == unset_dirpos)
        {
        if (angle2 > -pi/4.0 && angle2 <= pi/4.0) yoffset = depth2;
        else if (angle2 > pi/4.0 && angle2 <= 3.0*pi/4.0) xoffset = -width2;
        else if (angle2 > 3.0*pi/4.0 && angle2 <= 5.0*pi/4.0) yoffset = -depth2;
        else xoffset = width2;
        if (lastarc->cw) { xoffset = -xoffset; yoffset = -yoffset; }
        }
      }

    else if (baseitem->type == i_curve)
      {
      item_curve *lastcurve = (item_curve *)baseitem;
      xx = lastcurve->x1;
      yy = lastcurve->y1;
      }

    else  /* It must be a line */
      {
      item_line *lastline = (item_line *)baseitem;

      x = lastline->x;
      y = lastline->y;
      xx = x + lastline->width;
      yy = y + lastline->depth;

      if (box->joinpoint == unset_dirpos)
        {
	if (abs(xx - x) > abs(yy - y))
          {
          xoffset = width2;
          if (xx < x) xoffset = -xoffset;
          }
        else
          {
          yoffset = depth2;
          if (yy < y) yoffset = -yoffset;
          }
        }
      }

    /* Common to all */

    switch (box->joinpoint)
      {
      case north:
      yoffset = -depth2;
      break;

      case northeast:
      xoffset = -x_corner;
      yoffset = -y_corner;
      break;

      case east:
      xoffset = -width2;
      break;

      case southeast:
      xoffset = -x_corner;
      yoffset = +y_corner;
      break;

      case south:
      yoffset = +depth2;
      break;

      case southwest:
      xoffset = +x_corner;
      yoffset = +y_corner;
      break;

      case west:
      xoffset = +width2;
      break;

      case northwest:
      xoffset = +x_corner;
      yoffset = -y_corner;
      break;

      case centre:
      break;
      }

    if (box->pointjoined)
      {
      box->x = joined_xx + xoffset;
      box->y = joined_yy + yoffset;
      }
    else
      {
      box->x = xx + xoffset;
      box->y = yy + yoffset;
      }
    }
  }

/* Read any associated strings, then connect to chain, updating the last item
and sorting the label. */

readstringchain((item *)box, just_centre);
chain_label((item *)box);
}


/* Individual entries to bce() */

void
c_box(void)
{
bce(box_box, box_args);
}


void
c_circle(void)
{
if (item_arg2)
  bce(box_circle, circle_args);
else
  bce(box_ellipse, box_args);
}



/*************************************************
*               The LINE and ARROW commands      *
*************************************************/

void
c_line(void)
{
item_arc  *lastarc;
item_box  *lastbox;
item_curve *lastcurve;
item_line *lastline;
item_line *line = getstore(sizeof(item_line));

/* Initialize with default parameters */

line->type = i_line;
line->style = item_arg1;
line->linedepth = env->linedepth;
line->fontdepth = env->fontdepth;
line->next = NULL;
line->strings = NULL;
line->dash1 = 0;
line->dash2 = 0;
line->thickness = env->linethickness;
line->colour = env->linecolour;
line->shapefilled = env->shapefilled;
line->arrow_filled = env->arrowfilled;
line->arrow_start = FALSE;
line->arrow_end = item_arg2;
line->arrow_x = env->arrow_x;
line->arrow_y = env->arrow_y;
line->level = env->level;

/* Mark various values as "unset" */

line->width = line->depth = line->x = line->y = UNSET;
line->endx = line->endy = line->alignx = line->aligny = UNSET;

/* Read optional parameters and sort out dashing */

options((item *)line, item_arg2? arrow_args : line_args);
if (line->dash1) { line->dash1 = env->linedash1; line->dash2 = env->linedash2; }
if (line->level > max_level) max_level = line->level;
if (line->level < min_level) min_level = line->level;

/* If no end point is given set the default size if required; if any option was
encountered, both dimensions will have been set, so we test only one. */

if (line->endx == UNSET)
  {
  if (line->width == UNSET) switch (env->direction)
    {
    case north:
    line->width = 0;
    line->depth = env->line_vd;
    break;

    case south:
    line->width = 0;
    line->depth = -(env->line_vd);
    break;

    case east:
    line->width = env->line_hw;
    line->depth = 0;
    break;

    case west:
    line->width = -(env->line_hw);
    line->depth = 0;
    break;
    }
  }

/* Only one of an endpoint or a width/depth may be specified. When an endpoint
is given, width/depth are set later once the start point is known. */

else if (line->width != UNSET || line->depth != UNSET) error_moan(38);

/* Set up default starting position if required. Again, both coordinates
will have been set by any positioning option. */

if (line->x == UNSET)
  {
  int quadrant;

  /* If this is the first item, start it at the origin */

  if (baseitem == NULL)
    {
    line->x = 0;
    line->y = 0;
    }

  /* Otherwise the start depends on the previous item */

  else switch(baseitem->type)
    {
    case i_arc:
    lastarc = (item_arc *)baseitem;
    line->x = lastarc->x1;
    line->y = lastarc->y1;
    break;

    case i_curve:
    lastcurve = (item_curve *)baseitem;
    line->x = lastcurve->x1;
    line->y = lastcurve->y1;
    break;

    case i_box:
    lastbox = (item_box *)baseitem;

    if (line->width == UNSET || line->depth == UNSET)
      quadrant = env->direction;
    else if (abs(line->depth) < abs(line->width))
      quadrant = (line->width > 0)? east : west;
    else quadrant = (line->depth > 0)? north : south;

    switch (quadrant)
      {
      case north:
      line->x = lastbox->x;
      line->y = lastbox->y + lastbox->depth/2;
      break;
      case south:
      line->x = lastbox->x;
      line->y = lastbox->y - lastbox->depth/2;
      break;
      case east:
      line->x = lastbox->x + lastbox->width/2;
      line->y = lastbox->y;
      break;
      case west:
      line->x = lastbox->x - lastbox->width/2;
      line->y = lastbox->y;
      break;
      }
    break;

    case i_line:
    lastline = (item_line *)baseitem;
    line->x = lastline->x + lastline->width;
    line->y = lastline->y + lastline->depth;
    break;
    }
  }

/* If an absolute position for the end point was set, we can now use it to set
the width and depth. This cannot be done earlier, because the starting point
may only just have been set. */

if (line->endx != UNSET)
  {
  line->width = line->endx - line->x;
  line->depth = line->endy - line->y;
  }

/* If an alignment for the end point was set, use the x or y coordinate
according as we have a vertical or horizontal line. */

if (line->alignx != UNSET)
  {
  if (line->width == 0) line->depth = line->aligny - line->y;
    else if (line->depth == 0) line->width = line->alignx - line->x;
      else error_moan(42);
  }

/* Read any associated strings, then connect to chain, updating the last item
and sorting the label. */

readstringchain((item *)line, (line->depth == 0)? just_centre : just_left);
chain_label((item *)line);
}

/* End of rditem.c */
