/*************************************************
*                      ASPIC                     *
*************************************************/
 
/* Copyright (c) University of Cambridge 1991 - 2008 */
/* Created: February 1991 */
/* Last modified: February 2008 */
 
 
 
#include "aspic.h"
 
 

/*************************************************
*              UTF-8 tables                      *
*************************************************/

/* These are the breakpoints for different numbers of bytes in a UTF-8
character. */

const int utf8_table1[] =
  { 0x7f, 0x7ff, 0xffff, 0x1fffff, 0x3ffffff, 0x7fffffff};

/* These are the indicator bits and the mask for the data bits to set in the
first byte of a character, indexed by the number of additional bytes. */

const int utf8_table2[] = { 0,    0xc0, 0xe0, 0xf0, 0xf8, 0xfc};
const int utf8_table3[] = { 0xff, 0x1f, 0x0f, 0x07, 0x03, 0x01};

/* Table of the number of extra characters, indexed by the first character
masked with 0x3f. The highest number for a valid UTF-8 character is in fact
0x3d. */

const uschar utf8_table4[] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5 };



/*************************************************
*            Tables of named entities            *
*************************************************/

/* This table must be in collating sequence of entity name, because it is
searched by binary chop. */

entity_block entity_list[] = {
  { US"AElig",     0x00c6 },
  { US"Aacute",    0x00c1 },
  { US"Abreve",    0x0102 },
  { US"Acirc",     0x00c2 },
  { US"Agrave",    0x00c0 },
  { US"Amacr",     0x0100 },
  { US"Aogon",     0x0104 },
  { US"Aring",     0x00c5 },
  { US"Atilde",    0x00c3 },
  { US"Auml",      0x00c4 },
  { US"Cacute",    0x0106 },
  { US"Ccaron",    0x010c },
  { US"Ccedil",    0x00c7 },
  { US"Ccirc",     0x0108 },
  { US"Cdot",      0x010a },
  { US"Dagger",    0x2021 },
  { US"Dcaron",    0x010e },
  { US"Dstrok",    0x0110 },
  { US"ENG",       0x014a },
  { US"ETH",       0x00d0 },
  { US"Eacute",    0x00c9 },
  { US"Ecaron",    0x011a },
  { US"Ecirc",     0x00ca },
  { US"Edot",      0x0116 },
  { US"Egrave",    0x00c8 },
  { US"Emacr",     0x0112 },
  { US"Eogon",     0x0118 },
  { US"Euml",      0x00cb },
  { US"Euro",      0x20ac },
  { US"Gbreve",    0x011e },
  { US"Gcedil",    0x0122 },
  { US"Gcirc",     0x011c },
  { US"Gdot",      0x0120 },
  { US"Hcirc",     0x0124 },
  { US"Hstrok",    0x0126 },
  { US"IJlig",     0x0132 },
  { US"Iacute",    0x00cd },
  { US"Icirc",     0x00ce },
  { US"Idot",      0x0130 },
  { US"Igrave",    0x00cc },
  { US"Imacr",     0x012a },
  { US"Iogon",     0x012e },
  { US"Itilde",    0x0128 },
  { US"Iuml",      0x00cf },
  { US"Jcirc",     0x0134 },
  { US"Kcedil",    0x0136 },
  { US"Lacute",    0x0139 },
  { US"Lcaron",    0x013d },
  { US"Lcedil",    0x013b },
  { US"Lmidot",    0x013f },
  { US"Lstrok",    0x0141 },
  { US"Nacute",    0x0143 },
  { US"Ncaron",    0x0147 },
  { US"Ncedil",    0x0145 },
  { US"Ntilde",    0x00d1 },
  { US"OElig",     0x0152 },
  { US"Oacute",    0x00d3 },
  { US"Ocirc",     0x00d4 },
  { US"Odblac",    0x0150 },
  { US"Ograve",    0x00d2 },
  { US"Omacr",     0x014c },
  { US"Oslash",    0x00d8 },
  { US"Otilde",    0x00d5 },
  { US"Ouml",      0x00d6 },
  { US"Racute",    0x0154 },
  { US"Rcaron",    0x0158 },
  { US"Rcedil",    0x0156 },
  { US"Sacute",    0x015a },
  { US"Scaron",    0x0160 },
  { US"Scedil",    0x015e },
  { US"Scirc",     0x015c },
  { US"THORN",     0x00de },
  { US"Tcaron",    0x0164 },
  { US"Tcedil",    0x0162 },
  { US"Tstrok",    0x0166 },
  { US"Uacute",    0x00da },
  { US"Ubreve",    0x016c },
  { US"Ucirc",     0x00db },
  { US"Udblac",    0x0170 },
  { US"Ugrave",    0x00d9 },
  { US"Umacr",     0x016a },
  { US"Uogon",     0x0172 },
  { US"Uring",     0x016e },
  { US"Utilde",    0x0168 },
  { US"Uuml",      0x00dc },
  { US"Wcirc",     0x0174 },
  { US"Yacute",    0x00dd },
  { US"Ycirc",     0x0176 },
  { US"Yuml",      0x0178 },
  { US"Zacute",    0x0179 },
  { US"Zcaron",    0x017d },
  { US"Zdot",      0x017b },
  { US"aacute",    0x00e1 },
  { US"abreve",    0x0103 },
  { US"acirc",     0x00e2 },
  { US"aelig",     0x00e6 },
  { US"agrave",    0x00e0 },
  { US"amacr",     0x0101 },
  { US"amp",       0x0026 },
  { US"aogon",     0x0105 },
  { US"aring",     0x00e5 },
  { US"atilde",    0x00e3 },
  { US"auml",      0x00e4 },
  { US"blank",     0x2423 }, 
  { US"blk12",     0x2592 }, 
  { US"blk14",     0x2591 }, 
  { US"blk34",     0x2593 }, 
  { US"block",     0x2588 }, 
  { US"brvbar",    0x00a6 },
  { US"bull",      0x2022 }, 
  { US"cacute",    0x0107 },
  { US"caret",     0x2041 }, 
  { US"ccaron",    0x010d },
  { US"ccedil",    0x00e7 },
  { US"ccirc",     0x0109 },
  { US"cdot",      0x010b },
  { US"cent",      0x00a2 },
  { US"check",     0x2713 },
  { US"cir",       0x25cb }, 
  { US"clubs",     0x2663 },
  { US"copy",      0x00a9 },
  { US"copysr",    0x2117 }, 
  { US"cross",     0x2717 },
  { US"curren",    0x00a4 },
  { US"dagger",    0x2020 },
  { US"darr",      0x2193 },
  { US"dash",      0x2010 }, 
  { US"dcaron",    0x010f },
  { US"deg",       0x00b0 },
  { US"diams",     0x2666 },
  { US"divide",    0x00f7 },
  { US"dlcrop",    0x230d }, 
  { US"drcrop",    0x230c }, 
  { US"dstrok",    0x0111 },
  { US"dtri",      0x25bf }, 
  { US"dtrif",     0x25be }, 
  { US"eacute",    0x00e9 },
  { US"ecaron",    0x011b },
  { US"ecirc",     0x00ea },
  { US"edot",      0x0117 },
  { US"egrave",    0x00e8 },
  { US"emacr",     0x0113 },
  { US"eng",       0x014b },
  { US"eogon",     0x0119 },
  { US"eth",       0x00f0 },
  { US"euml",      0x00eb },
  { US"female",    0x2640 }, 
  { US"ffilig",    0xfb03 }, 
  { US"fflig",     0xfb00 }, 
  { US"ffllig",    0xfb04 }, 
  { US"filig",     0xfb01 },
  { US"flat",      0x266d }, 
  { US"fllig",     0xfb02 },
  { US"frac12",    0x00bd },
  { US"frac13",    0x2153 }, 
  { US"frac14",    0x00bc },
  { US"frac15",    0x2155 }, 
  { US"frac16",    0x2159 }, 
  { US"frac18",    0x215b }, 
  { US"frac23",    0x2154 }, 
  { US"frac25",    0x2156 }, 
  { US"frac34",    0x00be },
  { US"frac35",    0x2157 }, 
  { US"frac38",    0x215c }, 
  { US"frac45",    0x2158 }, 
  { US"frac56",    0x215a }, 
  { US"frac58",    0x215d }, 
  { US"frac78",    0x215e }, 
  { US"gacute",    0x01f5 }, 
  { US"gbreve",    0x011f },
  { US"gcirc",     0x011d },
  { US"gdot",      0x0121 },
  { US"gt",        0x003e },
  { US"half",      0x00bd },
  { US"hcirc",     0x0125 },
  { US"hearts",    0x2665 },
  { US"hellip",    0x2026 },
  { US"horbar",    0x2015 }, 
  { US"hstrok",    0x0127 },
  { US"hybull",    0x2043 }, 
  { US"iacute",    0x00ed },
  { US"icirc",     0x00ee },
  { US"iexcl",     0x00a1 },
  { US"igrave",    0x00ec },
  { US"ijlig",     0x0133 },
  { US"imacr",     0x012b },
  { US"incare",    0x2105 }, 
  { US"inodot",    0x0131 },
  { US"iogon",     0x012f },
  { US"iquest",    0x00bf },
  { US"itilde",    0x0129 },
  { US"iuml",      0x00ef },
  { US"jcirc",     0x0135 },
  { US"kcedil",    0x0137 },
  { US"kgreen",    0x0138 },
  { US"lacute",    0x013a },
  { US"laquo",     0x00ab },
  { US"larr",      0x2190 },
  { US"lcaron",    0x013e },
  { US"lcedil",    0x013c },
  { US"ldquo",     0x201c },
  { US"ldquor",    0x201e },
  { US"lhblk",     0x2584 }, 
  { US"lmidot",    0x0140 },
  { US"loz",       0x25ca },
  { US"lsquo",     0x2018 },
  { US"lsquor",    0x201a },
  { US"lstrok",    0x0142 },
  { US"lt",        0x003c },
  { US"ltri",      0x25c3 }, 
  { US"ltrif",     0x25c2 }, 
  { US"male",      0x2642 }, 
  { US"malt",      0x2720 },
  { US"marker",    0x25ae }, 
  { US"mdash",     0x2014 },
  { US"micro",     0x00b5 },
  { US"middot",    0x00b7 },
  { US"mldr",      0x2026 },
  { US"nacute",    0x0144 },
  { US"napos",     0x0149 },
  { US"natur",     0x266e }, 
  { US"nbsp",      0x00a0 },
  { US"ncaron",    0x0148 },
  { US"ncedil",    0x0146 },
  { US"ndash",     0x2013 },
  { US"nldr",      0x2025 }, 
  { US"not",       0x00ac },
  { US"ntilde",    0x00f1 },
  { US"oacute",    0x00f3 },
  { US"ocirc",     0x00f4 },
  { US"odblac",    0x0151 },
  { US"oelig",     0x0153 },
  { US"ograve",    0x00f2 },
  { US"ohm",       0x2126 }, 
  { US"omacr",     0x014d },
  { US"ordf",      0x00aa },
  { US"ordm",      0x00ba },
  { US"oslash",    0x00f8 },
  { US"otilde",    0x00f5 },
  { US"ouml",      0x00f6 },
  { US"para",      0x00b6 },
  { US"phone",     0x260e },
  { US"plusmn",    0x00b1 },
  { US"pound",     0x00a3 },
  { US"racute",    0x0155 },
  { US"raquo",     0x00bb },
  { US"rarr",      0x2192 },
  { US"rcaron",    0x0159 },
  { US"rcedil",    0x0157 },
  { US"rdquo",     0x201d },
  { US"rdquor",    0x201d },
  { US"rect",      0x25ad }, 
  { US"reg",       0x00ae },
  { US"rsquo",     0x2019 },
  { US"rsquor",    0x2019 },
  { US"rtri",      0x25b9 }, 
  { US"rtrif",     0x25b8 }, 
  { US"rx",        0x211e }, 
  { US"sacute",    0x015b },
  { US"scaron",    0x0161 },
  { US"scedil",    0x015f },
  { US"scirc",     0x015d },
  { US"sect",      0x00a7 },
  { US"sext",      0x2736 },
  { US"sharp",     0x266f }, 
  { US"shy",       0x00ad },
  { US"spades",    0x2660 },
  { US"squ",       0x25a1 }, 
  { US"squf",      0x25aa }, 
  { US"sup1",      0x00b9 },
  { US"sup2",      0x00b2 },
  { US"sup3",      0x00b3 },
  { US"szlig",     0x00df },
  { US"target",    0x2316 }, 
  { US"tcaron",    0x0165 },
  { US"tcedil",    0x0163 },
  { US"telrec",    0x2315 }, 
  { US"thorn",     0x00fe },
  { US"times",     0x00d7 },
  { US"trade",     0x2122 },
  { US"tstrok",    0x0167 },
  { US"uacute",    0x00fa },
  { US"uarr",      0x2191 },
  { US"ubreve",    0x016d },
  { US"ucirc",     0x00fb },
  { US"udblac",    0x0171 },
  { US"ugrave",    0x00f9 },
  { US"uhblk",     0x2580 }, 
  { US"ulcrop",    0x230f }, 
  { US"umacr",     0x016b },
  { US"uogon",     0x0173 },
  { US"urcrop",    0x230e }, 
  { US"uring",     0x016f },
  { US"utilde",    0x0169 },
  { US"utri",      0x25b5 }, 
  { US"utrif",     0x25b4 }, 
  { US"uuml",      0x00fc },
  { US"vellip",    0x22ee }, 
  { US"wcirc",     0x0175 },
  { US"yacute",    0x00fd },
  { US"ycirc",     0x0177 },
  { US"yen",       0x00a5 },
  { US"yuml",      0x00ff },
  { US"zacute",    0x017a },
  { US"zcaron",    0x017e },
  { US"zdot",      0x017c }
};

int entity_list_count = sizeof(entity_list)/sizeof(entity_block);


/*************************************************
*               Character tables                 *
*************************************************/

/* This table lists the characters that are in the second PS font, with values
greater than 384 (below this, Unicode encoding is used). This is a sufficiently 
small list that a linear search isn't a bad strategy. */

int nonulist[] = {
  0x0394,
  0x20ac,
  0x0218,
  0x021a,
  0x0306,
  0x030c,
  0x0302,
  0x0326,
  0x2020,
  0x2021,
  0x0307,
  0x2026,
  0x2014,
  0x2013,
  0xfb01,
  0xfb02,
  0x0192,
  0x2044,
  0x2265,
  0x2039,
  0x203a,
  0x030b,
  0x2264,
  0x25ca,
  0x2212,
  0x2260,
  0x0328,
  0x2202,
  0x00b7,
  0x2031,
  0x201e,
  0x201c,
  0x201d,
  0x2018,
  0x2019,
  0x201a,
  0x221a,
  0x030a,
  0x0219,
  0x2211,
  0x021b,
  0x0303,
  0x2122
};

int nonucount = sizeof(nonulist)/sizeof(int);


/* This table translates from Unicode code points into characters in the
special fonts, currently Symbol and Dingbats. Some of these characters do
appear in the Standard Encoding as well, but they are not present in every
Adobe font, so are sometimes used from here. Characters in the Symbol font that
are generally available (such as exclam, digits, plus, period, etc) are not
included here, as they would never be used, so it would bloat the table
unnecessarily. */

u2sencod u2slist[] = {
  { 0x0391, SF_SYMB,  65 },       /* Alpha */
  { 0x0392, SF_SYMB,  66 },       /* Beta */
  { 0x0393, SF_SYMB,  71 },       /* Gamma */
  { 0x0394, SF_SYMB,  68 },       /* Delta */
  { 0x0395, SF_SYMB,  69 },       /* Epsilon */
  { 0x0396, SF_SYMB,  90 },       /* Zeta */
  { 0x0397, SF_SYMB,  72 },       /* Eta */
  { 0x0398, SF_SYMB,  81 },       /* Theta */
  { 0x0399, SF_SYMB,  73 },       /* Iota */
  { 0x039a, SF_SYMB,  75 },       /* Kappa */
  { 0x039b, SF_SYMB,  76 },       /* Lambda */
  { 0x039c, SF_SYMB,  77 },       /* Mu */
  { 0x039d, SF_SYMB,  78 },       /* Nu */
  { 0x039e, SF_SYMB,  88 },       /* Xi */
  { 0x039f, SF_SYMB,  79 },       /* Omicron */
  { 0x03a0, SF_SYMB,  80 },       /* Pi */
  { 0x03a1, SF_SYMB,  82 },       /* Rho */
  { 0x03a3, SF_SYMB,  83 },       /* Sigma */
  { 0x03a4, SF_SYMB,  84 },       /* Tau */
  { 0x03a5, SF_SYMB,  85 },       /* Upsilon */
  { 0x03a6, SF_SYMB,  70 },       /* Phi */
  { 0x03a7, SF_SYMB,  67 },       /* Chi */
  { 0x03a8, SF_SYMB,  89 },       /* Psi */
  { 0x03a9, SF_SYMB,  87 },       /* Omega */
  { 0x03b1, SF_SYMB,  97 },       /* alpha */
  { 0x03b2, SF_SYMB,  98 },       /* beta */
  { 0x03b3, SF_SYMB, 103 },       /* gamma */
  { 0x03b4, SF_SYMB, 100 },       /* delta */
  { 0x03b5, SF_SYMB, 101 },       /* epsilon */
  { 0x03b6, SF_SYMB, 122 },       /* zeta */
  { 0x03b7, SF_SYMB, 104 },       /* eta */
  { 0x03b8, SF_SYMB, 113 },       /* theta */
  { 0x03b9, SF_SYMB, 105 },       /* iota */
  { 0x03ba, SF_SYMB, 107 },       /* kappa */
  { 0x03bb, SF_SYMB, 108 },       /* lambda */
  { 0x03bc, SF_SYMB, 109 },       /* mu */
  { 0x03bd, SF_SYMB, 110 },       /* nu */
  { 0x03be, SF_SYMB, 120 },       /* xi */
  { 0x03bf, SF_SYMB, 111 },       /* omicron */
  { 0x03c0, SF_SYMB, 112 },       /* pi */
  { 0x03c1, SF_SYMB, 114 },       /* rho */
  { 0x03c2, SF_SYMB,  86 },       /* sigma1 */
  { 0x03c3, SF_SYMB, 115 },       /* sigma */
  { 0x03c4, SF_SYMB, 116 },       /* tau */
  { 0x03c5, SF_SYMB, 117 },       /* upsilon */
  { 0x03c6, SF_SYMB, 106 },       /* phi1 */
  { 0x03c7, SF_SYMB,  99 },       /* chi */
  { 0x03c8, SF_SYMB, 121 },       /* psi */
  { 0x03c9, SF_SYMB, 119 },       /* omega */
  { 0x03d1, SF_SYMB,  74 },       /* theta1 */
  { 0x03d2, SF_SYMB, 161 },       /* Upsilon1 */
  { 0x03d5, SF_SYMB, 102 },       /* phi */
  { 0x03d6, SF_SYMB, 118 },       /* omega1 */
  { 0x12aa, SF_SYMB, 239 },       /* braceex */
  { 0x2032, SF_SYMB, 162 },       /* minute */
  { 0x2033, SF_SYMB, 178 },       /* second */
  { 0x20ac, SF_SYMB, 160 },       /* Euro */
  { 0x2111, SF_SYMB, 193 },       /* Ifraktur */
  { 0x2118, SF_SYMB, 195 },       /* weierstrass */
  { 0x211c, SF_SYMB, 194 },       /* Rfraktur */
  { 0x2135, SF_SYMB, 192 },       /* aleph */
  { 0x2190, SF_SYMB, 172 },       /* arrowleft */
  { 0x2191, SF_SYMB, 173 },       /* arrowup */
  { 0x2192, SF_SYMB, 174 },       /* arrowright */
  { 0x2193, SF_SYMB, 175 },       /* arrowdown */
  { 0x2194, SF_SYMB, 171 },       /* arrowboth */
  { 0x21b5, SF_SYMB, 191 },       /* carriagereturn */
  { 0x21d0, SF_SYMB, 220 },       /* arrowdblleft */
  { 0x21d1, SF_SYMB, 221 },       /* arrowdblup */
  { 0x21d2, SF_SYMB, 222 },       /* arrowdblright */
  { 0x21d3, SF_SYMB, 223 },       /* arrowdbldown */
  { 0x21d4, SF_SYMB, 219 },       /* arrowdblboth */
  { 0x2200, SF_SYMB,  34 },       /* universal */
  { 0x2202, SF_SYMB, 182 },       /* partialdiff */
  { 0x2203, SF_SYMB,  36 },       /* existential */
  { 0x2205, SF_SYMB, 198 },       /* emptyset */
  { 0x2207, SF_SYMB, 209 },       /* gradient */
  { 0x2208, SF_SYMB, 206 },       /* element */
  { 0x2209, SF_SYMB, 207 },       /* notelement */
  { 0x220d, SF_SYMB,  39 },       /* suchthat */
  { 0x220f, SF_SYMB, 213 },       /* product */
  { 0x2211, SF_SYMB, 229 },       /* summation */
  { 0x2215, SF_SYMB, 164 },       /* fraction */
  { 0x221a, SF_SYMB, 214 },       /* radical */
  { 0x221d, SF_SYMB, 181 },       /* proportional */
  { 0x221e, SF_SYMB, 165 },       /* infinity */
  { 0x2220, SF_SYMB, 208 },       /* angle */
  { 0x2229, SF_SYMB, 199 },       /* intersection */
  { 0x222a, SF_SYMB, 200 },       /* union */
  { 0x222b, SF_SYMB, 242 },       /* integral */
  { 0x2234, SF_SYMB,  92 },       /* therefore */
  { 0x223c, SF_SYMB, 123 },       /* similar */
  { 0x2245, SF_SYMB,  64 },       /* congruent */
  { 0x2248, SF_SYMB, 187 },       /* approxequal */
  { 0x2260, SF_SYMB, 185 },       /* notequal */
  { 0x2261, SF_SYMB, 186 },       /* equivalence */
  { 0x2264, SF_SYMB, 163 },       /* lessequal */
  { 0x2265, SF_SYMB, 179 },       /* greaterequal */
  { 0x2282, SF_SYMB, 204 },       /* propersubset */
  { 0x2283, SF_SYMB, 201 },       /* propersuperset */
  { 0x2284, SF_SYMB, 203 },       /* notsubset */
  { 0x2286, SF_SYMB, 205 },       /* reflexsubset */
  { 0x2287, SF_SYMB, 202 },       /* reflexsuperset */
  { 0x2295, SF_SYMB, 197 },       /* circleplus */
  { 0x2297, SF_SYMB, 196 },       /* circlemultiply */
  { 0x22a5, SF_SYMB,  93 },       /* perpendicular */
  { 0x22c0, SF_SYMB, 217 },       /* logicaland */
  { 0x22c1, SF_SYMB, 218 },       /* logicalor */
  { 0x22c5, SF_SYMB, 215 },       /* dotmath */
  { 0x2320, SF_SYMB, 243 },       /* integraltp */
  { 0x2321, SF_SYMB, 245 },       /* integralbt */
  { 0x2329, SF_SYMB, 225 },       /* angleleft */
  { 0x232a, SF_SYMB, 241 },       /* angleright */
  { 0x239b, SF_SYMB, 230 },       /* parenlefttp */
  { 0x239c, SF_SYMB, 231 },       /* parenleftex */
  { 0x239d, SF_SYMB, 232 },       /* parenleftbt */
  { 0x239e, SF_SYMB, 246 },       /* parenrighttp */
  { 0x239f, SF_SYMB, 247 },       /* parenrightex */
  { 0x23a0, SF_SYMB, 248 },       /* parenrightbt */
  { 0x23a1, SF_SYMB, 233 },       /* bracketlefttp */
  { 0x23a2, SF_SYMB, 234 },       /* bracketleftex */
  { 0x23a3, SF_SYMB, 235 },       /* bracketleftbt */
  { 0x23a4, SF_SYMB, 249 },       /* bracketrighttp */
  { 0x23a5, SF_SYMB, 250 },       /* bracketrightex */
  { 0x23a6, SF_SYMB, 251 },       /* bracketrightbt */
  { 0x23a7, SF_SYMB, 236 },       /* bracelefttp */
  { 0x23a8, SF_SYMB, 237 },       /* braceleftmid */
  { 0x23a9, SF_SYMB, 238 },       /* braceleftbt */
  { 0x23ab, SF_SYMB, 252 },       /* bracerighttp */
  { 0x23ac, SF_SYMB, 253 },       /* bracerightmid */
  { 0x23ad, SF_SYMB, 254 },       /* bracerightbt */
  { 0x23ae, SF_SYMB, 244 },       /* integralex */
  { 0x25a0, SF_DBAT, 110 },       /* filled square */
  { 0x25b2, SF_DBAT, 115 },       /* filled point up triangle */
  { 0x25bc, SF_DBAT, 116 },       /* filled point down triangle */
  { 0x25c6, SF_DBAT, 117 },       /* filled diamond */
  { 0x25ca, SF_SYMB, 224 },       /* lozenge */
  { 0x25d7, SF_DBAT, 119 },       /* right half circle, filled */
  { 0x260e, SF_DBAT,  37 },       /* telephone */
  { 0x261b, SF_DBAT,  42 },       /* filled hand pointing right */
  { 0x261e, SF_DBAT,  43 },       /* unfilled hand pointing right */
  { 0x2660, SF_SYMB, 170 },       /* spade */
  { 0x2663, SF_SYMB, 167 },       /* club */
  { 0x2665, SF_SYMB, 169 },       /* heart */
  { 0x2666, SF_SYMB, 168 },       /* diamond */
  { 0x26ab, SF_DBAT, 108 },       /* filled circle */
  { 0x2701, SF_DBAT,  33 },
  { 0x2702, SF_DBAT,  34 },
  { 0x2703, SF_DBAT,  35 },
  { 0x2704, SF_DBAT,  36 },
  { 0x2706, SF_DBAT,  38 },
  { 0x2707, SF_DBAT,  39 },
  { 0x2708, SF_DBAT,  40 },
  { 0x2709, SF_DBAT,  41 },
  { 0x270c, SF_DBAT,  44 },
  { 0x270d, SF_DBAT,  45 },
  { 0x270e, SF_DBAT,  46 },
  { 0x270f, SF_DBAT,  47 },
  { 0x2710, SF_DBAT,  48 },
  { 0x2711, SF_DBAT,  49 },
  { 0x2712, SF_DBAT,  50 },
  { 0x2713, SF_DBAT,  51 },
  { 0x2714, SF_DBAT,  52 },
  { 0x2715, SF_DBAT,  53 },
  { 0x2716, SF_DBAT,  54 },
  { 0x2717, SF_DBAT,  55 },
  { 0x2718, SF_DBAT,  56 },
  { 0x2719, SF_DBAT,  57 },
  { 0x271a, SF_DBAT,  58 },
  { 0x271b, SF_DBAT,  59 },
  { 0x271c, SF_DBAT,  60 },
  { 0x271d, SF_DBAT,  61 },
  { 0x271e, SF_DBAT,  62 },
  { 0x271f, SF_DBAT,  63 },
  { 0x2720, SF_DBAT,  64 },
  { 0x2721, SF_DBAT,  65 },
  { 0x2722, SF_DBAT,  66 },
  { 0x2723, SF_DBAT,  67 },
  { 0x2724, SF_DBAT,  68 },
  { 0x2725, SF_DBAT,  69 },
  { 0x2726, SF_DBAT,  70 },
  { 0x2727, SF_DBAT,  71 },
  { 0x2729, SF_DBAT,  73 },
  { 0x272a, SF_DBAT,  74 },
  { 0x272b, SF_DBAT,  75 },
  { 0x272c, SF_DBAT,  76 },
  { 0x272d, SF_DBAT,  77 },
  { 0x272e, SF_DBAT,  78 },
  { 0x272f, SF_DBAT,  79 },
  { 0x2730, SF_DBAT,  80 },
  { 0x2731, SF_DBAT,  81 },
  { 0x2732, SF_DBAT,  82 },
  { 0x2733, SF_DBAT,  83 },
  { 0x2734, SF_DBAT,  84 },
  { 0x2735, SF_DBAT,  85 },
  { 0x2736, SF_DBAT,  86 },
  { 0x2737, SF_DBAT,  87 },
  { 0x2738, SF_DBAT,  88 },
  { 0x2739, SF_DBAT,  89 },
  { 0x273a, SF_DBAT,  90 },
  { 0x273b, SF_DBAT,  91 },
  { 0x273c, SF_DBAT,  92 },
  { 0x273d, SF_DBAT,  93 },
  { 0x273e, SF_DBAT,  94 },
  { 0x273f, SF_DBAT,  95 },
  { 0x2740, SF_DBAT,  96 },
  { 0x2741, SF_DBAT,  97 },
  { 0x2742, SF_DBAT,  98 },
  { 0x2743, SF_DBAT,  99 },
  { 0x2744, SF_DBAT, 100 },
  { 0x2745, SF_DBAT, 101 },
  { 0x2746, SF_DBAT, 102 },
  { 0x2747, SF_DBAT, 103 },
  { 0x2748, SF_DBAT, 104 },
  { 0x2749, SF_DBAT, 105 },
  { 0x274a, SF_DBAT, 106 },
  { 0x274b, SF_DBAT, 107 },
  { 0x274d, SF_DBAT, 109 },
  { 0x274f, SF_DBAT, 111 },
  { 0x2750, SF_DBAT, 112 },
  { 0x2751, SF_DBAT, 113 },
  { 0x2752, SF_DBAT, 114 },
  { 0x2756, SF_DBAT, 118 },
  { 0x2758, SF_DBAT, 120 },
  { 0x2759, SF_DBAT, 121 },
  { 0x275a, SF_DBAT, 122 },
  { 0x275b, SF_DBAT, 123 },
  { 0x275c, SF_DBAT, 124 },
  { 0x275d, SF_DBAT, 125 },
  { 0x275e, SF_DBAT, 126 },
  { 0x2761, SF_DBAT, 161 },
  { 0x2762, SF_DBAT, 162 },
  { 0x2763, SF_DBAT, 163 },
  { 0x2764, SF_DBAT, 164 },
  { 0x2765, SF_DBAT, 165 },
  { 0x2766, SF_DBAT, 166 },
  { 0x2767, SF_DBAT, 167 },
  { 0x2776, SF_DBAT, 182 },
  { 0x2777, SF_DBAT, 183 },
  { 0x2778, SF_DBAT, 184 },
  { 0x2779, SF_DBAT, 185 },
  { 0x277a, SF_DBAT, 186 },
  { 0x277b, SF_DBAT, 187 },
  { 0x277c, SF_DBAT, 188 },
  { 0x277d, SF_DBAT, 189 },
  { 0x277e, SF_DBAT, 190 },
  { 0x277f, SF_DBAT, 191 },
  { 0x2780, SF_DBAT, 192 },
  { 0x2781, SF_DBAT, 193 },
  { 0x2782, SF_DBAT, 194 },
  { 0x2783, SF_DBAT, 195 },
  { 0x2784, SF_DBAT, 196 },
  { 0x2785, SF_DBAT, 197 },
  { 0x2786, SF_DBAT, 198 },
  { 0x2787, SF_DBAT, 199 },
  { 0x2788, SF_DBAT, 200 },
  { 0x2789, SF_DBAT, 201 },
  { 0x278a, SF_DBAT, 202 },
  { 0x278b, SF_DBAT, 203 },
  { 0x278c, SF_DBAT, 204 },
  { 0x278d, SF_DBAT, 205 },
  { 0x278e, SF_DBAT, 206 },
  { 0x278f, SF_DBAT, 207 },
  { 0x2790, SF_DBAT, 208 },
  { 0x2791, SF_DBAT, 209 },
  { 0x2792, SF_DBAT, 210 },
  { 0x2793, SF_DBAT, 211 },
  { 0x2794, SF_DBAT, 212 },
  { 0x2798, SF_DBAT, 216 },
  { 0x2799, SF_DBAT, 217 },
  { 0x279a, SF_DBAT, 218 },
  { 0x279b, SF_DBAT, 219 },
  { 0x279c, SF_DBAT, 220 },
  { 0x279d, SF_DBAT, 221 },
  { 0x279e, SF_DBAT, 222 },
  { 0x279f, SF_DBAT, 223 },
  { 0x27a0, SF_DBAT, 224 },
  { 0x27a1, SF_DBAT, 225 },
  { 0x27a2, SF_DBAT, 226 },
  { 0x27a3, SF_DBAT, 227 },
  { 0x27a4, SF_DBAT, 228 },
  { 0x27a5, SF_DBAT, 229 },
  { 0x27a6, SF_DBAT, 230 },
  { 0x27a7, SF_DBAT, 231 },
  { 0x27a8, SF_DBAT, 232 },
  { 0x27a9, SF_DBAT, 233 },
  { 0x27aa, SF_DBAT, 234 },
  { 0x27ab, SF_DBAT, 235 },
  { 0x27ac, SF_DBAT, 236 },
  { 0x27ad, SF_DBAT, 237 },
  { 0x27ae, SF_DBAT, 238 },
  { 0x27af, SF_DBAT, 239 },
  { 0x27b1, SF_DBAT, 241 },
  { 0x27b2, SF_DBAT, 242 },
  { 0x27b3, SF_DBAT, 243 },
  { 0x27b4, SF_DBAT, 244 },
  { 0x27b5, SF_DBAT, 245 },
  { 0x27b6, SF_DBAT, 246 },
  { 0x27b7, SF_DBAT, 247 },
  { 0x27b8, SF_DBAT, 248 },
  { 0x27b9, SF_DBAT, 249 },
  { 0x27ba, SF_DBAT, 250 },
  { 0x27bb, SF_DBAT, 251 },
  { 0x27bc, SF_DBAT, 252 },
  { 0x27bd, SF_DBAT, 253 },
  { 0x27be, SF_DBAT, 254 }
};

int u2scount = sizeof(u2slist)/sizeof(u2sencod);

/* These characters in the Symbol font are omitted because they are also in
the standard encoding for all other Adobe fonts, and so would never be used
from the Symbol font. These are the characters I've noticed. There may be some
that were overlooked...

ampersand
asteriskmath
bar
braceleft
braceright
bracketleft
bracketright
bullet
colon
comma
degree
divide
eight
ellipsis
equal
exclam
five
four
greater
less
logicalnot
minus
multiply
nine
numbersign
one
parenleft
parenright
percent
period
plus
plusminus
question
semicolon
seven
six
slash
space
three
two
underscore
zero

These characters in the Symbol font are omitted because there are no Unicode
equivalents (that I have yet found :-).

arrowhorizex
arrowvertex
florin
radicalex

These are just typographic variations:

copyrightsans
copyrightserif
registersans
registerserif
trademarksans
trademarkserif
*/

/* End of tables.c */
