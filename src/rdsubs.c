/*************************************************
*                      ASPIC                     *
*************************************************/

/* Copyright (c) University of Cambridge 1991 - 2023 */
/* Created: February 1991 */
/* Last modified: January 2023 */

/* This module contains miscellaneous functions that are called while the input
is being read. */


#include "aspic.h"


/*************************************************
*            Local variables                     *
*************************************************/

static BOOL wordread;   /* Set TRUE when a word has been read but not used. */



/*************************************************
*           Find a position on a curve           *
*************************************************/

/* Called when reading items, and also when determining the bounding box.

Arguments:
  pp        points to curve item
  t         fraction along the curve
  xp        where to return the relative x value
  yp        where to return the relative y value

Returns:    nothing
*/

void
find_curvepos(item_curve *pp, double t, int *xp, int *yp)
{
int x1 = pp->x0 + pp->cx1;
int x2 = pp->x0 + pp->cx2;
int y1 = pp->y0 + pp->cy1;
int y2 = pp->y0 + pp->cy2;

double ax = (double)(pp->x1 - 3*x2 + 3*x1 - pp->x0);
double bx = (double)(3*x2 - 6*x1 + 3*pp->x0);
double cx = (double)(3*(x1 - pp->x0));

double ay = (double)(pp->y1 - 3*y2 + 3*y1 - pp->y0);
double by = (double)(3*y2 - 6*y1 + 3*pp->y0);
double cy = (double)(3*(y1 - pp->y0));

*xp = (int)(ax*t*t*t + bx*t*t + cx*t) + pp->x0;
*yp = (int)(ay*t*t*t + by*t*t + cy*t) + pp->y0;
}



/*************************************************
*            Find labelled item                  *
*************************************************/

/* Finds the item with the given label.

Argument:  the label name
Returns:   a pointer to the item
*/

item *
findlabel(uschar *word)
{
for (label *ii = label_base; ii != NULL; ii = ii->next)
  if (Ustrcmp(word, ii->name) == 0) return ii->itemptr;
return NULL;
}



/*************************************************
*       Add item to chain and label it           *
*************************************************/

/* This is called for drawing items that are allowed to be labelled. It is not
called for the "text" item. Put the given item on the item chain and set it up
as the base item. If nextlabel points to one or more labels, point each of them
back to this item and then add the label(s) to the chain of labels.

Argument :  the new item
Returns:    nothing
*/

void
chain_label(item *newitem)
{
if (lastitem == NULL) main_item_base = newitem; else lastitem->next = newitem;
baseitem = lastitem = newitem;

if (nextlabel != NULL)
  {
  label *lastlabel = nextlabel;
  for (;;)
    {
    lastlabel->itemptr = newitem;
    if (lastlabel->next == NULL) break;
    lastlabel = lastlabel->next;
    }
  lastlabel->next = label_base;
  label_base = nextlabel;
  nextlabel = NULL;
  }
}



/*************************************************
*        Substitute variables in a line          *
*************************************************/

/* The size of the output buffer is assumed to be INPUT_LINESIZE. We look
for the special notation &$ that is used in Aspic macros, and do not treat
that $ as introducing a variable.

Arguments:
  raw        the raw input line
  cooked     where to put the cooked input line

Returns:     nothing
*/

static void
subs_vars(uschar *raw, uschar *cooked)
{
int left = INPUT_LINESIZE;
BOOL bracketed, toolong;
uschar *p, *s, *t;
uschar name[64];

substituting = TRUE;   /* Errors to reflect raw line */

for (s = raw, t = cooked; *s != 0; )
  {
  if (left < 2)
    {
    /* LCOV_EXCL_START */
    error_moan(26);
    exit(EXIT_FAILURE);
    /* LCOV_EXCL_STOP */
    }

  if (*s == '&')
    {
    *t++ = *s++;
    left--;
    if (*s == '$' || *s == '&') { *t++ = *s++; left--; }
    continue;
    }

  if (*s != '$')   { *t++ = *s++; left--; continue; }

  if (s[1] == '$')
    {
    *t++ = '$';
    s += 2;
    left--;
    continue;
    }

  if (*(++s) == '{')
    {
    bracketed = TRUE;
    s++;
    }
  else bracketed = FALSE;

  toolong = FALSE;
  p = name;
  while (isalpha(*s) || isdigit(*s)) 
    {
    if (!toolong)
      {  
      if ((size_t)(p - name) >= sizeof(name) - 1) toolong = TRUE;
        else *p++ = *s;
      } 
    s++;   
    } 
  *p = 0;

  subs_ptr = s - raw;   /* Offset for errors */

  if (bracketed)
    {
    if (*s == '}') s++; else error_moan(27, name);
    }

  if (*name == 0) error_moan(17); 
  else if (toolong) error_moan(43); 
  else
    {
    tree_node *tn = tree_search(varroot, name);
    if (tn == NULL)
      {
      error_moan(6, "", name);
      }
    else
      {
      int len = Ustrlen(tn->value);
      if (left < len + 1)
        {
        error_moan(25, name);
        exit(EXIT_FAILURE);
        }
      Ustrcpy(t, tn->value);
      t += len;
      left -= len;
      }
    }
  }

*t = 0;
substituting = FALSE;
}



/*************************************************
*           Get to next character in input       *
*************************************************/

/* The current line is in in_line, and the current offset is in chptr. Advance
the offset if that brings us to the end of the line, set up the next line. At
this stage we are assuming that all characters are ASCII. Handle &$ in macro
lines. */

void
nextch(void)
{
if (in_line[++chptr] != 0) return;

/* Copy the just-processed line to in_prev. */

if (in_line[0] != 0 && in_line[0] != '\n') Ustrcpy(in_prev, in_line);

/* If a macro is active but there are no more lines, revert to the previous
input environment. */

if (macactive != NULL)
  {
  macro *m = macactive;
  if (m->nextline == NULL)
    {
    macactive = m->previous;
    freemacro(m);
    free_in_line(in_line);
    in_line = in_line_stack[--mac_stack_ptr];
    chptr = chptr_stack[mac_stack_ptr];
    macro_id = mac_count_stack[mac_stack_ptr];
    }

  /* Get the next line from an active macro, substituting arguments. */

  else
    {
    uschar *t = in_line;
    uschar *f = (m->nextline)->text;

    while (*f != 0)
      {
      if (*f == '&')
        {
        if (f[1] == '&')
          {
          *t++ = '&';
          f += 2;
          }
        else if (f[1] == '$')
          {
          sprintf(CS t, "%d", macro_id);      /* Avoid ANSI use of sprintf() yield */
          t += Ustrlen(t);                    /* 'cause other libraries are different */
          f += 2;
          }
        else
          {
          mac_arg *ap = m->args;
	  int n = 0;
          while (isdigit((int)*(++f))) n = n*10 + (*f) - '0';
          while (--n > 0 && ap != NULL) ap = ap->next;
          if (ap != NULL) { Ustrcpy(t, ap->text); t += Ustrlen(ap->text); }
          }
        }
      else *t++ = *f++;
      }
    *t = 0;
    chptr = 0;
    m->nextline = (m->nextline)->next;
    }
  }

/* Not in a macro. Read into in_raw, then scan for variables into in_line,
unless variable substitution is disabled. Handle reverting at the end of an
included file. */

else while (!endfile)
  {
  if (Ufgets(in_raw, INPUT_LINESIZE, main_input) == NULL)
    {
    if (included_from == NULL)  /* End of the main input */
      {
      endfile = TRUE;
      chptr = 0;
      in_line[0] = 0;
      }
    else                        /* End of an included file */
      {
      includestr *s = included_from;
      free_in_line(in_line);
      fclose(main_input);
      main_input = s->prevfile;
      in_line = file_line_stack[--inc_stack_ptr];
      chptr = file_chptr_stack[inc_stack_ptr];
      included_from = s->prev;
      s->prev = spare_included;
      spare_included = s;
      nextch();
      break;
      }
    }
  else   /* Next line has been read */
    {
    /* LCOV_EXCL_START */
    if (Ustrlen(in_raw) == INPUT_LINESIZE - 1 &&
        in_raw[INPUT_LINESIZE - 1] != '\n')
      {
      reading = FALSE;      /* Stops it trying to reflect the line */
      error_moan(35, INPUT_LINESIZE - 1);
      exit(EXIT_FAILURE);   /* Best not try to continue */
      }
    /* LCOV_EXCL_STOP */
    if (no_variables) Ustrcpy(in_line, in_raw); else subs_vars(in_raw, in_line);
    chptr = 0;
    if (in_line[chptr] != '#') break;
    }
  }
}



/*************************************************
*         Get to next significant character      *
*************************************************/

/* The scan starts at the current character. */

void
nextsigch(void)
{
while (in_line[chptr] == ' ' || in_line[chptr] == '\n')
  {
  nextch();
  if (endfile) break;
  }
}



/*************************************************
*      Get next Unicode character in s string    *
*************************************************/

/* This deals with UTF-8 characters and with &xxx; codings. It advances
the chptr, leaving it at the final byte of the character. */

static int
nextUchar(void)
{
int c = in_line[++chptr];

/* Handle '&' coding */

if (c == '&')
  {
  /* Handle &#...; and &#x...; */

  if (in_line[chptr+1] == '#')
    {
    uschar *endptr;
    unsigned long longvalue;
    int base = 10;
    int st = chptr + 2;

    if (in_line[chptr+2] == 'x')
      {
      base = 16;
      st++;
      }

    /* This mess is to avoid a warning about type-punned variables from gcc */
      {
      char *t1 = CS (in_line + st);
      char *t2;
      longvalue = strtoul(t1, &t2, base);
      endptr = US t2;
      }

    /* If no ; at end, just leave the byte value in c */

    if (*endptr == ';')
      {
      chptr = endptr - in_line;
      c = (int)longvalue;
      }
    }

  /* Handle &name; ignore unknown or if ; is missing */

  else if (isalpha(in_line[chptr+1]))
    {
    uschar *name = in_line + chptr + 1;
    uschar *p = name + 1;
    while isalnum(*p) p++;
    if (*p == ';')
      {
      entity_block *bot, *mid, *top;

      *p = 0;
      bot = entity_list;
      top = entity_list + entity_list_count;
      while (top > bot)
        {
        int x;
        mid = bot + (top - bot)/2;
        x = Ustrcmp(mid->name, name);
        if (x == 0) break;
        if (x < 0) bot = mid + 1; else top = mid;
        }

      *p = ';';

      if (top > bot)
        {
        chptr = p - in_line;
        c = mid->value;
        }
      }
    }
  }

/* Handle wide characters */

else if (c >= 128)
  {
  int savechptr = chptr;
  int d = c << 1;
  int i;

  for (i = 0; i < 6; i++)          /* i is number of additional bytes */
    {
    if ((d & 0x80) == 0) break;
    d <<= 1;
    }

  /* Values of 0 or 6 indicate invalid UTF-8: leave c unchanged */

  if (i > 0 && i <= 6)
    {
    int j;
    int s = 6*i;
    c = (c & utf8_table3[i]) << s;
    for (j = 0; j < i; j++)
      {
      int cc = in_line[++chptr];
      if ((cc & 0xc0) != 0x80)     /* Bad UTF-8: restore ptr */
        {
        chptr = savechptr;
        c = in_line[chptr];        /* Restore single byte */
        break;
        }
      s -= 6;
      c |= (cc & 0x3f) << s;
      }
    }
  }

/* If enabled, translate certain ASCII character values when they occur as
single bytes (not if specified via an escape). */

else if (translate_chars) switch (c)
  {
  case '`':
  if (in_line[chptr+1] == '`')
    {
    chptr++;
    c = 0x201c;
    }
  else c = 0x2018;
  break;

  case '\'':
  if (in_line[chptr+1] == '\'')
    {
    chptr++;
    c = 0x201D;
    }
  else c = 0x2019;
  break;

  case '-':
  if (in_line[chptr+1] == '-')
    {
    chptr++;
    c = 0x2013;
    }
  break;

  default:
  break;
  }

return c;
}



/*************************************************
*             Read next word                     *
*************************************************/

/* The result is put in the global "word". If "wordread" is TRUE, there's
already a previous word that was not used. */

void
readword(void)
{
int n = 0;
if (!wordread)
  {
  while (isalpha((int)in_line[chptr]) || isdigit((int)in_line[chptr]))
    {
    if (n > WORD_SIZE - 2) { error_moan(36); exit(EXIT_FAILURE); }
    word[n++] = in_line[chptr++];
    }
  word[n] = 0;
  nextsigch();
  }
wordread = FALSE;
}



/*************************************************
*              Standardize word                  *
*************************************************/

/* Rather than fill up tables of commands and options with alternative
spellings, we have a single standardizing fuction. The change are:

. Change "gray" to "grey"
. Change "greyness" to "grey" (the old long form is deprecated)
. Change "color" to "colour"

We don't need to do more than one of each change as no command or option words
require more. The result is put in wordstd rather than modifying the original
so that the original can be used in error messages. As none of the recognized
words are longer than 20 characters, we don't bother with longer ones. This
guarantees there is always room to insert the "u" into "colour".

Note that "centre" does not occur in any commands or options so is not
considered here. There is only one test for it, later in this module. */

void
standardize_word(void)
{
uschar *p;
size_t len = Ustrlen(word);

memcpy(wordstd, word, len + 1);
if (len > 20) return;

p = Ustrstr(wordstd, "gray");
if (p != NULL) p[2] = 'e';

if (len >= 8 && Ustrcmp(wordstd + len - 8, "greyness") == 0)
  wordstd[len - 4] = 0;

p = Ustrstr(wordstd, "color");
if (p != NULL)
  {
  p += 4;
  memmove(p+1, p, len + 1 - (p - wordstd));
  *p = 'u';
  }
}



/*************************************************
*                Read integer                    *
*************************************************/

int
readint(void)
{
int n = 0;
int sign = 1;
if (in_line[chptr] == '-')
  {
  sign = -1;
  nextch();
  }
while (isdigit((int)in_line[chptr])) n = n * 10 + in_line[chptr++] - '0';
nextsigch();
return n * sign;
}



/*************************************************
*	     Read fixed point number		 *
*************************************************/

/* Afterwards, chptr points at the character immediately after the number on
the input line. */

int
readnumber(void)
{
int n = 0;
int sign = 1;

if (in_line[chptr] == '-')
  {
  sign = -1;
  nextch();
  }
else if (in_line[chptr] == '+') nextch();

while (isdigit((int)in_line[chptr])) n = n * 10 + in_line[chptr++] - '0';
n = n * 1000;
if (in_line[chptr] == '.')
  {
  int m = 100;
  while (isdigit((int)in_line[++chptr]))
    {
    n += (in_line[chptr] - '0') * m;
    m /= 10;
    }
  }
return n * sign;
}



/*************************************************
*                Read vector value               *
*************************************************/

/* Called when '(' has been read, in order to read two dimensions.

Arguments:
  xx          where to put the first dimension
  yy          where to put the second dimension

Returns:      nothing
*/

static void
readvector(int *xx, int *yy)
{
nextch();
nextsigch();
if (!isdigit((int)in_line[chptr]) && in_line[chptr] != '-')
  {
  error_moan(11, "Number");
  return;
  }
else
  {
  *xx = mag(readnumber());
  if (in_line[chptr] != ',')
    {
    error_moan(11, "Comma");
    return;
    }

  nextch();
  nextsigch();
  if (!isdigit((int)in_line[chptr]) && in_line[chptr] != '-') error_moan(11, "Number"); else
    {
    *yy = mag(readnumber());
    if (in_line[chptr] != ')')
      {
      error_moan(11, "Closing parenthesis");
      return;
      }
    }
  }
while (in_line[chptr] != ')' && !endfile) nextch();
nextch();
nextsigch();
}



/*************************************************
*       Convert character value to UTF-8         *
*************************************************/

/* This function takes an integer value in the range 0 - 0x7fffffff
and encodes it as a UTF-8 character in 0 to 6 bytes.

Arguments:
  cvalue     the character value
  buffer     pointer to buffer for result - at least 6 bytes long

Returns:     number of characters placed in the buffer
*/

static int
ord2utf8(int cvalue, uschar *buffer)
{
register int i, j;
for (i = 0; i < 6; i++) if (cvalue <= utf8_table1[i]) break;
buffer += i;
for (j = i; j > 0; j--)
 {
 *buffer-- = 0x80 | (cvalue & 0x3f);
 cvalue >>= 6;
 }
*buffer = utf8_table2[i] | cvalue;
return i + 1;
}



/*************************************************
*           Read string and its options          *
*************************************************/

/* The opening quote is in in_line[chptr] at the start.

Argument:  the default justification (just_left, just_right, just_centre)
Returns:   pointer to a stringchain item
*/

static stringchain *
readstring(int justify)
{
int rotate = UNSET;
int xadjust = 0;
int yadjust = 0;
int size = 0;
int chcount = 0;
int font = env->setfont;
int startchptr = chptr;
int endchptr;
BOOL needSymbol = FALSE;
BOOL needDingbats = FALSE;
colour rgb = env->textcolour;
uschar *s;
stringchain *t;

/* Scan for the end of the string, counting the number of bytes needed. We also
check for special font requirements. */

for (;;)
  {
  int c;

  /* A string must all be on one line. Assume " at EOL. */

  if ((c = nextUchar()) == '\n' || c == 0)
    {
    error_moan(21);
    break;
    }

  if (c == '\"' && in_line[++chptr] != '\"') break;

  /* Note: we can't just take the output size from the number of input bytes
  consumed because nextUchar() processes &xxx; characters and does some quote
  fudging. */

  if (c <= 0x7f) size++;
  else if (c <= 0x7ff) size += 2;
  else if (c <= 0xffff) size += 3;
  else if (c <= 0x1fffff) size += 4;

  /* These sizes are never found in modern Unicode. */
  /* LCOV_EXCL_START */
  else if (c <= 0x3ffffff) size += 5;
  else size += 6;
  /* LCOV_EXCL_STOP */

  chcount++;  /* Counts characters, not bytes */

  /* See if this character is in the Symbol or Dingbats font */

  if (c >= 384)
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
      if (mid->which == SF_SYMB) needSymbol = TRUE;
        else needDingbats = TRUE;
      }
    }
  }

/* Handle vector position adjustment */

if (in_line[chptr] == '(') readvector(&xadjust, &yadjust);

/* Handle other string options */

while (in_line[chptr] == '/')
  {
  int ch = in_line[++chptr];

  if (strchr("lrc", ch) != NULL &&
       (in_line[++chptr] == '/' ||
        in_line[chptr] == ';' ||
        isspace(in_line[chptr])))
    {
    justify = (ch == 'l')? just_left : (ch == 'r')? just_right : just_centre;
    }
  else if (ch == '+' || ch == '-')
    {
    rotate = readnumber();
    }
  else if (isdigit(ch))
    {
    BOOL hasfraction;
    int k = chptr + 1;

    while (isdigit((int)in_line[k])) k++;
    hasfraction = in_line[k] == '.';

    k = readnumber();
    if (!hasfraction && in_line[chptr] != ',')
      {
      font = k/1000;
      }

    else  /* Read r,g,b */
      {
      rgb.red = rgb.green = rgb.blue = k;
      if (in_line[chptr] == ',')
        {
        chptr++;
        rgb.green = readnumber();
        if (in_line[chptr] == ',')
          {
          chptr++;
          rgb.blue = readnumber();
          }
        }

      if (rgb.red > 1000 || rgb.green > 1000 || rgb.blue > 1000)
        error_moan(20);
      }
    }

  else error_moan(11, "/l, /r, /c, /<font>, /{+-}<rotate>, or /<r>,<g>,<b>");
  }

/* Ensure that the font has been bound. */

if (font_base != NULL)
  {
  bindfont *b;
  for (b = font_base; b != NULL; b = b->next)
    {
    if (font == b->number)
      {
      if (needSymbol) b->needSymbol = TRUE;
      if (needDingbats) b->needDingbats = TRUE;
      break;
      }
    }
  if (b == NULL)
    {
    error_moan(4, font);
    font = 0;
    }
  }

/* Now set up the text item. Convert all characters to UTF-8 format. */

t = getstore(size + 1 + offsetof(stringchain, text));

t->next = NULL;
t->justify = justify;
t->font = font;
t->rgb = rgb;
t->xadjust = xadjust;
t->yadjust = yadjust;
t->rotate = rotate;
t->chcount = chcount;

endchptr = chptr;
chptr = startchptr;

s = t->text;
for (;;)
  {
  int c;
  if ((c = nextUchar()) == 0) break;  /* Error already given */
  if (c == '\"') { if (in_line[++chptr] != '\"') break; }
  s += ord2utf8(c, s);
  }
*s = 0;

chptr = endchptr;
nextsigch();
strings_exist = TRUE;
return t;
}



/*************************************************
*          Read a chain of strings               *
*************************************************/

/* This function reads one or more strings enclosed in double quotes.

Arguments:
  p          item to which the chain is to be attached
  justify    default justification for strings

Returns:     nothing
*/

void
readstringchain(item *p, int justify)
{
stringchain *s = NULL;
int default_rotate = 0;

while (in_line[chptr] == '\"')
  {
  stringchain *ss = readstring(justify);
  if (ss->rotate == UNSET) ss->rotate = default_rotate;
    else default_rotate = ss->rotate;
  ss->rrotate = (double)(ss->rotate * pi) / 180000.0;
  if (s == NULL) p->strings = ss; else s->next = ss;
  s = ss;
  }
}



/*************************************************
*              Read join value                   *
*************************************************/

/* Various formats of join definition are supported.

Arguments:
  position      if TRUE, allow start/end/middle
  moanifnone    if TRUE, moan if no join direction found

Returns:        a join direction value (north, south, etc.)
*/

static int
readjoin(BOOL position, BOOL moanifnone)
{
int dir = -1;

readword();
if (Ustrcmp(word, "") == 0)
  {
  error_moan(11, "word");
  dir = north;   /* Stops missing complaint */
  }

else if (Ustrcmp(word, "top") == 0)
  {
  dir = north;
  readword();
  if (Ustrcmp(word, "right") == 0) dir = northeast;
    else if (Ustrcmp(word, "left") == 0) dir = northwest;
      else if (word[0]) wordread = TRUE;
  }

else if (Ustrcmp(word, "bottom") == 0)
  {
  dir = south;
  readword();
  if (Ustrcmp(word, "right") == 0) dir = southeast;
    else if (Ustrcmp(word, "left") == 0) dir = southwest;
      else if (word[0]) wordread = TRUE;
  }

else if (Ustrcmp(word, "left") == 0) dir = west;
else if (Ustrcmp(word, "right") == 0) dir = east;
else if (Ustrcmp(word, "centre") == 0 || Ustrcmp(word, "center") == 0)
  dir = centre;
else if (position)
  {
  if (Ustrcmp(word, "start") == 0) dir = start;
  else if (Ustrcmp(word, "end") == 0) dir = end;
  else if (Ustrcmp(word, "middle") == 0) dir = middle;
  }

if (dir < 0)
  {
  if (moanifnone)
    error_moan(11, "top, bottom, left, right, centre, start, end, or middle");
  else wordread = TRUE;
  }

return dir;
}



/*************************************************
*               Read position                    *
*************************************************/

/* A position may be specified in a number of different ways.

Arguments:
  xx             where to put the x coordinate
  yy             where to put the y coordinate

Returns:         TRUE if a position was successfully read
*/

static BOOL
readposition(int *xx, int *yy)
{
item *relative = baseitem;
item_box *rbox;
item_curve *rcurve;
item_line *rline;
item_arc *rarc;

int fraction = 0;
int dir;

*xx = *yy = 0;
if (in_line[chptr] == '(') { readvector(xx, yy); return TRUE; }

if (isdigit((int)in_line[chptr]))
  {
  fraction = readnumber();
  if (in_line[chptr] == '/')
    {
    nextch();
    if (!isdigit((int)in_line[chptr]))
      {
      error_moan(11, "number");
      return FALSE;
      }
    else fraction = (fraction * 1000)/readnumber();
    }
  nextsigch();
  }

/* Read and check for "top", "bottom", etc. */

dir = readjoin(TRUE, FALSE);

/* If dir = -1 then we haven't found such a word, but have not complained. See
if the word is a label, to support constructions such as "line right from A".
For such a thing, change the base item and return FALSE (to indicate not having
set an absolute position). */

if (dir < 0)
  {
  readword();
  relative = findlabel(word);
  if (relative != NULL) baseitem = relative;
    else error_moan(11, "top, bottom, left, right, centre, start, end, middle, or label");
  return FALSE;
  }

/* We have found a direction, expect optional "of <label>" */

readword();
if (Ustrcmp(word, "of") == 0)
  {
  readword();
  relative = findlabel(word);
  if (relative == NULL) { error_moan(10, word); return FALSE; }
  }
else if (word[0]) wordread = TRUE;

/* After "goto *", or if there have been no previous items, "relative" might be
unset here, which is an error. */

if (relative == NULL) { error_moan(12); return FALSE; }

/* Compute the join position on the relative item. Not all joins are valid. */

switch (relative->type)
  {
  case i_arc:
  rarc = (item_arc *)relative;
  switch(dir)
    {
    case north:
    case northeast:
    case east:
    case southeast:
    case south:
    case southwest:
    case west:
    case northwest:
    error_moan(13, "arc");
    break;

    case centre:
    *xx = rarc->x;
    *yy = rarc->y;
    break;

    case start:
    case end:
    case middle:
    if (dir == middle) fraction = 500;
    if (fraction)
      {
      double radius = ((double)(rarc->radius));
      double angle;
      if (dir == end) fraction = 1000 - fraction;
      angle = rarc->angle1 + (double)fraction * (rarc->angle2 - rarc->angle1) / 1000.0;
      *xx = rarc->x + (int )(radius * cos(angle));
      *yy = rarc->y + (int )(radius * sin(angle));
      fraction = 0;
      }
    else if (dir == start)
      {
      *xx = rarc->x0;
      *yy = rarc->y0;
      }
    else
      {
      *xx = rarc->x1;
      *yy = rarc->y1;
      }
    break;
    }
  break;

  case i_curve:
  rcurve = (item_curve *)relative;
  switch(dir)
    {
    case north:
    case northeast:
    case east:
    case southeast:
    case south:
    case southwest:
    case west:
    case northwest:
    case centre:
    error_moan(13, "curve");
    break;

    case start:
    case end:
    case middle:
    if (dir == middle) fraction = 500;
    if (fraction)
      {
      if (dir == end) fraction = 1000 - fraction;
      find_curvepos(rcurve, (double)fraction/1000.0, xx, yy);
      fraction = 0;
      }
    else if (dir == start)
      {
      *xx = rcurve->x0;
      *yy = rcurve->y0;
      }
    else
      {
      *xx = rcurve->x1;
      *yy = rcurve->y1;
      }
    break;
    }
  break;

  case i_box:
  rbox = (item_box *)relative;
  switch (dir)
    {
    case start:
    case end:
    case middle:
    error_moan(13, "box");
    break;

    case north:
    *xx = rbox->x;
    *yy = rbox->y + rbox->depth/2;
    if (fraction) *xx += ((fraction - 500)*rbox->width)/1000;
    fraction = 0;
    break;

    case northeast:
    if (rbox->boxtype == box_box)
      {
      *xx = rbox->x + rbox->width/2;
      *yy = rbox->y + rbox->depth/2;
      }
    else
      {
      *xx = rbox->x + (int )((double)((int )(rbox->width/2))*cos(0.25*pi));
      *yy = rbox->y + (int )((double)((int )(rbox->depth/2))*sin(0.25*pi));
      }
    break;

    case east:
    *xx = rbox->x + rbox->width/2;
    *yy = rbox->y;
    if (fraction) *yy += ((fraction - 500)*rbox->depth)/1000;
    fraction = 0;
    break;

    case southeast:
    if (rbox->boxtype == box_box)
      {
      *xx = rbox->x + rbox->width/2;
      *yy = rbox->y - rbox->depth/2;
      }
    else
      {
      *xx = rbox->x + (int )((double)((int )(rbox->width/2))*cos(-0.25*pi));
      *yy = rbox->y + (int )((double)((int )(rbox->depth/2))*sin(-0.25*pi));
      }
    break;

    case south:
    *xx = rbox->x;
    *yy = rbox->y - rbox->depth/2;
    if (fraction) *xx += ((fraction - 500)*rbox->width)/1000;
    fraction = 0;
    break;

    case southwest:
    if (rbox->boxtype == box_box)
      {
      *xx = rbox->x - rbox->width/2;
      *yy = rbox->y - rbox->depth/2;
      }
    else
      {
      *xx = rbox->x + (int )((double)((int )(rbox->width/2))*cos(1.25*pi));
      *yy = rbox->y + (int )((double)((int )(rbox->depth/2))*sin(1.25*pi));
      }
    break;

    case west:
    *xx = rbox->x - rbox->width/2;
    *yy = rbox->y;
    if (fraction) *yy += ((fraction - 500)*rbox->depth)/1000;
    fraction = 0;
    break;

    case northwest:
    if (rbox->boxtype == box_box)
      {
      *xx = rbox->x - rbox->width/2;
      *yy = rbox->y + rbox->depth/2;
      }
    else
      {
      *xx = rbox->x + (int )((double)((int )(rbox->width/2))*cos(0.75*pi));
      *yy = rbox->y + (int )((double)((int )(rbox->depth/2))*sin(0.75*pi));
      }
    break;

    case centre:
    *xx = rbox->x;
    *yy = rbox->y;
    break;
    }
  break;

  case i_line:
  rline = (item_line *)relative;
  switch (dir)
    {
    case north:
    case northeast:
    case east:
    case southeast:
    case south:
    case southwest:
    case west:
    case northwest:
    case centre:
    error_moan(13, "line");
    *xx = rline->x + rline->width;
    *yy = rline->y + rline->depth;
    break;

    case start:
    *xx = rline->x;
    *yy = rline->y;
    if (fraction )
      {
      *xx += (fraction * rline->width)/1000;
      *yy += (fraction * rline->depth)/1000;
      fraction = 0;
      }
    break;

    case end:
    *xx = rline->x + rline->width;
    *yy = rline->y + rline->depth;
    if (fraction)
      {
      *xx -= (fraction * rline->width)/1000;
      *yy -= (fraction * rline->depth)/1000;
      fraction = 0;
      }
    break;

    case middle:
    *xx = rline->x + rline->width/2;
    *yy = rline->y + rline->depth/2;
    break;
    }
  break;
  }

/* Give error is a fraction was applied inappropriately. */

if (fraction != 0) { error_moan(14); return FALSE; }

/* Deal with vector offsets */

readword();
if (Ustrcmp(word, "plus") == 0)
  {
  if (in_line[chptr] == '(')
    {
    int a, b;
    readvector(&a, &b);
    *xx += a;
    *yy += b;
    }
  else error_moan(11, "Parenthesized vector (x,y)");
  }
else if (word[0]) wordread = TRUE;

/* Indicate x and y set */

return TRUE;
}



/*************************************************
*            Handle optional parameters          *
*************************************************/

/* This function reads things that are optional, depending on the item being
read. The values are put into the item's block.

Arguments:
  p            the item block
  table        the relevant optional items

Returns:       nothing
*/

void
options(item *p, arg_item *table)
{
wordread = FALSE;

/* Loop for each options word */

while (wordread || isalpha((int)in_line[chptr]))
  {
  arg_item *pp = table;
  readword();
  standardize_word();   /* Allows for spelling variations */

  /* Search the list for this command */

  for (; (pp->name)[0] != 0; pp++)
    {
    if (Ustrcmp(wordstd, pp->name) == 0)  /* Compare standardized word */
      {
      /* Found the word; switch on its type */

      int arg1 = pp->arg1;
      int arg2 = pp->arg2;
      int type = pp->type;

      switch(type)
        {
        case opt_bool:     /* Boolean, no data. Set and/or unset flags */
	if (arg1 >= 0) *(int *)(((uschar *)p) + arg1) = TRUE;
	if (arg2 >= 0) *(int *)(((uschar *)p) + arg2) = FALSE;
        break;

        case opt_xline:    /* x or y distances for lines and arrows */
        case opt_xnline:   /* set first arg +/-; zero second arg if unset */
        case opt_yline:
        case opt_ynline:
          {
	  int value = (type == opt_xline || type == opt_xnline)? env->line_hw : env->line_vd;
	  int sign = (type == opt_xnline || type == opt_ynline)? (-1) : (+1);
          if (isdigit((int)in_line[chptr])) value = mag(readnumber());
	  if (arg1 >= 0) *(int *)(((uschar *)p) + arg1) = value * sign;
	  if (arg2 >= 0 && (*(int *)(((uschar *)p + arg2)) == UNSET))
	    *(int *)(((uschar *)p) + arg2) = 0;
          }
        break;

        case opt_dim:      /* single dimension, magnified */
	if (!isdigit((int)in_line[chptr])) error_moan(8); else *(int *)(((uschar *)p) + arg1) =
          mag(readnumber());
        break;

        case opt_angle:    /* single angle -- don't magnify! */
	if (!isdigit((int)in_line[chptr])) error_moan(11, "unsigned angle");
          else *(int *)(((uschar *)p) + arg1) = readnumber();
        break;

        case opt_grey:     /* grey level -- don't magnify! */
	if (!isdigit((int)in_line[chptr])) error_moan(11, "grey level");
          else
            {
            colour *c = (colour *)(((uschar *)p) + arg1);
            c->red = c->green = c->blue = readnumber();
            }
        break;

        case opt_colour:   /* colour rgb -- don't magnify! */
          {
          colour *c = (colour *)(((uschar *)p) + arg1);
          if (!isdigit((int)in_line[chptr]) && in_line[chptr] != '-')
            error_moan(11, "colour values");
          else
            {
            c->red = readnumber();
            if ((int)in_line[chptr] == ',')
              {
              chptr++;
              while (isspace((int)in_line[chptr])) chptr++;
              }
            if (!isdigit((int)in_line[chptr]))
              error_moan(11, "green and blue values");
            else
              {
              c->green = readnumber();
              if ((int)in_line[chptr] == ',')
                {
                chptr++;
                while (isspace((int)in_line[chptr])) chptr++;
                }
              if (!isdigit((int)in_line[chptr]))
                error_moan(11, "blue value");
              else c->blue = readnumber();
              }

            if (c->red > 1000 || c->green > 1000 || c->blue > 1000)
              error_moan(20);
            }
          }
        break;

        case opt_colgrey:   /* colour rgb or grey level -- don't magnify! */
          {
          colour *c = (colour *)(((uschar *)p) + arg1);
          if (!isdigit((int)in_line[chptr]) && in_line[chptr] != '-')
            error_moan(11, "grey level or colour values");
          else
            {
            c->red = c->green = c->blue = readnumber();

            if (in_line[chptr] == ',') chptr++;
            while (isspace((int)in_line[chptr])) chptr++;
            if (isdigit((int)in_line[chptr]))
              {
              c->green = readnumber();
              if (in_line[chptr] == ',') chptr++;
              while (isspace((int)in_line[chptr])) chptr++;

              if (!isdigit((int)in_line[chptr]))
                error_moan(11, "blue value");
              else c->blue = readnumber();
              }

            if (c->red > 1000 || c->green > 1000 || c->blue > 1000)
              error_moan(20);
            }
          }
        break;

        case opt_int:      /* integer, +ve or -ve */
        if (!isdigit((int)in_line[chptr]) && in_line[chptr] != '-')
          error_moan(11, "integer");
        else *(int *)(((uschar *)p) + arg1) = readint();
        break;

        case opt_at:       /* absolute position */
          {
	  int x, y;
          if (readposition(&x, &y))
            {
	    *(int *)(((uschar *)p) + arg1) = x;
	    *(int *)(((uschar *)p) + arg2) = y;
            }
          }
        break;

        case opt_dir:     /* direction, given by table value */
	*(int *)(((uschar *)p) + arg1) = (int)arg2;
        break;

	case opt_join:	  /* position specified by joining point */
          {
	  int newpoint = readjoin(FALSE, TRUE);
          if (newpoint >= 0)
            {
	    *(int *)(((uschar *)p) + arg1) = newpoint;
            readword();
            if (Ustrcmp(word, "to") == 0)
              {
              /* This yields FALSE if it just changes the base item */
              if (readposition(&joined_xx, &joined_yy))
                *(int *)(((uschar *)p) + arg2) = TRUE;
              }
            else if (word[0]) wordread = TRUE;
            if (baseitem == NULL) error_moan(22);
            }
          }
        break;
        }

      nextsigch();
      break;
      }
    }

  /* For unknown options, give a message and abandon the rest of this command.
  The error function skips to semicolon or newline, but this gets to the end of
  a multi-line coment. */

  if ((pp->name)[0] == 0)
    {
    error_moan(7, word);
    while (in_line[chptr] != ';' && !endfile) nextch();
    }
  }
}

/* End of rdsubs.c */
