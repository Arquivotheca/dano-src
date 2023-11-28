/*
 * True Type Font to Adobe Type 1 font converter 
 * By Mark Heath <mheath@netspace.net.au> 
 * Based on ttf2pfa by Andrew Weeks <ccsaw@bath.ac.uk> 
 * With help from Frank M. Siegert <fms@this.net> 
 *
***********************************************************************
 *
 * Sergey Babkin <babkin@bellatlantic.net>, <sab123@hotmail.com>
 *
 * Added post-processing of resulting outline to correct the errors
 * both introduced during conversion and present in the original font,
 * autogeneration of hints (has yet to be improved though) and BlueValues,
 * scaling to 1000x1000 matrix, option to print the result on STDOUT,
 * support of Unicode to CP1251 conversion, optimization  of the
 * resulting font code by space (that improves the speed too). Excluded
 * the glyphs that are unaccessible through the encoding table from
 * the output file. Added the built-in Type1 assembler (taken from
 * the `t1utils' package).
 *
***********************************************************************
 *
 * Thomas Henlich <thenlich@rcs.urz.tu-dresden.de>
 *
 * Added generation of .afm file (font metrics)
 *
***********************************************************************
 *
 * Bug Fixes: 
************************************************************************
 *
 * Sun, 21 Jun 1998 Thomas Henlich <thenlich@Rcs1.urz.tu-dresden.de> 
 * 1. "width" should be "short int" because otherwise: 
 *     characters with negative widths (e.g. -4) become *very* wide (65532) 
 * 2. the number of /CharStrings is numglyphs and not numglyphs+1 
 *
***********************************************************************
 *
 *
 *
 * The resultant font file produced by this program still needs to be ran
 * through t1asm (from the t1utils archive) to produce a completely valid
 * font. 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <ctype.h>
#include <math.h>

#include <Application.h>
#include <Node.h>
#include <unistd.h>
#include <string.h>

#include "ttf2pt1.h"
#include "t1asm.h"

#define FPF //fprintf

/*
 * The order of descriptions is important: if we can't guess the
 * language we just call all the conversion routines in order untill
 * we find one that understands this glyph.
 */

static int unicode_latin1(int unival);
static int unicode_latin4(int unival);
static int unicode_latin5(int unival);
static int unicode_russian(int unival);

bool ttf2pt1::newishFlag;

struct uni_language uni_lang[]= {
	/* pseudo-language for all the languages using Latin1 */
	{
		unicode_latin1, 
		"latin1",
		"works for most of the western languages",
		{ "en_", "de_", "fr_", "nl_", "no_", "da_", "it_" }
	},
	{
		unicode_latin4, 
		"latin4",
		"works for Baltic languages",
		{ "lt_", "lv_" } /* doubt about ee_ */
	},
	{
		unicode_latin5, 
		"latin5",
		"for turkish",
		{ "tr_" }
	},
	{
		unicode_russian,
		"russian",
		"in Windows encoding",
		{ "ru_" }
	},
};


static char    *ISOLatin1Encoding[256] = {
	".null", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", "CR", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	"space", "exclam", "quotedbl", "numbersign",
	"dollar", "percent", "ampersand", "quoteright",
	"parenleft", "parenright", "asterisk", "plus",
	"comma", "hyphen", "period", "slash",
	"zero", "one", "two", "three",
	"four", "five", "six", "seven",
	"eight", "nine", "colon", "semicolon",
	"less", "equal", "greater", "question",
	"at", "A", "B", "C",
	"D", "E", "F", "G",
	"H", "I", "J", "K",
	"L", "M", "N", "O",
	"P", "Q", "R", "S",
	"T", "U", "V", "W",
	"X", "Y", "Z", "bracketleft",
	"backslash", "bracketright", "asciicircum", "underscore",
	"grave", "a", "b", "c",
	"d", "e", "f", "g",
	"h", "i", "j", "k",
	"l", "m", "n", "o",
	"p", "q", "r", "s",
	"t", "u", "v", "w",
	"x", "y", "z", "braceleft",
	"bar", "braceright", "asciitilde", ".notdef",
	".notdef", ".notdef", "quotesinglbase", "florin",
	"quotedblbase", "ellipsis", "dagger", "daggerdbl",
	"circumflex", "perthousand", "Scaron", "guilsinglleft",
	"OE", ".notdef", ".notdef", ".notdef",
	".notdef", "quoteleft", "quoteright", "quotedblleft",
	"quotedblright", "bullet", "endash", "emdash",
	"tilde", "trademark", "scaron", "guilsinglright",
	"oe", ".notdef", ".notdef", "Ydieresis",
	"nbspace", "exclamdown", "cent", "sterling",
	"currency", "yen", "brokenbar", "section",
	"dieresis", "copyright", "ordfeminine", "guillemotleft",
	"logicalnot", "sfthyphen", "registered", "macron",
	"degree", "plusminus", "twosuperior", "threesuperior",
	"acute", "mu", "paragraph", "periodcentered",
	"cedilla", "onesuperior", "ordmasculine", "guillemotright",
	"onequarter", "onehalf", "threequarters", "questiondown",
	"Agrave", "Aacute", "Acircumflex", "Atilde",
	"Adieresis", "Aring", "AE", "Ccedilla",
	"Egrave", "Eacute", "Ecircumflex", "Edieresis",
	"Igrave", "Iacute", "Icircumflex", "Idieresis",
	"Eth", "Ntilde", "Ograve", "Oacute",
	"Ocircumflex", "Otilde", "Odieresis", "multiply",
	"Oslash", "Ugrave", "Uacute", "Ucircumflex",
	"Udieresis", "Yacute", "Thorn", "germandbls",
	"agrave", "aacute", "acircumflex", "atilde",
	"adieresis", "aring", "ae", "ccedilla",
	"egrave", "eacute", "ecircumflex", "edieresis",
	"igrave", "iacute", "icircumflex", "idieresis",
	"eth", "ntilde", "ograve", "oacute",
	"ocircumflex", "otilde", "odieresis", "divide",
	"oslash", "ugrave", "uacute", "ucircumflex",
	"udieresis", "yacute", "thorn", "ydieresis"
};

static char    *adobe_StandardEncoding[256] = {
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	"space", "exclam", "quotedbl", "numbersign",
	"dollar", "percent", "ampersand", "quoteright",
	"parenleft", "parenright", "asterisk", "plus",
	"comma", "hyphen", "period", "slash",
	"zero", "one", "two", "three",
	"four", "five", "six", "seven",
	"eight", "nine", "colon", "semicolon",
	"less", "equal", "greater", "question",
	"at", "A", "B", "C", "D", "E", "F", "G",
	"H", "I", "J", "K", "L", "M", "N", "O",
	"P", "Q", "R", "S", "T", "U", "V", "W",
	"X", "Y", "Z", "bracketleft",
	"backslash", "bracketright", "asciicircum", "underscore",
	"grave", "a", "b", "c", "d", "e", "f", "g",
	"h", "i", "j", "k", "l", "m", "n", "o",
	"p", "q", "r", "s", "t", "u", "v", "w",
	"x", "y", "z", "braceleft",
	"bar", "braceright", "asciitilde", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", "exclamdown", "cent", "sterling",
	"fraction", "yen", "florin", "section",
	"currency", "quotesingle", "quotedblleft", "guillemotleft",
	"guilsinglleft", "guilsinglright", "fi", "fl",
	".notdef", "endash", "dagger", "daggerdbl",
	"periodcentered", ".notdef", "paragraph", "bullet",
	"quotesinglbase", "quotedblbase", "quotedblright", "guillemotright",
	"ellipsis", "perthousand", ".notdef", "questiondown",
	".notdef", "grave", "acute", "circumflex",
	"tilde", "macron", "breve", "dotaccent",
	"dieresis", ".notdef", "ring", "cedilla",
	".notdef", "hungarumlaut", "ogonek", "caron",
	"emdash", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", "AE", ".notdef", "ordfeminine",
	".notdef", ".notdef", ".notdef", ".notdef",
	"Lslash", "Oslash", "OE", "ordmasculine",
	".notdef", ".notdef", ".notdef", ".notdef",
	".notdef", "ae", ".notdef", ".notdef",
	".notdef", "dotlessi", ".notdef", ".notdef",
	"lslash", "oslash", "oe", "germandbls",
	".notdef", ".notdef", ".notdef", ".notdef"
};

static char    *mac_glyph_names[258] = {
	".notdef", ".null", "CR",
	"space", "exclam", "quotedbl", "numbersign",
	"dollar", "percent", "ampersand", "quotesingle",
	"parenleft", "parenright", "asterisk", "plus",
	"comma", "hyphen", "period", "slash",
	"zero", "one", "two", "three",
	"four", "five", "six", "seven",
	"eight", "nine", "colon", "semicolon",
	"less", "equal", "greater", "question",
	"at", "A", "B", "C",
	"D", "E", "F", "G",
	"H", "I", "J", "K",
	"L", "M", "N", "O",
	"P", "Q", "R", "S",
	"T", "U", "V", "W",
	"X", "Y", "Z", "bracketleft",
	"backslash", "bracketright", "asciicircum", "underscore",
	"grave", "a", "b", "c",
	"d", "e", "f", "g",
	"h", "i", "j", "k",
	"l", "m", "n", "o",
	"p", "q", "r", "s",
	"t", "u", "v", "w",
	"x", "y", "z", "braceleft",
	"bar", "braceright", "asciitilde", "Adieresis",
	"Aring", "Ccedilla", "Eacute", "Ntilde",
	"Odieresis", "Udieresis", "aacute", "agrave",
	"acircumflex", "adieresis", "atilde", "aring",
	"ccedilla", "eacute", "egrave", "ecircumflex",
	"edieresis", "iacute", "igrave", "icircumflex",
	"idieresis", "ntilde", "oacute", "ograve",
	"ocircumflex", "odieresis", "otilde", "uacute",
	"ugrave", "ucircumflex", "udieresis", "dagger",
	"degree", "cent", "sterling", "section",
	"bullet", "paragraph", "germandbls", "registered",
	"copyright", "trademark", "acute", "dieresis",
	"notequal", "AE", "Oslash", "infinity",
	"plusminus", "lessequal", "greaterequal", "yen",
	"mu", "partialdiff", "summation", "product",
	"pi", "integral", "ordfeminine", "ordmasculine",
	"Omega", "ae", "oslash", "questiondown",
	"exclamdown", "logicalnot", "radical", "florin",
	"approxequal", "increment", "guillemotleft", "guillemotright",
	"ellipsis", "nbspace", "Agrave", "Atilde",
	"Otilde", "OE", "oe", "endash",
	"emdash", "quotedblleft", "quotedblright", "quoteleft",
	"quoteright", "divide", "lozenge", "ydieresis",
	"Ydieresis", "fraction", "currency", "guilsinglleft",
	"guilsinglright", "fi", "fl", "daggerdbl",
	"periodcentered", "quotesinglbase", "quotedblbase", "perthousand",
	"Acircumflex", "Ecircumflex", "Aacute", "Edieresis",
	"Egrave", "Iacute", "Icircumflex", "Idieresis",
	"Igrave", "Oacute", "Ocircumflex", "applelogo",
	"Ograve", "Uacute", "Ucircumflex", "Ugrave",
	"dotlessi", "circumflex", "tilde", "macron",
	"breve", "dotaccent", "ring", "cedilla",
	"hungarumlaut", "ogonek", "caron", "Lslash",
	"lslash", "Scaron", "scaron", "Zcaron",
	"zcaron", "brokenbar", "Eth", "eth",
	"Yacute", "yacute", "Thorn", "thorn",
	"minus", "multiply", "onesuperior", "twosuperior",
	"threesuperior", "onehalf", "onequarter", "threequarters",
	"franc", "Gbreve", "gbreve", "Idot",
	"Scedilla", "scedilla", "Cacute", "cacute",
	"Ccaron", "ccaron", "dmacron"
};


ttf2pt1::ttf2pt1()
{
	newishFlag = true;
	
	uni_lang_converter = 0;

	optimize = 1;
	smooth = 1;
	transform  = 1;
	hints = 1;
	absolute = 0;
	debug = 0;
	trybold = 1;
	reverse = 1;
	encode = 0;
	pfbflag = 0;
	wantafm = 0;
	
	name_table = NULL;
	head_table = NULL;
	hhea_table = NULL;
	kern_table = NULL;
	cmap_table = NULL;
	hmtx_table = NULL;
	glyf_start = NULL;
	maxp_table = NULL;
	post_table = NULL;
	short_loca_table = NULL;
	long_loca_table = NULL;
	
	ps_fmt_3 = 0;
	unicode = 0;
	Unknown_glyph = "UNKN";
}

ttf2pt1::~ttf2pt1()
{
	delete glyph_list;
}




 int
ttf2pt1::sign(
     int x
)
{
	if (x > 0)
		return 1;
	else if (x < 0)
		return -1;
	else
		return 0;
}

/*
 * Scale the values according to the scale_factor
 */

 int
ttf2pt1::scale(
      int val
)
{
	return (int) (val > 0 ? scale_factor * val + 0.5
		      : scale_factor * val - 0.5);
}

GENTRY         *
ttf2pt1::newgentry(void)
{
	GENTRY         *ge;

	ge = (GENTRY*)malloc(sizeof(GENTRY));

	if (ge == 0) {
		FPF(stderr, "***** Memory allocation error *****\n");
		exit(1);
	}
	ge->next = ge->prev = ge->first = 0;
	return ge;
}

/*
 * * SB * Routines to print out Postscript functions with optimization
 */

 void
ttf2pt1::rmoveto(
	int dx,
	int dy
)
{
	if (optimize && dx == 0 && dy == 0)	/* for special pathologic
						 * case */
		return;
	else if (optimize && dx == 0)
		fprintf(pfa_file, "%d vmoveto\n", dy);
	else if (optimize && dy == 0)
		fprintf(pfa_file, "%d hmoveto\n", dx);
	else
		fprintf(pfa_file, "%d %d rmoveto\n", dx, dy);
}

 void
ttf2pt1::rlineto(
	int dx,
	int dy
)
{
	if (optimize && dx == 0 && dy == 0)	/* for special pathologic
						 * case */
		return;
	else if (optimize && dx == 0)
		fprintf(pfa_file, "%d vlineto\n", dy);
	else if (optimize && dy == 0)
		fprintf(pfa_file, "%d hlineto\n", dx);
	else
		fprintf(pfa_file, "%d %d rlineto\n", dx, dy);
}

 void
ttf2pt1::rrcurveto(
	  int dx1,
	  int dy1,
	  int dx2,
	  int dy2,
	  int dx3,
	  int dy3
)
{
	/* first two ifs are for crazy cases that occur surprisingly often */
	if (optimize && dx1 == 0 && dx2 == 0 && dx3 == 0)
		rlineto(0, dy1 + dy2 + dy3);
	else if (optimize && dy1 == 0 && dy2 == 0 && dy3 == 0)
		rlineto(dx1 + dx2 + dx3, 0);
	else if (optimize && dy1 == 0 && dx3 == 0)
		fprintf(pfa_file, "%d %d %d %d hvcurveto\n",
			dx1, dx2, dy2, dy3);
	else if (optimize && dx1 == 0 && dy3 == 0)
		fprintf(pfa_file, "%d %d %d %d vhcurveto\n",
			dy1, dx2, dy2, dx3);
	else
		fprintf(pfa_file, "%d %d %d %d %d %d rrcurveto\n",
			dx1, dy1, dx2, dy2, dx3, dy3);
}

 void
ttf2pt1::closepath(void)
{
	fprintf(pfa_file, "closepath\n");
}

/*
** Routine that checks integrity of the path, for debugging
*/

 void
ttf2pt1::assertpath(
	   GENTRY * from,
	   int line,
	   char *name
)
{
	GENTRY         *first, *pe, *ge;

	if(from==0)
		return;
	pe = from->prev;
	for (ge = from; ge != 0; pe = ge, ge = ge->next) {
		if (pe != ge->prev) {
			FPF(stderr, "**** assertpath: called from line %d (%s) ****\n", line, name);
			FPF(stderr, "unidirectional chain 0x%x -next-> 0x%x -prev-> 0x%x \n",
				pe, ge, ge->prev);
			exit(1);
		}
		if (ge->type == GE_MOVE)
			first = ge->next;
		if (ge->first && ge->first != first) {
			FPF(stderr, "**** assertpath: called from line %d (%s) ****\n", line, name);
			FPF(stderr, "broken loop 0x%x -...-> 0x%x -first-> 0x%x \n",
				first, ge, ge->first);
			exit(1);
		}
		if (ge->type == GE_PATH) {
			if (ge->prev == 0) {
				FPF(stderr, "**** assertpath: called from line %d (%s) ****\n", line, name);
				FPF(stderr, "empty path at 0x%x \n", ge);
				exit(1);
			}
			if (ge->prev->first == 0) {
				FPF(stderr, "**** assertpath: called from line %d (%s) ****\n", line, name);
				FPF(stderr, "path without backlink at 0x%x \n", pe);
				exit(1);
			}
		}
	}
}

/*
 * * SB * Routines to save the generated data about glyph
 */

 void
ttf2pt1::g_rmoveto(
	  GLYPH * g,
	  int x,
	  int y)
{
	GENTRY         *oge;

	if (0)
		FPF(stderr, "%s: rmoveto(%d, %d)\n", g->name, x, y);
	if ((oge = g->lastentry) != 0) {
		if (oge->type == GE_MOVE) {	/* just eat up the first move */
			oge->x3 = x;
			oge->y3 = y;
		} else if (oge->type == GE_LINE || oge->type == GE_CURVE) {
			FPF(stderr, "Glyph %s: MOVE in middle of path\n", g->name);
		} else {
			GENTRY         *nge;

			nge = newgentry();
			nge->type = GE_MOVE;
			nge->x3 = x;
			nge->y3 = y;

			oge->next = nge;
			nge->prev = oge;
			g->lastentry = nge;
		}
	} else {
		GENTRY         *nge;

		nge = newgentry();
		nge->type = GE_MOVE;
		nge->x3 = x;
		nge->y3 = y;
		g->entries = g->lastentry = nge;
	}

}

 void
ttf2pt1::g_rlineto(
	  GLYPH * g,
	  int x,
	  int y)
{
	GENTRY         *oge, *nge;

	if (0)
		FPF(stderr, "%s: rlineto(%d, %d)\n", g->name, x, y);
	nge = newgentry();
	nge->type = GE_LINE;
	nge->x3 = x;
	nge->y3 = y;

	if ((oge = g->lastentry) != 0) {
		if (x == oge->x3 && y == oge->y3) {	/* empty line */
			/* ignore it or we will get in troubles later */
			free(nge);
			return;
		}
		if (g->path == 0)
			g->path = nge;

		oge->next = nge;
		nge->prev = oge;
		g->lastentry = nge;
	} else {
		FPF(stderr, "Glyph %s: LINE outside of path\n", g->name);
#if 0
		g->entries = g->lastentry = nge;
#else
		free(nge);
#endif
	}

}

 void
ttf2pt1::g_rrcurveto(
	    GLYPH * g,
	    int x1,
	    int y1,
	    int x2,
	    int y2,
	    int x3,
	    int y3)
{
	GENTRY         *oge, *nge;

	oge = g->lastentry;

	if (0)
		FPF(stderr, "%s: rrcurveto(%d, %d, %d, %d, %d, %d)\n"
			,g->name, x1, y1, x2, y2, x3, y3);
	if (oge && oge->x3 == x1 && x1 == x2 && x2 == x3)	/* check if it's
								 * actually a line */
		g_rlineto(g, x1, y3);
	else if (oge && oge->y3 == y1 && y1 == y2 && y2 == y3)
		g_rlineto(g, x3, y1);
	else {
		nge = newgentry();
		nge->type = GE_CURVE;
		nge->x1 = x1;
		nge->y1 = y1;
		nge->x2 = x2;
		nge->y2 = y2;
		nge->x3 = x3;
		nge->y3 = y3;

		if (oge != 0) {
			if (x3 == oge->x3 && y3 == oge->y3) {
				free(nge);	/* consider this curve empty */
				/* ignore it or we will get in troubles later */
				return;
			}
			if (g->path == 0)
				g->path = nge;

			oge->next = nge;
			nge->prev = oge;
			g->lastentry = nge;
		} else {
			FPF(stderr, "Glyph %s: CURVE outside of path\n", g->name);
#if 0
			g->entries = g->lastentry = nge;
#else
			free(nge);
#endif
		}
	}
}

 void
ttf2pt1::g_closepath(
	    GLYPH * g
)
{
	GENTRY         *oge, *nge;

	if (0)
		FPF(stderr, "%s: closepath\n", g->name);

	oge = g->lastentry;

	if (g->path == 0) {
		FPF(stderr, "Warning: **** closepath on empty path in glyph \"%s\" ****\n",
			g->name);
		if (oge == 0) {
			FPF(stderr, "No previois entry\n");
		} else {
			FPF(stderr, "Previous entry type: %c\n", oge->type);
			if (oge->type == GE_MOVE) {
				g->lastentry = oge->prev;
				if (oge->prev == 0)
					g->entries = 0;
			}
		}
		return;
	}
	if (oge != 0) {		/* only if we actually have a path */
		nge = newgentry();
		nge->type = GE_PATH;
		oge->first = g->path;
		g->path = 0;
		oge->next = nge;
		nge->prev = oge;
		g->lastentry = nge;
	}
}

/*
 * * SB * Routines to smooth and fix the glyphs
 */

/*
** we don't want to see the curves with coinciding middle and
** outer points
*/

 void
ttf2pt1::fixcvends(
	  GENTRY * ge
)
{
	int             dx, dy;
	int             x0, y0, x1, y1, x2, y2, x3, y3;

	if (ge->type != GE_CURVE)
		return;

	x0 = ge->prev->x3;
	y0 = ge->prev->y3;
	x1 = ge->x1;
	y1 = ge->y1;
	x2 = ge->x2;
	y2 = ge->y2;
	x3 = ge->x3;
	y3 = ge->y3;


	/* look at the start of the curve */
	if (x1 == x0 && y1 == y0) {
		dx = x2 - x1;
		dy = y2 - y1;

		if (dx == 0 && dy == 0
		    || x2 == x3 && y2 == y3) {
			/* Oops, we actually have a straight line */
			/*
			 * if it's small, we hope that it will get optimized
			 * later
			 */
			if (abs(x3 - x0) <= 2 || abs(y3 - y0) <= 2) {
				ge->x1 = x3;
				ge->y1 = y3;
				ge->x2 = x0;
				ge->y2 = y0;
			} else {/* just make it a line */
				ge->type = GE_LINE;
			}
		} else {
			if (abs(dx) < 4 && abs(dy) < 4) {	/* consider it very
								 * small */
				ge->x1 = x2;
				ge->y1 = y2;
			} else if (abs(dx) < 8 && abs(dy) < 8) {	/* consider it small */
				ge->x1 += dx / 2;
				ge->y1 += dy / 2;
			} else {
				ge->x1 += dx / 4;
				ge->y1 += dy / 4;
			}
			/* make sure that it's still on the same side */
			if (abs(x3 - x0) * abs(dy) < abs(y3 - y0) * abs(dx)) {
				if (abs(x3 - x0) * abs(ge->y1 - y0) > abs(y3 - y0) * abs(ge->x1 - x0))
					ge->x1 += sign(dx);
			} else {
				if (abs(x3 - x0) * abs(ge->y1 - y0) < abs(y3 - y0) * abs(ge->x1 - x0))
					ge->y1 += sign(dy);
			}

			ge->x2 += (x3 - x2) / 8;
			ge->y2 += (y3 - y2) / 8;
			/* make sure that it's still on the same side */
			if (abs(x3 - x0) * abs(y3 - y2) < abs(y3 - y0) * abs(x3 - x2)) {
				if (abs(x3 - x0) * abs(y3 - ge->y2) > abs(y3 - y0) * abs(x3 - ge->x2))
					ge->y1 -= sign(y3 - y2);
			} else {
				if (abs(x3 - x0) * abs(y3 - ge->y2) < abs(y3 - y0) * abs(x3 - ge->x2))
					ge->x1 -= sign(x3 - x2);
			}

		}
	} else if (x2 == x3 && y2 == y3) {
		dx = x1 - x2;
		dy = y1 - y2;

		if (dx == 0 && dy == 0) {
			/* Oops, we actually have a straight line */
			/*
			 * if it's small, we hope that it will get optimized
			 * later
			 */
			if (abs(x3 - x0) <= 2 || abs(y3 - y0) <= 2) {
				ge->x1 = x3;
				ge->y1 = y3;
				ge->x2 = x0;
				ge->y2 = y0;
			} else {/* just make it a line */
				ge->type = GE_LINE;
			}
		} else {
			if (abs(dx) < 4 && abs(dy) < 4) {	/* consider it very
								 * small */
				ge->x2 = x1;
				ge->y2 = y1;
			} else if (abs(dx) < 8 && abs(dy) < 8) {	/* consider it small */
				ge->x2 += dx / 2;
				ge->y2 += dy / 2;
			} else {
				ge->x2 += dx / 4;
				ge->y2 += dy / 4;
			}
			/* make sure that it's still on the same side */
			if (abs(x3 - x0) * abs(dy) < abs(y3 - y0) * abs(dx)) {
				if (abs(x3 - x0) * abs(ge->y2 - y3) > abs(y3 - y0) * abs(ge->x2 - x3))
					ge->x2 += sign(dx);
			} else {
				if (abs(x3 - x0) * abs(ge->y2 - y3) < abs(y3 - y0) * abs(ge->x2 - x3))
					ge->y2 += sign(dy);
			}

			ge->x1 += (x0 - x1) / 8;
			ge->y1 += (y0 - y1) / 8;
			/* make sure that it's still on the same side */
			if (abs(x3 - x0) * abs(y0 - y1) < abs(y3 - y0) * abs(x0 - x1)) {
				if (abs(x3 - x0) * abs(y0 - ge->y1) > abs(y3 - y0) * abs(x0 - ge->x1))
					ge->y1 -= sign(y0 - y1);
			} else {
				if (abs(x3 - x0) * abs(y0 - ge->y1) < abs(y3 - y0) * abs(x0 - ge->x1))
					ge->x1 -= sign(x0 - x1);
			}

		}
	}
}

/* if we have any curves that are in fact flat but
** are not horizontal nor vertical, substitute
** them also with lines
*/

 void
ttf2pt1::flattencurves(
	      GLYPH * g
)
{
	GENTRY         *ge;
	int             x0, y0, x1, y1, x2, y2, x3, y3;

	for (ge = g->entries; ge != 0; ge = ge->next) {
		if (ge->type != GE_CURVE)
			continue;

		x0 = ge->prev->x3;
		y0 = ge->prev->y3;
		x1 = ge->x1;
		y1 = ge->y1;
		x2 = ge->x2;
		y2 = ge->y2;
		x3 = ge->x3;
		y3 = ge->y3;

		if ((x1 - x0) * (y2 - y1) == (x2 - x1) * (y1 - y0)
		    && (x1 - x0) * (y3 - y2) == (x3 - x2) * (y1 - y0)) {
			ge->type = GE_LINE;
		}
	}
}

/*
** After transformations we want to make sure that the resulting
** curve is going in the same quadrant as the original one,
** because rounding errors introduced during transformations
** may make the result completeley wrong.
**
** `dir' argument describes the direction of the original curve,
** it is the superposition of two values for the front and
** rear ends of curve:
**
** 1 - goes over the line connecting the ends
** 0 - coincides with the line connecting the ends
** -1 - goes under the line connecting the ends
*/

/* front end */
#define CVDIR_FUP	0x02	/* goes over the line connecting the ends */
#define CVDIR_FEQUAL	0x01	/* coincides with the line connecting the
				 * ends */
#define CVDIR_FDOWN	0x00	/* goes under the line connecting the ends */

/* rear end */
#define CVDIR_RSAME	0x30	/* is the same as for the front end */
#define CVDIR_RUP	0x20	/* goes over the line connecting the ends */
#define CVDIR_REQUAL	0x10	/* coincides with the line connecting the
				 * ends */
#define CVDIR_RDOWN	0x00	/* goes under the line connecting the ends */

 void
ttf2pt1::fixcvdir(
	 GENTRY * ge,
	 int dir
)
{
	int             a, b, c, d;
	double          kk, kk1, kk2;
	int             changed;
	int             fdir, rdir;

	fdir = (dir & 0x0F) - CVDIR_FEQUAL;
	if ((dir & 0xF0) == CVDIR_RSAME)
		rdir = fdir;
	else
		rdir = ((dir & 0xF0) - CVDIR_REQUAL) >> 4;

	fixcvends(ge);

	c = sign(ge->x3 - ge->prev->x3);	/* note the direction of
						 * curve */
	d = sign(ge->y3 - ge->prev->y3);

	a = ge->y2 - ge->y1;
	b = ge->x2 - ge->x1;
	kk = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
	a = ge->y1 - ge->prev->y3;
	b = ge->x1 - ge->prev->x3;
	kk1 = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
	a = ge->y3 - ge->y2;
	b = ge->x3 - ge->x2;
	kk2 = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));

	changed = 1;
	while (changed) {
		if (0) {
			/* for debugging */
			FPF(stderr, "fixcwdir %d %d (%d %d %d %d %d %d) %f %f %f\n",
				fdir, rdir,
				ge->x1 - ge->prev->x3,
				ge->y1 - ge->prev->y3,
				ge->x2 - ge->x1,
				ge->y2 - ge->y1,
				ge->x3 - ge->x2,
				ge->y3 - ge->y2,
				kk1, kk, kk2);
		}
		changed = 0;

		if (fdir > 0) {
			if (kk1 > kk) {	/* the front end has problems */
				if (c * (ge->x1 - ge->prev->x3) > 0) {
					ge->x1 -= c;
					changed = 1;
				} if (d * (ge->y2 - ge->y1) > 0) {
					ge->y1 += d;
					changed = 1;
				}
				/* recalculate the coefficients */
				a = ge->y2 - ge->y1;
				b = ge->x2 - ge->x1;
				kk = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
				a = ge->y1 - ge->prev->y3;
				b = ge->x1 - ge->prev->x3;
				kk1 = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
			}
		} else if (fdir < 0) {
			if (kk1 < kk) {	/* the front end has problems */
				if (c * (ge->x2 - ge->x1) > 0) {
					ge->x1 += c;
					changed = 1;
				} if (d * (ge->y1 - ge->prev->y3) > 0) {
					ge->y1 -= d;
					changed = 1;
				}
				/* recalculate the coefficients */
				a = ge->y1 - ge->prev->y3;
				b = ge->x1 - ge->prev->x3;
				kk1 = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
				a = ge->y2 - ge->y1;
				b = ge->x2 - ge->x1;
				kk = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
			}
		}
		if (rdir > 0) {
			if (kk2 < kk) {	/* the rear end has problems */
				if (c * (ge->x2 - ge->x1) > 0) {
					ge->x2 -= c;
					changed = 1;
				} if (d * (ge->y3 - ge->y2) > 0) {
					ge->y2 += d;
					changed = 1;
				}
				/* recalculate the coefficients */
				a = ge->y2 - ge->y1;
				b = ge->x2 - ge->x1;
				kk = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
				a = ge->y3 - ge->y2;
				b = ge->x3 - ge->x2;
				kk2 = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
			}
		} else if (rdir < 0) {
			if (kk2 > kk) {	/* the rear end has problems */
				if (c * (ge->x3 - ge->x2) > 0) {
					ge->x2 += c;
					changed = 1;
				} if (d * (ge->y2 - ge->y1) > 0) {
					ge->y2 -= d;
					changed = 1;
				}
				/* recalculate the coefficients */
				a = ge->y2 - ge->y1;
				b = ge->x2 - ge->x1;
				kk = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
				a = ge->y3 - ge->y2;
				b = ge->x3 - ge->x2;
				kk2 = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
			}
		}
	}
	fixcvends(ge);
}

/* Get the directions of ends of curve for further usage */

 int
ttf2pt1::getcvdir(
	 GENTRY * ge
)
{
	int             a, b;
	double          k, k1, k2;
	int             dir = 0;

	a = ge->y2 - ge->y1;
	b = ge->x2 - ge->x1;
	k = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
	a = ge->y1 - ge->prev->y3;
	b = ge->x1 - ge->prev->x3;
	k1 = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));
	a = ge->y3 - ge->y2;
	b = ge->x3 - ge->x2;
	k2 = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : ((double) b / (double) a));

	if (k1 < k)
		dir |= CVDIR_FUP;
	else if (k1 > k)
		dir |= CVDIR_FDOWN;
	else
		dir |= CVDIR_FEQUAL;

	if (k2 > k)
		dir |= CVDIR_RUP;
	else if (k2 < k)
		dir |= CVDIR_RDOWN;
	else
		dir |= CVDIR_REQUAL;

	return dir;
}

/* a function just to test the work of fixcvdir() */
 void
ttf2pt1::testfixcvdir(
	     GLYPH * g
)
{
	GENTRY         *ge;
	int             dir;

#if 0
	if (!strcmp(g->name, "numbersign"))
		debug = 1;
	else
		debug = 0;
#endif

	for (ge = g->entries; ge != 0; ge = ge->next) {
		if (ge->type == GE_CURVE) {
			dir = getcvdir(ge);
			fixcvdir(ge, dir);
		}
	}

	debug = 0;
}


/* check whether we can fix up the curve to change its size by (dx,dy) */
/* 0 means NO, 1 means YES */

 int
ttf2pt1::checkcv(
	GENTRY * ge,
	int dx,
	int dy
)
{
	int             xdep, ydep;

	if (ge->type != GE_CURVE)
		return 0;

	xdep = ge->x3 - ge->prev->x3;
	ydep = ge->y3 - ge->prev->y3;

	if (ge->type == GE_CURVE
	    && (xdep * (xdep + dx)) > 0
	    && (ydep * (ydep + dy)) > 0) {
		return 1;
	} else
		return 0;
}

/* connect the ends of open contours */

 void
ttf2pt1::closepaths(
	   GLYPH * g
)
{
	GENTRY         *ge, *fge;
	int             x = 0, y = 0;
	int             dir;

	for (ge = g->entries; ge != 0; ge = ge->next) {
		if ((fge = ge->first) != 0) {
			if (fge->prev == 0 || fge->prev->type != GE_MOVE) {
				FPF(stderr, "Glyph %s got strange beginning of path\n",
					g->name);
			}
			fge = fge->prev;
			if (fge->x3 != ge->x3 || fge->y3 != ge->y3) {
				/* we have to fix this open path */

				FPF(stderr, "Glyph %s got path open by dx=%d dy=%d\n",
				g->name, fge->x3 - ge->x3, fge->y3 - ge->y3);

				if (abs(ge->x3 - fge->x3) <= 2 && abs(ge->y3 - fge->y3) <= 2) {
					/*
					 * small change, try to correct what
					 * we have
					 */
					int             xopen, yopen, xdep,
					                ydep, fxdep, fydep;

					xopen = fge->x3 - ge->x3;
					yopen = fge->y3 - ge->y3;
					xdep = ge->x3 - ge->prev->x3;
					ydep = ge->y3 - ge->prev->y3;
					fxdep = fge->next->x3 - fge->x3;
					fydep = fge->next->y3 - fge->y3;

					/* first try to fix a curve */
					if (checkcv(ge, xopen, yopen)) {
						dir = getcvdir(ge);
						ge->x2 += xopen;
						ge->x3 += xopen;
						ge->y2 += yopen;
						ge->y3 += yopen;
						fixcvdir(ge, dir);
					} else if (checkcv(fge->next, xopen, yopen)) {
						dir = getcvdir(fge->next);
						fge->x3 -= xopen;
						fge->next->x1 -= xopen;
						fge->y3 -= yopen;
						fge->next->y1 -= yopen;
						fixcvdir(fge->next, dir);

						/* then try to fix a line */
					} else if (ge->type == GE_LINE) {
						ge->x3 += xopen;
						ge->y3 += yopen;
					} else if (fge->next->type == GE_LINE) {
						fge->x3 -= xopen;
						fge->y3 -= yopen;

						/*
						 * and as the last resort
						 * draw a new line
						 */
					} else {
						GENTRY         *nge;

						nge = newgentry();
						nge->x3 = fge->x3;
						nge->y3 = fge->y3;
						nge->type = GE_LINE;

						nge->prev = ge;
						nge->next = ge->next;
						nge->first = ge->first;

						ge->next->prev = nge;
						ge->next = nge;
						ge->first = 0;
						ge = nge;
					}
				} else {
					/* big change, add new line */
					GENTRY         *nge;

					nge = newgentry();
					nge->x3 = fge->x3;
					nge->y3 = fge->y3;
					nge->type = GE_LINE;

					nge->prev = ge;
					nge->next = ge->next;
					nge->first = ge->first;

					ge->next->prev = nge;
					ge->next = nge;
					ge->first = 0;
					ge = nge;
				}
			}
		}
	}
}

 void
ttf2pt1::smoothjoints(
	     GLYPH * g
)
{
	GENTRY         *ge, *ne;
	int             dx1, dy1, dx2, dy2, k;
	int             dir;

	if (g->entries == 0)	/* nothing to do */
		return;

	for (ge = g->entries->next; ge != 0; ge = ge->next) {
		if (ge->first) {
			ne = ge->first;
		} else {
			ne = ge->next;	/* previous entry */
		}

		/*
		 * although there should be no one-line path * and any path
		 * must end with CLOSEPATH, * nobody can say for sure
		 */

		if (ge == ne || ne == 0)
			continue;

		/* now handle various joints */

		if (ge->type == GE_LINE && ne->type == GE_LINE) {
			dx1 = ge->x3 - ge->prev->x3;
			dy1 = ge->y3 - ge->prev->y3;
			dx2 = ne->x3 - ge->x3;
			dy2 = ne->y3 - ge->y3;

			/* check whether they have the same direction */
			/* and the same slope */
			/* then we can join them into one line */

			if (dx1 * dx2 >= 0 && dy1 * dy2 >= 0 && dx1 * dy2 == dy1 * dx2) {
				if (ge->first) {
					/*
					 * move the starting point of the
					 * path
					 */
					ne->prev->x3 = ge->x3 = ne->x3;
					ne->prev->y3 = ge->y3 = ne->y3;

					/* get rid of the last line of path */
					ne->prev->next = ne->next;
					ne->next->prev = ne->prev;
					ge->first = ne->next;
					free(ne);
				} else {
					/* extend the previous line */
					ge->x3 = ne->x3;
					ge->y3 = ne->y3;

					/* and get rid of the next line */
					ge->first = ne->first;
					ne->prev->next = ne->next;
					ne->next->prev = ne->prev;
					free(ne);
				}
			}
		} else if (ge->type == GE_LINE && ne->type == GE_CURVE) {
			fixcvends(ne);

			dx1 = ge->x3 - ge->prev->x3;
			dy1 = ge->y3 - ge->prev->y3;
			dx2 = ne->x1 - ge->x3;
			dy2 = ne->y1 - ge->y3;

			/* if the line is nearly horizontal and we can fix it */
			if (dx1 != 0 && 5 * abs(dy1) / abs(dx1) == 0
			    && checkcv(ne, 0, -dy1)
			    && abs(dy1) <= 4) {
				dir = getcvdir(ne);
				ge->y3 -= dy1;
				ne->y1 -= dy1;
				fixcvdir(ne, dir);
				if (ge->first)
					ne->prev->y3 -= dy1;
				dy1 = 0;
			} else if (dy1 != 0 && 5 * abs(dx1) / abs(dy1) == 0
				   && checkcv(ne, -dx1, 0)
				   && abs(dx1) <= 4) {
				/* the same but vertical */
				dir = getcvdir(ne);
				ge->x3 -= dx1;
				ne->x1 -= dx1;
				fixcvdir(ne, dir);
				if (ge->first)
					ne->prev->x3 -= dx1;
				dx1 = 0;
			}
			/*
			 * if line is horizontal and curve begins nearly
			 * horizontally
			 */
			if (dy1 == 0 && dx2 != 0 && 5 * abs(dy2) / abs(dx2) == 0) {
				dir = getcvdir(ne);
				ne->y1 -= dy2;
				fixcvdir(ne, dir);
				dy2 = 0;
			} else if (dx1 == 0 && dy2 != 0 && 5 * abs(dx2) / abs(dy2) == 0) {
				/* the same but vertical */
				dir = getcvdir(ne);
				ne->x1 -= dx2;
				fixcvdir(ne, dir);
				dx2 = 0;
			}
		} else if (ge->type == GE_CURVE && ne->type == GE_LINE) {
			fixcvends(ge);

			dx1 = ge->x3 - ge->x2;
			dy1 = ge->y3 - ge->y2;
			dx2 = ne->x3 - ge->x3;
			dy2 = ne->y3 - ge->y3;

			/* if the line is nearly horizontal and we can fix it */
			if (dx2 != 0 && 5 * abs(dy2) / abs(dx2) == 0
			    && checkcv(ge, 0, dy2)
			    && abs(dy2) <= 4) {
				dir = getcvdir(ge);
				ge->y3 += dy2;
				ge->y2 += dy2;
				fixcvdir(ge, dir);
				if (ge->first)
					ne->prev->y3 += dy2;
				dy2 = 0;
			} else if (dy2 != 0 && 5 * abs(dx2) / abs(dy2) == 0
				   && checkcv(ge, dx2, 0)
				   && abs(dx2) <= 4) {
				/* the same but vertical */
				dir = getcvdir(ge);
				ge->x3 += dx2;
				ge->x2 += dx2;
				fixcvdir(ge, dir);
				if (ge->first)
					ne->prev->x3 += dx2;
				dx2 = 0;
			}
			/*
			 * if line is horizontal and curve ends nearly
			 * horizontally
			 */
			if (dy2 == 0 && dx1 != 0 && 5 * abs(dy1) / abs(dx1) == 0) {
				dir = getcvdir(ge);
				ge->y2 += dy1;
				fixcvdir(ge, dir);
				dy1 = 0;
			} else if (dx2 == 0 && dy1 != 0 && 5 * abs(dx1) / abs(dy1) == 0) {
				/* the same but vertical */
				dir = getcvdir(ge);
				ge->x2 += dx1;
				fixcvdir(ge, dir);
				dx1 = 0;
			}
		} else if (ge->type == GE_CURVE && ne->type == GE_CURVE) {
			fixcvends(ge);
			fixcvends(ne);

			dx1 = ge->x3 - ge->x2;
			dy1 = ge->y3 - ge->y2;
			dx2 = ne->x1 - ge->x3;
			dy2 = ne->y1 - ge->y3;

			/*
			 * check if we have a rather smooth joint at extremal
			 * point
			 */
			/* left or right extremal point */
			if (abs(dx1) <= 4 && abs(dx2) <= 4
			    && dy1 != 0 && 5 * abs(dx1) / abs(dy1) == 0
			    && dy2 != 0 && 5 * abs(dx2) / abs(dy2) == 0
			    && (ge->y3 < ge->prev->y3 && ne->y3 < ge->y3
				|| ge->y3 > ge->prev->y3 && ne->y3 > ge->y3)
			  && (ge->x3 - ge->prev->x3) * (ne->x3 - ge->x3) < 0
				) {
				dir = getcvdir(ge);
				ge->x2 += dx1;
				dx1 = 0;
				fixcvdir(ge, dir);
				dir = getcvdir(ne);
				ne->x1 -= dx2;
				dx2 = 0;
				fixcvdir(ne, dir);
			}
			/* top or down extremal point */
			else if (abs(dy1) <= 4 && abs(dy2) <= 4
				 && dx1 != 0 && 5 * abs(dy1) / abs(dx1) == 0
				 && dx2 != 0 && 5 * abs(dy2) / abs(dx2) == 0
				 && (ge->x3 < ge->prev->x3 && ne->x3 < ge->x3
				|| ge->x3 > ge->prev->x3 && ne->x3 > ge->x3)
				 && (ge->y3 - ge->prev->y3) * (ne->y3 - ge->y3) < 0
				) {
				dir = getcvdir(ge);
				ge->y2 += dy1;
				dy1 = 0;
				fixcvdir(ge, dir);
				dir = getcvdir(ne);
				ne->y1 -= dy2;
				dy2 = 0;
				fixcvdir(ne, dir);
			}
			/* or may be we just have a smooth junction */
			else if (dx1 * dx2 >= 0 && dy1 * dy2 >= 0
				 && 10 * abs(k = abs(dx1 * dy2) - abs(dy1 * dx2)) < (abs(dx1 * dy2) + abs(dy1 * dx2))) {
				int             tries[6][4];
				int             results[6];
				int             i, b;

				/* build array of changes we are going to try */
				/* uninitalized entries are 0 */
				if (k > 0) {
					/*static*/ int      t1[6][4] = {
						{0, 0, 0, 0},
						{-1, 0, 1, 0},
						{-1, 0, 0, 1},
						{0, -1, 1, 0},
						{0, -1, 0, 1},
					{-1, -1, 1, 1}};
					memcpy(tries, t1, sizeof tries);
				} else {
					/*static*/ int      t1[6][4] = {
						{0, 0, 0, 0},
						{1, 0, -1, 0},
						{1, 0, 0, -1},
						{0, 1, -1, 0},
						{0, 1, 0, -1},
					{1, 1, -1, -1}};
					memcpy(tries, t1, sizeof tries);
				}

				/* now try the changes */
				results[0] = abs(k);
				for (i = 1; i < 6; i++) {
					results[i] = abs((abs(dx1) + tries[i][0]) * (abs(dy2) + tries[i][1]) -
							 (abs(dy1) + tries[i][2]) * (abs(dx2) + tries[i][3]));
				}

				/* and find the best try */
				k = abs(k);
				b = 0;
				for (i = 1; i < 6; i++)
					if (results[i] < k) {
						k = results[i];
						b = i;
					}
				/* and finally apply it */
				if (dx1 < 0)
					tries[b][0] = -tries[b][0];
				if (dy2 < 0)
					tries[b][1] = -tries[b][1];
				if (dy1 < 0)
					tries[b][2] = -tries[b][2];
				if (dx2 < 0)
					tries[b][3] = -tries[b][3];

				dir = getcvdir(ge);
				ge->x2 -= tries[b][0];
				ge->y2 -= tries[b][2];
				fixcvdir(ge, dir);
				dir = getcvdir(ne);
				ne->x1 += tries[b][3];
				ne->y1 += tries[b][1];
				fixcvdir(ne, dir);
			}
		}
	}
}

/* debugging: print out stems of a glyph */
 void
ttf2pt1::debugstems(
	   char *name,
	   STEM * hstems,
	   int nhs,
	   STEM * vstems,
	   int nvs
)
{
	int             i;

	fprintf(pfa_file, "%% %s\n", name);
	fprintf(pfa_file, "%% %d horizontal stems:\n", nhs);
	for (i = 0; i < nhs; i++)
		fprintf(pfa_file, "%%     %d (%d...%d) %c %c%c\n", hstems[i].value,
			hstems[i].from, hstems[i].to,
			((hstems[i].flags & ST_UP) ? 'U' : 'D'),
			((hstems[i].flags & ST_FLAT) ? 'F' : '-'),
			((hstems[i].flags & ST_END) ? 'E' : '-'));
	fprintf(pfa_file, "%% %d vertical stems:\n", nvs);
	for (i = 0; i < nvs; i++)
		fprintf(pfa_file, "%%     %d (%d...%d) %c %c%c\n", vstems[i].value,
			vstems[i].from, vstems[i].to,
			((vstems[i].flags & ST_UP) ? 'U' : 'D'),
			((vstems[i].flags & ST_END) ? 'E' : '-'),
			((vstems[i].flags & ST_FLAT) ? 'F' : '-'));
}

/* add pseudo-stems for the limits of the Blue zones to the stem array */
 int
ttf2pt1::addbluestems(
	STEM *s,
	int n
)
{
	int i;

	for(i=0; i<nblues; i+=2) {
		s[n].value=bluevalues[i];
		s[n].flags=ST_UP|ST_ZONE|ST_TOPZONE;
		/* don't overlap with anything */
		s[n].origin=s[n].from=s[n].to= -10000+i;
		n++;
		s[n].value=bluevalues[i+1];
		s[n].flags=ST_ZONE|ST_TOPZONE;
		/* don't overlap with anything */
		s[n].origin=s[n].from=s[n].to= -10000+i+1;
		n++;
	}
	for(i=0; i<notherb; i+=2) {
		s[n].value=otherblues[i];
		s[n].flags=ST_UP|ST_ZONE;
		/* don't overlap with anything */
		s[n].origin=s[n].from=s[n].to= -10000+i+nblues;
		n++;
		s[n].value=otherblues[i+1];
		s[n].flags=ST_ZONE;
		/* don't overlap with anything */
		s[n].origin=s[n].from=s[n].to= -10000+i+1+nblues;
		n++;
	}
	return n;
}

/* sort stems in array */
 void
ttf2pt1::sortstems(
	  STEM * s,
	  int n
)
{
	int             i, j;
	STEM            x;


	/* a simple sorting */
	/* hm, the ordering criteria is not quite simple :-) 
	 * ST_UP always goes under not ST_UP
	 * ST_ZONE goes on the most outer side
	 * ST_END goes towards inner side after ST_ZONE
	 * ST_FLAT goes on the inner side
	 */

	for (i = 0; i < n; i++)
		for (j = i + 1; j < n; j++) {
			if(s[i].value < s[j].value)
				continue;
			if(s[i].value == s[j].value) {
				if( (s[i].flags & ST_UP) < (s[j].flags & ST_UP) )
					continue;
				if( (s[i].flags & ST_UP) == (s[j].flags & ST_UP) ) {
					if( s[i].flags & ST_UP ) {
						if(
						(s[i].flags & (ST_ZONE|ST_FLAT|ST_END) ^ ST_FLAT)
							>
						(s[j].flags & (ST_ZONE|ST_FLAT|ST_END) ^ ST_FLAT)
						)
							continue;
					} else {
						if(
						(s[i].flags & (ST_ZONE|ST_FLAT|ST_END) ^ ST_FLAT)
							<
						(s[j].flags & (ST_ZONE|ST_FLAT|ST_END) ^ ST_FLAT)
						)
							continue;
					}
				}
			}
			x = s[j];
			s[j] = s[i];
			s[i] = x;
		}
}

/* check whether two stem borders overlap */

 int
ttf2pt1::stemoverlap(
	    STEM * s1,
	    STEM * s2
)
{
	int             result;

	if (s1->from <= s2->from && s1->to >= s2->from
	    || s2->from <= s1->from && s2->to >= s1->from)
		result = 1;
	else
		result = 0;

	if (debug)
		fprintf(pfa_file, "%% overlap %d(%d..%d)x%d(%d..%d)=%d\n",
			s1->value, s1->from, s1->to, s2->value, s2->from, s2->to, result);
	return result;
}

/* 
 * check if the stem [border] is in an appropriate blue zone
 */

 int
ttf2pt1::steminblue(
	STEM *s
)
{
	int i, val;

	val=s->value;
	if(s->flags & ST_UP) {
		/* painted size up, look at lower zones */
		if(nblues>=2 && val>=bluevalues[0] && val<=bluevalues[1] )
			return 1;
		for(i=0; i<notherb; i++) {
			if( val>=otherblues[i] && val<=otherblues[i+1] )
				return 1;
		}
	} else {
		/* painted side down, look at upper zones */
		for(i=2; i<nblues; i++) {
			if( val>=bluevalues[i] && val<=bluevalues[i+1] )
				return 1;
		}
	}

	return 0;
}

/* eliminate invalid stems, join equivalent lines and remove nested stems */
 int
ttf2pt1::joinstems(
	  STEM * s,
	  int nold,
	  int useblues /* do we use the blue values ? */
)
{
#define MAX_STACK	1000
	STEM            stack[MAX_STACK];
	int             nstack = 0;
	int             sbottom = 0;
	int             nnew;
	int             i, j, k;
	int             a, b, c, w1, w2, w3;
	int             fw, fd;
	/*
	 * priority of the last found stem: 
	 * 0 - nothing found yet 
	 * 1 - has ST_END in it (one or more) 
	 * 2 - has no ST_END and no ST_FLAT, can override only one stem 
	 *     with priority 1 
	 * 3 - has no ST_END and at least one ST_FLAT, can override one 
	 *     stem with priority 2 or any number of stems with priority 1
	 * 4 (handled separately) - has ST_BLUE, can override anything
	 */
	int             readystem = 0;
	int             pri;
	int             nlps = 0;	/* number of non-committed
					 * lowest-priority stems */

	if(useblues) {
		/*
		 * traverse the list of Blue Values, mark the lowest upper
		 * stem in each bottom zone and the topmost lower stem in
		 * each top zone with ST_BLUE
		 */

		/* top zones */
		for(i=2; i<nblues; i+=2) {
			a=bluevalues[i]; b=bluevalues[i+1];
			if(debug)
				fprintf(pfa_file, "%% looking at blue zone %d...%d\n", a, b);
			for(j=nold-1; j>=0; j--) {
				if( s[j].flags & (ST_INVALID|ST_ZONE|ST_UP|ST_END) )
					continue;
				c=s[j].value;
				if(c<a) /* too low */
					break;
				if(c<=b) { /* found the topmost stem border */
					/* mark all the stems with the same value */
					if(debug)
						fprintf(pfa_file, "%% found D BLUE at %d\n", s[j].value);
					/* include ST_END values */
					while( s[j+1].value==c && (s[j+1].flags & ST_ZONE)==0 )
						j++;
					s[j].flags |= ST_BLUE;
					for(j--; j>=0 && s[j].value==c 
							&& (s[j].flags & (ST_UP|ST_ZONE))==0 ; j--)
						s[j].flags |= ST_BLUE;
					break;
				}
			}
		}
		/* baseline */
		if(nblues>=2) {
			a=bluevalues[0]; b=bluevalues[1];
			for(j=0; j<nold; j++) {
				if( (s[j].flags & (ST_INVALID|ST_ZONE|ST_UP|ST_END))!=ST_UP )
					continue;
				c=s[j].value;
				if(c>b) /* too high */
					break;
				if(c>=a) { /* found the topmost stem border */
					/* mark all the stems with the same value */
					if(debug)
						fprintf(pfa_file, "%% found U BLUE at %d\n", s[j].value);
					/* include ST_END values */
					while( s[j-1].value==c && (s[j-1].flags & ST_ZONE)==0 )
						j--;
					s[j].flags |= ST_BLUE;
					for(j++; j<nold && s[j].value==c
							&& (s[j].flags & (ST_UP|ST_ZONE))==ST_UP ; j++)
						s[j].flags |= ST_BLUE;
					break;
				}
			}
		}
		/* bottom zones: the logic is the same as for baseline */
		for(i=0; i<notherb; i+=2) {
			a=otherblues[i]; b=otherblues[i+1];
			for(j=0; j<nold; j++) {
				if( (s[j].flags & (ST_INVALID|ST_UP|ST_ZONE|ST_END))!=ST_UP )
					continue;
				c=s[j].value;
				if(c>b) /* too high */
					break;
				if(c>=a) { /* found the topmost stem border */
					/* mark all the stems with the same value */
					if(debug)
						fprintf(pfa_file, "%% found U BLUE at %d\n", s[j].value);
					/* include ST_END values */
					while( s[j-1].value==c && (s[j-1].flags & ST_ZONE)==0 )
						j--;
					s[j].flags |= ST_BLUE;
					for(j++; j<nold && s[j].value==c
							&& (s[j].flags & (ST_UP|ST_ZONE))==ST_UP ; j++)
						s[j].flags |= ST_BLUE;
					break;
				}
			}
		}
	}

	for (i = 0, nnew = 0; i < nold; i++) {
		if (s[i].flags & (ST_UP|ST_ZONE)) {
			if(s[i].flags & ST_BLUE) {
				/* we just HAVE to use this value */
				if (readystem)
					nnew += 2;
				readystem=0;

				/* remember the list of Blue zone stems with the same value */
				for(a=i, i++; i<nold && s[a].value==s[i].value
					&& (s[i].flags & ST_BLUE); i++)
					{}
				b=i; /* our range is a <= i < b */
				c= -1; /* index of our best guess up to now */
				pri=0;
				/* try to find a match, don't cross blue zones */
				for(; i<nold && (s[i].flags & ST_BLUE)==0; i++) {
					if(s[i].flags & ST_UP)
						if(s[i].flags & ST_TOPZONE)
							break;
						else
							continue;
					for(j=a; j<b; j++) {
						if(!stemoverlap(&s[j], &s[i]) )
							continue;
						/* consider priorities */
						if( ( (s[j].flags|s[i].flags) & (ST_FLAT|ST_END) )==ST_FLAT ) {
							c=i;
							goto bluematch;
						}
						if( ((s[j].flags|s[i].flags) & ST_END)==0 )  {
							if(pri < 2) {
								c=i; pri=2;
							}
						} else {
							if(pri == 0) {
								c=i; pri=1;
							}
						}
					}
				}
			bluematch:
				/* clean up the stack */
				nstack=sbottom=0;
				readystem=0;
				/* add this stem */
				s[nnew++]=s[a];
				if(c<0) { /* make one-dot-wide stem */
					if(nnew>=b) { /* have no free space */
						for(j=nold; j>=b; j--) /* make free space */
							s[j]=s[j-1];
						b++;
						nold++;
					}
					s[nnew]=s[a];
					s[nnew].flags &= ~(ST_UP|ST_BLUE);
					nnew++;
					i=b-1;
				} else {
					s[nnew++]=s[c];
					i=c; /* skip up to this point */
				}
				if (debug)
					fprintf(pfa_file, "%% +stem %d...%d U BLUE\n",
						s[nnew-2].value, s[nnew-1].value);
			} else {
				if (nstack >= MAX_STACK) {
					FPF(stderr, "Warning: **** stem stack overflow ****\n");
					nstack = 0;
				}
				stack[nstack++] = s[i];
			}
		} else if(s[i].flags & ST_BLUE) {
			/* again, we just HAVE to use this value */
			if (readystem)
				nnew += 2;
			readystem=0;

			/* remember the list of Blue zone stems with the same value */
			for(a=i, i++; i<nold && s[a].value==s[i].value
				&& (s[i].flags & ST_BLUE); i++)
				{}
			b=i; /* our range is a <= i < b */
			c= -1; /* index of our best guess up to now */
			pri=0;
			/* try to find a match */
			for (i = nstack - 1; i >= 0; i--) {
				if( (stack[i].flags & ST_UP)==0 )
					if( (stack[i].flags & (ST_ZONE|ST_TOPZONE))==ST_ZONE )
						break;
					else
						continue;
				for(j=a; j<b; j++) {
					if(!stemoverlap(&s[j], &stack[i]) )
						continue;
					/* consider priorities */
					if( ( (s[j].flags|stack[i].flags) & (ST_FLAT|ST_END) )==ST_FLAT ) {
						c=i;
						goto bluedownmatch;
					}
					if( ((s[j].flags|stack[i].flags) & ST_END)==0 )  {
						if(pri < 2) {
							c=i; pri=2;
						}
					} else {
						if(pri == 0) {
							c=i; pri=1;
						}
					}
				}
			}
		bluedownmatch:
			/* if found no match make a one-dot-wide stem */
			if(c<0) {
				c=0;
				stack[0]=s[b-1];
				stack[0].flags |= ST_UP;
				stack[0].flags &= ~ST_BLUE;
			}
			/* remove all the stems conflicting with this one */
			readystem=0;
			for(j=nnew-2; j>=0; j-=2) {
				if (debug)
					fprintf(pfa_file, "%% ?stem %d...%d -- %d\n",
						s[j].value, s[j+1].value, stack[c].value);
				if(s[j+1].value < stack[c].value) /* no conflict */
					break;
				if(s[j].flags & ST_BLUE) {
					/* oops, we don't want to spoil other blue zones */
					stack[c].value=s[j+1].value+1;
					break;
				}
				if( (s[j].flags|s[j+1].flags) & ST_END ) {
					if (debug)
						fprintf(pfa_file, "%% -stem %d...%d p=1\n",
							s[j].value, s[j+1].value);
					continue; /* pri==1, silently discard it */
				}
				/* we want to discard no nore than 2 stems of pri>=2 */
				if( ++readystem > 2 ) {
					/* change our stem to not conflict */
					stack[c].value=s[j+1].value+1;
					break;
				} else {
					if (debug)
						fprintf(pfa_file, "%% -stem %d...%d p>=2\n",
							s[j].value, s[j+1].value);
					continue;
				}
			}
			nnew=j+2;
			/* add this stem */
			if(nnew>=b-1) { /* have no free space */
				for(j=nold; j>=b-1; j--) /* make free space */
					s[j]=s[j-1];
				b++;
				nold++;
			}
			s[nnew++]=stack[c];
			s[nnew++]=s[b-1];
			/* clean up the stack */
			nstack=sbottom=0;
			readystem=0;
			/* set the next position to search */
			i=b-1;
			if (debug)
				fprintf(pfa_file, "%% +stem %d...%d D BLUE\n",
					s[nnew-2].value, s[nnew-1].value);
		} else if (nstack > 0) {

			/*
			 * check whether our stem overlaps with anything in
			 * stack
			 */
			for (j = nstack - 1; j >= sbottom; j--) {
				if (s[i].value <= stack[j].value)
					break;
				if (stack[j].flags & ST_ZONE)
					continue;

				if ((s[i].flags & ST_END)
				    || (stack[j].flags & ST_END))
					pri = 1;
				else if ((s[i].flags & ST_FLAT)
					 || (stack[j].flags & ST_FLAT))
					pri = 3;
				else
					pri = 2;

				if (pri < readystem && s[nnew + 1].value >= stack[j].value
				    || !stemoverlap(&stack[j], &s[i]))
					continue;

				if (readystem > 1 && s[nnew + 1].value < stack[j].value) {
					nnew += 2;
					readystem = 0;
					nlps = 0;
				}
				/*
				 * width of the previous stem (if it's
				 * present)
				 */
				w1 = s[nnew + 1].value - s[nnew].value;

				/* width of this stem */
				w2 = s[i].value - stack[j].value;

				if (readystem == 0) {
					/* nothing yet, just add a new stem */
					s[nnew] = stack[j];
					s[nnew + 1] = s[i];
					readystem = pri;
					if (pri == 1)
						nlps = 1;
					else if (pri == 2)
						sbottom = j;
					else {
						sbottom = j + 1;
						while (sbottom < nstack
						       && stack[sbottom].value <= stack[j].value)
							sbottom++;
					}
					if (debug)
						fprintf(pfa_file, "%% +stem %d...%d p=%d n=%d\n",
							stack[j].value, s[i].value, pri, nlps);
				} else if (pri == 1) {
					if (stack[j].value > s[nnew + 1].value) {
						/*
						 * doesn't overlap with the
						 * previous one
						 */
						nnew += 2;
						nlps++;
						s[nnew] = stack[j];
						s[nnew + 1] = s[i];
						if (debug)
							fprintf(pfa_file, "%% +stem %d...%d p=%d n=%d\n",
								stack[j].value, s[i].value, pri, nlps);
					} else if (w2 < w1) {
						/* is narrower */
						s[nnew] = stack[j];
						s[nnew + 1] = s[i];
						if (debug)
							fprintf(pfa_file, "%% /stem %d...%d p=%d n=%d %d->%d\n",
								stack[j].value, s[i].value, pri, nlps, w1, w2);
					}
				} else if (pri == 2) {
					if (readystem == 2) {
						/* choose the narrower stem */
						if (w1 > w2) {
							s[nnew] = stack[j];
							s[nnew + 1] = s[i];
							sbottom = j;
							if (debug)
								fprintf(pfa_file, "%% /stem %d...%d p=%d n=%d\n",
									stack[j].value, s[i].value, pri, nlps);
						}
						/* else readystem==1 */
					} else if (stack[j].value > s[nnew + 1].value) {
						/*
						 * value doesn't overlap with
						 * the previous one
						 */
						nnew += 2;
						nlps = 0;
						s[nnew] = stack[j];
						s[nnew + 1] = s[i];
						sbottom = j;
						readystem = pri;
						if (debug)
							fprintf(pfa_file, "%% +stem %d...%d p=%d n=%d\n",
								stack[j].value, s[i].value, pri, nlps);
					} else if (nlps == 1
						   || stack[j].value > s[nnew - 1].value) {
						/*
						 * we can replace the top
						 * stem
						 */
						nlps = 0;
						s[nnew] = stack[j];
						s[nnew + 1] = s[i];
						readystem = pri;
						sbottom = j;
						if (debug)
							fprintf(pfa_file, "%% /stem %d...%d p=%d n=%d\n",
								stack[j].value, s[i].value, pri, nlps);
					}
				} else if (readystem == 3) {	/* that means also
								 * pri==3 */
					/* choose the narrower stem */
					if (w1 > w2) {
						s[nnew] = stack[j];
						s[nnew + 1] = s[i];
						sbottom = j + 1;
						while (sbottom < nstack
						       && stack[sbottom].value <= stack[j].value)
							sbottom++;
						if (debug)
							fprintf(pfa_file, "%% /stem %d...%d p=%d n=%d\n",
								stack[j].value, s[i].value, pri, nlps);
					}
				} else if (pri == 3) {
					/*
					 * we can replace as many stems as
					 * neccessary
					 */
					nnew += 2;
					while (nnew > 0 && s[nnew - 1].value >= stack[j].value) {
						nnew -= 2;
						if (debug)
							fprintf(pfa_file, "%% -stem %d..%d\n",
								s[nnew].value, s[nnew + 1].value);
					}
					nlps = 0;
					s[nnew] = stack[j];
					s[nnew + 1] = s[i];
					readystem = pri;
					sbottom = j + 1;
					while (sbottom < nstack
					       && stack[sbottom].value <= stack[j].value)
						sbottom++;
					if (debug)
						fprintf(pfa_file, "%% +stem %d...%d p=%d n=%d\n",
							stack[j].value, s[i].value, pri, nlps);
				}
			}
		}
	}
	if (readystem)
		nnew += 2;

	/* change the 1-pixel-wide stems to 20-pixel-wide stems if possible 
	 * the constant 20 is recommended in the Type1 manual 
	 */
	if(useblues) {
		for(i=0; i<nnew; i+=2) {
			if(s[i].value != s[i+1].value)
				continue;
			if( ((s[i].flags ^ s[i+1].flags) & ST_BLUE)==0 )
				continue;
			if( s[i].flags & ST_BLUE ) {
				if(nnew>i+2 && s[i+2].value<s[i].value+20)
					s[i+1].value=s[i+2].value-1;
				else
					s[i+1].value+=19;
			} else {
				if(i>0 && s[i-1].value>s[i].value-20)
					s[i].value=s[i-1].value+1;
				else
					s[i].value-=19;
			}
		}
	}
	/* make sure that no stem it stretched between
	 * a top zone and a bottom zone
	 */
	if(useblues) {
		for(i=0; i<nnew; i+=2) {
			a=10000; /* lowest border of top zone crosing the stem */
			b= -10000; /* highest border of bottom zone crossing the stem */

			for(j=2; j<nblues; j++) {
				c=bluevalues[j];
				if( c>=s[i].value && c<=s[i+1].value && c<a )
					a=c;
			}
			if(nblues>=2) {
				c=bluevalues[1];
				if( c>=s[i].value && c<=s[i+1].value && c>b )
					b=c;
			}
			for(j=1; j<notherb; j++) {
				c=otherblues[j];
				if( c>=s[i].value && c<=s[i+1].value && c>b )
					b=c;
			}
			if( a!=10000 && b!= -10000 ) { /* it is stretched */
				/* split the stem into 2 ghost stems */
				for(j=nnew+1; j>i+1; j--) /* make free space */
					s[j]=s[j-2];
				nnew+=2;

				if(s[i].value+19 >= a)
					s[i+1].value=a-1;
				else
					s[i+1].value=s[i].value+19;

				if(s[i+3].value-19 <= b)
					s[i+2].value=b+1;
				else
					s[i+2].value=s[i+3].value-19;

				i+=2;
			}
		}
	}
	/* look for triple stems */
	for (i = 0; i < nnew; i += 2) {
		if (nnew - i >= 6) {
			a = s[i].value + s[i + 1].value;
			b = s[i + 2].value + s[i + 3].value;
			c = s[i + 4].value + s[i + 5].value;

			w1 = s[i + 1].value - s[i].value;
			w2 = s[i + 3].value - s[i + 2].value;
			w3 = s[i + 5].value - s[i + 4].value;

			fw = w3 - w1;	/* fuzz in width */
			fd = ((c - b) - (b - a));	/* fuzz in distance
							 * (doubled) */

			/* we are able to handle some fuzz */
			/*
			 * it doesn't hurt if the declared stem is a bit
			 * narrower than actual unless it's an edge in
			 * a blue zone
			 */
			if (abs(abs(fd) - abs(fw)) * 5 < w2
			    && abs(fw) * 20 < (w1 + w3)) {	/* width dirrerence <10% */

				if(useblues) { /* check that we don't disturb any blue stems */
					j=c; k=a;
					if (fw > 0) {
						if (fd > 0) {
							if( s[i+5].flags & ST_BLUE )
								continue;
							j -= fw;
						} else {
							if( s[i+4].flags & ST_BLUE )
								continue;
							j += fw;
						}
					} else if(fw < 0) {
						if (fd > 0) {
							if( s[i+1].flags & ST_BLUE )
								continue;
							k -= fw;
						} else {
							if( s[i].flags & ST_BLUE )
								continue;
							k += fw;
						}
					}
					pri = ((j - b) - (b - k));

					if (pri > 0) {
						if( s[i+2].flags & ST_BLUE )
							continue;
					} else if(pri < 0) {
						if( s[i+3].flags & ST_BLUE )
							continue;
					}
				}

				/*
				 * first fix up the width of 1st and 3rd
				 * stems
				 */
				if (fw > 0) {
					if (fd > 0) {
						s[i + 5].value -= fw;
						c -= fw;
					} else {
						s[i + 4].value += fw;
						c += fw;
					}
				} else {
					if (fd > 0) {
						s[i + 1].value -= fw;
						a -= fw;
					} else {
						s[i].value += fw;
						a += fw;
					}
				}
				fd = ((c - b) - (b - a));

				if (fd > 0) {
					s[i + 2].value += abs(fd) / 2;
				} else {
					s[i + 3].value -= abs(fd) / 2;
				}

				s[i].flags |= ST_3;
				i += 4;
			}
		}
	}

	return (nnew & ~1);	/* number of lines must be always even */
}

 void
ttf2pt1::buildstems(
	   GLYPH * g
)
{
	STEM            hs[MAX_STEMS], vs[MAX_STEMS];	/* temporary working
							 * storage */
	STEM           *sp;
	GENTRY         *ge, *nge, *pge;
	int             nx, ny;

#if 0
	if (!strcmp(g->name, "zero"))
		debug = 1;
#endif

	g->nhs = g->nvs = 0;

	/* first search the whole character for possible stem points */

	for (ge = g->entries; ge != 0; ge = ge->next) {
		if (ge->type == GE_CURVE) {

			/*
			 * SURPRISE! 
			 * We consider the stems bounded by the
			 * H/V ends of the curves as flat ones.
			 *
			 * But we don't include the point on the
			 * other end into the range.
			 */

			/* first check the beginning of curve */
			/* if it is horizontal, add a hstem */
			if (ge->y1 == ge->prev->y3) {
				hs[g->nhs].value = ge->y1;

				if (ge->x1 < ge->prev->x3)
					hs[g->nhs].flags = ST_FLAT | ST_UP;
				else
					hs[g->nhs].flags = ST_FLAT;

				hs[g->nhs].origin = ge->prev->x3;

				if ((hs[g->nhs].flags & ST_UP) ?
				    (ge->y3 >= ge->prev->y3) :
				    (ge->y3 <= ge->prev->y3)
					) {
					if (ge->x3 < ge->prev->x3) {
						hs[g->nhs].from = ge->x3+1;
						hs[g->nhs].to = ge->prev->x3;
						if(hs[g->nhs].from > hs[g->nhs].to)
							hs[g->nhs].from--;
					} else {
						hs[g->nhs].from = ge->prev->x3;
						hs[g->nhs].to = ge->x3-1;
						if(hs[g->nhs].from > hs[g->nhs].to)
							hs[g->nhs].to++;
					}
				} else {
					hs[g->nhs].from
						= hs[g->nhs].to = ge->prev->x3;
				}
				if (ge->x1 != ge->prev->x3)
					g->nhs++;
			}
			/* if it is vertical, add a vstem */
			else if (ge->x1 == ge->prev->x3) {
				vs[g->nvs].value = ge->x1;

				if (ge->y1 > ge->prev->y3)
					vs[g->nvs].flags = ST_FLAT | ST_UP;
				else
					vs[g->nvs].flags = ST_FLAT;

				vs[g->nvs].origin = ge->prev->y3;

				if ((vs[g->nvs].flags & ST_UP) ?
				    (ge->x3 >= ge->prev->x3) :
				    (ge->x3 <= ge->prev->x3)
					) {
					if (ge->y3 < ge->prev->y3) {
						vs[g->nvs].from = ge->y3+1;
						vs[g->nvs].to = ge->prev->y3;
						if(vs[g->nvs].from > vs[g->nvs].to)
							vs[g->nvs].from--;
					} else {
						vs[g->nvs].from = ge->prev->y3;
						vs[g->nvs].to = ge->y3-1;
						if(vs[g->nvs].from > vs[g->nvs].to)
							vs[g->nvs].to++;
					}
				} else {
					vs[g->nvs].from
						= vs[g->nvs].to = ge->prev->y3;
				}

				if (ge->y1 != ge->prev->y3)
					g->nvs++;
			}
			/* then check the end of curve */
			/* if it is horizontal, add a hstem */
			/* and possibly two vstems at the ends */
			if (ge->y3 == ge->y2) {
				hs[g->nhs].value = ge->y3;

				if (ge->x3 < ge->x2)
					hs[g->nhs].flags = ST_FLAT | ST_UP;
				else
					hs[g->nhs].flags = ST_FLAT;

				hs[g->nhs].origin = ge->x3;

				if ((hs[g->nhs].flags & ST_UP) ?
				    (ge->y3 <= ge->prev->y3) :
				    (ge->y3 >= ge->prev->y3)
					) {
					if (ge->x3 < ge->prev->x3) {
						hs[g->nhs].from = ge->x3;
						hs[g->nhs].to = ge->prev->x3-1;
						if( hs[g->nhs].from > hs[g->nhs].to )
							hs[g->nhs].to++;
					} else {
						hs[g->nhs].from = ge->prev->x3+1;
						hs[g->nhs].to = ge->x3;
						if( hs[g->nhs].from > hs[g->nhs].to )
							hs[g->nhs].from--;
					}
				} else {
					hs[g->nhs].from
						= hs[g->nhs].to = ge->x3;
				}

				if (ge->x3 != ge->x2)
					g->nhs++;
			}
			/* if it is vertical, add a vstem */
			else if (ge->x3 == ge->x2) {
				vs[g->nvs].value = ge->x3;

				if (ge->y3 > ge->y2)
					vs[g->nvs].flags = ST_FLAT | ST_UP;
				else
					vs[g->nvs].flags = ST_FLAT;

				vs[g->nvs].origin = ge->y3;

				if ((vs[g->nvs].flags & ST_UP) ?
				    (ge->x3 <= ge->prev->x3) :
				    (ge->x3 >= ge->prev->x3)
					) {
					if (ge->y3 < ge->prev->y3) {
						vs[g->nvs].from = ge->y3;
						vs[g->nvs].to = ge->prev->y3-1;
						if( vs[g->nvs].from > vs[g->nvs].to )
							vs[g->nvs].to++;
					} else {
						vs[g->nvs].from = ge->prev->y3+1;
						vs[g->nvs].to = ge->y3;
						if( vs[g->nvs].from > vs[g->nvs].to )
							vs[g->nvs].from--;
					}
				} else {
					vs[g->nvs].from
						= vs[g->nvs].to = ge->prev->y3;
				}

				if (ge->y3 != ge->y2)
					g->nvs++;
			} else {

				/*
				 * check the end of curve for a not smooth
				 * local extremum
				 */
				if (ge->first)
					nge = ge->first;
				else
					nge = ge->next;

				if (nge == 0)
					continue;
				else if (nge->type == GE_LINE) {
					nx = nge->x3;
					ny = nge->y3;
				} else if (nge->type == GE_CURVE) {
					nx = nge->x1;
					ny = nge->y1;
				} else
					continue;

				/* check for vertical extremums */
				if (ge->y3 > ge->y2 && ge->y3 > ny
				    || ge->y3 < ge->y2 && ge->y3 < ny) {
					hs[g->nhs].value = ge->y3;
#if 0
					hs[g->nhs].origin = ge->x3;
					hs[g->nhs].from = -4000;
					hs[g->nhs].to = 4000;
#else
					hs[g->nhs].from
						= hs[g->nhs].to
						= hs[g->nhs].origin = ge->x3;
#endif
					if (ge->x3 < ge->x2
					    || nx < ge->x3)
						hs[g->nhs].flags = ST_UP;
					else
						hs[g->nhs].flags = 0;

					if (ge->x3 != ge->x2 || nx != ge->x3)
						g->nhs++;
				}
				/*
				 * the same point may be both horizontal and
				 * vertical extremum
				 */
				/* check for horizontal extremums */
				if (ge->x3 > ge->x2 && ge->x3 > nx
				    || ge->x3 < ge->x2 && ge->x3 < nx) {
					vs[g->nvs].value = ge->x3;
#if 0
					vs[g->nvs].origin = ge->y3;
					vs[g->nvs].from = -4000;
					vs[g->nvs].to = 4000;
#else
					vs[g->nvs].from
						= vs[g->nvs].to
						= vs[g->nvs].origin = ge->y3;
#endif
					if (ge->y3 > ge->y2
					    || ny > ge->y3)
						vs[g->nvs].flags = ST_UP;
					else
						vs[g->nvs].flags = 0;

					if (ge->y3 != ge->y2 || ny != ge->y3)
						g->nvs++;
				}
			}

		} else if (ge->type == GE_LINE) {
			if (ge->first)
				nge = ge->first;
			else
				nge = ge->next;

			/* if it is horizontal, add a hstem */
			/* and the ends as vstems */
			if (ge->y3 == ge->prev->y3) {
				hs[g->nhs].value = ge->y3;
				if (ge->x3 < ge->prev->x3) {
					hs[g->nhs].flags = ST_FLAT | ST_UP;
					hs[g->nhs].from = ge->x3;
					hs[g->nhs].to = ge->prev->x3;
				} else {
					hs[g->nhs].flags = ST_FLAT;
					hs[g->nhs].from = ge->prev->x3;
					hs[g->nhs].to = ge->x3;
				}

				if (ge->x3 != ge->prev->x3) {
					g->nhs++;

					pge = ge->prev;
					if (pge->type == GE_MOVE) {
						for (pge = ge; pge->first == 0; pge = pge->next) {
						}
					}
					/* add beginning as vstem */
					vs[g->nvs].value = pge->x3;
					vs[g->nvs].origin
						= vs[g->nvs].from
						= vs[g->nvs].to = pge->y3;

					if (pge->y3 > pge->prev->y3)
						vs[g->nvs].flags = ST_UP | ST_END;
					else
						vs[g->nvs].flags = ST_END;
					g->nvs++;

					/* add end as vstem */
					vs[g->nvs].value = ge->x3;
					vs[g->nvs].origin
						= vs[g->nvs].from
						= vs[g->nvs].to = ge->y3;

					if (nge->y3 > ge->y3)
						vs[g->nvs].flags = ST_UP | ST_END;
					else
						vs[g->nvs].flags = ST_END;
					g->nvs++;
				}
			}
			/* if it is vertical, add a vstem */
			/* and the ends as hstems */
			else if (ge->x3 == ge->prev->x3) {
				vs[g->nvs].value = ge->x3;
				if (ge->y3 > ge->prev->y3) {
					vs[g->nvs].flags = ST_FLAT | ST_UP;
					vs[g->nvs].from = ge->prev->y3;
					vs[g->nvs].to = ge->y3;
				} else {
					vs[g->nvs].flags = ST_FLAT;
					vs[g->nvs].from = ge->y3;
					vs[g->nvs].to = ge->prev->y3;
				}

				if (ge->y3 != ge->prev->y3) {
					g->nvs++;

					pge = ge->prev;
					if (pge->type == GE_MOVE) {
						for (pge = ge; pge->first == 0; pge = pge->next) {
						}
					}
					/* add beginning as hstem */
					hs[g->nhs].value = pge->y3;
					hs[g->nhs].origin
						= hs[g->nhs].from
						= hs[g->nhs].to = pge->x3;

					if (pge->x3 < pge->prev->x3)
						hs[g->nhs].flags = ST_UP | ST_END;
					else
						hs[g->nhs].flags = ST_END;
					g->nhs++;

					/* add end as hstem */
					hs[g->nhs].value = ge->y3;
					hs[g->nhs].origin
						= hs[g->nhs].from
						= hs[g->nhs].to = ge->x3;

					if (nge->x3 < ge->x3)
						hs[g->nhs].flags = ST_UP | ST_END;
					else
						hs[g->nhs].flags = ST_END;
					g->nhs++;
				}
			}
			/*
			 * check the end of line for a not smooth local
			 * extremum
			 */
			if (ge->first)
				nge = ge->first;
			else
				nge = ge->next;

			if (nge == 0)
				continue;
			else if (nge->type == GE_LINE) {
				nx = nge->x3;
				ny = nge->y3;
			} else if (nge->type == GE_CURVE) {
				nx = nge->x1;
				ny = nge->y1;
			} else
				continue;

			/* check for vertical extremums */
			if (ge->y3 > ge->prev->y3 && ge->y3 > ny
			    || ge->y3 < ge->prev->y3 && ge->y3 < ny) {
				hs[g->nhs].value = ge->y3;
#if 0
				hs[g->nhs].origin = ge->x3;
				hs[g->nhs].from = -4000;
				hs[g->nhs].to = 4000;
#else
				hs[g->nhs].from
					= hs[g->nhs].to
					= hs[g->nhs].origin = ge->x3;
#endif
				if (ge->x3 < ge->prev->x3
				    || nx < ge->x3)
					hs[g->nhs].flags = ST_UP;
				else
					hs[g->nhs].flags = 0;

				if (ge->x3 != ge->prev->x3 || nx != ge->x3)
					g->nhs++;
			}
			/*
			 * the same point may be both horizontal and vertical
			 * extremum
			 */
			/* check for horizontal extremums */
			if (ge->x3 > ge->prev->x3 && ge->x3 > nx
			    || ge->x3 < ge->prev->x3 && ge->x3 < nx) {
				vs[g->nvs].value = ge->x3;
#if 0
				vs[g->nvs].origin = ge->y3;
				vs[g->nvs].from = -4000;
				vs[g->nvs].to = 4000;
#else
				vs[g->nvs].from
					= vs[g->nvs].to
					= vs[g->nvs].origin = ge->y3;
#endif
				if (ge->y3 > ge->prev->y3
				    || ny > ge->y3)
					vs[g->nvs].flags = ST_UP;
				else
					vs[g->nvs].flags = 0;

				if (ge->y3 != ge->prev->y3 || ny != ge->y3)
					g->nvs++;
			}
		}
	}

	g->nhs=addbluestems(hs, g->nhs);
	sortstems(hs, g->nhs);
	sortstems(vs, g->nvs);

	if (debug)
		debugstems(g->name, hs, g->nhs, vs, g->nvs);

	if (debug)
		fprintf(pfa_file, "%% %s: joining horizontal stems\n", g->name);
	g->nhs = joinstems(hs, g->nhs, 1);
	if (debug)
		fprintf(pfa_file, "%% %s: joining vertical stems\n", g->name);
	g->nvs = joinstems(vs, g->nvs, 0);

	if (debug)
		debugstems(g->name, hs, g->nhs, vs, g->nvs);

	if ((sp = (STEM*)malloc(sizeof(STEM) * g->nhs)) == 0) {
		FPF(stderr, "**** not enough memory for stems ****\n");
		exit(1);
	}
	g->hstems = sp;
	memcpy(sp, hs, sizeof(STEM) * g->nhs);

	if ((sp = (STEM*)malloc(sizeof(STEM) * g->nvs)) == 0) {
		FPF(stderr, "**** not enough memory for stems ****\n");
		exit(1);
	}
	g->vstems = sp;
	memcpy(sp, vs, sizeof(STEM) * g->nvs);

	debug = 0;
}

/* convert weird curves that are close to lines into lines.
** the flag shows whether we should straighten only zigzags
** or otler lines too
*/

 void
ttf2pt1::straighten(
	   GLYPH * g,
	   int zigonly
)
{
	GENTRY         *ge, *pge, *nge, *oge;
	int             dx, dy;
	int             dir;
	int             prevlen, nextlen;	/* length of prev/next line
						 * if it's flat */
	int             psx, psy, nsx, nsy;	/* stretchability limits */
	int             n;
	int             svdir;

	for (ge = g->entries; ge != 0; ge = ge->next) {
		if (ge->type != GE_CURVE)
			continue;

		pge = ge->prev;
		if (ge->first)
			nge = ge->first;
		else
			nge = ge->next;

		dx = dy = 0;
		prevlen = nextlen = 0;

		/*
		 * if current curve is almost a vertical line, and it *
		 * doesn't begin or end horizontally * (and the prev/next
		 * line doesn't join smoothly ?)
		 */
		if (ge->y3 != ge->y2 && ge->y1 != pge->y3
		    && abs(ge->x3 - pge->x3) <= 2
		    && (!zigonly && (abs(ge->x3 - pge->x3) <= 1 || abs(ge->y3 - pge->y3) >= 10)
		     || sign(ge->x1 - pge->x3) + sign(ge->x2 - ge->x3) == 0)
			) {

			dx = ge->x3 - pge->x3;
			dir = sign(ge->y3 - pge->y3);
			ge->type = GE_LINE;

			/*
			 * suck in all the sequence of such almost lines *
			 * going in the same direction * but not deviating
			 * too far from vertical
			 */

			while (abs(dx) <= 5 && nge->type == GE_CURVE
			       && nge->y3 != nge->y2 && nge->y1 != ge->y3
			       && abs(nge->x3 - ge->x3) <= 2
			       && (!zigonly && (abs(nge->x3 - ge->x3) <= 1 || abs(nge->y3 - ge->y3) >= 10)
				   || sign(nge->x1 - ge->x3) + sign(nge->x2 - nge->x3) == 0)
			       && dir == sign(nge->y3 - ge->y3)) {
				ge->y3 = nge->y3;
				ge->x3 = nge->x3;

				if (ge->first) {
					/*
					 * move the start point of the
					 * contour
					 */
					nge->prev->x3 = nge->x3;
					nge->prev->y3 = nge->y3;
					nge->prev->next = nge->next;
					nge->next->prev = nge->prev;
					ge->first = nge->next;
					free(nge);
					nge = ge->first;
				} else {
					ge->first = nge->first;
					ge->next = nge->next;
					nge->next->prev = ge;
					free(nge);
					if (ge->first)
						nge = ge->first;
					else
						nge = ge->next;
				}

				dx = ge->x3 - pge->x3;
			}

			/* now check what do we have as previous/next line */

			if (pge->type == GE_LINE && pge->x3 == pge->prev->x3)
				prevlen = abs(pge->y3 - pge->prev->y3);
			if (nge->type == GE_LINE && nge->x3 == ge->x3)
				nextlen = abs(nge->y3 - ge->y3);
		}
		/*
		 * if current curve is almost a horizontal line, and it *
		 * doesn't begin or end vertucally * (and the prev/next line
		 * doesn't join smoothly ?)
		 */
		else if (ge->x3 != ge->x2 && ge->x1 != pge->x3
			 && abs(ge->y3 - pge->y3) <= 2
			 && (!zigonly && (abs(ge->y3 - pge->y3) <= 1 || abs(ge->x3 - pge->x3) >= 10)
			     || sign(ge->y1 - pge->y3) + sign(ge->y2 - ge->y3) == 0)
			) {

			dy = ge->y3 - pge->y3;
			dir = sign(ge->x3 - pge->x3);
			ge->type = GE_LINE;

			/*
			 * suck in all the sequence of such almost lines *
			 * going in the same direction * but doesn't deviate
			 * too far from horizontal
			 */

			while (abs(dy) <= 5 && nge->type == GE_CURVE
			       && nge->x3 != nge->x2 && nge->x1 != ge->x3
			       && abs(nge->y3 - ge->y3) <= 2
			       && (!zigonly && (abs(nge->y3 - ge->y3) <= 1 || abs(nge->x3 - ge->x3) >= 10)
				   || sign(nge->y1 - ge->y3) + sign(nge->y2 - nge->y3) == 0)
			       && dir == sign(nge->x3 - ge->x3)) {
				ge->x3 = nge->x3;
				ge->y3 = nge->y3;

				if (ge->first) {
					/*
					 * move the start point of the
					 * contour
					 */
					nge->prev->y3 = nge->y3;
					nge->prev->x3 = nge->x3;
					nge->prev->next = nge->next;
					nge->next->prev = nge->prev;
					ge->first = nge->next;
					free(nge);
					nge = ge->first;
				} else {
					ge->first = nge->first;
					ge->next = nge->next;
					nge->next->prev = ge;
					free(nge);
					if (ge->first)
						nge = ge->first;
					else
						nge = ge->next;
				}

				dy = ge->y3 - pge->y3;
			}

			/* now check what do we have as previous/next line */

			if (pge->type == GE_LINE && pge->y3 == pge->prev->y3)
				prevlen = abs(pge->x3 - pge->prev->x3);
			if (nge->type == GE_LINE && nge->y3 == ge->y3)
				nextlen = abs(nge->x3 - ge->x3);
		}
		if (prevlen != 0) {
			/* join the previous line with current */
			ge->next->prev = pge;
			pge->next = ge->next;
			pge->first = ge->first;

			pge->x3 = ge->x3;
			pge->y3 = ge->y3;

			free(ge);
			ge = pge;
			pge = ge->prev;
		}
		if (nextlen != 0) {
			/* join the next line with current */
			ge->x3 = nge->x3;
			ge->y3 = nge->y3;

			if (ge->first) {
				ge->first = nge->next;
				nge->prev->x3 = nge->x3;
				nge->prev->y3 = nge->y3;
				nge->next->prev = nge->prev;
				nge->prev->next = nge->next;
				free(nge);
				nge = ge->first;
			} else {
				ge->first = nge->first;
				ge->next = nge->next;
				nge->next->prev = ge;
				free(nge);
				if (ge->first)
					nge = ge->first;
				else
					nge = ge->next;
			}

		}
		/* if we have to align the lines */
		if (dx != 0 || dy != 0) {

			/* find the stretchability limits of prev element */
			if (pge->type == GE_LINE) {
				psx = pge->x3 - pge->prev->x3;
				psy = pge->y3 - pge->prev->y3;
			} else if (pge->type == GE_CURVE) {
				psx = pge->x2 - pge->x1;
				psy = pge->y2 - pge->y1;
			} else {/* beginning of contour, can't stretch */
				psx = psy = 0;
			}
			if (sign(psx) == sign(dx))
				psx = 1000;	/* unlimited */
			else
				psx = abs(psx);
			if (sign(psy) == sign(dy))
				psy = 1000;	/* unlimited */
			else
				psy = abs(psy);

			/* find the stretchability limits of next element */
			if (nge->type == GE_LINE) {
				nsx = nge->x3 - ge->x3;
				nsy = nge->y3 - ge->y3;
			} else if (pge->type == GE_CURVE) {
				nsx = nge->x2 - nge->x1;
				nsy = nge->y2 - nge->y1;
			} else {/* beginning of contour, can't stretch */
				nsx = nsy = 0;
			}
			if (sign(nsx) == sign(-dx))
				nsx = 1000;	/* unlimited */
			else
				nsx = abs(nsx);
			if (sign(nsy) == sign(-dy))
				nsy = 1000;	/* unlimited */
			else
				nsy = abs(nsy);

			/* convert the stretchability limits to actual values */
			if (psx + nsx < abs(dx)) {	/* if we have troubles */
				n = abs(dx) - (psx + nsx);
				if (nsx != 0 && nge->type == GE_CURVE
				    && psx != 0 && pge->type == GE_CURVE) {	/* split the difference */
					psx += n / 2;
					nsx += n - n / 2;
				} else if (nsx != 0 && nge->type == GE_CURVE) {
					nsx += n;
				} else if (psx != 0 && pge->type == GE_CURVE) {
					psx += n;
				} else if (nsx != 0 && nge->type == GE_LINE
				      && psx != 0 && pge->type == GE_LINE) {	/* split the difference */
					psx += n / 2;
					nsx += n - n / 2;
				} else if (nsx != 0 && nge->type == GE_LINE) {
					nsx += n;
				} else if (psx != 0 && pge->type == GE_LINE) {
					psx += n;
				} else {
					/*
					 * we got no other choice than to
					 * insert a piece
					 */
					GENTRY         *zzge;

					zzge = newgentry();
					zzge->type = GE_LINE;
					zzge->x3 = ge->x3;
					zzge->y3 = ge->y3;
					zzge->next = ge->next;
					zzge->prev = ge;
					zzge->first = ge->first;

					ge->next->prev = zzge;
					ge->next = zzge;
					ge->first = 0;
					ge->x3 -= dx;
					if (abs(ge->y3 - pge->y3) >= 10)
						ge->y3 -= 5 * sign(ge->y3 - pge->y3);
					else
						ge->y3 -= (ge->y3 - pge->y3) / 2;

					pge = ge;
					ge = zzge;
					dx = psx = nsx = 0;

				}
			} else if (psx + nsx > abs(dx)) {	/* try to spilt fairly */
				if (prevlen >= 2 * nextlen) {
					if (nsx >= abs(dx))
						nsx = abs(dx);
					psx = abs(dx) - nsx;
				} else if (nextlen >= 2 * prevlen) {
					if (psx >= abs(dx))
						psx = abs(dx);
					nsx = abs(dx) - psx;
				} else {
					n = abs(dx) / 2;
					if (psx <= n)
						nsx = abs(dx) - psx;
					else if (nsx <= n)
						psx = abs(dx) - nsx;
					else {
						psx = n;
						nsx = abs(dx) - psx;
					}
				}
			}
			if (psy + nsy < abs(dy)) {	/* if we have troubles */
				n = abs(dy) - (psy + nsy);
				if (nsy != 0 && nge->type == GE_CURVE
				    && psy != 0 && pge->type == GE_CURVE) {	/* split the difference */
					psy += n / 2;
					nsy += n - n / 2;
				} else if (nsy != 0 && nge->type == GE_CURVE) {
					nsy += n;
				} else if (psy != 0 && pge->type == GE_CURVE) {
					psy += n;
				} else if (nsy != 0 && nge->type == GE_LINE
				      && psy != 0 && pge->type == GE_LINE) {	/* split the difference */
					psy += n / 2;
					nsy += n - n / 2;
				} else if (nsy != 0 && nge->type == GE_LINE) {
					nsy += n;
				} else if (psy != 0 && pge->type == GE_LINE) {
					psy += n;
				} else {
					/*
					 * we got no other choice than to
					 * insert a piece
					 */
					GENTRY         *zzge;

					zzge = newgentry();
					zzge->type = GE_LINE;
					zzge->x3 = ge->x3;
					zzge->y3 = ge->y3;
					zzge->next = ge->next;
					zzge->prev = ge;
					zzge->first = ge->first;

					ge->next->prev = zzge;
					ge->next = zzge;
					ge->first = 0;
					ge->y3 -= dy;
					if (abs(ge->x3 - pge->x3) >= 10)
						ge->x3 -= 5 * sign(ge->x3 - pge->x3);
					else
						ge->x3 -= (ge->x3 - pge->x3) / 2;

					pge = ge;
					ge = zzge;
					dy = psy = nsy = 0;

				}
			} else if (psy + nsy > abs(dy)) {	/* try to spilt fairly */
				if (prevlen >= 2 * nextlen) {
					if (nsy >= abs(dy))
						nsy = abs(dy);
					psy = abs(dy) - nsy;
				} else if (nextlen >= 2 * prevlen) {
					if (psy >= abs(dy))
						psy = abs(dy);
					nsy = abs(dy) - psy;
				} else {
					n = abs(dy) / 2;
					if (psy <= n)
						nsy = abs(dy) - psy;
					else if (nsy <= n)
						psy = abs(dy) - nsy;
					else {
						psy = n;
						nsy = abs(dy) - psy;
					}
				}
			}
			/* now stretch the neigboring elements */
			if (dx != 0) {
				dx = sign(dx);

				if (nsx != 0) {
					ge->x3 -= dx * nsx;
					if (ge->first)
						nge->prev->x3 -= dx * nsx;
					if (nge->type == GE_CURVE) {
						svdir = getcvdir(nge);
						nge->x1 -= dx * nsx;
						fixcvdir(nge, svdir);
					}
				}
				if (psx != 0) {
					pge->x3 += dx * psx;
					if (pge->type == GE_CURVE) {
						svdir = getcvdir(pge);
						pge->x2 += dx * psx;
						fixcvdir(pge, svdir);
					}
				}
			}
			if (dy != 0) {
				dy = sign(dy);

				if (nsy != 0) {
					ge->y3 -= dy * nsy;
					if (ge->first)
						nge->prev->y3 -= dy * nsy;
					if (nge->type == GE_CURVE) {
						svdir = getcvdir(nge);
						nge->y1 -= dy * nsy;
						fixcvdir(nge, svdir);
					}
				}
				if (psy != 0) {
					pge->y3 += dy * psy;
					if (pge->type == GE_CURVE) {
						svdir = getcvdir(pge);
						pge->y2 += dy * psy;
						fixcvdir(pge, svdir);
					}
				}
			}
		}
	}
}

/* find the approximate length of curve */

 double
ttf2pt1::curvelen(
	 int x0, int y0, int x1, int y1,
	 int x2, int y2, int x3, int y3
)
{
	double          len = 0.;
	double          dx, dy;
	double          step, t, mt, n, x = (double) x0, y = (double) y0;
	int             i;

	x1 *= 3;
	y1 *= 3;
	x2 *= 3;
	y2 *= 3;

	step = 1. / 10.;
	t = 0;
	len = 0;
	for (i = 1; i <= 10; i++) {
		t += step;
		mt = 1 - t;

		n = x0 * mt * mt * mt + x1 * mt * mt * t + x2 * mt * t * t + x3 * t * t * t;
		dx = n - x;
		x = n;

		n = y0 * mt * mt * mt + y1 * mt * mt * t + y2 * mt * t * t + y3 * t * t * t;
		dy = n - y;
		y = n;

		len += sqrt(dx * dx + dy * dy);
	}

	return len;
}

/* split the zigzag-like curves into two parts */

 void
ttf2pt1::splitzigzags(
	     GLYPH * g
)
{
	GENTRY         *ge, *nge;
	double          k, k1, k2;
	int             a, b, c, d;

	for (ge = g->entries; ge != 0; ge = ge->next) {
		if (ge->type != GE_CURVE)
			continue;

		a = ge->y2 - ge->y1;
		b = ge->x2 - ge->x1;
		k = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : (double) b / (double) a);
		a = ge->y1 - ge->prev->y3;
		b = ge->x1 - ge->prev->x3;
		k1 = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : (double) b / (double) a);
		a = ge->y3 - ge->y2;
		b = ge->x3 - ge->x2;
		k2 = fabs(a == 0 ? (b == 0 ? 1. : 100000.) : (double) b / (double) a);

		/* if the curve is not a zigzag */
		if (k1 >= k && k2 <= k || k1 <= k && k2 >= k) {
			fixcvends(ge);
			continue;
		}
		/*
		 * we probably have also to check that the curve is in one *
		 * quadrant but we don't (yet)
		 */

		/* split the curve */
		nge = newgentry();
		nge->type = GE_CURVE;

		nge->next = ge->next;
		ge->next->prev = nge;

		ge->next = nge;
		nge->prev = ge;

		nge->first = ge->first;
		ge->first = 0;

		/*
		 * 2000 is for predictable truncating/rounding of negative
		 * values
		 */
		a = ge->prev->x3;
		b = ge->x1;
		c = ge->x2;
		d = ge->x3;
		nge->x3 = d;
		nge->x2 = (c + d + 4001) / 2 - 2000;
		nge->x1 = (b + 2 * c + d + 8002) / 4 - 2000;
		ge->x3 = (a + b * 3 + c * 3 + d + 16004) / 8 - 2000;
		ge->x2 = (a + 2 * b + c + 8002) / 4 - 2000;
		ge->x1 = (a + b + 4001) / 2 - 2000;

		a = ge->prev->y3;
		b = ge->y1;
		c = ge->y2;
		d = ge->y3;
		nge->y3 = d;
		nge->y2 = (c + d + 4001) / 2 - 2000;
		nge->y1 = (b + 2 * c + d + 8002) / 4 - 2000;
		ge->y3 = (a + b * 3 + c * 3 + d + 16004) / 8 - 2000;
		ge->y2 = (a + 2 * b + c + 8002) / 4 - 2000;
		ge->y1 = (a + b + 4001) / 2 - 2000;

		if (nge->x3 == ge->x3 && nge->y3 == ge->y3) {
			/* oops, the curve is too small to split */
			ge->first = nge->first;
			ge->next = nge->next;
			nge->next->prev = ge;
			free(nge);
		} else if (ge->x3 == ge->prev->x3 && ge->y3 == ge->prev->y3) {
			/* oops again, the curve is too small to split */
			nge->prev = ge->prev;
			*ge = *nge;
			ge->next->prev = ge;
			free(nge);
		} else {
			/* check whether we have created more zigzags */

			/* the first new curve */
			/* guess the direction by the front end */
			fixcvdir(ge,
				 (k1 < k ? CVDIR_FUP : (k1 > k ? CVDIR_FDOWN : CVDIR_FEQUAL)) | CVDIR_RSAME
				);

			ge = nge;
		}
		/* check whether we have created more zigzags */

		/* the second new curve or the whole unsplit curve */
		/* guess the direction by the rear end */
		fixcvdir(ge,
			 (k2 > k ? CVDIR_FUP : (k2 < k ? CVDIR_FDOWN : CVDIR_FEQUAL)) | CVDIR_RSAME
			);
	}
}

/* force conciseness: substitute 2 or more curves going in the
** same quadrant with one curve
*/

 void
ttf2pt1::forceconcise(
	     GLYPH * g
)
{
	GENTRY         *ge, *nge;
	double          firstlen, lastlen, sumlen, scale;
	int             dxw1, dyw1, dxw2, dyw2;
	int             dxb1, dyb1, dxe1, dye1;
	int             dxb2, dyb2, dxe2, dye2;
	int             n;	/* number of curves we are going to sum */

	for (ge = g->entries; ge != 0; ge = ge->next) {
		if (ge->type != GE_CURVE)
			continue;

		n = 1;

		/* the whole direction of curve */
		dxw1 = ge->x3 - ge->prev->x3;
		dyw1 = ge->y3 - ge->prev->y3;

		while (1) {
			/* the whole direction of curve */
			dxw1 = ge->x3 - ge->prev->x3;
			dyw1 = ge->y3 - ge->prev->y3;

			/* directions of  ends of curve */
			dxb1 = ge->x1 - ge->prev->x3;
			dyb1 = ge->y1 - ge->prev->y3;
			dxe1 = ge->x3 - ge->x2;
			dye1 = ge->y3 - ge->y2;

			/* if this curve ends horizontally or vertically */
			/* or crosses quadrant boundaries */
			if (dxe1 == 0 || dye1 == 0 || dxb1 * dxe1 < 0 || dyb1 * dye1 < 0)
				break;

			if (ge->first)
				nge = ge->first;
			else
				nge = ge->next;

			if (nge->type != GE_CURVE)
				break;

			dxw2 = nge->x3 - ge->x3;
			dyw2 = nge->y3 - ge->y3;

			dxb2 = nge->x1 - ge->x3;
			dyb2 = nge->y1 - ge->y3;
			dxe2 = nge->x3 - nge->x2;
			dye2 = nge->y3 - nge->y2;

			/* if curve changes direction */
			if (sign(dxw1) != sign(dxw2) || sign(dyw1) != sign(dyw2))
				break;

			/* if the next curve crosses quadrant boundaries */
			/* or joints very abruptly */
			if (dxb2 * dxe2 < 0 || dyb2 * dye2 < 0
			    || sign(dxb2) != sign(dxe1) || sign(dyb2) != sign(dye1))
				break;

			/* if the arch is going in other direction */
			if (sign(abs(dxb1 * dyw1) - abs(dyb1 * dxw1))
			    * sign(abs(dxe2 * dyw2) - abs(dye2 * dxw2)) >= 0)
				break;

			/* OK, it seeme like we can join these two curves */
			if (n == 1) {
				firstlen = sumlen = curvelen(ge->prev->x3, ge->prev->y3,
							     ge->x1, ge->y1, ge->x2, ge->y2, ge->x3, ge->y3);
			}
			lastlen = curvelen(ge->x3, ge->y3,
					   nge->x1, nge->y1, nge->x2, nge->y2, nge->x3, nge->y3);
			sumlen += lastlen;
			n++;

			ge->x2 = nge->x2;
			ge->y2 = nge->y2;
			ge->x3 = nge->x3;
			ge->y3 = nge->y3;

			if (ge->first) {
				nge->prev->x3 = ge->x3;
				nge->prev->y3 = ge->y3;
				nge->prev->next = nge->next;
				nge->next->prev = nge->prev;
				ge->first = nge->next;
			} else {
				ge->first = nge->first;
				ge->next = nge->next;
				nge->next->prev = ge;
			}
			free(nge);
		}

		if (n > 1) {	/* we got something to do */
			dxb1 = ge->x1 - ge->prev->x3;
			dyb1 = ge->y1 - ge->prev->y3;
			dxe1 = ge->x3 - ge->x2;
			dye1 = ge->y3 - ge->y2;

			/* scale the first segment */
			scale = sumlen / firstlen;
			ge->x1 = ge->prev->x3 + (int) (0.5 + scale * dxb1);
			ge->y1 = ge->prev->y3 + (int) (0.5 + scale * dyb1);

			/* scale the last segment */
			scale = sumlen / lastlen;
			ge->x2 = ge->x3 - (int) (0.5 + scale * dxe1);
			ge->y2 = ge->y3 - (int) (0.5 + scale * dye1);
		}
	}
}

/*
 * Try to force fixed width of characters
 */

 void
ttf2pt1::alignwidths(void)
{
	int             i;
	int             n = 0, avg, max = 0, min = 3000, sum = 0, x;

	for (i = 0; i < numglyphs; i++) {
		if (glyph_list[i].flags & GF_USED) {
			x = glyph_list[i].width;

			if (x != 0) {
				if (x < min)
					min = x;
				if (x > max)
					max = x;

				sum += x;
				n++;
			}
		}
	}

	if (n == 0)
		return;

	avg = sum / n;

	FPF(stderr, "widths: max=%d avg=%d min=%d\n", max, avg, min);

	/* if less than 5% variation from average */
	/* force fixed width */
	if (20 * (avg - min) < avg && 20 * (max - avg) < avg) {
		for (i = 0; i < numglyphs; i++) {
			if (glyph_list[i].flags & GF_USED)
				glyph_list[i].width = avg;
		}
		post_table->isFixedPitch = htonl(1);
	}
}

/*
 SB:
 An important note about the BlueValues.

 The Adobe documentation says that the maximal width of a Blue zone
 is connected to the value of BlueScale, which is by default 0.039625.
 The BlueScale value defines, at which point size the overshoot
 suppression be disabled.

 The formula for it that is given in the manual is:

  BlueScale=point_size/240, for a 300dpi device

 that makes us wonder what is this 240 standing for. Incidentally
 240=72*1000/300, where 72 is the relation between inches and points,
 1000 is the size of the glyph matrix, and 300dpi is the resolution of
 the output device. Knowing that we can recalculate the formula for
 the font size in pixels rather than points:

  BlueScale=pixel_size/1000

 That looks a lot simpler than the original formula, does not it ?
 And the limitation about the maximal width of zone also looks
 a lot simpler after the transformation:

  max_width < 1000/pixel_size

 that ensures that even at the maximal pixel size when the overshoot
 suppression is disabled the zone width will be less than one pixel.
 This is important, failure to comply to this limit will result in
 really ugly fonts (been there, done that). But knowing the formula
 for the pixel width, we see that in fact we can use the maximal width
 of 24, not 23 as specified in the manual.

*/

#define MAXBLUEWIDTH (24)

/*
 * Find the indexes of the most frequent values
 * in the hystogram, sort them in ascending order, and save which one
 * was the best one (if asked).
 * Returns the number of values found (may be less than maximal because
 * we ignore the zero values)
 */

#define MAXHYST	(2000)		/* size of the hystogram */
#define HYSTBASE 500

 int
ttf2pt1::besthyst(
	 short *hyst,		/* the hystogram */
	 int base,		/* the base point of the hystogram */
	 int *best,		/* the array for indexes of best values */
	 int nbest,		/* its allocated size */
	 int width,		/* minimal difference between indexes */
	 int *bestindp		/* returned top point */
)
{
	unsigned char   hused[MAXHYST / 8 + 1];
	int             i, max, j, w;
	int             nf = 0;

	width--;

	for (i = 0; i <= MAXHYST / 8; i++)
		hused[i] = 0;

	max = 1;
	for (i = 0; i < nbest && max != 0; i++) {
		best[i] = 0;
		max = 0;
		for (j = 1; j < MAXHYST - 1; j++) {
			w = hyst[j];

			if (w > max && (hused[j >> 3] & (1 << (j & 0x07))) == 0) {
				best[i] = j;
				max = w;
			}
		}
		if (max != 0) {
			for (j = best[i] - width; j <= best[i] + width; j++) {
				if (j >= 0 && j < MAXHYST)
					hused[j >> 3] |= (1 << (j & 0x07));
			}
			best[i] -= base;
			nf = i + 1;
		}
	}

	if (bestindp)
		*bestindp = best[0];

	/* sort the indexes in ascending order */
	for (i = 0; i < nf; i++) {
		for (j = i + 1; j < nf; j++)
			if (best[j] < best[i]) {
				w = best[i];
				best[i] = best[j];
				best[j] = w;
			}
	}

	return nf;
}

/*
 * Find the next best Blue zone in the hystogram.
 * Return the weight of the found zone.
 */

 int
ttf2pt1::bestblue(
	 short *zhyst,		/* the zones hystogram */
	 short *physt,		/* the points hystogram */
	 short *ozhyst,		/* the other zones hystogram */
	 int *bluetab		/* where to put the found zone */
)
{
	int             i, j, w, max, ind, first, last;

	/* find the highest point in the zones hystogram */
	/* if we have a plateau, take its center */
	/* if we have multiple peaks, take the first one */

	max = -1;
	first = last = -10;
	for (i = 0; i <= MAXHYST - MAXBLUEWIDTH; i++) {
		w = zhyst[i];
		if (w > max) {
			first = last = i;
			max = w;
		} else if (w == max) {
			if (last == i - 1)
				last = i;
		}
	}
	ind = (first + last) / 2;

	if (max == 0)		/* no zones left */
		return 0;

	/* now we reuse `first' and `last' as inclusive borders of the zone */
	first = ind;
	last = ind + (MAXBLUEWIDTH - 1);

	/* our maximal width is far too big, so we try to make it narrower */
	w = max;
	j = (w & 1);		/* a pseudo-random bit */
	while (1) {
		while (physt[first] == 0)
			first++;
		while (physt[last] == 0)
			last--;
		if (last - first < (MAXBLUEWIDTH * 2 / 3) || (max - w) * 10 > max)
			break;

		if (physt[first] < physt[last]
		    || physt[first] == physt[last] && j) {
			if (physt[first] * 20 > w)	/* if weight is >5%,
							 * stop */
				break;
			w -= physt[first];
			first++;
			j = 0;
		} else {
			if (physt[last] * 20 > w)	/* if weight is >5%,
							 * stop */
				break;
			w -= physt[last];
			last--;
			j = 1;
		}
	}

	/* save our zone */
	bluetab[0] = first - HYSTBASE;
	bluetab[1] = last - HYSTBASE;

	/* invalidate all the zones overlapping with this one */
	/* the constant of 2 is determined by the default value of BlueFuzz */
	for (i = first - (MAXBLUEWIDTH - 1) - 2; i <= last + 2; i++)
		if (i >= 0 && i < MAXHYST) {
			zhyst[i] = 0;
			ozhyst[i] = 0;
		}
	return w;
}

/*
 * Try to find the Blue Values, bounding box and italic angle
 */

 void
ttf2pt1::findblues(void)
{
	/* hystograms for upper and lower zones */
	short           hystl[MAXHYST];
	short           hystu[MAXHYST];
	short           zuhyst[MAXHYST];
	short           zlhyst[MAXHYST];
	int             nchars;
	int             ns;
	int             i, j, k, w, max;
	GENTRY         *ge;
	GLYPH          *g;
	double          ang;

	/* find the lowest and highest points of glyphs */
	/* and by the way build the values for FontBBox */
	/* and build the hystogram for the ItalicAngle */

	/* re-use hystl for the hystogram of italic angle */

	bbox[0] = bbox[1] = 5000;
	bbox[2] = bbox[3] = -5000;

	for (i = 0; i < MAXHYST; i++)
		hystl[i] = 0;

	nchars = 0;

	for (i = 0, g = glyph_list; i < numglyphs; i++, g++) {
		if (g->flags & GF_USED) {
			nchars++;

			g->rymin = 5000;
			g->rymax = -5000;
			for (ge = g->entries; ge != 0; ge = ge->next) {
				if (ge->type == GE_LINE) {

					j = ge->y3 - ge->prev->y3;
					k = ge->x3 - ge->prev->x3;
					if (j > 0)
						ang = atan2(-k, j) * 180.0 / M_PI;
					else
						ang = atan2(k, -j) * 180.0 / M_PI;

					k /= 100;
					j /= 100;
					if (ang > -45.0 && ang < 45.0) {
						/*
						 * be careful to not overflow
						 * the counter
						 */
						hystl[HYSTBASE + (int) (ang * 10.0)] += (k * k + j * j) / 4;
					}
					if (ge->y3 == ge->prev->y3) {
						if (ge->y3 <= g->rymin) {
							g->rymin = ge->y3;
							g->flatymin = 1;
						}
						if (ge->y3 >= g->rymax) {
							g->rymax = ge->y3;
							g->flatymax = 1;
						}
					} else {
						if (ge->y3 < g->rymin) {
							g->rymin = ge->y3;
							g->flatymin = 0;
						}
						if (ge->y3 > g->rymax) {
							g->rymax = ge->y3;
							g->flatymax = 0;
						}
					}
				} else if (ge->type == GE_CURVE) {
					if (ge->y3 < g->rymin) {
						g->rymin = ge->y3;
						g->flatymin = 0;
					}
					if (ge->y3 > g->rymax) {
						g->rymax = ge->y3;
						g->flatymax = 0;
					}
				}
				if (ge->type == GE_LINE || ge->type == GE_CURVE) {
					if (ge->x3 < bbox[0])
						bbox[0] = ge->x3;
					if (ge->x3 > bbox[2])
						bbox[2] = ge->x3;
					if (ge->y3 < bbox[1])
						bbox[1] = ge->y3;
					if (ge->y3 > bbox[3])
						bbox[3] = ge->y3;
				}
			}
		}
	}

	/* get the most popular angle */
	max = 0;
	w = 0;
	for (i = 0; i < MAXHYST; i++) {
		if (hystl[i] > w) {
			w = hystl[i];
			max = i;
		}
	}
	ang = (double) (max - HYSTBASE) / 10.0;
	FPF(stderr, "Guessed italic angle: %f\n", ang);
	if (italic_angle == 0.0)
		italic_angle = ang;

	/* build the hystogram of the lower points */
	for (i = 0; i < MAXHYST; i++)
		hystl[i] = 0;

	for (i = 0, g = glyph_list; i < numglyphs; i++, g++) {
		if ((g->flags & GF_USED)
		    && g->rymin + HYSTBASE >= 0 && g->rymin < MAXHYST - HYSTBASE) {
			hystl[g->rymin + HYSTBASE]++;
		}
	}

	/* build the hystogram of the upper points */
	for (i = 0; i < MAXHYST; i++)
		hystu[i] = 0;

	for (i = 0, g = glyph_list; i < numglyphs; i++, g++) {
		if ((g->flags & GF_USED)
		    && g->rymax + HYSTBASE >= 0 && g->rymax < MAXHYST - HYSTBASE) {
			hystu[g->rymax + HYSTBASE]++;
		}
	}

	/* build the hystogram of all the possible lower zones with max width */
	for (i = 0; i < MAXHYST; i++)
		zlhyst[i] = 0;

	for (i = 0; i <= MAXHYST - MAXBLUEWIDTH; i++) {
		for (j = 0; j < MAXBLUEWIDTH; j++)
			zlhyst[i] += hystl[i + j];
	}

	/* build the hystogram of all the possible upper zones with max width */
	for (i = 0; i < MAXHYST; i++)
		zuhyst[i] = 0;

	for (i = 0; i <= MAXHYST - MAXBLUEWIDTH; i++) {
		for (j = 0; j < MAXBLUEWIDTH; j++)
			zuhyst[i] += hystu[i + j];
	}

	/* find the baseline */
	w = bestblue(zlhyst, hystl, zuhyst, &bluevalues[0]);
	if (0)
		FPF(stderr, "BaselineBlue zone %d%% %d...%d\n", w * 100 / nchars,
				bluevalues[0], bluevalues[1]);

	if (w == 0)		/* no baseline, something weird */
		return;

	/* find the upper zones */
	for (nblues = 2; nblues < 14; nblues += 2) {
		w = bestblue(zuhyst, hystu, zlhyst, &bluevalues[nblues]);

		if (0)
			FPF(stderr, "Blue zone %d%% %d...%d\n", w * 100 / nchars, 
				bluevalues[nblues], bluevalues[nblues+1]);

		if (w * 20 < nchars)
			break;	/* don't save this zone */
	}

	/* find the lower zones */
	for (notherb = 0; notherb < 10; notherb += 2) {
		w = bestblue(zlhyst, hystl, zuhyst, &otherblues[notherb]);

		if (0)
			FPF(stderr, "OtherBlue zone %d%% %d...%d\n", w * 100 / nchars,
				otherblues[notherb], otherblues[notherb+1]);


		if (w * 20 < nchars)
			break;	/* don't save this zone */
	}

}

/*
 * Try to find the typical stem widths
 */

 void
ttf2pt1::stemstatistics(void)
{
	short           hyst[MAXHYST];
	int             best[12];
	int             i, j, w;
	int             nchars;
	int             ns;
	STEM           *s;
	GENTRY         *ge;
	GLYPH          *g;

	/* start with typical stem width */

	nchars=0;

	/* build the hystogram of horizontal stem widths */
	for (i = 0; i < MAXHYST; i++)
		hyst[i] = 0;

	for (i = 0, g = glyph_list; i < numglyphs; i++, g++) {
		if (g->flags & GF_USED) {
			nchars++;
			s = g->hstems;
			for (j = 0; j < g->nhs; j += 2) {
				if ((s[j].flags | s[j + 1].flags) & ST_END)
					continue;
				w = s[j + 1].value - s[j].value+1;
				if(w==20) /* split stems should not be counted */
					continue;
				if (w > 0 && w < MAXHYST - 1) {
					/*
					 * handle some fuzz present in
					 * converted fonts
					 */
					hyst[w] += 2;
					hyst[w - 1]++;
					hyst[w + 1]++;
				}
			}
		}
	}

	/* find 12 most frequent values */
	ns = besthyst(hyst, 0, best, 12, 3, &stdhw);

	/* store data in stemsnaph */
	for (i = 0; i < ns; i++)
		stemsnaph[i] = best[i];
	if (ns < 12)
		stemsnaph[ns] = 0;

	/* build the hystogram of vertical stem widths */
	for (i = 0; i < MAXHYST; i++)
		hyst[i] = 0;

	for (i = 0, g = glyph_list; i < numglyphs; i++, g++) {
		if (g->flags & GF_USED) {
			s = g->vstems;
			for (j = 0; j < g->nvs; j += 2) {
				if ((s[j].flags | s[j + 1].flags) & ST_END)
					continue;
				w = s[j + 1].value - s[j].value+1;
				if (w > 0 && w < MAXHYST - 1) {
					/*
					 * handle some fuzz present in
					 * converted fonts
					 */
					hyst[w] += 2;
					hyst[w - 1]++;
					hyst[w + 1]++;
				}
			}
		}
	}

	/* find 12 most frequent values */
	ns = besthyst(hyst, 0, best, 12, 3, &stdvw);

	/* store data in stemsnaph */
	for (i = 0; i < ns; i++)
		stemsnapv[i] = best[i];
	if (ns < 12)
		stemsnapv[ns] = 0;

}

/*
 * SB
 * A funny thing: TTF paths are going in reverse direction compared
 * to Type1. So after all (because the rest of logic uses TTF
 * path directions) we have to reverse the paths.
 *
 * It was a big headache to discover that.
 */

 void
ttf2pt1::reversepathsfromto(
		   GENTRY * from,
		   GENTRY * to
)
{
	GENTRY         *ge, *fge, *nge, *pge;
	GENTRY         *co, *cn, *last;

	for (ge = from; ge != 0 && ge != to; ge = ge->next) {
#if 0
		if (debug)
			FPF(stderr, "%% 0x%x <- 0x%x, 0x%x\n", ge, ge->prev, ge->first);
#endif

		if (ge->first) {
			/* we got a path, reverse it */
			nge = ge->next;
			fge = ge->first;
			pge = fge->prev;

			if (pge == 0) {
				FPF(stderr, "*** No MOVE before line !!! Fatal. ****\n");
				exit(1);
			}
			last = pge;

			/* go back on it and generate new entries */
			for (co = ge; co != pge; co = co->prev) {
				cn = newgentry();

				cn->type = co->type;
				if (co->type == GE_CURVE) {
					cn->x1 = co->x2;
					cn->y1 = co->y2;
					cn->x2 = co->x1;
					cn->y2 = co->y1;
				}
				cn->x3 = co->prev->x3;
				cn->y3 = co->prev->y3;

				cn->first = 0;
				last->next = cn;
				cn->prev = last;
				last = cn;
			}

			/* then connect the chain back */
			last->first = pge->next;
			last->next = nge;
			nge->prev = last;

			/* neccessary for open paths */
			pge->x3 = ge->x3;
			pge->y3 = ge->y3;

			/* cut the old path from list of entries */
			fge->prev = 0;
			ge->next = 0;

			/* and finally discard the old chain */
			for (co = fge; co != 0; co = cn) {
				cn = co->next;
				free(co);
			}

			/* reset to new path */
			ge = last;
		}
	}
}

 void
ttf2pt1::reversepaths(
	     GLYPH * g
)
{
	reversepathsfromto(g->entries, NULL);
}

#if 0

/*
** This function is commented out because the information
** collected by it is not used anywhere else yet. Now
** it only collects the directions of contours. And the
** direction of contours gets fixed already in draw_glyf().
**
***********************************************
**
** Here we expect that the paths are already closed.
** We also expect that the contours do not intersect
** and that curves doesn't cross any border of quadrant.
**
** Find which contours go inside which and what is
** their proper direction. Then fix the direction
** to make it right.
**
*/

#define MAXCONT	1000

static void
fixcontours(
	    GLYPH * g
)
{
	CONTOUR         cont[MAXCONT];
	short           ymax[MAXCONT];	/* the highest point */
	short           xofmax[MAXCONT];	/* X-coordinate of any point
						 * at ymax */
	short           ymin[MAXCONT];	/* the lowest point */
	short           xofmin[MAXCONT];	/* X-coordinate of any point
						 * at ymin */
	short           count[MAXCONT];	/* count of lines */
	char            dir[MAXCONT];	/* in which direction they must go */
	GENTRY         *start[MAXCONT], *minptr[MAXCONT], *maxptr[MAXCONT];
	int             ncont;
	int             i;
	int             dx1, dy1, dx2, dy2;
	GENTRY         *ge, *nge;

	/* find the contours and their most upper/lower points */
	ncont = 0;
	ymax[0] = -5000;
	ymin[0] = 5000;
	for (ge = g->entries; ge != 0; ge = ge->next) {
		if (ge->type == GE_LINE || ge->type == GE_CURVE) {
			if (ge->y3 > ymax[ncont]) {
				ymax[ncont] = ge->y3;
				xofmax[ncont] = ge->x3;
				maxptr[ncont] = ge;
			}
			if (ge->y3 < ymin[ncont]) {
				ymin[ncont] = ge->y3;
				xofmin[ncont] = ge->x3;
				minptr[ncont] = ge;
			}
		}
		if (ge->first) {
			start[ncont++] = ge->first;
			ymax[ncont] = -5000;
			ymin[ncont] = 5000;
		}
	}

	/* determine the directions of contours */
	for (i = 0; i < ncont; i++) {
		ge = minptr[i];
		if (ge->first)
			nge = ge->first;
		else
			nge = ge->next;

		if (ge->type == GE_CURVE) {
			dx1 = ge->x3 - ge->x2;
			dy1 = ge->y3 - ge->y2;

			if (dx1 == 0 && dy1 == 0) {	/* a pathological case */
				dx1 = ge->x3 - ge->x1;
				dy1 = ge->y3 - ge->y1;
			}
			if (dx1 == 0 && dy1 == 0) {	/* a more pathological
							 * case */
				dx1 = ge->x3 - ge->prev->x3;
				dy1 = ge->y3 - ge->prev->y3;
			}
		} else {
			dx1 = ge->x3 - ge->prev->x3;
			dy1 = ge->y3 - ge->prev->y3;
		}
		if (nge->type == GE_CURVE) {
			dx2 = ge->x3 - nge->x1;
			dy2 = ge->y3 - nge->y1;
			if (dx1 == 0 && dy1 == 0) {	/* a pathological case */
				dx2 = ge->x3 - nge->x2;
				dy2 = ge->y3 - nge->y2;
			}
			if (dx1 == 0 && dy1 == 0) {	/* a more pathological
							 * case */
				dx2 = ge->x3 - nge->x3;
				dy2 = ge->y3 - nge->y3;
			}
		} else {
			dx2 = ge->x3 - nge->x3;
			dy2 = ge->y3 - nge->y3;
		}

		/* compare angles */
		cont[i].direction = DIR_INNER;
		if (dy1 == 0) {
			if (dx1 < 0)
				cont[i].direction = DIR_OUTER;
		} else if (dy2 == 0) {
			if (dx2 > 0)
				cont[i].direction = DIR_OUTER;
		} else if (dx2 * dy1 < dx1 * dy2)
			cont[i].direction = DIR_OUTER;

		cont[i].ymin = ymin[i];
		cont[i].xofmin = xofmin[i];
	}

	/* save the information that may be needed further */
	g->ncontours = ncont;
	if (ncont > 0) {
		g->contours = malloc(sizeof(CONTOUR) * ncont);
		if (g->contours == 0) {
			FPF(stderr, "***** Memory allocation error *****\n");
			exit(1);
		}
		memcpy(g->contours, cont, sizeof(CONTOUR) * ncont);
	}
}

#endif

/*
 *
 */

static int
unicode_russian(
	int unival
)
{
	int res;
	static unsigned char used[256];
	if(ttf2pt1::newishFlag) {
		for(int i=0; i<256; i++) {
			used[i] = 0;
		}
		ttf2pt1::newishFlag = false;
	}

	if (unival <= 0x0081) {
		used[unival]=1;
		return unival;
	} else if (unival >= 0x0410 && unival < 0x0450) {	/* cyrillic letters */
		res = unival - 0x410 + 0xc0;
		used[res]=1;
		return res;
	} else if (unival >= 0x00a0 && unival <= 0x00bf
	&& unival!=0x00a3 && unival!=0x00b3) {
		used[unival]=1;
		return unival;
	} else {
		switch (unival) {
		case 0x0401:
			used[0xb3]=1;
			return 0xb3;	/* cyrillic YO */
		case 0x0451:
			used[0xa3]=1;
			return 0xa3;	/* cyrillic yo */
		}
	}

	/* there are enough broken fonts that pretend to be Latin1 */
	res=unicode_latin1(unival);
	if(res<256 && !used[res])
		return res;
	else
		return 0xffff;
}

static int
unicode_latin1(
	int unival
)
{
	int i, res;
	static unsigned char used[256];
	if(ttf2pt1::newishFlag) {
		for(int i=0; i<256; i++) {
			used[i] = 0;
		}
		ttf2pt1::newishFlag = false;
	}

	if (unival <= 0x0081) {
		return unival;
	} else if (unival >= 0x00a0 && unival <= 0x00ff) {
		return unival;
	} else {
		switch (unival) {
		case 0x008d:
			return 0x8d;
		case 0x008e:
			return 0x8e;
		case 0x008f:
			return 0x8f;
		case 0x0090:
			return 0x90;
		case 0x009d:
			return 0x9d;
		case 0x009e:
			return 0x9e;
		case 0x0152:
			return 0x8c;
		case 0x0153:
			return 0x9c;
		case 0x0160:
			return 0x8a;
		case 0x0161:
			return 0x9a;
		case 0x0178:
			return 0x9f;
		case 0x0192:
			return 0x83;
		case 0x02c6:
			return 0x88;
		case 0x02dc:
			return 0x98;
		case 0x2013:
			return 0x96;
		case 0x2014:
			return 0x97;
		case 0x2018:
			return 0x91;
		case 0x2019:
			return 0x92;
		case 0x201a:
			return 0x82;
		case 0x201c:
			return 0x93;
		case 0x201d:
			return 0x94;
		case 0x201e:
			return 0x84;
		case 0x2020:
			return 0x86;
		case 0x2021:
			return 0x87;
		case 0x2022:
			return 0x95;
		case 0x2026:
			return 0x85;
		case 0x2030:
			return 0x89;
		case 0x2039:
			return 0x8b;
		case 0x203a:
			return 0x9b;
		case 0x2122:
			return 0x99;
		default:
			return 0xffff;
		}
	}
}

static int
unicode_latin4(
	int unival
)
{
	int i, res;
	static unsigned char used[256];
	if(ttf2pt1::newishFlag) {
		for(int i=0; i<256; i++) {
			used[i] = 0;
		}
		ttf2pt1::newishFlag = false;
	}

	if (unival <= 0x0081) {
		return unival;
	} else {
		switch (unival) {
		case 0x201e:
			return 0x90; /* these two quotes are a hack only */
		case 0x201c:
			return 0x91; /* these two quotes are a hack only */
		case 0x008d:
			return 0x8d;
		case 0x008e:
			return 0x8e;
		case 0x008f:
			return 0x8f;
		case 0x009d:
			return 0x9d;
		case 0x009e:
			return 0x9e;
		case 0x0152:
			return 0x8c;
		case 0x0153:
			return 0x9c;
		case 0x0178:
			return 0x9f;
		case 0x0192:
			return 0x83;
		case 0x02c6:
			return 0x88;
		case 0x02dc:
			return 0x98;
		case 0x2013:
			return 0x96;
		case 0x2014:
			return 0x97;
		case 0x2019:
			return 0x92;
		case 0x201a:
			return 0x82;
		case 0x201d:
			return 0x94;
		case 0x2020:
			return 0x86;
		case 0x2021:
			return 0x87;
		case 0x2022:
			return 0x95;
		case 0x2026:
			return 0x85;
		case 0x2030:
			return 0x89;
		case 0x2039:
			return 0x8b;
		case 0x203a:
			return 0x9b;
		case 0x2122:
			return 0x99;
			
		case 0x00A0: 
			return 0xA0; /*  NO-BREAK SPACE */
		case 0x0104: 
			return 0xA1; /*  LATIN CAPITAL LETTER A WITH OGONEK */
		case 0x0138: 
			return 0xA2; /*  LATIN SMALL LETTER KRA */
		case 0x0156: 
			return 0xA3; /*  LATIN CAPITAL LETTER R WITH CEDILLA */
		case 0x00A4: 
			return 0xA4; /*  CURRENCY SIGN */
		case 0x0128: 
			return 0xA5; /*  LATIN CAPITAL LETTER I WITH TILDE */
		case 0x013B: 
			return 0xA6; /*  LATIN CAPITAL LETTER L WITH CEDILLA */
		case 0x00A7: 
			return 0xA7; /*  SECTION SIGN */
		case 0x00A8: 
			return 0xA8; /*  DIAERESIS */
		case 0x0160: 
			return 0xA9; /*  LATIN CAPITAL LETTER S WITH CARON */
		case 0x0112: 
			return 0xAA; /*  LATIN CAPITAL LETTER E WITH MACRON */
		case 0x0122: 
			return 0xAB; /*  LATIN CAPITAL LETTER G WITH CEDILLA */
		case 0x0166: 
			return 0xAC; /*  LATIN CAPITAL LETTER T WITH STROKE */
		case 0x00AD: 
			return 0xAD; /*  SOFT HYPHEN */
		case 0x017D: 
			return 0xAE; /*  LATIN CAPITAL LETTER Z WITH CARON */
		case 0x00AF: 
			return 0xAF; /*  MACRON */
		case 0x00B0: 
			return 0xB0; /*  DEGREE SIGN */
		case 0x0105: 
			return 0xB1; /*  LATIN SMALL LETTER A WITH OGONEK */
		case 0x02DB: 
			return 0xB2; /*  OGONEK */
		case 0x0157: 
			return 0xB3; /*  LATIN SMALL LETTER R WITH CEDILLA */
		case 0x00B4: 
			return 0xB4; /*  ACUTE ACCENT */
		case 0x0129: 
			return 0xB5; /*  LATIN SMALL LETTER I WITH TILDE */
		case 0x013C: 
			return 0xB6; /*  LATIN SMALL LETTER L WITH CEDILLA */
		case 0x02C7: 
			return 0xB7; /*  CARON */
		case 0x00B8: 
			return 0xB8; /*  CEDILLA */
		case 0x0161: 
			return 0xB9; /*  LATIN SMALL LETTER S WITH CARON */
		case 0x0113: 
			return 0xBA; /*  LATIN SMALL LETTER E WITH MACRON */
		case 0x0123: 
			return 0xBB; /*  LATIN SMALL LETTER G WITH CEDILLA */
		case 0x0167: 
			return 0xBC; /*  LATIN SMALL LETTER T WITH STROKE */
		case 0x014A: 
			return 0xBD; /*  LATIN CAPITAL LETTER ENG */
		case 0x017E: 
			return 0xBE; /*  LATIN SMALL LETTER Z WITH CARON */
		case 0x014B: 
			return 0xBF; /*  LATIN SMALL LETTER ENG */
		case 0x0100: 
			return 0xC0; /*  LATIN CAPITAL LETTER A WITH MACRON */
		case 0x00C1: 
			return 0xC1; /*  LATIN CAPITAL LETTER A WITH ACUTE */
		case 0x00C2: 
			return 0xC2; /*  LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
		case 0x00C3: 
			return 0xC3; /*  LATIN CAPITAL LETTER A WITH TILDE */
		case 0x00C4: 
			return 0xC4; /*  LATIN CAPITAL LETTER A WITH DIAERESIS */
		case 0x00C5: 
			return 0xC5; /*  LATIN CAPITAL LETTER A WITH RING ABOVE */
		case 0x00C6: 
			return 0xC6; /*  LATIN CAPITAL LIGATURE AE */
		case 0x012E: 
			return 0xC7; /*  LATIN CAPITAL LETTER I WITH OGONEK */
		case 0x010C: 
			return 0xC8; /*  LATIN CAPITAL LETTER C WITH CARON */
		case 0x00C9: 
			return 0xC9; /*  LATIN CAPITAL LETTER E WITH ACUTE */
		case 0x0118: 
			return 0xCA; /*  LATIN CAPITAL LETTER E WITH OGONEK */
		case 0x00CB: 
			return 0xCB; /*  LATIN CAPITAL LETTER E WITH DIAERESIS */
		case 0x0116: 
			return 0xCC; /*  LATIN CAPITAL LETTER E WITH DOT ABOVE */
		case 0x00CD: 
			return 0xCD; /*  LATIN CAPITAL LETTER I WITH ACUTE */
		case 0x00CE: 
			return 0xCE; /*  LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
		case 0x012A: 
			return 0xCF; /*  LATIN CAPITAL LETTER I WITH MACRON */
		case 0x0110: 
			return 0xD0; /*  LATIN CAPITAL LETTER D WITH STROKE */
		case 0x0145: 
			return 0xD1; /*  LATIN CAPITAL LETTER N WITH CEDILLA */
		case 0x014C: 
			return 0xD2; /*  LATIN CAPITAL LETTER O WITH MACRON */
		case 0x0136: 
			return 0xD3; /*  LATIN CAPITAL LETTER K WITH CEDILLA */
		case 0x00D4: 
			return 0xD4; /*  LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
		case 0x00D5: 
			return 0xD5; /*  LATIN CAPITAL LETTER O WITH TILDE */
		case 0x00D6: 
			return 0xD6; /*  LATIN CAPITAL LETTER O WITH DIAERESIS */
		case 0x00D7: 
			return 0xD7; /*  MULTIPLICATION SIGN */
		case 0x00D8: 
			return 0xD8; /*  LATIN CAPITAL LETTER O WITH STROKE */
		case 0x0172: 
			return 0xD9; /*  LATIN CAPITAL LETTER U WITH OGONEK */
		case 0x00DA: 
			return 0xDA; /*  LATIN CAPITAL LETTER U WITH ACUTE */
		case 0x00DB: 
			return 0xDB; /*  LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
		case 0x00DC: 
			return 0xDC; /*  LATIN CAPITAL LETTER U WITH DIAERESIS */
		case 0x0168: 
			return 0xDD; /*  LATIN CAPITAL LETTER U WITH TILDE */
		case 0x016A: 
			return 0xDE; /*  LATIN CAPITAL LETTER U WITH MACRON */
		case 0x00DF: 
			return 0xDF; /*  LATIN SMALL LETTER SHARP S */
		case 0x0101: 
			return 0xE0; /*  LATIN SMALL LETTER A WITH MACRON */
		case 0x00E1: 
			return 0xE1; /*  LATIN SMALL LETTER A WITH ACUTE */
		case 0x00E2: 
			return 0xE2; /*  LATIN SMALL LETTER A WITH CIRCUMFLEX */
		case 0x00E3: 
			return 0xE3; /*  LATIN SMALL LETTER A WITH TILDE */
		case 0x00E4: 
			return 0xE4; /*  LATIN SMALL LETTER A WITH DIAERESIS */
		case 0x00E5: 
			return 0xE5; /*  LATIN SMALL LETTER A WITH RING ABOVE */
		case 0x00E6: 
			return 0xE6; /*  LATIN SMALL LIGATURE AE */
		case 0x012F: 
			return 0xE7; /*  LATIN SMALL LETTER I WITH OGONEK */
		case 0x010D: 
			return 0xE8; /*  LATIN SMALL LETTER C WITH CARON */
		case 0x00E9: 
			return 0xE9; /*  LATIN SMALL LETTER E WITH ACUTE */
		case 0x0119: 
			return 0xEA; /*  LATIN SMALL LETTER E WITH OGONEK */
		case 0x00EB: 
			return 0xEB; /*  LATIN SMALL LETTER E WITH DIAERESIS */
		case 0x0117: 
			return 0xEC; /*  LATIN SMALL LETTER E WITH DOT ABOVE */
		case 0x00ED: 
			return 0xED; /*  LATIN SMALL LETTER I WITH ACUTE */
		case 0x00EE: 
			return 0xEE; /*  LATIN SMALL LETTER I WITH CIRCUMFLEX */
		case 0x012B: 
			return 0xEF; /*  LATIN SMALL LETTER I WITH MACRON */
		case 0x0111: 
			return 0xF0; /*  LATIN SMALL LETTER D WITH STROKE */
		case 0x0146: 
			return 0xF1; /*  LATIN SMALL LETTER N WITH CEDILLA */
		case 0x014D: 
			return 0xF2; /*  LATIN SMALL LETTER O WITH MACRON */
		case 0x0137: 
			return 0xF3; /*  LATIN SMALL LETTER K WITH CEDILLA */
		case 0x00F4: 
			return 0xF4; /*  LATIN SMALL LETTER O WITH CIRCUMFLEX */
		case 0x00F5: 
			return 0xF5; /*  LATIN SMALL LETTER O WITH TILDE */
		case 0x00F6: 
			return 0xF6; /*  LATIN SMALL LETTER O WITH DIAERESIS */
		case 0x00F7: 
			return 0xF7; /*  DIVISION SIGN */
		case 0x00F8: 
			return 0xF8; /*  LATIN SMALL LETTER O WITH STROKE */
		case 0x0173: 
			return 0xF9; /*  LATIN SMALL LETTER U WITH OGONEK */
		case 0x00FA: 
			return 0xFA; /*  LATIN SMALL LETTER U WITH ACUTE */
		case 0x00FB: 
			return 0xFB; /*  LATIN SMALL LETTER U WITH CIRCUMFLEX */
		case 0x00FC: 
			return 0xFC; /*  LATIN SMALL LETTER U WITH DIAERESIS */
		case 0x0169: 
			return 0xFD; /*  LATIN SMALL LETTER U WITH TILDE */
		case 0x016B: 
			return 0xFE; /*  LATIN SMALL LETTER U WITH MACRON */
		case 0x02D9: 
			return 0xFF; /*  DOT ABOVE */
			
			
		default:
			return 0xffff;
		}
	}
}

static int
unicode_latin5(
	int unival
)
{
	int i, res;
	static unsigned char used[256];
	if(ttf2pt1::newishFlag) {
		for(int i=0; i<256; i++) {
			used[i] = 0;
		}
		ttf2pt1::newishFlag = false;
	}

	if (unival <= 0x0081) {
		return unival;
	} else if (unival >= 0x00a0 && unival <= 0x00ff) {
		return unival;
	} else {
		switch (unival) {
		case 0x008d:
			return 0x8d;
		case 0x008e:
			return 0x8e;
		case 0x008f:
			return 0x8f;
		case 0x0090:
			return 0x90;
		case 0x009d:
			return 0x9d;
		case 0x009e:
			return 0x9e;
		case 0x011e:
			return 0xd0;	/* G breve */
		case 0x011f:
			return 0xf0;	/* g breve */
		case 0x0130:
			return 0xdd;	/* I dot */
		case 0x0131:
			return 0xfd;	/* dotless i */
		case 0x0152:
			return 0x8c;
		case 0x0153:
			return 0x9c;
		case 0x015e:
			return 0xde;	/* S cedilla */
		case 0x015f:
			return 0xfe;	/* s cedilla */
		case 0x0160:
			return 0x8a;
		case 0x0161:
			return 0x9a;
		case 0x0178:
			return 0x9f;
		case 0x0192:
			return 0x83;
		case 0x02c6:
			return 0x88;
		case 0x02dc:
			return 0x98;
		case 0x2013:
			return 0x96;
		case 0x2014:
			return 0x97;
		case 0x2018:
			return 0x91;
		case 0x2019:
			return 0x92;
		case 0x201a:
			return 0x82;
		case 0x201c:
			return 0x93;
		case 0x201d:
			return 0x94;
		case 0x201e:
			return 0x84;
		case 0x2020:
			return 0x86;
		case 0x2021:
			return 0x87;
		case 0x2022:
			return 0x95;
		case 0x2026:
			return 0x85;
		case 0x2030:
			return 0x89;
		case 0x2039:
			return 0x8b;
		case 0x203a:
			return 0x9b;
		case 0x2122:
			return 0x99;
		default:
			return 0xffff;
		}
	}
}

 int
ttf2pt1::unicode_to_win31(
		 int unival
)
{
	int i, res;
	static unsigned char used[256];

	if(newishFlag) {
		for(int i=0; i<256; i++) {
			used[i] = 0;
		}
		newishFlag = false;
	}

	if(unival<0) {
		FPF(stderr,"**** Internal error: unicode value < 0 ****\n");
		exit(1);
	}

	/* know the language */
	if(uni_lang_converter!=0)
		return (*uni_lang_converter)(unival);

	/* don't know the language, try all */
	for(i=0; i < sizeof uni_lang/sizeof(struct uni_language); i++) {
		res=(*uni_lang[i].conv)(unival);
		if(res==0xffff)
			continue;
		if(res & ~0xff) {
			FPF(stderr,"**** Internal error: broken unicode conversion ****\n");
			FPF(stderr,"**** language '%s' code 0x%04x ****\n", 
				uni_lang[i].name, unival);
			exit(1);
		}
		/* make sure that the priority is in the order of the language list */
		if(used[res]>i)
			return 0xffff;
		else {
			used[res]=250-i;
			return res;
		}
	}

	return 0xffff;
}

 void
ttf2pt1::WriteNameAttr(const char *path, const char *name)
{
	BNode pfa_node(path);
	if(pfa_node.InitCheck() != B_OK){
		FPF(stderr, "WriteNameAttr: Couldn't open pfa file.\n");
		return;
	}
	
	ssize_t err;
	err = pfa_node.WriteAttr("FontName", B_STRING_TYPE, 0, name, strlen(name)+1);
	if(err < 0){
		FPF(stderr, "WriteNameAttr: Error writing FontName attribute: %s.\n",
					strerror(err));
		return;
	}
}

 void
ttf2pt1::handle_name(const char *pfa_path)
{
	int             j, k, lang, len, platform;
	char           *p, *ptr, *string_area;
	char           *nbp = name_buffer;
	int             found3 = 0;

	string_area = (char *) name_table + ntohs(name_table->offset);
	name_record = &(name_table->nameRecords);

	for (j = 0; j < 8; j++) {
		name_fields[j] = NULL;
	}

	for (j = 0; j < ntohs(name_table->numberOfNameRecords); j++) {

		platform = ntohs(name_record->platformID);

		if (platform == 3) {

			found3 = 1;
			lang = ntohs(name_record->languageID) & 0xff;
			len = ntohs(name_record->stringLength);
			if (lang == 0 || lang == 9) {
				k = ntohs(name_record->nameID);
				if (k < 8) {
					name_fields[k] = nbp;

					p = string_area + ntohs(name_record->stringOffset);
					for (k = 0; k < len; k++) {
						if (p[k] != '\0') {
							if (p[k] == '(') {
								*nbp = '[';
							} else if (p[k] == ')') {
								*nbp = ']';
							} else {
								*nbp = p[k];
							}
							nbp++;
						}
					}
					*nbp = '\0';
					nbp++;
				}
			}
		}
		name_record++;
	}

	string_area = (char *) name_table + ntohs(name_table->offset);
	name_record = &(name_table->nameRecords);

	if (!found3) {
		for (j = 0; j < ntohs(name_table->numberOfNameRecords); j++) {

			platform = ntohs(name_record->platformID);

			if (platform == 1) {

				found3 = 1;
				lang = ntohs(name_record->languageID) & 0xff;
				len = ntohs(name_record->stringLength);
				if (lang == 0 || lang == 9) {
					k = ntohs(name_record->nameID);
					if (k < 8) {
						name_fields[k] = nbp;

						p = string_area + ntohs(name_record->stringOffset);
						for (k = 0; k < len; k++) {
							if (p[k] != '\0') {
								if (p[k] == '(') {
									*nbp = '[';
								} else if (p[k] == ')') {
									*nbp = ']';
								} else {
									*nbp = p[k];
								}
								nbp++;
							}
						}
						*nbp = '\0';
						nbp++;
					}
				}
			}
			name_record++;
		}
	}
	if (!found3) {
		FPF(stderr, "**** Cannot decode font name fields ****\n");
		exit(1);
	}
	if (name_fields[6] == NULL) {
		name_fields[6] = name_fields[4];
	}

#if 0
	if(pfa_path){
		// only do this if we're not writing to stdout
		WriteNameAttr(pfa_path, name_fields[6]);
	}
#else
	sprintf(psfontname, "%s", name_fields[6]);
#endif		
	p = name_fields[6];
	while (*p != '\0') {
		if (!isalnum(*p) && *p != '_') {
			*p = '-';
		}
		p++;
	}
}

 void
ttf2pt1::handle_cmap(void)
{
	int             num_tables = ntohs(cmap_table->numberOfEncodingTables);
	BYTE           *ptr;
	int             i, j, k, kk, size, format, offset, seg_c2, found,
	                set_ok;
	int             platform, encoding_id;
	TTF_CMAP_ENTRY *table_entry;
	TTF_CMAP_FMT0  *encoding0;
	TTF_CMAP_FMT4  *encoding4;
	USHORT          start, end, ro;
	short           delta, n;

	found = 0;

	for (i = 0; i < 256; i++) {
		encoding[i] = 0;
	}

	for (i = 0; i < num_tables && !found; i++) {
		table_entry = &(cmap_table->encodingTable[i]);
		offset = ntohl(table_entry->offset);
		encoding4 = (TTF_CMAP_FMT4 *) ((BYTE *) cmap_table + offset);
		format = ntohs(encoding4->format);
		platform = ntohs(table_entry->platformID);
		encoding_id = ntohs(table_entry->encodingID);

		if (platform == 3 && format == 4) {
			switch (encoding_id) {
			case 0:
//				fputs("Found Symbol Encoding\n", stderr);
				break;
			case 1:
//				fputs("Found Unicode Encoding\n", stderr);
				unicode = 1;
				break;
			default:
				FPF(stderr,
				"****MS Encoding ID %d not supported****\n",
					encoding_id);
//				fputs("Treating it like Symbol encoding\n", stderr);
				break;
			}

			found = 1;
			seg_c2 = ntohs(encoding4->segCountX2);
			cmap_n_segs = seg_c2 >> 1;
			ptr = (BYTE *) encoding4 + 14;
			cmap_seg_end = (USHORT *) ptr;
			cmap_seg_start = (USHORT *) (ptr + seg_c2 + 2);
			cmap_idDelta = (short *) (ptr + (seg_c2 * 2) + 2);
			cmap_idRangeOffset = (short *) (ptr + (seg_c2 * 3) + 2);

			for (j = 0; j < cmap_n_segs - 1; j++) {
				start = ntohs(cmap_seg_start[j]);
				end = ntohs(cmap_seg_end[j]);
				delta = ntohs(cmap_idDelta[j]);
				ro = ntohs(cmap_idRangeOffset[j]);

				for (k = start; k <= end; k++) {
					if (ro == 0) {
						n = k + delta;
					} else {
						n = ntohs(*((ro >> 1) + (k - start) +
						 &(cmap_idRangeOffset[j])));
						if (delta != 0)
						{
						 	/*  Not exactly sure how to deal with this circumstance,
						 		I suspect it never occurs */
						 	n += delta;
							fprintf (stderr,
								 "rangeoffset and delta both non-zero - %d/%d",
								 ro, delta);
						}
 					}
 					if(n<0 || n>=numglyphs) {
 						FPF(stderr, "Font contains a broken glyph code mapping, ignored\n");
 						continue;
					}
					if (glyph_list[n].unicode != -1) {
						if (strcmp(glyph_list[n].name, ".notdef") != 0) {
							FPF(stderr,
								"Glyph %s has >= two encodings (A), %4.4x & %4.4x\n",
							 glyph_list[n].name,
								glyph_list[n].unicode,
								k);
						}
						set_ok = 0;
					} else {
						set_ok = 1;
					}
					if (unicode) {
						kk = unicode_to_win31(k);
						if (set_ok) {
							glyph_list[n].unicode = k;
							glyph_list[n].char_no = kk;
						}
						if ((kk & ~0xff) == 0)
							encoding[kk] = n;
					} else {
						if ((k & 0xff00) == 0xf000) {
							encoding[k & 0x00ff] = n;
							if (set_ok) {
								glyph_list[n].char_no = k & 0x00ff;
								glyph_list[n].unicode = k;
							}
						} else {
							if (set_ok) {
								glyph_list[n].char_no = k;
								glyph_list[n].unicode = k;
							}
							FPF(stderr,
								"Glyph %s has non-symbol encoding %4.4x\n",
							 glyph_list[n].name,
								k & 0xffff);
							/*
							 * just use the code
							 * as it is
							 */
							if ((k & ~0xff) == 0)
								encoding[k] = n;
						}
					}
				}
			}
		}
	}

	if (!found) {
//		fputs("No Microsoft encoding, looking for MAC encoding\n", stderr);
		for (i = 0; i < num_tables && !found; i++) {
			table_entry = &(cmap_table->encodingTable[i]);
			offset = ntohl(table_entry->offset);
			encoding0 = (TTF_CMAP_FMT0 *) ((BYTE *) cmap_table + offset);
			format = ntohs(encoding0->format);
			platform = ntohs(table_entry->platformID);
			encoding_id = ntohs(table_entry->encodingID);

			if (format == 0) {
				found = 1;
				size = ntohs(encoding0->length) - 6;
				for (j = 0; j < size; j++) {
					n = encoding0->glyphIdArray[j];
					if (glyph_list[n].char_no != -1) {
						FPF(stderr,
							"Glyph %s has >= two encodings (B), %4.4x & %4.4x\n",
							glyph_list[n].name,
						      glyph_list[n].char_no,
							j);
					} else {
						glyph_list[n].char_no = j;
						if (j < 256) {
							encoding[j] = n;
						}
					}
				}
			}
		}
	}
	if (!found) {
		FPF(stderr, "**** No Recognised Encoding Table ****\n");
		exit(1);
	}
}

 void
ttf2pt1::handle_head(void)
{
	long_offsets = ntohs(head_table->indexToLocFormat);
	if (long_offsets != 0 && long_offsets != 1) {
		FPF(stderr, "**** indexToLocFormat wrong ****\n");
		exit(1);
	}
}

 void
ttf2pt1::draw_glyf(
	  int glyphno,
	  int parent,
	  short *xoff,
	  short *yoff,
	  double *matrix
)
{
	int             i, j, k, k1, len, first, cs, ce;
	/* We assume that hsbw always sets to(0, 0) */
	int             xlast = 0, ylast = 0;
	int             dx1, dy1, dx2, dy2, dx3, dy3;
	int             finished, nguide, contour_start, contour_end;
	short           ncontours, n_inst, last_point;
	USHORT         *contour_end_pt;
	BYTE           *ptr;
#define GLYFSZ	2000
	short           xcoord[GLYFSZ], ycoord[GLYFSZ], xrel[GLYFSZ], yrel[GLYFSZ];
	BYTE            flags[GLYFSZ];
	short           txoff, tyoff;
	GLYPH          *g;
	double          tx, ty;
	int             needreverse = 0;	/* transformation may require
						 * that */
	GENTRY         *lge;

	g = &glyph_list[parent];
	lge = g->lastentry;

	/*
	 * fprintf (stderr,"draw glyf: Matrx offset %d %d\n",xoff,yoff);
	 */

	if (long_offsets) {
		glyf_table = (TTF_GLYF *) (glyf_start + ntohl(long_loca_table[glyphno]));
		len = ntohl(long_loca_table[glyphno + 1]) - ntohl(long_loca_table[glyphno]);
	} else {
		glyf_table = (TTF_GLYF *) (glyf_start + (ntohs(short_loca_table[glyphno]) << 1));
		len = (ntohs(short_loca_table[glyphno + 1]) - ntohs(short_loca_table[glyphno])) << 1;
	}

	if (len <= 0) {
		FPF(stderr,
			"**** Composite glyph %s refers to non-existent glyph %s ****\n",
			glyph_list[parent].name,
			glyph_list[glyphno].name);
		return;
	}
	ncontours = ntohs(glyf_table->numberOfContours);
	if (ncontours <= 0) {
		FPF(stderr,
			"**** Composite glyph %s refers to composite glyph %s ****\n",
			glyph_list[parent].name,
			glyph_list[glyphno].name);
		return;
	}
	contour_end_pt = (USHORT *) ((char *) glyf_table + sizeof(TTF_GLYF));

	last_point = ntohs(contour_end_pt[ncontours - 1]);
	n_inst = ntohs(contour_end_pt[ncontours]);

	ptr = ((BYTE *) contour_end_pt) + (ncontours << 1) + n_inst + 2;
	j = k = 0;
	while (k <= last_point) {
		flags[k] = ptr[j];

		if (ptr[j] & REPEAT) {
			for (k1 = 0; k1 < ptr[j + 1]; k1++) {
				k++;
				flags[k] = ptr[j];
			}
			j++;
		}
		j++;
		k++;
	}

	for (k = 0; k <= last_point; k++) {
		if (flags[k] & XSHORT) {
			if (flags[k] & XSAME) {
				xrel[k] = ptr[j];
			} else {
				xrel[k] = -ptr[j];
			}
			j++;
		} else if (flags[k] & XSAME) {
			xrel[k] = 0;
		} else {
			xrel[k] = ptr[j] * 256 + ptr[j + 1];
			j += 2;
		}
		if (k == 0) {
			xcoord[k] = xrel[k];
		} else {
			xcoord[k] = xrel[k] + xcoord[k - 1];
		}

	}

	for (k = 0; k <= last_point; k++) {
		if (flags[k] & YSHORT) {
			if (flags[k] & YSAME) {
				yrel[k] = ptr[j];
			} else {
				yrel[k] = -ptr[j];
			}
			j++;
		} else if (flags[k] & YSAME) {
			yrel[k] = 0;
		} else {
			yrel[k] = ptr[j] * 256 + ptr[j + 1];
			j += 2;
		}
		if (k == 0) {
			ycoord[k] = yrel[k];
		} else {
			ycoord[k] = yrel[k] + ycoord[k - 1];
		}
	}

	txoff = *xoff;
	tyoff = *yoff;
	if (transform) {
		if (matrix) {
			for (i = 0; i < GLYFSZ; i++) {
				tx = xcoord[i];
				ty = ycoord[i];
				xcoord[i] = scale((int) (matrix[0] * tx + matrix[2] * ty + txoff));
				ycoord[i] = scale((int) (matrix[1] * tx + matrix[3] * ty + tyoff));
			}
		} else {
			for (i = 0; i < GLYFSZ; i++) {
				xcoord[i] = scale(xcoord[i] + txoff);
				ycoord[i] = scale(ycoord[i] + tyoff);
			}
		}
	} else {
		if (matrix) {
			for (i = 0; i < GLYFSZ; i++) {
				tx = xcoord[i];
				ty = ycoord[i];
				xcoord[i] = short((matrix[0] * tx + matrix[2] * ty) + txoff);
				ycoord[i] = short((matrix[1] * tx + matrix[3] * ty) + tyoff);
			}
		} else {
			for (i = 0; i < GLYFSZ; i++) {
				xcoord[i] += txoff;
				ycoord[i] += tyoff;
			}
		}
	}

	i = j = 0;
	first = 1;

	while (i <= ntohs(contour_end_pt[ncontours - 1])) {
		contour_end = ntohs(contour_end_pt[j]);

		if (first) {
			g_rmoveto(g, xcoord[i], ycoord[i]);
			xlast = xcoord[i];
			ylast = ycoord[i];
			ncurves++;
			contour_start = i;
			first = 0;
		} else if (flags[i] & ONOROFF) {
			g_rlineto(g, xcoord[i], ycoord[i]);
			xlast = xcoord[i];
			ylast = ycoord[i];
			ncurves++;
		} else {
			cs = i - 1;
			finished = nguide = 0;
			while (!finished) {
				if (i == contour_end + 1) {
					ce = contour_start;
					finished = 1;
				} else if (flags[i] & ONOROFF) {
					ce = i;
					finished = 1;
				} else {
					i++;
					nguide++;
				}
			}

			switch (nguide) {
			case 0:
				g_rlineto(g, xcoord[ce], ycoord[ce]);
				xlast = xcoord[ce];
				ylast = ycoord[ce];
				ncurves++;
				break;

			case 1:
				g_rrcurveto(g,
				      (xcoord[cs] + 2 * xcoord[cs + 1]) / 3,
				      (ycoord[cs] + 2 * ycoord[cs + 1]) / 3,
				      (2 * xcoord[cs + 1] + xcoord[ce]) / 3,
				      (2 * ycoord[cs + 1] + ycoord[ce]) / 3,
					    xcoord[ce],
					    ycoord[ce]
					);
				xlast = xcoord[ce];
				ylast = ycoord[ce];

				ncurves++;
				break;

			case 2:
				g_rrcurveto(g,
				     (-xcoord[cs] + 4 * xcoord[cs + 1]) / 3,
				     (-ycoord[cs] + 4 * ycoord[cs + 1]) / 3,
				      (4 * xcoord[cs + 2] - xcoord[ce]) / 3,
				      (4 * ycoord[cs + 2] - ycoord[ce]) / 3,
					    xcoord[ce],
					    ycoord[ce]
					);
				xlast = xcoord[ce];
				ylast = ycoord[ce];
				ncurves++;
				break;

			case 3:
				g_rrcurveto(g,
				      (xcoord[cs] + 2 * xcoord[cs + 1]) / 3,
				      (ycoord[cs] + 2 * ycoord[cs + 1]) / 3,
				  (5 * xcoord[cs + 1] + xcoord[cs + 2]) / 6,
				  (5 * ycoord[cs + 1] + ycoord[cs + 2]) / 6,
				      (xcoord[cs + 1] + xcoord[cs + 2]) / 2,
				       (ycoord[cs + 1] + ycoord[cs + 2]) / 2
					);

				g_rrcurveto(g,
				  (xcoord[cs + 1] + 5 * xcoord[cs + 2]) / 6,
				  (ycoord[cs + 1] + 5 * ycoord[cs + 2]) / 6,
				  (5 * xcoord[cs + 2] + xcoord[cs + 3]) / 6,
				  (5 * ycoord[cs + 2] + ycoord[cs + 3]) / 6,
				      (xcoord[cs + 3] + xcoord[cs + 2]) / 2,
				       (ycoord[cs + 3] + ycoord[cs + 2]) / 2
					);

				g_rrcurveto(g,
				  (xcoord[cs + 2] + 5 * xcoord[cs + 3]) / 6,
				  (ycoord[cs + 2] + 5 * ycoord[cs + 3]) / 6,
				      (2 * xcoord[cs + 3] + xcoord[ce]) / 3,
				      (2 * ycoord[cs + 3] + ycoord[ce]) / 3,
					    xcoord[ce],
					    ycoord[ce]
					);
				ylast = ycoord[ce];
				xlast = xcoord[ce];

				ncurves += 3;
				break;

			default:
				k1 = cs + nguide;
				g_rrcurveto(g,
				      (xcoord[cs] + 2 * xcoord[cs + 1]) / 3,
				      (ycoord[cs] + 2 * ycoord[cs + 1]) / 3,
				  (5 * xcoord[cs + 1] + xcoord[cs + 2]) / 6,
				  (5 * ycoord[cs + 1] + ycoord[cs + 2]) / 6,
				      (xcoord[cs + 1] + xcoord[cs + 2]) / 2,
				       (ycoord[cs + 1] + ycoord[cs + 2]) / 2
					);

				for (k = cs + 2; k <= k1 - 1; k++) {
					g_rrcurveto(g,
					(xcoord[k - 1] + 5 * xcoord[k]) / 6,
					(ycoord[k - 1] + 5 * ycoord[k]) / 6,
					(5 * xcoord[k] + xcoord[k + 1]) / 6,
					(5 * ycoord[k] + ycoord[k + 1]) / 6,
					    (xcoord[k] + xcoord[k + 1]) / 2,
					     (ycoord[k] + ycoord[k + 1]) / 2
						);

				}

				g_rrcurveto(g,
				      (xcoord[k1 - 1] + 5 * xcoord[k1]) / 6,
				      (ycoord[k1 - 1] + 5 * ycoord[k1]) / 6,
					  (2 * xcoord[k1] + xcoord[ce]) / 3,
					  (2 * ycoord[k1] + ycoord[ce]) / 3,
					    xcoord[ce],
					    ycoord[ce]
					);
				xlast = xcoord[ce];
				ylast = ycoord[ce];

				ncurves += nguide;
				break;
			}
		}
		if (i >= contour_end) {
			g_closepath(g);
			first = 1;
			i = contour_end + 1;
			j++;
		} else {
			i++;
		}
	}
	*xoff = xlast;
	*yoff = ylast;

	if (matrix) {
		/* guess whether do we need to reverse the results */

		int             x[3], y[3];
		int             max = 0, from, to;

		/* transform a triangle going in proper direction */
		/*
		 * the origin of triangle is in (0,0) so we know it in
		 * advance
		 */

		x[0] = y[0] = 0;
		x[1] = int(matrix[0] * 0 + matrix[2] * 300);
		y[1] = int(matrix[1] * 0 + matrix[3] * 300);
		x[2] = int(matrix[0] * 300 + matrix[2] * 0);
		y[2] = int(matrix[1] * 300 + matrix[3] * 0);

		/* then find the topmost point */
		for (i = 0; i < 4; i++)
			if (y[i] > y[max])
				max = i;
		from = (max + 3 - 1) % 3;
		to = (max + 1) % 3;

		needreverse = 0;

		/* special cases for horizontal lines */
		if (y[max] == y[from]) {
			if (x[max] < y[from])
				needreverse = 1;
		} else if (y[to] == y[from]) {
			if (x[to] < x[max])
				needreverse = 1;
		} else {	/* generic case */
			if ((x[to] - x[max]) * (y[max] - y[from])
			    > (x[max] - x[from]) * (y[to] - y[max]))
				needreverse = 1;
		}

		if (needreverse) {
			if (lge) {
				assertpath(lge->next, __LINE__, g->name);
				reversepathsfromto(lge->next, NULL);
			} else {
				assertpath(g->entries, __LINE__, g->name);
				reversepaths(g);
			}
		}
	}
}

 double
ttf2pt1::f2dot14(
	short x
)
{
	short           y = ntohs(x);
	return (y >> 14) + ((y & 0x3fff) / 16384.0);
}

 void
ttf2pt1::print_glyf(
	   int glyphno
)
{
	GLYPH          *g;
	GENTRY         *ge;
	int             x = 0, y = 0;
	int             i;

	g = &glyph_list[glyphno];

	fprintf(pfa_file, "/%s { \n", g->name);

	/* consider widths >3000 as bugs */
	fprintf(pfa_file, "0 %d hsbw\n", (g->scaledwidth < 3000 ? g->scaledwidth : 1000));

#if 0
	fprintf(pfa_file, "%% contours: ");
	for (i = 0; i < g->ncontours; i++)
		fprintf(pfa_file, "%s(%d,%d) ", (g->contours[i].direction == DIR_OUTER ? "out" : "in"),
			g->contours[i].xofmin, g->contours[i].ymin);
	fprintf(pfa_file, "\n");

	if (g->rymin < 5000)
		fprintf(pfa_file, "%d lower%s\n", g->rymin, (g->flatymin ? "flat" : "curve"));
	if (g->rymax > -5000)
		fprintf(pfa_file, "%d upper%s\n", g->rymax, (g->flatymax ? "flat" : "curve"));
#endif

	if (g->hstems)
		for (i = 0; i < g->nhs; i += 2) {
			if (g->hstems[i].flags & ST_3) {
				fprintf(pfa_file, "%d %d %d %d %d %d hstem3\n",
					g->hstems[i].value,
				g->hstems[i + 1].value - g->hstems[i].value,
					g->hstems[i + 2].value,
					g->hstems[i + 3].value - g->hstems[i + 2].value,
					g->hstems[i + 4].value,
					g->hstems[i + 5].value - g->hstems[i + 4].value
					);
				i += 4;
			} else {
				fprintf(pfa_file, "%d %d hstem\n", g->hstems[i].value,
				g->hstems[i + 1].value - g->hstems[i].value);
			}
		}

	if (g->vstems)
		for (i = 0; i < g->nvs; i += 2) {
			if (g->vstems[i].flags & ST_3) {
				fprintf(pfa_file, "%d %d %d %d %d %d vstem3\n",
					g->vstems[i].value,
				g->vstems[i + 1].value - g->vstems[i].value,
					g->vstems[i + 2].value,
					g->vstems[i + 3].value - g->vstems[i + 2].value,
					g->vstems[i + 4].value,
					g->vstems[i + 5].value - g->vstems[i + 4].value
					);
				i += 4;
			} else {
				fprintf(pfa_file, "%d %d vstem\n", g->vstems[i].value,
				g->vstems[i + 1].value - g->vstems[i].value);
			}
		}

	for (ge = g->entries; ge != 0; ge = ge->next) {
		switch (ge->type) {
		case GE_MOVE:
			if (absolute)
				fprintf(pfa_file, "%d %d amoveto\n", ge->x3, ge->y3);
			else
				rmoveto(ge->x3 - x, ge->y3 - y);
			if (0)
				FPF(stderr, "Glyph %s: print moveto(%d, %d)\n",
					g->name, ge->x3, ge->y3);
			x = ge->x3;
			y = ge->y3;
			break;
		case GE_LINE:
			if (absolute)
				fprintf(pfa_file, "%d %d alineto\n", ge->x3, ge->y3);
			else
				rlineto(ge->x3 - x, ge->y3 - y);
			x = ge->x3;
			y = ge->y3;
			break;
		case GE_CURVE:
			if (absolute)
				fprintf(pfa_file, "%d %d %d %d %d %d arcurveto\n",
					ge->x1, ge->y1, ge->x2, ge->y2, ge->x3, ge->y3);
			else
				rrcurveto(ge->x1 - x, ge->y1 - y,
					  ge->x2 - ge->x1, ge->y2 - ge->y1,
					  ge->x3 - ge->x2, ge->y3 - ge->y2);
			x = ge->x3;
			y = ge->y3;
			break;
		case GE_PATH:
			closepath();
			break;
		default:
			FPF(stderr, "Glyph %s: unknown entry type '%c'\n",
				g->name, ge->type);
			break;
		}
	}

	fprintf(pfa_file, "endchar } ND\n");
}

 void
ttf2pt1::convert_glyf(
	     int glyphno
)
{
	int             len, c;
	short           ncontours;
	USHORT          flagbyte, glyphindex, xscale, yscale, scale01,
	                scale10;
	SHORT           arg1, arg2, xoff, yoff;
	BYTE           *ptr;
	char           *bptr;
	SHORT          *sptr;
	double          matrix[6];
	GLYPH          *g;

	ncurves = 0;

	if (long_offsets) {
		glyf_table = (TTF_GLYF *) (glyf_start + ntohl(long_loca_table[glyphno]));
		len = ntohl(long_loca_table[glyphno + 1]) - ntohl(long_loca_table[glyphno]);
	} else {
		glyf_table = (TTF_GLYF *) (glyf_start + (ntohs(short_loca_table[glyphno]) << 1));
		len = (ntohs(short_loca_table[glyphno + 1]) - ntohs(short_loca_table[glyphno])) << 1;
	}

	c = glyph_list[glyphno].char_no;

	if (transform) {
		glyph_list[glyphno].scaledwidth = scale(glyph_list[glyphno].width);
	} else {
		glyph_list[glyphno].scaledwidth = glyph_list[glyphno].width;
	}

	glyph_list[glyphno].entries = 0;
	glyph_list[glyphno].lastentry = 0;
	glyph_list[glyphno].path = 0;
	if (len != 0) {
		ncontours = ntohs(glyf_table->numberOfContours);

		if (ncontours <= 0) {
			ptr = ((BYTE *) glyf_table + sizeof(TTF_GLYF));
			sptr = (SHORT *) ptr;
			xoff = 0;
			yoff = 0;
			do {
				flagbyte = ntohs(*sptr);
				sptr++;
				glyphindex = ntohs(*sptr);
				sptr++;

				if (flagbyte & ARG_1_AND_2_ARE_WORDS) {
					arg1 = ntohs(*sptr);
					sptr++;
					arg2 = ntohs(*sptr);
					sptr++;
				} else {
					bptr = (char *) sptr;
					arg1 = (signed char) bptr[0];
					arg2 = (signed char) bptr[1];
					sptr++;
				}
				matrix[1] = matrix[2] = 0.0;

				if (flagbyte & WE_HAVE_A_SCALE) {
					matrix[0] = matrix[3] = f2dot14(*sptr);
					sptr++;
				} else if (flagbyte & WE_HAVE_AN_X_AND_Y_SCALE) {
					matrix[0] = f2dot14(*sptr);
					sptr++;
					matrix[3] = f2dot14(*sptr);
					sptr++;
				} else if (flagbyte & WE_HAVE_A_TWO_BY_TWO) {
					matrix[0] = f2dot14(*sptr);
					sptr++;
					matrix[1] = f2dot14(*sptr);
					sptr++;
					matrix[2] = f2dot14(*sptr);
					sptr++;
					matrix[3] = f2dot14(*sptr);
					sptr++;
				} else {
					matrix[0] = matrix[3] = 1.0;
				}

				/*
				 * See *
				 * http://fonts.apple.com/TTRefMan/RM06/Chap6g
				 * lyf.html * matrix[0,1,2,3,4,5]=a,b,c,d,m,n
				 */

				if (fabs(matrix[0]) > fabs(matrix[1]))
					matrix[4] = fabs(matrix[0]);
				else
					matrix[4] = fabs(matrix[1]);
				if (fabs(fabs(matrix[0]) - fabs(matrix[2])) <= 33. / 65536.)
					matrix[4] *= 2.0;

				if (fabs(matrix[2]) > fabs(matrix[3]))
					matrix[5] = fabs(matrix[2]);
				else
					matrix[5] = fabs(matrix[3]);
				if (fabs(fabs(matrix[2]) - fabs(matrix[3])) <= 33. / 65536.)
					matrix[5] *= 2.0;

				/*
				 * fprintf (stderr,"Matrix Opp %hd
				 * %hd\n",arg1,arg2);
				 */
#if 0
				FPF(stderr, "Matrix: %f %f %f %f %f %f\n",
				 matrix[0], matrix[1], matrix[2], matrix[3],
					matrix[4], matrix[5]);
				FPF(stderr, "Offset: %d %d (%s)\n",
					arg1, arg2,
					((flagbyte & ARGS_ARE_XY_VALUES) ? "XY" : "index"));
#endif

				if (flagbyte & ARGS_ARE_XY_VALUES) {
					arg1 = short(arg1 * matrix[4]);
					arg2 = short(arg2 * matrix[5]);
				} else {
					/*
					 * must extract values from a glyph *
					 * but it seems to be too much pain *
					 * and it's not clear now that it
					 * would be really * used in any
					 * interesting font
					 */
				}

				draw_glyf(glyphindex, glyphno, &arg1, &arg2, matrix);

				/*
				 * we use absolute values now so we don't
				 * really need that
				 */
				xoff = arg1;
				yoff = arg2;

			} while (flagbyte & MORE_COMPONENTS);
		} else {
			arg1 = 0;
			arg2 = 0;
			matrix[0] = matrix[3] = matrix[4] = matrix[5] = 1.0;
			matrix[1] = matrix[2] = 0.0;
			draw_glyf(glyphno, glyphno, &arg1, &arg2, NULL);
		}

		assertpath(glyph_list[glyphno].entries, __LINE__, glyph_list[glyphno].name);

		closepaths(&glyph_list[glyphno]);
		assertpath(glyph_list[glyphno].entries, __LINE__, glyph_list[glyphno].name);

#if 0
		fixcontours(&glyph_list[glyphno]);
#endif

#if 0
		testfixcvdir(&glyph_list[glyphno]);
#endif

		if (smooth) {
			smoothjoints(&glyph_list[glyphno]);
			assertpath(glyph_list[glyphno].entries, __LINE__, glyph_list[glyphno].name);

			straighten(&glyph_list[glyphno], 1);
			assertpath(glyph_list[glyphno].entries, __LINE__, glyph_list[glyphno].name);

			splitzigzags(&glyph_list[glyphno]);
			assertpath(glyph_list[glyphno].entries, __LINE__, glyph_list[glyphno].name);

			forceconcise(&glyph_list[glyphno]);
			assertpath(glyph_list[glyphno].entries, __LINE__, glyph_list[glyphno].name);

			straighten(&glyph_list[glyphno], 0);
			assertpath(glyph_list[glyphno].entries, __LINE__, glyph_list[glyphno].name);

			smoothjoints(&glyph_list[glyphno]);
			assertpath(glyph_list[glyphno].entries, __LINE__, glyph_list[glyphno].name);

			flattencurves(&glyph_list[glyphno]);
		}

		if (ncurves > 100) {
			FPF(stderr,
			"**Glyf %s is too long, may have to be removed**\n",
				glyph_list[glyphno].name);
		}
	}
}

 int
ttf2pt1::handle_hmtx(void)
{
	int             i;
	int             n_hmetrics = ntohs(hhea_table->numberOfHMetrics);
	GLYPH          *g;
	LONGHORMETRIC  *hmtx_entry = hmtx_table;
	FWORD          *lsblist;

	for (i = 0; i < n_hmetrics; i++) {
		g = &(glyph_list[i]);
		g->width = ntohs(hmtx_entry->advanceWidth);
		g->lsb = ntohs(hmtx_entry->lsb);
		hmtx_entry++;
	}

	lsblist = (FWORD *) hmtx_entry;
	hmtx_entry--;

	for (i = n_hmetrics; i < numglyphs; i++) {
		g = &(glyph_list[i]);
		g->width = ntohs(hmtx_entry->advanceWidth);
		g->lsb = ntohs(lsblist[i - n_hmetrics]);
	}

	return 0;
}

 void
ttf2pt1::handle_post(void)
{
	int             i, len, n, found, npost;
	unsigned int    format;
	USHORT         *name_index;
	char           *ptr, *p;
	char          **ps_name_ptr = (char **) malloc(numglyphs * sizeof(char *));
/* 	int            *ps_name_len = (int *) malloc(numglyphs * sizeof(int));   No longer used */
	int             n_ps_names;

	format = ntohl(post_table->formatType);

	if (format == 0x00010000) {
		for (i = 0; i < 258 && i < numglyphs; i++) {
			glyph_list[i].name = mac_glyph_names[i];
		}
	} else if (format == 0x00020000) {
                npost = ntohs(post_table->numGlyphs);
                if (numglyphs != npost) {
                        /* This is an error in the font, but we can now cope */
                        FPF(stderr, "**** Postscript table size mismatch %d/%d ****\n",
                                npost, numglyphs);
                }
                n_ps_names = 0;
                name_index = &(post_table->glyphNameIndex);

                /* This checks the integrity of the post table */       
                for (i=0; i<npost; i++) {
                    n = ntohs(name_index[i]);
                    if (n > n_ps_names + 257) {
                        n_ps_names = n - 257;
                    }
                }

                ptr = (char *) post_table + 34 + (numglyphs << 1);
                i = 0;
                while (*ptr > 0 && i < n_ps_names) {
                        len = *ptr;
                        /* previously the program wrote nulls into the table. If the table
                           was corrupt, this could put zeroes anywhere, leading to obscure bugs,
                           so now I malloc space for the names. Yes it is much less efficient */
                           
                        if ((p = (char*)malloc(len+1)) == NULL) {
                            fprintf (stderr, "****malloc failed line %d\n", __LINE__);
                            exit(-1);
                        }
                        
                        ps_name_ptr[i] = p;
                        strncpy(p, ptr+1, len);
                        p[len] = '\0';
                        i ++;
                        ptr += len + 1;
                }
        
                if (i != n_ps_names)
                {
                    fprintf (stderr, "** Postscript Name mismatch %d != %d **\n",
                             i, n_ps_names);
                    n_ps_names = i;
                }

                /*
                 * for (i=0; i<n_ps_names; i++) { FPF(stderr, "i=%d,
                 * len=%d, name=%s\n", i, ps_name_len[i], ps_name_ptr[i]); }
                 */

                for (i = 0; i < npost; i++) {
                        n = ntohs(name_index[i]);
                        if (n < 258) {
                                glyph_list[i].name = mac_glyph_names[n];
                        } else if (n < 258 + n_ps_names) {
                                glyph_list[i].name = ps_name_ptr[n - 258];
                        } else {
                                glyph_list[i].name = (char*)malloc(10);
                                sprintf(glyph_list[i].name, "_%d", n);
                                FPF(stderr,
                                        "**** Glyph No. %d has no postscript name, becomes %s ****\n",
                                        i, glyph_list[i].name);
                        }
                }
                /* Now fake postscript names for all those beyond the end of the table */
                if (npost < numglyphs) {
                    for (i=npost; i<numglyphs; i++) {
                        if ((glyph_list[i].name = (char*)malloc(10)) == NULL)
                        {
                            fprintf (stderr, "****malloc failed line %d\n", __LINE__);
                            exit(-1);
                        }
                        sprintf(glyph_list[i].name, "_%d", i);
                        FPF(stderr,
                                "** Glyph No. %d has no postscript name, becomes %s **\n",
                                i, glyph_list[i].name);
                    }
                }
	} else if (format == 0x00030000) {
//		fputs("No postscript table, using default\n", stderr);
		ps_fmt_3 = 1;
	} else if (format == 0x00028000) {
		ptr = (char *) &(post_table->numGlyphs);
		for (i = 0; i < numglyphs; i++) {
			glyph_list[i].name = mac_glyph_names[i + ptr[i]];
		}
	} else {
		FPF(stderr,
			"**** Postscript table in wrong format %x ****\n",
			format);
		exit(1);
	}

	/* check for names with wrong characters */
	for (n = 0; n < numglyphs; n++) {
		int             c;
		for (i = 0; (c = glyph_list[n].name[i]) != 0; i++) {
			if (!(isalnum(c) || c == '.')) {
				FPF(stderr, "Glyph %d has bad characters in name(%s), ",
					n, glyph_list[n].name);
				glyph_list[n].name = (char*)malloc(10);
				sprintf(glyph_list[n].name, "_%d", n);
				FPF(stderr, "changing to %s\n", glyph_list[n].name);
				break;
			}
		}
	}


	if (!ps_fmt_3) {
		for (n = 0; n < numglyphs; n++) {
			found = 0;
			for (i = 0; i < n && !found; i++) {
				if (strcmp(glyph_list[i].name, glyph_list[n].name) == 0) {
					glyph_list[n].name = (char*)malloc(10);
					sprintf(glyph_list[n].name, "_%d", n);
					FPF(stderr,
						"Glyph %d has the same name as %d: (%s), changing to %s\n",
						n, i,
						glyph_list[i].name,
						glyph_list[n].name);
					found = 1;
				}
			}
		}
	}
}

 void
ttf2pt1::handle_kern(void)
{
	TTF_KERN_SUB   *subtable;
	TTF_KERN_ENTRY *kern_entry;
	int             i, j;
	int             ntables = ntohs(kern_table->nTables);
	int             npairs,npairs_used;
	char           *ptr = (char *) kern_table + 4;

	for (i = 0; i < ntables; i++) {
		subtable = (TTF_KERN_SUB *) ptr;
		if ((ntohs(subtable->coverage) & 0xff00) == 0) {
			npairs = (short) ntohs(subtable->nPairs);
			kern_entry = (TTF_KERN_ENTRY *) (ptr + sizeof(TTF_KERN_SUB));

			npairs_used = 0;
			for (j = 0; j < npairs; j++) {
			  if (glyph_list[ntohs(kern_entry->left)].flags & GF_USED &&
			      glyph_list[ntohs(kern_entry->right)].flags & GF_USED)
			    npairs_used ++;
			  kern_entry++;
			}

			fprintf(afm_file, "StartKernPairs %hd\n", npairs_used);
			kern_entry = (TTF_KERN_ENTRY *) (ptr + sizeof(TTF_KERN_SUB));
			for (j = 0; j < npairs; j++) {
			  if (glyph_list[ntohs(kern_entry->left)].flags & GF_USED &&
			      glyph_list[ntohs(kern_entry->right)].flags & GF_USED)
			    fprintf(afm_file, "KPX %s %s %d\n",
				    glyph_list[ntohs(kern_entry->left)].name,
				    glyph_list[ntohs(kern_entry->right)].name,
				    scale((short) ntohs(kern_entry->value)));
			  kern_entry++;
			}
			fprintf(afm_file, "EndKernPairs\n");
		}
		ptr += subtable->length;
	}
}

int32
ttf2pt1::convert_tt(const char *ttFont, const char *psFont)
{
	int             i;
	time_t          now;
	struct stat     statbuf;
	char            filename[100];
	int             c,nchars,nmetrics;
	int             ws;
	int             forcebold= -1; /* -1 means "don't know" */
	char           *lang;
	bool			use_stdout;

	encode = 1;
	
	/* try to guess the language by the locale used */
	if(uni_lang_converter==0 && (lang=getenv("LANG"))!=0 ) {
		for(i=0; i < sizeof uni_lang/sizeof(struct uni_language); i++) {
			if( !strncmp(uni_lang[i].name, lang, strlen(uni_lang[i].name)) ) {
				uni_lang_converter=uni_lang[i].conv;
				goto got_a_language;
			}
		}
		/* no full name ? try aliases */
		for(i=0; i < sizeof uni_lang/sizeof(struct uni_language); i++) {
			for(c=0; c<MAXUNIALIAS; c++)
				if( uni_lang[i].alias[c]!=0
				&& !strncmp(uni_lang[i].alias[c], lang, strlen(uni_lang[i].alias[c])) ) {
					uni_lang_converter=uni_lang[i].conv;
					goto got_a_language;
				}
		}
	got_a_language:
		if(uni_lang_converter!=0) 
			FPF(stderr, "Using language '%s' for Unicode fonts\n", uni_lang[i].name);
	}

	if (stat(ttFont, &statbuf) == -1) {
		FPF(stderr, "**** Cannot access %s ****\n", ttFont);
		return B_ERROR;
	}
	if ((filebuffer = (char*)malloc(statbuf.st_size)) == NULL) {
		FPF(stderr, "**** Cannot malloc space for file ****\n");
		return B_ERROR;
	}
	if ((ttf_file = open(ttFont, O_RDONLY, 0)) == -1) {
		FPF(stderr, "**** Cannot open %s ****\n", ttFont);
		return B_ERROR;
	} else {
		FPF(stderr, "Processing file %s\n", ttFont);
	}

	if (read(ttf_file, filebuffer, statbuf.st_size) != statbuf.st_size) {
		FPF(stderr, "**** Could not read whole file ****\n");
		return B_ERROR;
	}

	close(ttf_file);

	directory = (TTF_DIRECTORY *) filebuffer;

	if (ntohl(directory->sfntVersion) != 0x00010000) {
		FPF(stderr,
			"****Unknown File Version number [%x], or not a TrueType file****\n",
			directory->sfntVersion);
		return B_ERROR;
	}

	use_stdout = false;
	sprintf(filename, "%s.afm", psFont) ;
	if ((afm_file = fopen(filename, "w+")) == NULL) {
		FPF(stderr, "**** Cannot create %s ****\n", filename);
		return B_ERROR;
	}

	char unencoded_output[512];
	sprintf(unencoded_output, "%s", tmpnam(NULL));
	if ((pfa_file = fopen(unencoded_output, "a+")) == NULL) {
		FPF(stderr, "**** Cannot create %s ****\n", filename);
		return B_ERROR;
	} else {
		FPF(stderr, "Creating file %s\n", filename);
	}


	dir_entry = &(directory->list);

	for (i = 0; i < ntohs(directory->numTables); i++) {

		if (memcmp(dir_entry->tag, "name", 4) == 0) {
			name_table = (TTF_NAME *) (filebuffer + ntohl(dir_entry->offset));
		} else if (memcmp(dir_entry->tag, "head", 4) == 0) {
			head_table = (TTF_HEAD *) (filebuffer + ntohl(dir_entry->offset));
		} else if (memcmp(dir_entry->tag, "hhea", 4) == 0) {
			hhea_table = (TTF_HHEA *) (filebuffer + ntohl(dir_entry->offset));
		} else if (memcmp(dir_entry->tag, "post", 4) == 0) {
			post_table = (TTF_POST_HEAD *) (filebuffer + ntohl(dir_entry->offset));
		} else if (memcmp(dir_entry->tag, "glyf", 4) == 0) {
			glyf_start = (BYTE *) (filebuffer + ntohl(dir_entry->offset));
		} else if (memcmp(dir_entry->tag, "cmap", 4) == 0) {
			cmap_table = (TTF_CMAP *) (filebuffer + ntohl(dir_entry->offset));
		} else if (memcmp(dir_entry->tag, "kern", 4) == 0) {
			kern_table = (TTF_KERN *) (filebuffer + ntohl(dir_entry->offset));
		} else if (memcmp(dir_entry->tag, "maxp", 4) == 0) {
			maxp_table = (TTF_MAXP *) (filebuffer + ntohl(dir_entry->offset));
		} else if (memcmp(dir_entry->tag, "hmtx", 4) == 0) {
			hmtx_table = (LONGHORMETRIC *) (filebuffer + ntohl(dir_entry->offset));
		} else if (memcmp(dir_entry->tag, "loca", 4) == 0) {
			long_loca_table = (ULONG *) (filebuffer + ntohl(dir_entry->offset));
			short_loca_table = (USHORT *) long_loca_table;
		} else if (memcmp(dir_entry->tag, "EBDT", 4) == 0 ||
			   memcmp(dir_entry->tag, "EBLC", 4) == 0 ||
			   memcmp(dir_entry->tag, "EBSC", 4) == 0) {
			FPF(stderr, "Font contains bitmaps\n");
		}
		dir_entry++;
	}

	if(use_stdout == false){
		handle_name(unencoded_output);
	} else {
		handle_name(NULL);
	}
	
	handle_head();

	numglyphs = ntohs(maxp_table->numGlyphs);
	FPF(stderr, "numglyphs = %d\n", numglyphs);

	glyph_list = (GLYPH *) calloc(1, numglyphs * sizeof(GLYPH));

	for (i = 0; i < numglyphs; i++) {
		glyph_list[i].char_no = -1;
		glyph_list[i].unicode = -1;
		glyph_list[i].name = Unknown_glyph;
		glyph_list[i].flags = 0;
	}

	handle_post();

	handle_hmtx();

	handle_cmap();

	if (ps_fmt_3) {
		
		for (i = 0; i < 256; i++) {
			if (encoding[i] != 0) {
				glyph_list[encoding[i]].name = ISOLatin1Encoding[i];
			} else {
				glyph_list[encoding[i]].name = ".notdef";
			}
		}
	}
	scale_factor = 1000.0 / (double) ntohs(head_table->unitsPerEm);
	if (ntohs(head_table->unitsPerEm) == 1000)
		transform = 0;	/* nothing to transform */

	for (i = 0; i < 256; i++) {
		glyph_list[encoding[i]].flags |= GF_USED;
	}

	for (i = 0; i < numglyphs; i++) {
		if (glyph_list[i].flags & GF_USED) {
			convert_glyf(i);
		}
	}

	italic_angle = (short) (ntohs(post_table->italicAngle.upper)) +
		((short) ntohs(post_table->italicAngle.lower) / 65536.0);

	if (italic_angle > 45.0 || italic_angle < -45.0)
		italic_angle = 0.0;	/* consider buggy */

	if (hints) {
		findblues();
		for (i = 0; i < numglyphs; i++) {
			if (glyph_list[i].flags & GF_USED) {
				buildstems(&glyph_list[i]);
				assertpath(glyph_list[i].entries, __LINE__, glyph_list[i].name);
			}
		}
		stemstatistics();
	} else {
		if (transform) {
			bbox[0] = scale((short) ntohs(head_table->xMin));
			bbox[1] = scale((short) ntohs(head_table->yMin));
			bbox[2] = scale((short) ntohs(head_table->xMax));
			bbox[3] = scale((short) ntohs(head_table->yMax));
		} else {
			bbox[0] = (short) ntohs(head_table->xMin);
			bbox[1] = (short) ntohs(head_table->yMin);
			bbox[2] = (short) ntohs(head_table->xMax);
			bbox[3] = (short) ntohs(head_table->yMax);
		}
	}
	if (reverse)
		for (i = 0; i < numglyphs; i++) {
			if (glyph_list[i].flags & GF_USED) {
				reversepaths(&glyph_list[i]);
				assertpath(glyph_list[i].entries, __LINE__, glyph_list[i].name);
			}
		}


#if 0
	/*
	** It seems to bring troubles. The problem is that some
	** styles of the font may be recognized as fixed-width
	** while other styles of the same font as proportional.
	** So it's better to be commented out yet.
	*/
	if (tryfixed) 
		alignwidths();
#endif

	if(trybold) {
		char *str;

		forcebold=0;
		str=name_fields[4];
		for(i=0; str[i]!=0; i++) {
			if( (str[i]=='B'
				|| str[i]=='b' 
					&& ( i==0 || !isalpha(str[i-1]) )
				)
			&& !strncmp("old",&str[i+1],3)
			&& !islower(str[i+4])
			) {
				forcebold=1;
				break;
			}
		}
		/* make another try */
		if(forcebold==0) {
			str=name_fields[6];
			for(i=0; str[i]!=0; i++) {
				if( (str[i]=='B'
					|| str[i]=='b' 
						&& ( i==0 || !isalpha(str[i-1]) )
					)
				&& !strncmp("old",&str[i+1],3)
				&& !islower(str[i+4])
				) {
					forcebold=1;
					break;
				}
			}
		}
	}

	fprintf(pfa_file, "%%!PS-AdobeFont-1.0 %s %s\n", name_fields[6], name_fields[0]);
	time(&now);
	fprintf(pfa_file, "%%%%CreationDate: %s", ctime(&now));
	fprintf(pfa_file, "%% Converted from TrueType font %s by ttf2pt1 program\n%%\n", ttFont);
	fprintf(pfa_file, "%%%%EndComments\n");
	fprintf(pfa_file, "12 dict begin\n/FontInfo 9 dict dup begin\n");

	FPF(stderr, "FontName %s\n", name_fields[6]);


	fprintf(pfa_file, "/version (%s) readonly def\n", name_fields[5]);

	fprintf(pfa_file, "/Notice (%s) readonly def\n", name_fields[0]);

	fprintf(pfa_file, "/FullName (%s) readonly def\n", name_fields[4]);
	fprintf(pfa_file, "/FamilyName (%s) readonly def\n", name_fields[1]);

	fprintf(pfa_file, "/Weight (%s) readonly def\n", name_fields[2]);

	fprintf(pfa_file, "/ItalicAngle %f def\n", italic_angle);
	fprintf(pfa_file, "/isFixedPitch %s def\n",
		ntohl(post_table->isFixedPitch) ? "true" : "false");

#if 0
	nchars = numglyphs;
#else
	/* we don't print out the unused glyphs */
	nchars = 0;
	for (i = 0; i < numglyphs; i++) {
		if (glyph_list[i].flags & GF_USED) {
			nchars++;
		}
	}
#endif

    fprintf(afm_file, "StartFontMetrics 4.1\n");
    fprintf(afm_file, "FontName %s\n", name_fields[6]);
    fprintf(afm_file, "FullName %s\n", name_fields[4]);
    fprintf(afm_file, "Notice %s\n", name_fields[0]);
    fprintf(afm_file, "FamilyName %s\n", name_fields[1]);
    fprintf(afm_file, "Weight %s\n", name_fields[2]);
    fprintf(afm_file, "Version %s\n", name_fields[5]);
    fprintf(afm_file, "Characters %d\n", numglyphs);
    fprintf(afm_file, "ItalicAngle %.1f\n", italic_angle);

    fprintf(afm_file, "Ascender %d\n",
            scale((short)ntohs(hhea_table->ascender)));
    fprintf(afm_file, "Descender %d\n",
            scale((short)ntohs(hhea_table->descender)));

	if (transform) {
		fprintf(pfa_file, "/UnderlinePosition %d def\n",
			scale((short) ntohs(post_table->underlinePosition)));

		fprintf(pfa_file, "/UnderlineThickness %hd def\nend readonly def\n",
		      (short) scale(ntohs(post_table->underlineThickness)));

		fprintf(afm_file, "UnderlineThickness %d\n",
			(short) scale(ntohs(post_table->underlineThickness)));

		fprintf(afm_file, "UnderlinePosition %d\n",
			scale((short) ntohs(post_table->underlinePosition)));

	} else {
		fprintf(pfa_file, "/UnderlinePosition %hd def\n",
			(short) ntohs(post_table->underlinePosition));

		fprintf(pfa_file, "/UnderlineThickness %hd def\nend readonly def\n",
			(short) ntohs(post_table->underlineThickness));
		fprintf(afm_file, "UnderlineThickness %d\n",
			(short) ntohs(post_table->underlineThickness));

		fprintf(afm_file, "UnderlinePosition %d\n",
			(short) ntohs(post_table->underlinePosition));

	}

    fprintf(afm_file, "IsFixedPitch %s\n",
            ntohl(post_table->isFixedPitch) ? "true" : "false" );
    fprintf(afm_file, "FontBBox %d %d %d %d\n",
		bbox[0], bbox[1], bbox[2], bbox[3]);

	fprintf(pfa_file, "/FontName /%s def\n", name_fields[6]);
	fprintf(pfa_file, "/PaintType 0 def\n/StrokeWidth 0 def\n");
	/* Im not sure if these are fixed */
	fprintf(pfa_file, "/FontType 1 def\n");

	if (transform) {
		fprintf(pfa_file, "/FontMatrix [0.001 0 0 0.001 0 0] def\n");
	} else {
		fprintf(pfa_file, "/FontMatrix [%9.7f 0 0 %9.7f 0 0] def\n",
			scale_factor / 1000.0, scale_factor / 1000.0);
	}

	fprintf(pfa_file, "/FontBBox {%d %d %d %d} readonly def\n",
		bbox[0], bbox[1], bbox[2], bbox[3]);

	fprintf(pfa_file, "/Encoding 256 array\n");
	/* determine number of elements for metrics table */
	nmetrics = 0;
 	for (i = 0; i < 256; i++) {
		c = glyph_list[encoding[i]].char_no;
		if (c != -1 /* && c == i Take this term out, otherwise count is wrong */)
		  nmetrics++;
	}
	fprintf(afm_file, "StartCharMetrics %d\n", nmetrics);

 	for (i = 0; i < 256; i++) {
		fprintf(pfa_file,
			"dup %d /%s put\n", i, glyph_list[encoding[i]].name);
		c = glyph_list[encoding[i]].char_no;
		if (c != -1) {
		  if (long_offsets) {
		    glyf_table = (TTF_GLYF *) (glyf_start + ntohl(long_loca_table[encoding[i]]));
		  } else {
		    glyf_table = (TTF_GLYF *) (glyf_start + (ntohs(short_loca_table[encoding[i]]) << 1));
		  }

		  fprintf(afm_file, "C %d ; WX %d ; N %s ; B %d %d %d %d ;\n",
			  i,
			  scale(glyph_list[encoding[i]].width),
			  glyph_list[encoding[i]].name,
			  scale((short)ntohs(glyf_table->xMin)),
			  scale((short)ntohs(glyf_table->yMin)),
			  scale((short)ntohs(glyf_table->xMax)),
			  scale((short)ntohs(glyf_table->yMax)));
		}
	}

	fprintf(pfa_file, "readonly def\ncurrentdict end\ncurrentfile eexec\n");
	fprintf(pfa_file, "dup /Private 15 dict dup begin\n");

	fprintf(pfa_file, "/RD{string currentfile exch readstring pop}executeonly def\n");
	fprintf(pfa_file, "/ND{noaccess def}executeonly def\n");
	fprintf(pfa_file, "/NP{noaccess put}executeonly def\n");

	if(forcebold==0)
		fprintf(pfa_file, "/ForceBold false def\n");
	else if(forcebold==1)
		fprintf(pfa_file, "/ForceBold true def\n");

	fprintf(pfa_file, "/BlueValues [ ");
	for (i = 0; i < nblues; i++)
		fprintf(pfa_file, "%d ", bluevalues[i]);
	fprintf(pfa_file, "] def\n");

	fprintf(pfa_file, "/OtherBlues [ ");
	for (i = 0; i < notherb; i++)
		fprintf(pfa_file, "%d ", otherblues[i]);
	fprintf(pfa_file, "] def\n");

	if (stdhw != 0)
		fprintf(pfa_file, "/StdHW [ %d ] def\n", stdhw);
	if (stdvw != 0)
		fprintf(pfa_file, "/StdVW [ %d ] def\n", stdvw);
	fprintf(pfa_file, "/StemSnapH [ ");
	for (i = 0; i < 12 && stemsnaph[i] != 0; i++)
		fprintf(pfa_file, "%d ", stemsnaph[i]);
	fprintf(pfa_file, "] def\n");
	fprintf(pfa_file, "/StemSnapV [ ");
	for (i = 0; i < 12 && stemsnapv[i] != 0; i++)
		fprintf(pfa_file, "%d ", stemsnapv[i]);
	fprintf(pfa_file, "] def\n");

	fprintf(pfa_file, "/MinFeature {16 16} def\n");
	/* Are these fixed also ? */
	fprintf(pfa_file, "/password 5839 def\n");
	fprintf(pfa_file, "/Subrs 1 array\ndup 0 {\nreturn\n} NP\n ND\n");

	fprintf(pfa_file, "2 index /CharStrings %d dict dup begin\n", nchars);

	for (i = 0; i < numglyphs; i++) {
		if (glyph_list[i].flags & GF_USED) {
			print_glyf(i);
		}
	}


	fprintf(pfa_file, "end\nend\nreadonly put\n");
	fprintf(pfa_file, "noaccess put\n");
	fprintf(pfa_file, "dup/FontName get exch definefont pop\n");
	fprintf(pfa_file, "mark currentfile closefile\n");
	fclose(pfa_file);

    fprintf(afm_file, "EndCharMetrics\n");

    if (kern_table != NULL) {
        fprintf(afm_file, "StartKernData\n");
        handle_kern();
        fprintf(afm_file, "EndKernData\n");
    } else {
//        fputs("No Kerning data\n", stderr);
    }
    fprintf(afm_file, "EndFontMetrics\n");
    fclose(afm_file);

	FPF(stderr, "Finished - font files created\n");

	fclose(pfa_file);

	t1asm* assembler = new t1asm;

	sprintf(filename, "%s.%s", psFont, encode ? (pfbflag ? "pfb" : "pfa") : "t1a" );
	assembler->assemble_t1(pfbflag, unencoded_output, filename);
	WriteNameAttr(filename, psfontname);
	delete assembler;

	// clean up temporary files...
	unlink(unencoded_output);
	sprintf(filename, "%s.afm", psFont);
	unlink(filename);

	return 0;
}

