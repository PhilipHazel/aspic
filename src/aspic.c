/*************************************************
*                      ASPIC                     *
*************************************************/

/* Copyright (c) University of Cambridge 1991 - 2023 */
/* Created: February 1991 */
/* Last modified: January 2023 */

/* ASPIC is an Amazingly Simple PICture composing program. It reads a
description of a line-art picture, and outputs commands for another program to
draw it. Aspic can output encapsulated PostScript (eps) or Scalable Vector
Graphics (svg).

This module contains the globals variables, main program, the error-handline
function, memory allocator, and some other commonly used functions. */


#include "aspic.h"



/*************************************************
*                 Global variables               *
*************************************************/

FILE *main_input;
FILE *out_file;

includestr *included_from = NULL;   /* chain for included files */
includestr *spare_included = NULL;  /* chain of spare blocks */
uschar **file_line_stack;           /* line stack for included files */
int *file_chptr_stack;              /* chptr stack ditto */
int inc_stack_ptr = 0;              /* stack position */
void *spare_lines = NULL;           /* chain of re-usable input lines */

item *main_item_base;               /* root of chain of drawing items */

double pi;
colour black;
colour unfilled;

bindfont *font_base = NULL;    /* chain of font bindings */
BOOL translate_chars = FALSE;  /* don't translate by default */

uschar **in_line_stack;        /* stack of pointers to saved in_lines */
int *chptr_stack;	       /* stack of saved chptrs */
int max_level = 0;             /* uppermost level used */
int min_level = 0;             /* lowermost level used */
int *mac_count_stack;          /* stack of macro invocation counts */
int mac_stack_ptr = 0;         /* the stack position */
int macro_count;	       /* for generating id's */
int macro_id;
int minimum_thickness = 0;     /* this is ok for EPS */
int outstyle = OUT_UNSET;      /* output style */
int resolution = 1;            /* default resolution is now exact */
int subs_ptr;                  /* offset for substitution errors */

int joined_xx;                 /* Coordinates of explicit join position */
int joined_yy;

macro *macroot = NULL;         /* root of all macros */
macro *macactive = NULL;       /* chain of active macros */
macro *spare_macros = NULL;    /* chain of re-usable macro blocks */

BOOL no_variables = FALSE;     /* variables are available by default */
BOOL reading = FALSE;          /* true while reading */
BOOL strings_exist = FALSE;    /* at least one item has a string */
BOOL substituting = FALSE;     /* true while substituting variables */
BOOL testing = FALSE;          /* suppress version in output */

tree_node *varroot = NULL;     /* root of variables tree */



/*************************************************
*               Static (local) variables         *
*************************************************/

static void *mem_anchor = NULL;
static void *mem_current = NULL;
static size_t mem_top = MEMORY_CHUNKSIZE;
static BOOL had_error = FALSE;
static int error_count = 0;



/*************************************************
*                Error messages                  *
*************************************************/

static uschar *error_messages[] = {
  US"Unrecognized command line option \"%s\"",              /* 0 */
  US"Failed to open %s for %s: %s",                         /* 1 */
  US"Unknown aspic command \"%s\"",                         /* 2 */
  US"Semicolon expected (unexpected text follows command)", /* 3 */
  US"Font %d has not been bound",                           /* 4 */
  US"Font number must be greater than 0",                   /* 5 */
  US"Unknown%s variable \"%s\"",                            /* 6 */
  US"Unknown option word \"%s\"",                           /* 7 */
  US"Dimension expected",                                   /* 8 */
  US"Label \"%s\" incorrectly placed (may only precede drawing command)", /* 9 */
  US"Can't find item labelled \"%s\"",                      /* 10 */
  US"%s expected",                                          /* 11 */
  US"No previous item",                                     /* 12 */
  US"Inappropriate position descriptor applied to a %s",    /* 13 */
  US"Inappropriate fraction encountered",                   /* 14 */
  US"Internal memory error: %d bytes requested (max %d)",   /* 15 */
  US"Command word expected - processing abandoned",         /* 16 */
  US"Empty variable name",                                  /* 17 */
  US"No stacked environment to restore",                    /* 18 */
  US"Too many constraints for arc",                         /* 19 */
  US"Grey level or RGB value must not be greater than 1.0", /* 20 */
  US"Closing quote missing; string terminated at end of line",  /* 21 */
  US"No previous item to join to",                          /* 22 */
  US"\"depth\" or \"via\" for arc given without end point", /* 23 */
  US"An arc cannot be constructed using the given via point", /* 24 */
  US"Line too long while substituting \"%s\" - processing abandoned", /* 25 */
  US"Line too long while substituting - processing abandoned",  /* 26 */
  US"Missing } after \"${%s\"",                             /* 27 */
  US"Only one of -[e]ps or -svg is allowed",                /* 28 */
  US"File name expected",                                   /* 29 */
  US"\"include\" is not allowed in a macro",                /* 30 */
  US"Memory allocation failure for malloc(%d)",             /* 31 */
  US"Call to atexit() failed",                              /* 32 */
  US"Missing \"to\" parameter for curve",                   /* 33 */
  US"Curve length %g is too short",                         /* 34 */
  US"Input line is too long (max %d) - processing abandoned", /* 35 */
  US"Word is too long - processing abandoned",              /* 36 */
  US"Duplicate label \"%s\"",                               /* 37 */
  US"Width/depth and an endpoint are mutually exclusive",   /* 38 */
  US"Macro name \"%s\" is not allowed - matches a command name",  /* 39 */
  US"End of file while reading macro \"%s\" - processing abandoned", /* 40 */
  US"Recursive macro call not allowed - processing abandoned", /* 41 */
  US"The \"align\" option is not valid for a sloping line", /* 42 */
  US"Variable name is too long in substitution",            /* 43 */ 
  };

#define ERROR_COUNT (sizeof(error_messages)/sizeof(char *))



/*************************************************
*                Error function                  *
*************************************************/

/* Error messages go to stderr, with information about the line at fault
if there is one.

Arguments:
  n            the error number
  ...          substitutions into the message

Returns:       nothing
*/

void
error_moan(int n, ...)
{
int  ptr;
va_list ap;
va_start(ap, n);

had_error = TRUE;

fprintf(stderr, "Aspic: ");
if (n >= (int)ERROR_COUNT) fprintf(stderr, "Unknown error number %d\n", n);
  else vfprintf(stderr, CS error_messages[n], ap);
fprintf(stderr, "\n");
va_end(ap);

/* If not in the reading phase, there is no input line to reflect */

if (reading)
  {
  if (substituting)
    {
    ptr = subs_ptr;
    fprintf(stderr, "%s", CS in_raw);
    if (in_raw[Ustrlen(in_raw)-1] != '\n') fprintf(stderr, "\n");
    }
  else if (chptr > 0)
    {
    ptr = chptr;
    fprintf(stderr, "%s", CS in_line);
    if (in_line[Ustrlen(in_line)-1] != '\n') fprintf(stderr, "\n");
    }
  else
    {
    ptr = Ustrlen(in_prev);
    fprintf(stderr, "%s", CS in_prev);
    if (in_prev[ptr-1] != '\n') fprintf(stderr, "\n");
    }

  for (int i = 0; i < ptr; i++) fprintf(stderr, " ");
  fprintf(stderr, "^\n");

  /* Except for substitution errors, skip to next semicolon or end of line in
  order to reduce error cascades. */

  if (!substituting)
    {
    BOOL instring = FALSE;
    for (int ch = in_line[chptr];
         ch != '\n' && ch != 0 && (instring || ch != ';');
         ch = in_line[++chptr])
      {
      if (ch == '"')
        {
        if (!instring) instring = TRUE;
          else if (in_line[chptr+1] != '"') instring = FALSE;
            else chptr++;
        }
      }
    }
  }

if (++error_count > MAX_ERRORS)
  {
  /* LCOV_EXCL_START */
  fprintf(stderr, "Aspic: Too many errors - processing abandoned\n");
  exit(EXIT_FAILURE);
  /* LCOV_EXCL_STOP */
  }
}



/*************************************************
*              Memory allocator                  *
*************************************************/

/* Small blocks are carved out of larger chunks. The size is rounded up to a
multiple of the pointer size, which should mean that each block is aligned for
any data type.

Argument:   size wanted
Returns:    pointer to the store
*/

void *
getstore(size_t size)
{
void *yield;
size_t available = MEMORY_CHUNKSIZE - mem_top;

size = (size + sizeof(char *) - 1);
size -= size % sizeof(char *);

if (size > MEMORY_CHUNKSIZE - sizeof(char *))
  {
  /* LCOV_EXCL_START */
  error_moan(15, size);
  exit(EXIT_FAILURE);
  /* LCOV_EXCL_STOP */
  }

if (available < size)
  {
  char *newblock = malloc(MEMORY_CHUNKSIZE);
  if (newblock == NULL)
    {
    /* LCOV_EXCL_START */
    error_moan(31, MEMORY_CHUNKSIZE);
    exit(EXIT_FAILURE);
    /* LCOV_EXCL_STOP */
    }
  *((void **)newblock) = mem_anchor;
  mem_anchor = newblock;
  mem_current = newblock;
  mem_top = sizeof(char *);
  }

yield = (char *)mem_current + mem_top;
mem_top += size;

return yield;
}



/*************************************************
*               Get a new input line             *
*************************************************/

/* Freed input lines (after macro calls) are saved on a chain for re-use. */

void *
get_in_line(void)
{
void *yield;
if (spare_lines == NULL) return getstore(INPUT_LINESIZE);
yield = spare_lines;
spare_lines = ((char **)spare_lines)[0];
return yield;
}



/*************************************************
*        Save a re-usable input line             *
*************************************************/

void
free_in_line(uschar *p)
{
((char **)p)[0] = spare_lines;
spare_lines = p;
}



/*************************************************
*          Get a new macro block                 *
*************************************************/

macro *
getmacro(void)
{
macro *yield;
if (spare_macros == NULL) return getstore(sizeof(macro));
yield = spare_macros;
spare_macros = yield->previous;
return yield;
}



/*************************************************
*           Save a re-usable macro block         *
*************************************************/

void
freemacro(macro *p)
{
p->previous = spare_macros;
spare_macros = p;
}



/*************************************************
*             Close all input files              *
*************************************************/

/* Called after reading is complete, or on premature exit. Close any currently
open included files and the original input unless it is stdin. */

static void
close_all_input(void)
{
while (included_from != NULL)
  {
  fclose(main_input);
  main_input = included_from->prevfile;
  included_from = included_from->prev;
  }
if (main_input != NULL && main_input != stdin)
  {
  fclose(main_input);
  main_input = NULL;
  }
}



/*************************************************
*              Exit tidy-up function             *
*************************************************/

/* Automatically called for any exit. Frees memory. */

static void
tidy_up(void)
{
void *p = mem_anchor;
close_all_input();
while (p != NULL)
  {
  void *q = p;
  p = (void *)(*((char **)p));
  free(q);
  }
mem_anchor = mem_current = NULL;  /*Tidiness */
}



/*************************************************
*         Set up the timestamp string            *
*************************************************/

/* This is a local function, used to initialize the $date variable.

Arguments:
  timebuf       where to put the output
  size          size of the buffer

Returns:        timebuf
*/

static uschar *
time_stamp(uschar *timebuf, int size)
{
int diff_hour, diff_min, len;
time_t now = time(NULL);
struct tm *gmt;
struct tm local;

memcpy(&local, localtime(&now), sizeof(struct tm));
gmt = gmtime(&now);

diff_min = 60*(local.tm_hour - gmt->tm_hour) + local.tm_min - gmt->tm_min;
if (local.tm_year != gmt->tm_year)
  diff_min += (local.tm_year > gmt->tm_year)? 1440 : -1440; /* LCOV_EXCL_LINE */
else if (local.tm_yday != gmt->tm_yday)
  diff_min += (local.tm_yday > gmt->tm_yday)? 1440 : -1440; /* LCOV_EXCL_LINE */
diff_hour = diff_min/60;
diff_min  = abs(diff_min - diff_hour*60);

len = Ustrftime(timebuf, size, "%a, ", &local);
(void) sprintf(CS timebuf + len, "%02d ", local.tm_mday);
len += Ustrlen(timebuf + len);
len += Ustrftime(timebuf + len, size - len, "%b %Y %H:%M:%S", &local);
(void) sprintf(CS timebuf + len, " %+03d%02d", diff_hour, diff_min);

return timebuf;
}



/*************************************************
*                 Usage                          *
*************************************************/

/* f is either stdout or stderr, depending on whether this is called by
-help or as a result of an error.

Arguments:
  f            file on which to write

Returns:       nothing
*/

static
void usage(FILE *f)
{
fprintf(f, "Usage: aspic [<options>] [<input> [<output>]]\n\n");
fprintf(f, "Options:\n");
fprintf(f, "  -[-]help       show usage information and exit\n");
fprintf(f, "  -nv            disable variable substitutions\n");
fprintf(f, "  -[e]ps         generate Encapsulated PostScript\n");
fprintf(f, "  -svg           generate SVG\n");
fprintf(f, "  -testing       used by 'make test'\n");
fprintf(f, "  -tr            translate quotes and double-hyphens\n");
fprintf(f, "  -v             show version and exit\n");
fprintf(f, "  -[-]version    show version and exit\n\n");

fprintf(f, "The default output format is Encapsulated PostScript.\n");
fprintf(f, "Only one of -[e]ps or -svg is permitted.\n");
fprintf(f, "Default output file is base <input> with .eps or .svg extension.\n");
fprintf(f, "Omit file names or use \"-\" for stdin and stdout.\n");
}



/*************************************************
*		    Entry point			 *
*************************************************/

int
main(int argc, char **argv)
{
tree_node *tn;
uschar timebuf[sizeof("www, dd-mmm-yyyy hh:mm:ss +zzzz")];
int firstarg = 1;       /* points after options */
BOOL input_is_stdin = FALSE;

if (atexit(tidy_up) != 0)
  {
  /* LCOV_EXCL_START */
  error_moan(32);
  exit(EXIT_FAILURE);
  /* LCOV_EXCL_STOP */
  }

/* Get memory for input lines */

in_raw  = get_in_line();
in_line = get_in_line();
in_prev = get_in_line();
in_prev[0] = 0;		/* to avoid junk in error messages */

/* Get memory for various stacks */

in_line_stack = getstore(MAC_STACKSIZE * sizeof(uschar *));
chptr_stack = getstore(MAC_STACKSIZE * sizeof(int));
mac_count_stack = getstore(MAC_STACKSIZE * sizeof(int));

file_line_stack = getstore(MAC_STACKSIZE * sizeof(uschar *));
file_chptr_stack = getstore(MAC_STACKSIZE * sizeof(int));

/* Some standard values */

pi = 4 * atan2(1.0, 1.0);
black.red = black.green = black.blue = 0;
unfilled.red = unfilled.green = unfilled.blue = -1000;

/* Handle command-line options */

while (firstarg < argc && argv[firstarg][0] == '-' && argv[firstarg][1] != 0)
  {
  uschar *arg = US argv[firstarg++];
  if (Ustrcmp(arg, "-nv") == 0)
    no_variables = TRUE;
  else if (Ustrcmp(arg, "-testing") == 0)
    testing = TRUE;
  else if (Ustrcmp(arg, "-ps") == 0 || Ustrcmp(arg, "-eps") == 0)
    { if (outstyle == OUT_UNSET) outstyle = OUT_EPS; else error_moan(28); }
  else if (Ustrcmp(arg, "-svg") == 0)
    { if (outstyle == OUT_UNSET) outstyle = OUT_SVG; else error_moan(28); }
  else if (Ustrcmp(arg, "-tr") == 0)
    translate_chars = TRUE;
  else if (Ustrcmp(arg, "-v") == 0 || Ustrcmp(arg, "-version") == 0 ||
           Ustrcmp(arg, "--version") == 0)
    {
    printf("\rAspic %s\n", testing? "" : Version_String);
    exit(EXIT_SUCCESS);
    }
  else if (Ustrcmp(arg, "-help") == 0 || Ustrcmp(arg, "--help") == 0)
    {
    printf("\rAspic %s\n", testing? "": Version_String);
    usage(stdout);
    exit(EXIT_SUCCESS);
    }
  else
    {
    error_moan(0, arg);
    usage(stderr);
    exit(EXIT_FAILURE);
    }
  }

/* Default output style is EPS */

if (outstyle == OUT_UNSET) outstyle = OUT_EPS;

/* If no file name is given, or it is "-", read the standard input. Otherwise,
try to open the input file. */

if (firstarg >= argc || Ustrcmp(argv[firstarg], "-") == 0)
  {
  main_input = stdin;
  input_is_stdin = TRUE;
  }
else
  {
  if ((main_input = fopen(argv[firstarg], "r")) == NULL)
    {
    error_moan(1, argv[firstarg], "input", strerror(errno));
    exit(EXIT_FAILURE);
    }
  }

/* Set up some default value for certain conventional variables. Put the data
in malloc memory so it can be freed just like other variables. */

tn = getstore(sizeof(tree_node) + 7);
Ustrcpy(tn->name, "creator");
tn->value = getstore(8);
Ustrcpy(tn->value, "Unknown");
(void)tree_insertnode(&varroot, tn);

tn = getstore(sizeof(tree_node) + 4);
Ustrcpy(tn->name, "date");
tn->value = getstore(sizeof(timebuf) + 1);
Ustrcpy(tn->value, time_stamp(timebuf, sizeof(timebuf)));
(void)tree_insertnode(&varroot, tn);

tn = getstore(sizeof(tree_node) + 5);
Ustrcpy(tn->name, "title");
tn->value = getstore(8);
Ustrcpy(tn->value, "Unknown");
(void)tree_insertnode(&varroot, tn);

/* Initialization that depends on the output style */

switch (outstyle)
  {
  case OUT_EPS: init_ps(); break;
  case OUT_SVG: init_sv(); break;
  }

/* Process the input and then write the output if successful. */

reading = TRUE;
read_inputfile();
reading = FALSE;
close_all_input();

if (had_error)
  {
  fprintf(stderr, "Aspic: No output generated\n");
  }
else
  {
  char *outname = NULL;
  char outnamebuff[256];

  /* If there is no output file name, and input is not stdin, create a name by
  adjusting the extension. */

  if (firstarg + 1 >= argc)
    {
    if (!input_is_stdin)
      {
      char *dot;
      outname = outnamebuff;
      strcpy(outnamebuff, argv[firstarg]);
      dot = strrchr(outnamebuff, '.');
      if (dot == NULL) dot = outnamebuff + strlen(outnamebuff);
      switch (outstyle)
        {
        case OUT_EPS: strcpy(dot, ".eps"); break;
        case OUT_SVG: strcpy(dot, ".svg"); break;
        }
      }
    }

  /* If there is an argument other than "-", set it as the output file name */

  else if (Ustrcmp(argv[firstarg + 1], "-") != 0)
    outname = argv[firstarg + 1];

  /* If we have an output file name, try to open it; otherwise output is to the
  standard output. */

  if (outname != NULL)
    {
    if ((out_file = fopen(outname, "w")) == NULL)
      {
      error_moan(1, outname, "output", strerror(errno));
      exit(EXIT_FAILURE);
      }
    }
  else out_file = stdout;

  /* Generate output of the appropriate type */

  switch(outstyle)
    {
    case OUT_EPS: write_ps(); break;
    case OUT_SVG: write_sv(); break;
    }

  if (out_file != stdout) fclose(out_file);
  }

return had_error? EXIT_FAILURE : EXIT_SUCCESS;
}

/* End of aspic.c */
