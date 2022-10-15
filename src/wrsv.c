/*************************************************
*                      ASPIC                     *
*************************************************/

/* Copyright (c) University of Cambridge 1991 - 2022 */
/* Created: February 1991 */
/* Last modified: October 2022 */


/* This module generates output in SVG (Scalar Vector Graphics) format. */


#include "aspic.h"



/*************************************************
*               Local variables                  *
*************************************************/

static int  bbox[4];

static colour line_fill_colour;
static colour stroke_colour;

static int  stroke_thickness;
static int  stroke_dash1;
static int  stroke_dash2;

static int  at_x;
static int  at_y;

static BOOL fillpending;
static BOOL strokepending;

static item *pathstart;



/*************************************************
*          Local initialization                  *
*************************************************/

void
init_sv(void)
{
bindfont *f = getstore(sizeof(bindfont) + Ustrlen("Times-Roman"));

f->next = font_base;
font_base = f;
f->number = 0;
f->size = 12000;
f->needSymbol = f->needDingbats = FALSE;
Ustrcpy(f->name, "Times-Roman");

minimum_thickness = 200;    /* So that zero does something */
}



/*************************************************
*             Drawing functions                  *
*************************************************/

/* This is an absolute move. It is called only at the start of a general path
that is not a close shape. */

static
void move(int x, int y)
{
x = x - bbox[0];
y = y - bbox[1];
fprintf(out_file, "<path d=\"M %s %s\n", fixed(rnd(x)), fixed(rnd(-y)));
}


/* Relative line */

static void
rline(int x, int y)
{
fprintf(out_file, "l %s %s\n", fixed(rnd(x)), fixed(rnd(-y)));
}


/* Relative bezier */

static void
rbezier(int x1, int y1, int x2, int y2, int x3, int y3)
{
fprintf(out_file, "c %s %s %s %s %s %s\n",
  fixed(rnd(x1)), fixed(rnd(-y1)),
  fixed(rnd(x2)), fixed(rnd(-y2)),
  fixed(rnd(x3)), fixed(rnd(-y3)));
}



/*************************************************
*       Set fill and stroke parameters           *
*************************************************/

/*
Arguments:
  fill        buffer into which to write the fill data
  fc          the fill colour
  stroke      buffer into which to write the stroke data
  sp          TRUE if stroke wanted
  sc          stroke colour
  lw          linewidth
  d1, d2      dash parameters

Returns:      nothing
*/

static void
sort_fill_stroke(uschar *fill, colour fc, uschar *stroke, BOOL sp, colour sc,
  int lw, int d1, int d2)
{
if (lw < minimum_thickness) lw = minimum_thickness;

if (samecolour(fc, unfilled)) Ustrcpy(fill, "\"none\"");
  else sprintf(CS fill, "\"#%02X%02X%02X\"",
    (fc.red   * 255)/1000,
    (fc.green * 255)/1000,
    (fc.blue  * 255)/1000);

if (!sp) Ustrcpy(stroke, "\"none\""); else
  {
  sprintf(CS stroke, "\"#%02X%02X%02X\" stroke-width=\"%s\"",
    (sc.red   * 255)/1000,
    (sc.green * 255)/1000,
    (sc.blue  * 255)/1000,
    fixed(lw));

  if (d1 != 0)
    {
    uschar *p = stroke;
    while (*p != 0) p++;
    sprintf(CS p, " stroke-dasharray=\"%s,%s\"", fixed(d1), fixed(d2));
    }
  }
}




/*************************************************
*             Process strings                    *
*************************************************/

/* Write out all the string items attached to a given item */

static void
write_strings(item *p)
{
int  x, y;
stringchain *s = p->strings;

if (s == NULL) return;  /* There are no strings */

stringpos(p, &x, &y);      /* Find the right position for the strings */

for (;;)
  {
  int depth;

  if (s->text[0] != 0)
    {
    uschar *ss = s->text;
    uschar *fx = fixed(rnd(x - bbox[0] + s->xadjust));
    uschar *fy = fixed(rnd(-y + bbox[1] - s->yadjust));
    uschar fill[12];
    uschar stroke[128];

    fprintf(out_file, "<text x=\"%s\" y=\"%s\"", fx, fy);

    if (s->rotate != 0)
      fprintf(out_file, " transform=\"rotate(%s,%s,%s)\"", fixed(-s->rotate),
        fx, fy);

    fprintf(out_file, " text-anchor=\"%s\"",
      (s->justify == just_left)? US"start" :
      (s->justify == just_right)? US"end" : US"middle");

    if (s->rgb.red != 0 || s->rgb.green != 0 || s->rgb.blue != 0)
      {
      sort_fill_stroke(fill, s->rgb, stroke, FALSE, s->rgb, 0, 0, 0);
      fprintf(out_file, " fill=%s", fill);
      }

    if (s->font != 0)
      {
      bindfont *b;

      for (b = font_base; b != NULL; b = b->next)
        {
        if (b->number == s->font)
          {
          uschar *hyphen = Ustrchr(b->name, '-');
          uschar *family;
          uschar *weight = NULL;
          uschar *style = NULL;
          uschar fambuff[64];

          if (hyphen == NULL)
            {
            family = b->name;
            }
          else
            {
            Ustrncpy(fambuff, b->name, hyphen - b->name);
            fambuff[hyphen - b->name] = 0;
            family = fambuff;

            if (Ustrcmp(hyphen+1, "Italic") == 0 ||
                Ustrcmp(hyphen+1, "BoldItalic") == 0)
              style = US"italic";

            if (Ustrcmp(hyphen+1, "Bold") == 0 ||
                Ustrcmp(hyphen+1, "BoldItalic") == 0)
              weight = US"bold";
            }

          fprintf(out_file, " font-family=\"%s\" font-size=\"%s\"",
            family, fixed(b->size));
          if (weight != NULL)
            fprintf(out_file, " font-weight=\"%s\"", weight);
          if (style != NULL)
            fprintf(out_file, " font-style=\"%s\"", style);
          break;
          }
        }
      }

    fprintf(out_file, ">");
    while (*ss != 0)
      {
      int c;
      GETCHARINC(c, ss);
      if (c == '<') fprintf(out_file, "&lt;");
      else if (c == '>') fprintf(out_file, "&gt;");
      else if (c == '&') fprintf(out_file, "&amp;");
      else if (c < 127) fputc(c, out_file);
      else fprintf(out_file, "&#x%x;", c);
      }
    fprintf(out_file, "</text>\n");
    }

  /* Move on to the next string; if we are not done, move down by its depth,
  where "down" may be in any direction for a rotated string. */

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



/*************************************************
*          End line stroking/filling             *
*************************************************/

/* This function is called when any existing path should either be filled or
stroked or both. Afterwards, output any texts that are associated with the
lines of the path, from its start to the current item.

Arguments:
  current     the current item, or NULL if we're at the end

Returns:      nothing
*/

static void
end_line_fillstroke(item *current)
{
uschar fill[12];
uschar stroke[128];

if (!samecolour(line_fill_colour, unfilled) || strokepending)
  {
  sort_fill_stroke(fill, line_fill_colour, stroke, strokepending,
    stroke_colour, stroke_thickness, stroke_dash1, stroke_dash2);
  fprintf(out_file, "\" fill=%s stroke=%s/>\n", fill, stroke);
  }

while (pathstart != NULL && pathstart != current)
  {
  write_strings(pathstart);
  pathstart = pathstart->next;
  }

line_fill_colour = unfilled;
strokepending = fillpending = FALSE;
pathstart = NULL;
}



/*************************************************
*             Draw an arrow head                 *
*************************************************/

static void
arrowhead(int x, int y, int xx, int yy, double angle, colour filled)
{
uschar fill[32];
double s = sin(angle);
double c = cos(angle);

int x1 = (int )((double)yy*s*0.5);
int y1 = (int )((double)yy*c*0.5);
int x2 = (int )((double)xx*c);
int y2 = (int )((double)xx*s);

fprintf(out_file, "<path d=\"M %s %s\n", fixed(x - bbox[0]),
  fixed(-y + bbox[1]));

rline(x1, -y1);
rline(x2 - x1, y2 + y1);
rline(-x2 -x1, y1 - y2);
rline(x1, -y1);

if (samecolour(filled, unfilled)) Ustrcpy(fill, "\"none\"");
  else sprintf(CS fill, "\"#%02X%02X%02X\"",
    (filled.red   * 255)/1000,
    (filled.green * 255)/1000,
    (filled.blue  * 255)/1000);

fprintf(out_file,
  "\" fill=%s stroke=\"#000000\" stroke-width=\"0.4\"/>\n", fill);
}



/*************************************************
*            Draw an elliptical arc              *
*************************************************/

static void arc(int clockwise, int x, int y, int radius1, int radius2,
  double angle1, double angle2)
{
if (!clockwise)
  {
  while (angle1 > angle2) angle2 += 2.0*pi;
  while (angle2 - angle1 > 0.5*pi)
    {
    smallarc(radius1, radius2, angle1, angle1 + 0.49*pi, rbezier);
    angle1 += 0.49*pi;
    }
  }

else
  {
  while (angle1 < angle2) angle2 -= 2.0*pi;
  while (angle1 - angle2 > 0.5*pi)
    {
    smallarc(radius1, radius2, angle1, angle1 - 0.49*pi, rbezier);
    angle1 -= 0.49*pi;
    }
  }

smallarc(radius1, radius2, angle1, angle2, rbezier);

at_x = x + (int )((double)radius1 * cos(angle2));
at_y = y + (int )((double)radius2 * sin(angle2));
}



/*************************************************
*               Process an arc                   *
*************************************************/

/*
Arguments:
  p               the arc item
  move_needed     TRUE if move() needed
  startx          where to move to
  starty

Returns:          nothing
*/

static void
write_arc(item_arc *p, BOOL move_needed, int startx, int starty)
{
double radius = (double)p->radius;
double angle1 = p->angle1;
double angle2 = p->angle2;

if (fillpending)
  {
  if (move_needed) move(startx, starty);
  arc(p->cw, p->x, p->y, p->radius, p->radius, angle1, angle2);
  return;
  }

/* Nothing to do if invisible, except write the strings. */

if (p->style == is_invi)
  {
  write_strings((item *)p);
  return;
  }

/* Draw the arc */

if (move_needed) move(startx, starty);
arc(p->cw, p->x, p->y, p->radius, p->radius, angle1, angle2);

/* Draw the arrow heads as necessary; first ensure the path is drawn and texts
upto and including this arc are output. */

if (p->arrow_start || p->arrow_end)
  {
  double tilt = asin((double)(p->arrow_x) / (2.0*radius));

  end_line_fillstroke((item *)p->next);

  if (p->arrow_start)
    {
    double angle = (p->cw)? (angle1 + pi/2.0 + tilt) : (angle1 - pi/2.0 - tilt);
    arrowhead(p->x + (int )(radius * cos(angle1)), p->y + (int )(radius * sin(angle1)),
      p->arrow_x, p->arrow_y, angle, p->arrow_filled);
    }

  if (p->arrow_end)
    {
    double angle = (p->cw)? (angle2 - pi/2.0 - tilt) : (angle2 + pi/2.0 + tilt);
    arrowhead(p->x + (int )(radius * cos(angle2)), p->y + (int )(radius * sin(angle2)),
      p->arrow_x, p->arrow_y, angle, p->arrow_filled);
    }
  }
}



/*************************************************
*               Process a curve                  *
*************************************************/

/*
Arguments:
  p               the arc item
  move_needed     TRUE if move() needed

Returns:          nothing
*/

static void
write_curve(item_curve *p, BOOL move_needed)
{
at_x = p->x1;
at_y = p->y1;

if (fillpending)
  {
  if (move_needed) move(p->x0, p->y0);
  rbezier(p->cx1, p->cy1, p->cx2, p->cy2, p->x1 - p->x0, p->y1 - p->y0);
  return;
  }

/* Nothing to do if invisible, except write the strings. */

if (p->style == is_invi)
  {
  write_strings((item *)p);
  return;
  }

/* Draw the curve */

if (move_needed) move(p->x0, p->y0);
rbezier(p->cx1, p->cy1, p->cx2, p->cy2, p->x1 - p->x0, p->y1 - p->y0);
}



/*************************************************
*              Process a box                     *
*************************************************/

/* Note that box items are also used for circles and ellipses. */

static void
write_box(item_box *p)
{
int x = p->x - bbox[0];
int y = p->y - bbox[1];
uschar fill[12];
uschar stroke[128];

sort_fill_stroke(fill, p->shapefilled, stroke, p->style != is_invi, p->colour,
  p->thickness, p->dash1, p->dash2);

if (p->boxtype == box_box)
  fprintf(out_file, "<rect x=\"%s\" y=\"%s\" width=\"%s\" height=\"%s\" "
    "fill=%s stroke=%s/>\n",
    fixed(rnd(x - p->width/2)),
    fixed(rnd(-y - p->depth/2)),
    fixed(p->width),
    fixed(p->depth),
    fill,
    stroke);

else if (p->boxtype == box_circle)
  fprintf(out_file, "<circle cx=\"%s\" cy=\"%s\" r=\"%s\" "
    "fill=%s stroke=%s/>\n",
    fixed(rnd(x)),
    fixed(rnd(-y)),
    fixed(p->width/2),
    fill,
    stroke);

else
  fprintf(out_file, "<ellipse cx=\"%s\" cy=\"%s\" rx=\"%s\" ry=\"%s\" "
    "fill=%s stroke=%s/>\n",
    fixed(rnd(x)),
    fixed(rnd(-y)),
    fixed(p->width/2),
    fixed(p->depth/2),
    fill,
    stroke);

write_strings((item *)p);
}



/*************************************************
*               Process a line                   *
*************************************************/

static void
write_line(item_line *p, BOOL move_needed)
{
double angle = 0.0;
int x1 = p->x, y1 = p->y;
int xw = p->width, yd = p->depth;
int xx = 0, yy = 0;

/* Filling: generate the line even if it is invisible; no arrow can be
involved. */

if (fillpending)
  {
  if (move_needed) move(x1, y1);
  rline(xw, yd);
  at_x = x1 + xw;
  at_y = y1 + yd;
  return;
  }

/* Not filling; no need to do anything for an invisible line, except write the
strings. */

if (p->style == is_invi)
  {
  write_strings((item *)p);
  return;
  }

/* If this is an arrow, compute data for arrow heads. */

if (p->arrow_start || p->arrow_end)
  {
  angle = atan2((double)p->depth, (double)p->width);
  xx = (int )(((double)p->arrow_x) * cos(angle));
  yy = (int )(((double)p->arrow_x) * sin(angle));
  }

/* Adjust the line according to the arrow heads. */

if (p->arrow_start)
  {
  x1 += xx;
  y1 += yy;
  xw -= xx;
  yd -= yy;
  }

if (p->arrow_end)
  {
  xw -= xx;
  yd -= yy;
  }

/* Draw the line before any arrow heads so that a forward arrow joined onto a
previous line gets the benefit of appropriate corner processing. */

if (move_needed) move(x1, y1);
rline(xw, yd);
at_x = x1 + xw;
at_y = y1 + yd;

/* Now draw the arrow heads if required; ensure that this line's texts
are output. */

if (p->arrow_start)
  {
  end_line_fillstroke((item *)p->next);
  arrowhead(x1, y1, p->arrow_x, p->arrow_y, angle + pi, p->arrow_filled);
  }

if (p->arrow_end)
  {
  end_line_fillstroke((item *)p->next);
  arrowhead(x1 + xw, y1 + yd, p->arrow_x, p->arrow_y, angle, p->arrow_filled);
  }
}



/*************************************************
*            Write SVG output file               *
*************************************************/

void
write_sv(void)
{
tree_node *tnc, *tnd;
int level;
int bboxthick = (drawbbox == NULL)? 0 : drawbbox->thickness;

line_fill_colour = unfilled;
strokepending = FALSE;
fillpending = FALSE;
pathstart = NULL;
at_x = at_y = 0;

/* Find the bounding box. */

find_bbox(bbox);

/* Output header material */

fprintf(out_file, "<?xml version=\"1.0\" standalone=\"no\"?>\n");
fprintf(out_file, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n");
fprintf(out_file, "  \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");

fprintf(out_file, "<svg width=\"%s\" height=\"%s\" version=\"1.1\"\n",
  fixed(bbox[2] - bbox[0] + bboxthick), fixed(bbox[3] - bbox[1] + bboxthick));

fprintf(out_file, "     xmlns=\"http://www.w3.org/2000/svg\">\n\n");

tnc = tree_search(varroot, US"creator");
tnd = tree_search(varroot, US"date");
fprintf(out_file, "<!-- created by %s on %s, using Aspic %s -->\n",
  tnc->value,  tnd->value, testing? "" : Version_String);

tnc = tree_search(varroot, US"title");
fprintf(out_file, "<title>%s</title>\n\n", tnc->value);

fprintf(out_file, "<g transform=\"translate(0,%s)\" "
  "font-family=\"Times\" font-size=\"12\">\n",
  fixed(bbox[3] - bbox[1] + bboxthick));

/* Draw a frame if wanted */

if (drawbbox != NULL)
  {
  drawbbox->width = bbox[2] - bbox[0];
  drawbbox->depth = bbox[3] - bbox[1];
  drawbbox->x = bbox[0] + drawbbox->width/2 + drawbbox->thickness/2;
  drawbbox->y = bbox[1] + drawbbox->depth/2 + drawbbox->thickness/2;
  write_box(drawbbox);
  }

/* Now process the items, repeating for each level */

for (level = min_level; level <= max_level; level++)
  {
  item *p;

  for (p = main_item_base; p != NULL; p = p->next)
    {
    BOOL restart = FALSE;
    BOOL move_needed = FALSE;
    int  startx = 0, starty = 0;
    item_arc *ppa;
    item_curve *ppc;
    item_line *ppl;

    if (p->level == level) switch (p->type)
      {
      case i_arc:
      ppa = (item_arc *)p;
      if (ppa->arrow_start) restart = TRUE;
      startx = p->x + (int )((double)ppa->radius * cos(ppa->angle1));
      starty = p->y + (int )((double)ppa->radius * sin(ppa->angle1));
      goto ARCLINE;

      case i_curve:
      ppc = (item_curve *)p;
      startx = ppc->x0;
      starty = ppc->y0;
      goto ARCLINE;

      case i_line:
      ppl = (item_line *)p;
      if (ppl->arrow_start) restart = TRUE;
      startx = p->x;
      starty = p->y;

      /* Common code for lines and arcs and curves */

      ARCLINE:

      if (startx != at_x || starty != at_y) restart = TRUE;

      /* Sort out the other conditions under which we have to terminate an
      existing path. */

      if (!samecolour(p->shapefilled, fillpending? line_fill_colour : unfilled))
        restart = TRUE;

      if (strokepending)
        {
        if (!samecolour(p->colour, stroke_colour) ||
            p->style == is_invi ||
            stroke_thickness != p->thickness ||
            stroke_dash1 != p->dash1 ||
            stroke_dash2 != p->dash2)
          restart = TRUE;
        }
      else
        {
        if (p->style != is_invi) restart = TRUE;
        }

      /* If starting a new path, end any previous one. */

      if (restart) end_line_fillstroke(p);

      /* Start stroking */

      if (!strokepending && p->style != is_invi)
        {
        stroke_thickness = p->thickness;
        stroke_dash1 = p->dash1;
        stroke_dash2 = p->dash2;
        stroke_colour = p->colour;
        strokepending = TRUE;
        pathstart = p;
        move_needed = TRUE;
        }

      /* Start filling */

      if (!fillpending && !samecolour(p->shapefilled, unfilled))
        {
        line_fill_colour = p->shapefilled;
        fillpending = TRUE;
        pathstart = p;
        move_needed = TRUE;
        }

      /* Write the arc or the line or the curve */

      if (p->type == i_arc)
        write_arc((item_arc *)p, move_needed, startx, starty);
      else if (p->type == i_curve)
        write_curve((item_curve *)p, move_needed);
      else
        write_line((item_line *)p, move_needed);
      break;

      case i_box:
      end_line_fillstroke(p);
      write_box((item_box *)p);
      break;

      case i_text:
      end_line_fillstroke(p);
      write_strings(p);
      break;
      }

    /* If hit an item at another level, stroke line and/or end line filling */

    else end_line_fillstroke(p);
    }

  end_line_fillstroke(NULL);
  }

fprintf(out_file, "</g></svg>\n");
}

/* End of wrsv.c */
