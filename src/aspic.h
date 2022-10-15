/*************************************************
*                       ASPIC                    *
*************************************************/

/* Copyright (c) University of Cambridge 1991 - 2022 */
/* Created: February 1991 */
/* Last modified: October 2022 */

/* General header file used by all modules */

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "mytypes.h"

/* Miscellaneous defines */

#define Version_String "2.00 (11-October-2022)"

#define UNSET INT_MAX          /* For unset parameters */
#define MEMORY_CHUNKSIZE 4096
#define INPUT_LINESIZE 256
#define MAC_STACKSIZE 20       /* Macro stacksize */
#define MAX_ERRORS 100
#define WORD_SIZE 256

/* Macro to apply magnification to a dimension */

#define mag(x) ((x * env->magnification)/1000)

/* Macro to get the next UTF-8 character, advancing the pointer. */

#define GETCHARINC(c, ptr) \
  c = *ptr++; \
  if ((c & 0xc0) == 0xc0) \
    { \
    int gcaa = utf8_table4[c & 0x3f];  /* Number of additional bytes */ \
    int gcss = 6*gcaa; \
    c = (c & utf8_table3[gcaa]) << gcss; \
    while (gcaa-- > 0) \
      { \
      gcss -= 6; \
      c |= (*ptr++ & 0x3f) << gcss; \
      } \
    }



/*************************************************
*             Enumerations                       *
*************************************************/

/* Output types */

enum { OUT_UNSET, OUT_EPS, OUT_SVG };

/* Item types - box is also used for circles and ellipses */

enum { i_arc, i_box, i_curve, i_line, i_text };
enum { box_box, box_circle, box_ellipse };

/* Item "style"s */

enum { is_norm, is_invi };

/* Text justifications */

enum { just_left, just_right, just_centre };

/* Directions and positions */

enum { north, northeast, east, southeast, south, southwest, west, northwest,
       centre, start, end, middle, unset_dirpos };

/* Types of optional argument */

enum { opt_bool,            /* no data; set first arg; unset second arg */
       opt_xline,           /* one x dimension; set first arg; zero second arg if UNSET */
       opt_xnline,          /* one x dimension; set first arg negative; zero second arg if UNSET */
       opt_yline,           /* one y dimension; set first arg; zero second arg if UNSET */
       opt_ynline,          /* one y dimension; set first arg negative; zero second arg if UNSET */
       opt_dim,             /* one dimension; second arg ignored */
       opt_colour,          /* colour */
       opt_colgrey,         /* colour or grey level */
       opt_grey,            /* grey level */
       opt_at,              /* absolute position */
       opt_join,	    /* position specified by joining point */
       opt_angle,           /* one angle */
       opt_int,             /* integer */
       opt_dir              /* no data; sets direction from table data */
};

/* Values in the special fonts characters table (Symbol or Dingbats) */

enum { SF_SYMB, SF_DBAT };



/*************************************************
*              Structures                        *
*************************************************/

/* Structure for included files */

typedef struct includestr {
  struct includestr *prev;
  FILE *prevfile;
} includestr;

/* Structure of command table entries */

typedef struct {
  uschar *name;
  void (*function)(void);
  int arg1;
  int arg2;
} command_item;

/* Structure of entry in list of arguments per command */

typedef struct {
  uschar *name;
  int type;
  int arg1;
  int arg2;
} arg_item;

/* Structure to hold a colour definition */

typedef struct colour {
  int red;
  int green;
  int blue;
} colour;

/* Entry in string chain */

typedef struct stringchain {
  struct stringchain *next;
  double rrotate;     /* Rotation in radians */
  int rotate;         /* Rotation in degrees */
  int xadjust;
  int yadjust;
  int justify;
  int font;
  int chcount;        /* Count of chars, not bytes */
  colour rgb;
  uschar text[1];     /* Variable length text */
} stringchain;

/* Structure for holding a bindfont request */

typedef struct bindfont {
  struct bindfont *next;
  int number;
  int size;
  BOOL needSymbol;
  BOOL needDingbats;
  uschar name[1];     /* Variable length font neme */
} bindfont;

/* Structure for the built-in list of named entities */

typedef struct entity_block {
  uschar *name;
  int value;
} entity_block;

/* Structures for macro handling */

typedef struct mac_line {
  struct mac_line *next;
  uschar text[1];     /* Variable length line */
} mac_line;

typedef struct mac_arg {
  struct mac_arg *next;
  uschar text[1];
} mac_arg;

typedef struct macro {
  struct macro *previous;
  uschar name[20];
  mac_line *nextline;
  int argcount;
  mac_arg *args;
} macro;

/* Environment variables are held in a structure for eash stacking. */

typedef struct environment {
  struct environment *previous;  /* link for stacking */
  int arcradius;       /* default arc radius */
  int arrow_x;         /* x distance for arrowhead */
  int arrow_y;         /* y distance for arrowhead */
  int boxdash1;        /* box dash parameters */
  int boxdash2;
  int boxwidth;        /* box width */
  int boxthickness;    /* thickness of edge of boxes */
  int boxdepth;        /* box depth */
  colour boxcolour;    /* colour of box edges */
  colour boxfilled;    /* colour of box filling */
  int cirdash1;        /* circle dash parameters */
  int cirdash2;
  colour circolour;    /* colour of circle edges */
  int cirradius;       /* default circle radius */
  int cirthickness;    /* default thickness of circle edges */
  colour cirfilled;    /* colour of circle filling */
  int direction;       /* default direction */
  int elldash1;        /* ellipse dash parameters */
  int elldash2;
  colour ellcolour;    /* ellipse edge colour */
  int ellwidth;        /* ellipse width */
  int elldepth;        /* ellipse depth */
  int ellthickness;    /* ellipse edge thickness */
  colour ellfilled;    /* colour of ellipse filling */
  colour shapefilled;  /* filled colour for shapes */
  colour arrowfilled;  /* filled colour for arrowheads */
  int fontdepth;       /* text font depth */
  int setfont;         /* font number */
  colour textcolour;   /* text colour */
  int level;           /* drawing level */
  int linedash1;       /* line dash parameters */
  int linedash2;
  int linedepth;       /* text linedepth */
  int linethickness;   /* thickness of lines */
  colour linecolour;   /* colour of lines */
  int line_hw;         /* default horizontal line width */
  int line_vd;         /* default vertical line depth */
  int magnification;   /* magnification */
} environment;

/* Use a macro for the common header for aspic "items" */

#define itemhdr \
  struct item *next; \
  stringchain *strings; \
  int level; \
  int type; \
  int style; \
  int dash1; \
  int dash2; \
  int linedepth; \
  int fontdepth; \
  int thickness; \
  colour colour; \
  colour shapefilled; \
  int x; \
  int y

/* Generic item structure */

typedef struct item {
  itemhdr;
} item;

/* Structure of label chain */

typedef struct label {
  struct label *next;
  item *itemptr;
  uschar name[20];
} label;

/* Arc item */

typedef struct {
  itemhdr;
  double angle1;
  double angle2;
  int direction;
  int radius;
  int angle;
  int cw;
  int depth;
  int via_x;
  int via_y;
  int arrow_start;
  int arrow_end;
  int arrow_x;
  int arrow_y;
  colour arrow_filled;
  int x0, y0, x1, y1;
} item_arc;

/* Box item (boxtype => box, circle, ellipse) */

typedef struct {
  itemhdr;
  int boxtype;
  int width;
  int depth;
  int joinpoint;
  int pointjoined;
} item_box;

/* Curve item */

typedef struct {
  itemhdr;
  int cw;
  int wavy;
  int x0, y0, x1, y1;
  int cx1, cy1, cx2, cy2;
  int cxs, cys;
} item_curve;

/* Line item */

typedef struct {
  itemhdr;
  int width;
  int depth;
  int endx;
  int endy;
  int alignx;
  int aligny;
  int arrow_start;
  int arrow_end;
  int arrow_x;
  int arrow_y;
  colour arrow_filled;
} item_line;

/* Text item */

typedef struct {
  itemhdr;
} item_text;

/* Tree nodes for variables and macros */

typedef struct tree_node {
  struct tree_node *left;         /* pointer to left child */
  struct tree_node *right;        /* pointer to right child */
  uschar *value;                  /* value of the node */
  uschar balance;                 /* balancing factor */
  uschar name[1];                 /* node name - variable length */
} tree_node;

/* Entries in the Unicode to special font char table */

typedef struct u2sencod {
  int ucode;                   /* Unicode code point */
  int which;                   /* Which of the special fonts */
  int scode;                   /* Code point in the special font */
} u2sencod;



/*************************************************
*                  Global variables              *
*************************************************/

extern FILE   *main_input;        /* source input file */
extern FILE   *out_file;          /* output file */
extern item_box *drawbbox;        /* box item for bounding box */

extern includestr *included_from; /* chain for included files */
extern includestr *spare_included;/* chain of spare blocks */
extern uschar **file_line_stack;  /* saved lines for included files */
extern int    *file_chptr_stack;  /* saved chptrs ditto */
extern int    inc_stack_ptr;      /* stack position */
extern void   *spare_lines;       /* chain of re-usable input lines */

extern item   *main_item_base;    /* base of chain of items */
extern item   *lastitem;          /* last on list of items read */
extern item   *baseitem;          /* item to base next item on */

extern label  *label_base;        /* base of chain of labels */
extern label  *nextlabel;         /* next label item */
extern environment *env;          /* input environment */

extern bindfont *font_base;       /* base of chain of font bindings */
extern BOOL   translate_chars;    /* TRUE to translate quotes and dash */

extern double pi;                 /* PI */

extern colour black;              /* For easy setting colours to black */
extern colour unfilled;           /* An "impossible" colour */

extern int    chptr;		  /* offset to next char */
extern int    drawbboxoffset;	  /* draw bounding box offset */
extern BOOL   endfile;		  /* TRUE when EOF reached */
extern int    item_arg1;	  /* parameter 1 for items */
extern int    item_arg2;	  /* parameter 2 for items */
extern int    joined_xx;          /* explicit join point */
extern int    joined_yy;
extern int    max_level;          /* uppermost level used */
extern int    min_level;          /* lowermost level used */
extern int    macro_count;        /* count of executed macros */
extern int    macro_id;		  /* this macro's id */
extern int    minimum_thickness;  /* minimum line thickness */
extern BOOL   no_variables;       /* disable variables */
extern int    resolution;         /* resolution of output device */
extern BOOL   strings_exist;      /* at least one item has a string */
extern int    subs_ptr;           /* error offset in raw buffer */

extern uschar word[];             /* next word in input */
extern uschar wordstd[];          /* ...with standardized spelling */

extern uschar *in_line;           /* current input line */
extern uschar *in_prev;           /* previous input line */
extern uschar *in_raw;            /* raw input line */

extern uschar **in_line_stack;    /* stack of pointers to saved in_lines */
extern int    *chptr_stack;       /* stack of saved chptrs */
extern int    mac_stack_ptr;      /* the stack position */
extern int    *mac_count_stack;   /* stack current count */

extern macro  *macroot;           /* root of all macros */
extern macro  *macactive;         /* chain of active macros */
extern macro  *spare_macros;      /* chain of re-usable macro blocks */

extern int    outstyle;           /* output style */
extern BOOL   reading;            /* TRUE while reading input */
extern BOOL   substituting;       /* TRUE while substituting variables */
extern BOOL   testing;            /* set when running tests */

extern tree_node *varroot;        /* variables root */

/* UTF-8 tables */

extern const int     utf8_table1[];
extern const int     utf8_table2[];
extern const int     utf8_table3[];
extern const uschar  utf8_table4[];

/* Entity list */

extern entity_block entity_list[];
extern int entity_list_count;

/* Character tables */

extern int nonulist[];
extern int nonucount;
extern int u2scount;
extern u2sencod u2slist[];



/*************************************************
*               Global functions                 *
*************************************************/

void c_arc(void);
void c_box(void);
void c_circle(void);
void c_curve(void);
void c_line(void);

void chain_label(item *);
void error_moan(int, ...);
void find_bbox(int  *);
uschar *fixed(int );
int  find_fontdepth(item *, stringchain *);
int  find_linedepth(item *, stringchain *);
item *findlabel(uschar *);
void find_curvepos(item_curve *, double, int *, int *);
void freechain(void);
void freemacro(macro *);
void free_in_line(uschar *);
macro *getmacro(void);
void *getstore(size_t);
void *get_in_line(void);
void init_ps(void);
void init_sv(void);
void nextch(void);
void nextsigch(void);
void options(item *, arg_item *);
int  readint(void);
int  readnumber(void);
void readstringchain(item *, int);
void readword(void);
int  read_conf_file(uschar *);
void read_inputfile(void);
int  rnd(int );
BOOL samecolour(colour, colour);
void smallarc(int, int, double, double, void (*)(int, int, int, int, int, int));
void standardize_word(void);
void stringpos(item *, int  *, int  *);
int  tree_insertnode(tree_node **, tree_node *);
tree_node *tree_search(tree_node *, uschar *);
void write_ps(void);
void write_sv(void);

/* End of aspic.h */
