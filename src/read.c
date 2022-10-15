/*************************************************
*                      ASPIC                     *
*************************************************/

/* Copyright (c) University of Cambridge 1991 - 2022 */
/* Created: February 1991 */
/* Last modified: October 2022 */

/* This module contains the code for reading the input, with the exception of
commands for drawing objects (see rditem.c) and a number of common subroutines
(see rdsubs.c). */

#include "aspic.h"

/* Define some shorthand */

#define oo offsetof



/*************************************************
*            Tables defining optional args       *
*************************************************/

static arg_item text_args[] = {
  { US"at",    opt_at,   oo(item_text, x), oo(item_text, y) },
  { US"level", opt_int,  oo(item_text, level),           -1 },
  { US"", 0, -1, -1 }
};

static arg_item drawbbox_args[] = {
  { US"dashed",      opt_bool, oo(item_box, dash1),                       -1 },
  { US"filled",      opt_colgrey, oo(item_box, shapefilled),              -1 },
  { US"thickness",   opt_dim,  oo(item_box, thickness),                   -1 },
  { US"colour",      opt_colour, oo(item_box, colour),                    -1 },
  { US"grey",        opt_grey, oo(item_box, colour),                      -1 },
  { US"", 0, -1, -1 }
};



/*************************************************
*              Global variables                  *
*************************************************/

environment *env = NULL;      /* chain of stacked environments */
environment *old_env = NULL;  /* chain of "freed" environments */

item *lastitem;               /* last on list of items read */
item *baseitem;               /* item to base next item on */
item_box *drawbbox = NULL;    /* bounding box parameters */

label *label_base;            /* base of list of labels */
label *nextlabel;             /* points to labels when command is read */

int chptr = 0;                /* offset in current input line */
int drawbboxoffset;           /* offset for drawn bounding box frame */
int item_arg1;                /* First arg for some items */
int item_arg2;                /* Second ditto */

BOOL endfile;                 /* TRUE at EOF */

uschar word[WORD_SIZE];       /* buffer for reading words */
uschar wordstd[WORD_SIZE];    /* copy with standardized spelling */

uschar *in_line;              /* line currently being processed */
uschar *in_prev;              /* previous linput line */
uschar *in_raw;               /* raw line, before variable substitution */



/*************************************************
*               The BINDFONT command             *
*************************************************/

static void
c_bindfont(void)
{
bindfont *f;
int number, size;
int n = 0;
uschar s[1024];

nextsigch();
number = readint();

if (number <= 0) { error_moan(5); return; }
if (in_line[chptr] != '"') { error_moan(11, "font name in quotes"); return; }

for (;;)
  {
  if (in_line[++chptr] == 0)
    {
    error_moan(11, "closing quote");
    break;
    }
  if (in_line[chptr] == '\"')
    if (in_line[++chptr] != '\"') break;
  s[n++] = in_line[chptr];
  }
s[n] = 0;

nextsigch();
size = readnumber();
if (size <= 0) { error_moan(11, "non-negative font size"); return; }

f = getstore(sizeof(bindfont) + Ustrlen(s));

f->next = font_base;
font_base = f;
f->number = number;
f->size = size;
f->needSymbol = f->needDingbats = FALSE;
Ustrcpy(f->name, s);
}



/*************************************************
*              The INCLUDE command               *
*************************************************/

static void
c_include(void)
{
FILE *nf;
includestr *s;
uschar *t = word;
BOOL isinmac = macactive != NULL;

nextsigch();
while (in_line[chptr] != 0 && in_line[chptr] != ';' &&
       !isspace((int)in_line[chptr]))
  *t++ = in_line[chptr++];
*t = 0;
nextsigch();

/* If we were in a macro at the start of this command, give an error because
"include" is not supported from within a macro. Must do it this way because by
now the current reading point might have exited the macro. Also grumble if no
file name is given. */

if (isinmac) { error_moan(30); return; }
if (word[0] == 0) { error_moan(29); return; }

/* Need this check here so we know the command is complete before doing the
include because afterwards carries on with this line. */

if (in_line[chptr] != ';') error_moan(3);

nf = fopen(CS word, "r");
if (nf == NULL)
  {
  error_moan(1, word, "input", strerror(errno));
  return;
  }

if (spare_included == NULL) s = getstore(sizeof(includestr)); else
  {
  s = spare_included;
  spare_included = s->prev;
  }

s->prev = included_from;
s->prevfile = main_input;
included_from = s;

file_line_stack[inc_stack_ptr] = in_line;
file_chptr_stack[inc_stack_ptr++] = chptr;

in_line = get_in_line();
in_line[0] = ';';
in_line[1] = '\n';
in_line[2] = 0;
chptr = 0;

main_input = nf;
}



/*************************************************
*               The SET command                  *
*************************************************/

static void
c_set(void)
{
tree_node *tn;
int n = 0;
uschar s[1024];

readword();
if (word[0] == 0) { error_moan(17); return; }

nextsigch();
if (in_line[chptr] != '"') { error_moan(11, "quoted string"); return; }

for (;;)
  {
  if (in_line[++chptr] == '\n' || in_line[chptr] == 0)
    {
    error_moan(21);
    break;
    }
  if (in_line[chptr] == '\"')
    if (in_line[++chptr] != '\"') break;
  s[n++] = in_line[chptr];
  }
s[n] = 0;

tn = tree_search(varroot, word);

if (tn == NULL)
  {
  tn = getstore(sizeof(tree_node) + Ustrlen(word));
  Ustrcpy(tn->name, word);
  (void)tree_insertnode(&varroot, tn);
  }

tn->value = getstore(n + 1);
Ustrcpy(tn->value, s);
}



/*************************************************
*               The TEXT command                 *
*************************************************/

static void
c_text(void)
{
item_arc  *lastarc;
item_box  *lastbox;
item_curve *lastcurve;
item_line *lastline;
item_text *text = getstore(sizeof(item_text));

/* Initialize with default parameters */

text->type = i_text;
text->style = item_arg1;
text->linedepth = env->linedepth;
text->fontdepth = env->fontdepth;
text->colour = env->textcolour;
text->next = NULL;
text->strings = NULL;
text->level = env->level;

if (text->level > max_level) max_level = text->level;
if (text->level < min_level) min_level = text->level;

/* Mark various values as "unset" */

text->x = text->y = UNSET;

/* Read optional parameters */

options((item *)text, text_args);

/* Set up default position if required. Both coordinates will have been set by
any positioning option, so we only need to test one. */

if (text->x == UNSET)
  {
  /* If this is the first item, put it at the origin */

  if (baseitem == NULL)
    {
    text->x = 0;
    text->y = 0;
    }

  /* Otherwise the position depends on the previous item, which
  can never be a text item. */

  else switch(baseitem->type)
    {
    case i_arc:
    lastarc = (item_arc *)baseitem;
    text->x = lastarc->x;
    text->y = lastarc->y;
    break;

    case i_curve:
    lastcurve = (item_curve *)baseitem;
    text->x = lastcurve->x;
    text->y = lastcurve->y;
    break;

    case i_box:
    lastbox = (item_box *)baseitem;
    text->x = lastbox->x;
    text->y = lastbox->y;
    break;

    case i_line:
    lastline = (item_line *)baseitem;
    text->x = lastline->x + lastline->width/2;
    text->y = lastline->y + lastline->depth/2;
    break;
    }
  }

/* Read any associated strings, then connect to chain. A text item never
becomes a base item, and it may not be labelled. */

readstringchain((item *)text, just_centre);
if (lastitem == NULL) main_item_base = (item *)text;
  else lastitem->next = (item *)text;
lastitem = (item *)text;
}



/*************************************************
*      The UP, DOWN, LEFT, and RIGHT commands    *
*************************************************/

static void
c_up(void)
{
env->direction = north;
}

static void
c_down(void)
{
env->direction = south;
}

static void
c_left(void)
{
env->direction = west;
}

static void
c_right(void)
{
env->direction = east;
}



/*************************************************
*            Overall parameters                  *
*************************************************/

static void
c_boundingbox(void)
{
drawbboxoffset = readnumber();

if (drawbbox == NULL) drawbbox = getstore(sizeof(item_box));

/* Only a few of the box item fields are relevant for the bounding box. The
size gets set later. */

drawbbox->style = is_norm;
drawbbox->boxtype = box_box;
drawbbox->dash1 = 0;
drawbbox->thickness = 400;
drawbbox->colour = black;
drawbbox->shapefilled = unfilled;
drawbbox->strings = NULL;

nextsigch();
options((item *)drawbbox, drawbbox_args);
if (drawbbox->dash1 != 0)
  {
  drawbbox->dash1 = env->boxdash1;
  drawbbox->dash2 = env->boxdash2;
  }
}



/*************************************************
*       Default dimension & other settings       *
*************************************************/

static void
c_env(void)
{
int value = readnumber();
if (item_arg2) value = mag(value);
*((int *)(((uschar *)env) + item_arg1)) = value;
}

static void
c_env2(void)
{
*((int *)(((uschar *)env) + item_arg1)) = readnumber();
if (in_line[chptr] == ',') chptr++;
nextsigch();
*((int *)(((uschar *)env) + item_arg2)) = readnumber();
}

static void
c_env3(void)
{
*((int *)(((uschar *)env) + item_arg1)) = readint();
}

/* Grey level */

static void
c_env4(void)
{
colour *c = (colour *)(((uschar *)env) + item_arg1);
c->red = c->green = c->blue = readnumber();
}

/* Colour */

static void
c_env5(void)
{
colour *c = (colour *)(((uschar *)env) + item_arg1);
c->red = readnumber();
if (in_line[chptr] == ',') chptr++;
nextsigch();
c->green = readnumber();
if (in_line[chptr] == ',') chptr++;
nextsigch();
c->blue = readnumber();
}

/* Grey level or colour; -1 means "not filled" for filling values. */

static void
c_env6(void)
{
colour *c = (colour *)(((uschar *)env) + item_arg1);
c->red = c->green = c->blue = readnumber();
if (in_line[chptr] == ',') chptr++;
while (isspace((int)in_line[chptr])) chptr++;
if (!isdigit((int)in_line[chptr])) return;
c->green = readnumber();
if (in_line[chptr] == ',') chptr++;
while (isspace((int)in_line[chptr])) chptr++;
c->blue = readnumber();
}



/*************************************************
*           Change output resolution             *
*************************************************/

static void
c_resolution(void)
{
resolution = readnumber();
}



/*************************************************
*           Change magnification                 *
*************************************************/

static void
c_mag(void)
{
int newmag = readnumber();
env->arcradius = (newmag * env->arcradius)/1000;
env->arrow_x = (newmag * env->arrow_x)/1000;
env->arrow_y = (newmag * env->arrow_y)/1000;
env->boxwidth = (newmag * env->boxwidth)/1000;
env->boxdash1 = (newmag * env->boxdash1)/1000;
env->boxdash2 = (newmag * env->boxdash2)/1000;
env->boxdepth = (newmag * env->boxdepth)/1000;
env->boxthickness = (newmag * env->boxthickness)/1000;
env->cirdash1 = (newmag * env->cirdash1)/1000;
env->cirdash2 = (newmag * env->cirdash2)/1000;
env->cirradius = (newmag * env->cirradius)/1000;
env->cirthickness = (newmag * env->cirthickness)/1000;
env->ellwidth = (newmag * env->ellwidth)/1000;
env->elldash1 = (newmag * env->elldash1)/1000;
env->elldash2 = (newmag * env->elldash2)/1000;
env->elldepth = (newmag * env->elldepth)/1000;
env->ellthickness = (newmag * env->ellthickness)/1000;
env->linedash1 = (newmag * env->linedash1)/1000;
env->linedash2 = (newmag * env->linedash2)/1000;
env->linedepth = (newmag * env->linedepth)/1000;
env->linethickness = (newmag * env->linethickness)/1000;
env->line_hw = (newmag * env->line_hw)/1000;
env->line_vd = (newmag * env->line_vd)/1000;
env->magnification = (newmag * env->magnification)/1000;
}



/*************************************************
*               The GOTO command                 *
*************************************************/

static void
c_goto(void)
{
if (in_line[chptr] == '*')
  {
  baseitem = NULL;
  chptr++;
  nextsigch();
  }
else
  {
  item *ii;
  readword();
  ii = findlabel(word);
  if (ii == NULL) error_moan(10, word);
  baseitem = ii;
  }
}



/*************************************************
*                   The MACRO command            *
*************************************************/

/* Local subroutine to search for end of macro text.

Arguments:
  ptr         current input pointer
  term        terminating character

Returns:      the end pointer
*/

static int
find_mac_end(int ptr, uschar term)
{
while (in_line[ptr] != term && in_line[ptr] != 0)
  {
  if (in_line[++ptr] == '\"')
    {
    while (in_line[++ptr] != '\"' && in_line[ptr] != 0) {};
    if (in_line[ptr] == 0)
      {
      int save_chptr = chptr;
      chptr = ptr;              /* To get correct reflection */
      error_moan(21);
      chptr = save_chptr;       /* But it is not expected to change */
      }
    }
  }
return ptr;
}

/* Find maximum argument reference in line */

static int
maxarg(uschar *s)
{
int n = 0;
while (*s != 0)
  {
  if (*s++ == '&')
    {
    if (*s == '&') s++; else
      {
      int m = 0;
      s--;
      while (isdigit((int)*(++s))) m = m*10 + (*s) - '0';
      if (m > n) n = m;
      }
    }
  }
return n;
}

/* Main routine for MACRO */

static void
c_macro(void)
{
uschar term = ';';
macro *m = getstore(sizeof(macro));
mac_line **ptrnext = &(m->nextline);

m->nextline = NULL;
m->argcount = 0;
readword();
Ustrcpy(m->name, word);

if (in_line[chptr] == '{') { term = '}'; chptr++; }

for (;;)
  {
  int n;
  int length = find_mac_end(chptr, term) - chptr;
  mac_line *line = getstore(length + 2 + offsetof(mac_line, text));
  *ptrnext = line;
  line->next = NULL;
  ptrnext = &(line->next);
  Ustrncpy(line->text, in_line+chptr, length);
  line->text[length] = 0;

  n = maxarg(line->text);
  if (n > m->argcount) m->argcount = n;

  chptr += length;
  if (in_line[chptr] == term)
    {
    line->text[length] = ' ';
    line->text[length+1] = 0;
    if (term == '}') { nextch(); nextsigch(); }
    break;
    }
  else
    {
    chptr--;       /* just before final 0 */
    nextch();
    if (endfile)
      {
      chptr = 0;   /* Make it reflect previous line */
      error_moan(40, m->name);
      exit(EXIT_FAILURE);
      }
    }
  }

/* Attach macro to chain */

m->previous = macroot;
macroot = m;
}



/*************************************************
*          Push and pop environment              *
*************************************************/

/* When an environment is popped, it is put onto the old_env chain so that its
memory block can be reused. */

static void
c_push(void)
{
environment *newenv;

if (old_env == NULL)
  {
  newenv = getstore(sizeof(environment));
  }
else
  {
  newenv = old_env;
  old_env = newenv->previous;
  }

memcpy(newenv, env, sizeof(environment));
newenv->previous = env;
env = newenv;
}

static void
c_pop(void)
{
if (env->previous == NULL) error_moan(18); else
  {
  environment *old = env;
  env = env->previous;
  old->previous = old_env;
  old_env = old;
  }
}



/*************************************************
*               Table of commands                *
*************************************************/

/* The functions for the drawing commands (c_arc, c_box, c_circle, c_curve, and
c_line) are in the rditem.c source file. */

static command_item cmdtab[] = {
  { US"arc",           c_arc,   is_norm, FALSE },
  { US"arcarrow",      c_arc,   is_norm,  TRUE },
  { US"arcradius",     c_env,   offsetof(environment, arcradius),     TRUE },
  { US"arrow",         c_line,  is_norm,  TRUE },
  { US"arrowfill",     c_env6,  offsetof(environment, arrowfilled),  FALSE },
  { US"arrowlength",   c_env,   offsetof(environment, arrow_x),       TRUE },
  { US"arrowwidth",    c_env,   offsetof(environment, arrow_y),       TRUE },
  { US"bindfont",      c_bindfont, 0, 0 },
  { US"boundingbox",   c_boundingbox,  0,    0 },
  { US"box",           c_box,   is_norm,     0 },
  { US"boxcolour",     c_env5,  offsetof(environment, boxcolour),    FALSE },
  { US"boxdash",       c_env2,  offsetof(environment, boxdash1), offsetof(environment,boxdash2) },
  { US"boxdepth",      c_env,   offsetof(environment, boxdepth),      TRUE },
  { US"boxfill",       c_env6,  offsetof(environment, boxfilled),    FALSE },
  { US"boxgrey",       c_env4,  offsetof(environment, boxcolour),    FALSE },
  { US"boxthickness",  c_env,   offsetof(environment, boxthickness),  TRUE },
  { US"boxwidth",      c_env,   offsetof(environment, boxwidth),      TRUE },
  { US"circle",        c_circle,is_norm,  TRUE },
  { US"circlecolour",  c_env5,  offsetof(environment, circolour),    FALSE },
  { US"circledash",    c_env2,  offsetof(environment, cirdash1), offsetof(environment,cirdash2) },
  { US"circlefill",    c_env6,  offsetof(environment, cirfilled),    FALSE },
  { US"circlegrey",    c_env4,  offsetof(environment, circolour),    FALSE },
  { US"circleradius",  c_env,   offsetof(environment, cirradius),     TRUE },
  { US"circlethickness", c_env, offsetof(environment, cirthickness),  TRUE },
  { US"curve",         c_curve, is_norm, FALSE },
  { US"down",          c_down,        0,     0 },
  { US"ellipse",       c_circle,is_norm, FALSE },
  { US"ellipsecolour", c_env5,  offsetof(environment, ellcolour),   FALSE },
  { US"ellipsedash",   c_env2,  offsetof(environment, elldash1), offsetof(environment,elldash2) },
  { US"ellipsedepth",  c_env,   offsetof(environment, elldepth),      TRUE },
  { US"ellipsefill",   c_env6,  offsetof(environment, ellfilled),    FALSE },
  { US"ellipsegrey",   c_env4, offsetof(environment, ellcolour),   FALSE },
  { US"ellipsethickness", c_env, offsetof(environment, ellthickness), TRUE },
  { US"ellipsewidth",  c_env,   offsetof(environment, ellwidth),      TRUE },
  { US"fontdepth",     c_env,   offsetof(environment, fontdepth),     TRUE },
  { US"goto",          c_goto,        0,     0 },
  { US"hlinelength",   c_env,   offsetof(environment, line_hw),       TRUE },
  { US"iarc",          c_arc,   is_invi,     0 },
  { US"ibox",          c_box,   is_invi,     0 },
  { US"icircle",       c_circle,is_invi,  TRUE },
  { US"icurve",        c_curve, is_invi, FALSE },
  { US"iellipse",      c_circle,is_invi, FALSE },
  { US"iline",         c_line,  is_invi, FALSE },
  { US"include",       c_include,     0,     0 },
  { US"left",          c_left,        0,     0 },
  { US"level",         c_env3,  offsetof(environment, level),        FALSE },
  { US"line",          c_line,  is_norm, FALSE },
  { US"linecolour",    c_env5,  offsetof(environment, linecolour),   FALSE },
  { US"linedash",      c_env2,  offsetof(environment, linedash1), offsetof(environment,linedash2) },
  { US"linegrey",      c_env4,  offsetof(environment, linecolour),   FALSE },
  { US"linethickness", c_env,   offsetof(environment, linethickness), TRUE },
  { US"magnify",       c_mag,         0,     0 },
  { US"macro",         c_macro,       0,     0 },
  { US"pop",           c_pop,         0,     0 },
  { US"push",          c_push,        0,     0 },
  { US"resolution",    c_resolution,  0,     0 },
  { US"right",         c_right,       0,     0 },
  { US"set",           c_set,         0,     0 },
  { US"setfont",       c_env3,  offsetof(environment, setfont),      FALSE },
  { US"shapefill",     c_env6,  offsetof(environment, shapefilled),  FALSE },
  { US"text",          c_text,        0,     0 },
  { US"textcolour",    c_env5,  offsetof(environment, textcolour),   FALSE },
  { US"textdepth",     c_env,   offsetof(environment, linedepth),     TRUE },
  { US"up",            c_up,          0,     0 },
  { US"vlinelength",   c_env,   offsetof(environment, line_vd),       TRUE }
};

#define cmdtab_count (sizeof(cmdtab)/sizeof(command_item))



/*************************************************
*              Obey a macro                      *
*************************************************/

static void
obey_macro(macro *m)
{
int argcount = 0;
macro *mm = getmacro();
mac_arg **ap;
memcpy(mm, m, sizeof(macro));

mm->args = NULL;
ap = &(mm->args);

while (in_line[chptr] != ';' && in_line[chptr] != '|' && argcount < m->argcount)
  {
  mac_arg *arg;
  int length;
  int p = chptr + 1;
  uschar term1 = ' ';
  uschar term2;

  if (in_line[chptr] == '\"' || in_line[chptr] == '\'') term1 = in_line[chptr];
  if (term1 == '\'') chptr++;
  term2 = (uschar)((term1 == ' ')? ';' : term1);

  while (in_line[p] != term1 && in_line[p] != term2 && in_line[p] != '\n') p++;
  if (in_line[p] == '\"') p++;

  length = p - chptr;
  arg = getstore(length + 1 + offsetof(mac_arg, text));
  arg->next = NULL;
  *ap = arg;
  ap = &(arg->next);

  Ustrncpy(arg->text, in_line+chptr, length);
  arg->text[length] = 0;

  chptr += length;
  if (in_line[chptr] == '\'') chptr++;
  nextsigch();
  argcount++;
  }

if (in_line[chptr] == '|') { nextch(); nextsigch(); }

in_line_stack[mac_stack_ptr] = in_line;
chptr_stack[mac_stack_ptr] = chptr;
mac_count_stack[mac_stack_ptr++] = macro_id;

in_line = get_in_line();
in_line[0] = 0;
chptr = -1;

mm->previous = macactive;
macactive = mm;
macro_id = macro_count++;
nextch();
}



/*************************************************
*                Read input file                 *
*************************************************/

void
read_inputfile()
{
env = getstore(sizeof(environment));
env->previous = NULL;

env->arcradius = 36000;
env->arrow_x = 10000;
env->arrow_y = 10000;
env->boxdash1 = 7000;
env->boxdash2 = 5000;
env->boxwidth = 72000;
env->boxdepth = 36000;
env->boxthickness = 500;
env->boxcolour = black;
env->boxfilled = unfilled;
env->cirdash1 = 7000;
env->cirdash2 = 5000;
env->cirradius = 36000;
env->cirthickness = 400;
env->circolour = black;
env->cirfilled = unfilled;
env->direction = east;
env->elldash1 = 7000;
env->elldash2 = 5000;
env->ellcolour = black;
env->ellwidth = 72000;
env->elldepth = 36000;
env->ellthickness = 400;
env->ellfilled = unfilled;
env->shapefilled = unfilled;
env->arrowfilled = unfilled;
env->setfont = 0;
env->textcolour = black;
env->fontdepth = 6000;
env->linedash1 = 7000;
env->linedash2 = 5000;
env->linedepth = 12000;
env->linethickness = 400;
env->linecolour = black;
env->line_hw = 72000;
env->line_vd = 36000;
env->magnification = 1000;
env->level = 0;

drawbboxoffset = 0;
macro_count = 0;

baseitem = lastitem = NULL;

label_base = NULL;
nextlabel = NULL;

chptr = 0;
in_line[0] = '\n';        /* initialize with null line */
in_line[1] = 0;
in_prev[0] = '\n';
in_prev[1] = 0;
endfile = FALSE;

/* The main loop */

for (;;)
  {
  nextsigch();
  while (in_line[chptr] == ';' && !endfile) { nextch(); nextsigch(); }
  if (endfile) break;
  readword();

  /* If no word present, either it's a comment or an error */

  if (word[0] == 0)
    {
    if (in_line[chptr] == '#')
      {
      while (in_line[++chptr] != 0) {};
      chptr--;
      }
    else
      {
      if (chptr == 0) chptr = 1;  /* Show this line, not previous */
      error_moan(16);
      exit(EXIT_FAILURE);   /* Give up */
      }
    }

  /* Deal with labels */

  else if (in_line[chptr] == ':')
    {
    nextch();
    if (findlabel(word) != NULL)
      {
      error_moan(37, word);
      }
    else
      {
      label *newlabel = getstore(sizeof(label));
      Ustrcpy(newlabel->name, word);
      newlabel->next = nextlabel;
      newlabel->itemptr = NULL;
      nextlabel = newlabel;
      }
    }

  /* Deal with command words -- built-in first, macro second */

  else
    {
    size_t i;

    standardize_word();  /* Allows for spelling variations */
    for (i = 0; i < cmdtab_count; i++)
      {
      if (Ustrcmp(wordstd, cmdtab[i].name) == 0)
        {
        item_arg1 = cmdtab[i].arg1;
        item_arg2 = cmdtab[i].arg2;
        cmdtab[i].function();

        /* If we have just defined a macro, check that its name does not
        conflict with a built-in command. We do this here because the list of
        commands isn't accessible from within the c_macro() function as it is
        defined later. */

        if (cmdtab[i].function == c_macro)
          {
          Ustrcpy(word, macroot->name);
          standardize_word();
          for (size_t ii = 0; ii < cmdtab_count; ii++)
            {
            if (Ustrcmp(wordstd, cmdtab[ii].name) == 0)
              {
              error_moan(39, word);
              break;
              }
            }
          }
        break;
        }
      }

    /* Not built-in; try macro */

    if (i >= cmdtab_count)
     {
     macro *m = macroot;
     while (m != NULL)
       {
       if (Ustrcmp(word, m->name) == 0)
         {
         for (macro *mm = macactive; mm != NULL; mm = mm->previous)
           {
           if (Ustrcmp(mm->name, m->name) == 0)  /* Recursion detected */
             {
             error_moan(41);
             exit(EXIT_FAILURE);
             }
           }
         obey_macro(m);
         break;
         }
       m = m->previous;
       }
     if (m == NULL) { error_moan(2, word); continue; }
     }

    /* Was built-in; check terminator */

    else
      {
      if (in_line[chptr++] != ';') { error_moan(3); continue; }
      }

    /* If a non-macro command was labelled, but the label hasn't been used up,
    it was on an inappropriate command. When a macro call is labelled, the
    label is either used or gives this error for the first command within the
    macro. */

    if (nextlabel != NULL && i < cmdtab_count)
      {
      while (nextlabel != NULL)
        {
        label *thislabel = nextlabel;
        error_moan(9, thislabel->name);
        nextlabel = thislabel->next;
        }
      }
    }
  }
}

/* End of read.c */
