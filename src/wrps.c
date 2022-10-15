/*************************************************
*                      ASPIC                     *
*************************************************/

/* Copyright (c) University of Cambridge 1991 - 2022 */
/* Created: February 1991 */
/* Last modified: October 2022 */

/* This module generates output as encapsulated PostScript.*/


#include "aspic.h"


/*************************************************
*               Local variables                  *
*************************************************/

static int bbox[4];

static colour line_fill_colour;
static colour stroke_colour;
static colour set_colour;

static int  set_linedash1;
static int  set_linedash2;
static int  set_linewidth;

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
init_ps(void)
{
bindfont *f = getstore(sizeof(bindfont) + Ustrlen("Times-Roman"));

f->next = font_base;
font_base = f;
f->number = 0;
f->size = 12000;
f->needSymbol = f->needDingbats = FALSE;
Ustrcpy(f->name, "Times-Roman");

resolution = 120;       /* 0.12 == 600 dpi for default resolution */
}



/*************************************************
*             Drawing functions                  *
*************************************************/

/* This is an absolute move. */

static
void move(int x, int y)
{
x = x - bbox[0];
y = y - bbox[1];
fprintf(out_file, "%s %s mymove\n", fixed(rnd(x)), fixed(rnd(y)));
}


/* Relative line */

static void
rline(int x, int y)
{
fprintf(out_file, "%s %s rlineto\n", fixed(rnd(x)), fixed(rnd(y)));
}


/* Relative bezier */

static void
rbezier(int x1, int y1, int x2, int y2, int x3, int y3)
{
fprintf(out_file, "%s %s %s %s %s %s rcurveto\n",
  fixed(rnd(x1)), fixed(rnd(y1)),
  fixed(rnd(x2)), fixed(rnd(y2)),
  fixed(rnd(x3)), fixed(rnd(y3)));
}



/*************************************************
*          Set line thickness & colour           *
*************************************************/

static void
set_thickness(int t)
{
if (t != set_linewidth)
  {
  if (t < minimum_thickness) t = minimum_thickness;
  fprintf(out_file, "%s setlinewidth\n", CS fixed(t));
  set_linewidth = t;
  }
}

static void
setcolour(colour c)
{
if (c.red   != set_colour.red ||
    c.green != set_colour.green ||
    c.blue  != set_colour.blue)
  {
  if (c.red == c.green && c.green == c.blue)
    {
    fprintf(out_file, "%s setgray\n", fixed(c.red));
    }
  else
    {
    fprintf(out_file, "%s %s %s setrgbcolor\n",
      fixed(c.red), fixed(c.green), fixed(c.blue));
    }
  set_colour = c;
  }
}



/*************************************************
*                  Set dashedness                *
*************************************************/

static void
set_dash(int dash1, int dash2)
{
if (dash1 != set_linedash1 || (dash1 != 0 && dash2 != set_linedash2))
  {
  if (dash1 == 0)
    fprintf(out_file, "[] 0 setdash\n");
  else
    fprintf(out_file, "[%s %s] 0 setdash\n", CS fixed(dash1), CS fixed(dash2));
  set_linedash1 = dash1;
  set_linedash2 = dash2;
  }
}



/*************************************************
*             Write strings                      *
*************************************************/

/* Write out all the string items attached to a given item. The characters in a
single string may end up in more than one PostScript font. */

static void
write_strings(item *p)
{
stringchain *s = p->strings;
int x, y;

if (s == NULL) return;     /* There are no strings */
stringpos(p, &x, &y);      /* Find the position for the strings */

for (;;)
  {
  int depth;
  uschar *ss = s->text;

  if (*ss != 0)
    {
    int currentoffset = -1;
    int count = 0;

    setcolour(s->rgb);
    move(x + s->xadjust, y + s->yadjust);

    while (*ss != 0)
      {
      int c, offset;
      GETCHARINC(c, ss);

      /* Chars < 256 are in the first PS font, Unicode encoded */

      if (c < 256) offset = 0;

      /* Chars < 384 are in the second PS font, Unicode encoded - 256 */

      else if (c < 384) { offset = 1; c -= 256; }

      /* Seek for non-Unicode encoded in the second PS font; if not found,
      seek characters in the Special or Dingbats fonts. */

      else
        {
        int i;
        for (i = 0; i < nonucount; i++)
          {
          if (c == nonulist[i])
            {
            offset = 1;
            c = i + 128;
            break;
            }
          }

        if (i >= nonucount)
          {
          u2sencod *bot, *mid, *top;

          bot = u2slist;
          top = u2slist + u2scount;
          while (top > bot)
            {
            mid = bot + (top - bot)/2;
            if (c == mid->ucode) break;
            if (mid->ucode < c ) bot = mid + 1; else top = mid;
            }

          if (top > bot)
            {
            offset = (mid->which == SF_SYMB)? 2 : 3;
            c = mid->scode;
            }
          else
            {
            c = 0x00a4;    /* Currency symbol for "unknown" */
            offset = 0;
            }
          }
        }

      /* We now have the value within the font and the font offset */

      if (offset != currentoffset)
        {
        if (currentoffset >= 0) fprintf(out_file, ") ");
        fprintf(out_file, "f%d (", 4*s->font + offset);
        currentoffset = offset;
        count++;
        }

      if (c == '(' || c == ')' || c == '\\') fputc('\\', out_file);
      if (c >= 32 && c < 127) fputc(c, out_file);
        else fprintf(out_file, "\\%.03o", c);
      }

    fprintf(out_file, ") %d ", count);
    if (s->rotate != 0) fprintf(out_file, "%s rot ", fixed(s->rotate));

    fprintf(out_file, "%s",
      (s->justify == just_left)? US"leftshow" :
      (s->justify == just_right)? US"rightshow" : US"centreshow");

    if (s->rotate != 0) fprintf(out_file, " grestore");
    fprintf(out_file, "\n");
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
if (line_fill_colour.red != unfilled.red)
  {
  setcolour(line_fill_colour);
  if (strokepending) fprintf(out_file, "gsave fill grestore\n");
    else fprintf(out_file, "fill\n");
  }

if (strokepending)
  {
  setcolour(stroke_colour);
  set_thickness(stroke_thickness);
  set_dash(stroke_dash1, stroke_dash2);
  fprintf(out_file, "stroke\n");
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
double s = sin(angle);
double c = cos(angle);

int x1 = (int )((double)yy*s*0.5);
int y1 = (int )((double)yy*c*0.5);
int x2 = (int )((double)xx*c);
int y2 = (int )((double)xx*s);

move(x, y);
rline(x1, -y1);
rline(x2 - x1, y2 + y1);
rline(-x2 -x1, y1 - y2);
rline(x1, -y1);

if (filled.red != unfilled.red)
  {
  setcolour(filled);
  fprintf(out_file, "gsave fill grestore\n");
  }

set_thickness(400);
setcolour(stroke_colour);
fprintf(out_file, "stroke\n");
}



/*************************************************
*            Draw an elliptical arc              *
*************************************************/

static void
arc(int clockwise, int x, int y, int radius1, int radius2, double angle1,
  double angle2)
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
  set_dash(0, 0);

  if (p->arrow_start)
    {
    double angle = (p->cw)? (angle1 + pi/2.0 + tilt) : (angle1 - pi/2.0 - tilt);
    arrowhead(p->x + (int)(radius * cos(angle1)), p->y + (int)(radius * sin(angle1)),
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

/* Note that circles and ellipses are coded as special kinds of "box" */

static void
write_box(item_box *p)
{
int x = p->x;
int y = p->y;
int width = p->width;
int depth = p->depth;

/* If invisible, just write the strings, unless filled */

if (p->style == is_invi && samecolour(p->shapefilled, unfilled))
  {
  write_strings((item *)p);
  return;
  }

/* Draw a rectangular box */

if (p->boxtype == box_box)
  {
  move(x - width/2, y - depth/2);
  rline(width, 0);
  rline(0, depth);
  rline(-width, 0);
  }

/* Draw a circle or ellipse */

else
  {
  move(x + width/2, y);
  arc(FALSE, x, y, width/2, depth/2, 0.0, 2.0*pi);
  }

fprintf(out_file, "closepath\n");

/* Handle filling and stroking */

if (!samecolour(p->shapefilled, unfilled))
  {
  if (p->style != is_invi) fprintf(out_file, "gsave ");
  setcolour(p->shapefilled);
  fprintf(out_file, "fill");
  if (p->style != is_invi) fprintf(out_file, " grestore");
  fprintf(out_file, "\n");
  }

if (p->style != is_invi)
  {
  set_thickness(p->thickness);
  set_dash(p->dash1, p->dash2);
  setcolour(p->colour);
  fprintf(out_file, "stroke\n");
  }

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
strings. Otherwise, arrange to draw the line before any arrow heads so that a
forward arrow joined onto a previous line gets the benefit of appropriate
corner processing. */

if (p->style == is_invi)
  {
  write_strings((item *)p);
  return;
  }

/* If this is an arrow, compute data for arrow heads */

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

/* Now draw the line */

if (move_needed) move(x1, y1);
rline(xw, yd);
at_x = x1 + xw;
at_y = y1 + yd;

/* Now draw the arrow heads if required; ensure that this line's texts
are output. */

if (p->arrow_start)
  {
  end_line_fillstroke((item *)p->next);
  set_dash(0, 0);
  arrowhead(x1, y1, p->arrow_x, p->arrow_y, angle + pi, p->arrow_filled);
  }

if (p->arrow_end)
  {
  end_line_fillstroke((item *)p->next);
  set_dash(0, 0);
  arrowhead(x1 + xw, y1 + yd, p->arrow_x, p->arrow_y, angle, p->arrow_filled);
  }
}



/*************************************************
*          Write PostScript output file          *
*************************************************/

/* The coordinates are adjusted so that the bottom left of the bounding box is
at (0,0). */

void
write_ps(void)
{
tree_node *tn;
int bboxthick = (drawbbox == NULL)? 0 : drawbbox->thickness;
int level;

set_colour = black;
line_fill_colour = unfilled;
strokepending = FALSE;
fillpending = FALSE;
pathstart = NULL;
at_x = at_y = 0;

/* Find the bounding box. */

find_bbox(bbox);

/* Output header material */

fprintf(out_file, "%%!PS-Adobe-2.0 EPSF-2.0\n");
tn = tree_search(varroot, US"title");
fprintf(out_file, "%%%%Title: %s\n", tn->value);
tn = tree_search(varroot, US"creator");
fprintf(out_file, "%%%%Creator: %s, using Aspic %s\n", tn->value,
  testing? "" : Version_String);
tn = tree_search(varroot, US"date");
fprintf(out_file, "%%%%CreationDate: %s\n", tn->value);
fprintf(out_file, "%%%%BoundingBox: 0 0 %s %s\n",
  fixed(bbox[2] - bbox[0] + bboxthick),
  fixed(bbox[3] - bbox[1] + bboxthick));
fprintf(out_file, "%%%%EndComments\n\n");

/* The move function checks to see if there is a current point. If not, it does
an absolute move. Otherwise, it computes a relative move and does it if it is
not a null operation. Because the current point will have been adjusted for
device space, the check for no move must have a small tolerance. */

fprintf(out_file,
  "/mymove{\n"
  "{currentpoint} stopped {moveto}{\n"
  "  exch 4 1 roll sub 3 1 roll exch sub\n"
  "  dup abs 0.01 lt 3 -1 roll dup abs 0.01 lt\n"
  "  3 -1 roll and {pop pop}{rmoveto} ifelse\n"
  "  } ifelse\n"
  "}def\n");

/* If no items have text strings, we do not need to output the showing
functions or the UTF-8 encoding vectors; nor do we need to output any font
bindings. For fonts that use Adobe's standard encoding (that is, normal text
fonts), we are going to bind each font twice, to give us 512 characters to play
with. This is sufficient to encode all existing characters in the normal fonts.
The first 256 characters are encoded with The second encoding vector uses
Unicode for the first 128 characters (Latin Extended-A). The remainder are used
arbitrarily for the remaining Adobe standardly encoded characters. This latter
part of the encoding must be kept in step with the appropriate data table in
the code. */

if (strings_exist)
  {
  /* These PostScript functions expect a list of (font, string) pairs on the
  stack, followed by a count of the number of pairs. */

  fprintf(out_file,
    "/leftshow{dup add /r exch def\n"   /* Twice the count is now in r */
    "{r 2 gt\n"
    "{r -2 roll exch setfont show /r r 2 sub def}\n"  /* Not last substring */
    "{exch setfont show exit}\n"                      /* Last substring */
    "ifelse}loop}bind def\n"
    "/findwidth{dup 2 mul 1 add copy /w 0 def\n"  /* Copy pairs, zero count */
    "1 exch 1 exch\n"                             /* Set loop parameters */
    "{pop exch setfont stringwidth pop w add /w exch def}for}bind def\n"
    "/centreshow{findwidth w 2 div neg 0 rmoveto leftshow}bind def\n"
    "/rightshow{findwidth w neg 0 rmoveto leftshow}bind def\n"
    "/rot{gsave currentpoint translate rotate}bind def\n");

  /* Encoding vectors */

  fprintf(out_file,
    "/LowerEncoding 256 array def\n"
    "LowerEncoding 0 [\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/space/exclam/quotedbl/numbersign\n"
    "/dollar/percent/ampersand/quotesingle\n"
    "/parenleft/parenright/asterisk/plus\n"
    "/comma/hyphen/period/slash\n"
    "/zero/one/two/three\n"
    "/four/five/six/seven\n"
    "/eight/nine/colon/semicolon\n"
    "/less/equal/greater/question\n"
    "/at/A/B/C/D/E/F/G/H/I/J/K/L/M/N/O\n"
    "/P/Q/R/S/T/U/V/W/X/Y/Z/bracketleft\n"
    "/backslash/bracketright/asciicircum/underscore\n"
    "/grave/a/b/c/d/e/f/g/h/i/j/k/l/m/n/o\n"
    "/p/q/r/s/t/u/v/w/x/y/z/braceleft\n"
    "/bar/braceright/asciitilde/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/currency/currency\n"
    "/space/exclamdown/cent/sterling\n"
    "/currency/yen/brokenbar/section\n"
    "/dieresis/copyright/ordfeminine/guillemotleft\n"
    "/logicalnot/hyphen/registered/macron\n"
    "/degree/plusminus/twosuperior/threesuperior\n"
    "/acute/mu/paragraph/bullet\n"
    "/cedilla/onesuperior/ordmasculine/guillemotright\n"
    "/onequarter/onehalf/threequarters/questiondown\n"
    "/Agrave/Aacute/Acircumflex/Atilde\n"
    "/Adieresis/Aring/AE/Ccedilla\n"
    "/Egrave/Eacute/Ecircumflex/Edieresis\n"
    "/Igrave/Iacute/Icircumflex/Idieresis\n"
    "/Eth/Ntilde/Ograve/Oacute\n"
    "/Ocircumflex/Otilde/Odieresis/multiply\n"
    "/Oslash/Ugrave/Uacute/Ucircumflex\n"
    "/Udieresis/Yacute/Thorn/germandbls\n"
    "/agrave/aacute/acircumflex/atilde\n"
    "/adieresis/aring/ae/ccedilla\n"
    "/egrave/eacute/ecircumflex/edieresis\n"
    "/igrave/iacute/icircumflex/idieresis\n"
    "/eth/ntilde/ograve/oacute\n"
    "/ocircumflex/otilde/odieresis/divide\n"
    "/oslash/ugrave/uacute/ucircumflex\n"
    "/udieresis/yacute/thorn/ydieresis\n"
    "]putinterval\n");

  fprintf(out_file,
    "/UpperEncoding 256 array def\n"
    "UpperEncoding 0 [\n"
    "/Amacron/amacron/Abreve/abreve\n"
    "/Aogonek/aogonek/Cacute/cacute\n"
    "/currency/currency/currency/currency\n"
    "/Ccaron/ccaron/Dcaron/dcaron\n"
    "/Dcroat/dcroat/Emacron/emacron\n"
    "/currency/currency/Edotaccent/edotaccent\n"
    "/Eogonek/eogonek/Ecaron/ecaron\n"
    "/currency/currency/Gbreve/gbreve\n"
    "/currency/currency/Gcommaaccent/gcommaaccent\n"
    "/currency/currency/currency/currency\n"
    "/currency/currency/Imacron/imacron\n"
    "/currency/currency/Iogonek/iogonek\n"
    "/Idotaccent/dotlessi/currency/currency\n"
    "/currency/currency/Kcommaaccent/kcommaaccent\n"
    "/currency/Lacute/lacute/Lcommaaccent\n"
    "/lcommaaccent/Lcaron/lcaron/currency\n"
    "/currency/Lslash/lslash/Nacute\n"
    "/nacute/Ncommaaccent/ncommaaccent/Ncaron\n"
    "/ncaron/currency/currency/currency\n"
    "/Omacron/omacron/currency/currency\n"
    "/Ohungarumlaut/ohungarumlaut/OE/oe\n"
    "/Racute/racute/Rcommaaccent/rcommaaccent\n"
    "/Rcaron/rcaron/Sacute/sacute\n"
    "/currency/currency/Scedilla/scedilla\n"
    "/Scaron/scaron/currency/currency\n"
    "/Tcaron/tcaron/currency/currency\n"
    "/currency/currency/Umacron/umacron\n"
    "/currency/currency/Uring/uring\n"
    "/Uhungarumlaut/uhungarumlaut/Uogonek/uogonek\n"
    "/currency/currency/currency/currency\n"
    "/Ydieresis/Zacute/zacute/Zdotaccent\n"
    "/zdotaccent/Zcaron/zcaron/currency\n"
    "/Delta/Euro/Scommaaccent/Tcommaaccent\n"
    "/breve/caron/circumflex/commaaccent\n"
    "/dagger/daggerdbl/dotaccent/ellipsis\n"
    "/emdash/endash/fi/fl\n"
    "/florin/fraction/greaterequal/guilsinglleft\n"
    "/guilsinglright/hungarumlaut/lessequal/lozenge\n"
    "/minus/notequal/ogonek/partialdiff\n"
    "/periodcentered/perthousand/quotedblbase/quotedblleft\n"
    "/quotedblright/quoteleft/quoteright/quotesinglbase\n"
    "/radical/ring/scommaaccent/summation\n"
    "/tcommaaccent/tilde/trademark\n"
    "]putinterval\n");

  /* The function bindspecialfont binds a single font and scales it. The
  arguments on the stack are a name for the font (e.g. f0), the font typeface
  name, and the font size. The function bindstdfont finds a font and, if it is
  standardly encoded, binds it twice with the two different encodings. The
  arguments on the stack are the two font names, the font typeface name, and
  the font size. If the font turns out not be be standardly encoded, the two
  fonts will end up the same. */

  fprintf(out_file,
    "/bindspecialfont{exch findfont exch scalefont def}bind def\n"
    "/bindstdfont{exch findfont exch scalefont\n"
    "dup dup/Encoding get StandardEncoding eq\n"
    "{maxlength dup dict/newfont0 exch def dict/newfont1 exch def\n"
    "dup\n"
    "{1 index/FID eq{pop pop}{newfont0 3 1 roll put}ifelse}forall\n"
    "{1 index/FID eq{pop pop}{newfont1 3 1 roll put}ifelse}forall\n"
    "newfont1/Encoding UpperEncoding put dup newfont1 definefont def\n"
    "newfont0/Encoding LowerEncoding put dup newfont0 definefont def\n"
    "}\n"
    "{3 1 roll def def}ifelse\n"
    "}bind def\n");

  /* Output font bindings. */

  for (bindfont *b = font_base; b != NULL; b = b->next)
    {
    fprintf(out_file, "/f%d /f%d /%s %s bindstdfont\n", 4*b->number,
      4*b->number + 1, b->name, fixed(b->size));
    if (b->needSymbol)
      fprintf(out_file, "/f%d /%s %s bindspecialfont\n", 4*b->number + 2,
        "Symbol", fixed(b->size));
    if (b->needDingbats)
      fprintf(out_file, "/f%d /%s %s bindspecialfont\n", 4*b->number + 3,
        "ZapfDingbats", fixed(b->size));
    }
  }

/* Draw a bounding box frame if wanted */

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

/* Output showpage at the end so the file can be viewed on its own. */

fprintf(out_file, "showpage\n");
}


/* End of wrps.c */
