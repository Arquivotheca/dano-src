#ifndef _TTF2PT1_H_
#define _TTF2PT1_H_

#include "ttf.h"

/* glyph entry, one drawing command */
typedef struct gentry {
	struct gentry  *next;	/* double linked list */
	struct gentry  *prev;
	struct gentry  *first;	/* first entry in path */
	int             x1, y1, x2, y2, x3, y3;	/* absolute values, NOT
						 * deltas */
	char            type;
#define GE_HSBW	'B'
#define GE_MOVE 'M'
#define GE_LINE 'L'
#define GE_CURVE 'C'
#define GE_PATH 'P'
}               GENTRY;

/* stem structure, describes one [hv]stem  */
/* acually, it describes one border of a stem */
/* the whole stem is a pair of these structures */

typedef struct stem {
	short           value;	/* value of X or Y coordinate */
	short           origin;	/* point of origin for curve stems */
	short           from, to;	/* values of other coordinate between
					 * which this stem is valid */
	short           flags;

	/* ordering of ST_END, ST_FLAT, ST_ZONE is IMPORTANT for sorting */
#define ST_END		0x01	/* end of line, lowest priority */
#define ST_FLAT		0x02	/* stem is defined by a flat line, not a
				 * curve */
#define ST_ZONE		0x04	/* pseudo-stem, the limit of a blue zone */
#define ST_UP		0x08	/* the black area is to up or right from
				 * value */
#define ST_INVALID	0x10	/* this was a wrong guess about a stem */
#define ST_3		0x20	/* first stem of [hv]stem3 */
#define ST_BLUE		0x40	/* stem is in blue zone */
#define ST_TOPZONE	0x80	/* 1 - top zone, 0 - bottom zone */
}               STEM;

#define MAX_STEMS	1500	/* we can't have more stems than path
				 * elements */

typedef struct contour {
	short           ymin, xofmin;
	short           inside;	/* inside which contour */
	char            direction;
#define DIR_OUTER 1
#define DIR_INNER 0
}               CONTOUR;

typedef struct glyph {
	int             char_no;/* Encoding of glyph */
	int             unicode;/* Unicode value of glyph */
	char           *name;	/* Postscript name of glyph */
	int             xMin, yMin, xMax, yMax;	/* values from TTF dictionary */
	int             lsb;
	short int       width;
	short           flags;
#define GF_USED	0x0001		/* whether is this glyph used in T1 font */

	GENTRY         *entries;/* doube linked list of entries */
	GENTRY         *lastentry;	/* the last inserted entry */
	GENTRY         *path;	/* beggining of the last path */
	int             scaledwidth;

	STEM           *hstems;
	STEM           *vstems;
	int             nhs, nvs;	/* numbers of stems */

	CONTOUR        *contours;	/* it is not used now */
	int             ncontours;

	int             rymin, rymax;	/* real values */
	/* do we have flat surfaces on top/bottom */
	char            flatymin, flatymax;

}               GLYPH;

/*
 * Decription of the supported conversions from Unicode
 *
 * SB
 * Yes, I know that the compiled-in conversion is stupid but
 * it is simple to implement and allows to not worry about the
 * filesystem context. After all, the source is always available
 * and adding another language to it is easy.
 *
 * The language name is supposed to be the same as the subdirectory name 
 * in the `encodings' directory (for possible future extensions). 
 * The primary use of aliases is for guessing based on the current 
 * locale.
 */

#define MAXUNIALIAS 10

struct uni_language {
	int	(*conv)(int unival); /* the conversion function */
	char *name; /* the language name */
	char *descr; /* description */
	char *alias[MAXUNIALIAS]; /* aliases of the language name */
};



class ttf2pt1
{
 public:
					ttf2pt1();
					~ttf2pt1();
	int32 			convert_tt(const char *ttFont, const char *psFont);
					
//	int 			unicode_russian(int unival);
//	int 			unicode_latin1(int unival);
//	int 			unicode_latin4(int unival);
//	int 			unicode_latin5(int unival);
	int 			unicode_to_win31(int unival);

	static bool		newishFlag;

 private:


	int				sign(int);
	int				scale(int);
	GENTRY*			newgentry();
	void			rmoveto(int,int);
	void 			rlineto(int,int);
	void			rrcurveto(int,int,int,int,int,int);
	void			closepath();
	void			assertpath(GENTRY*,int,char*);
	void			g_rmoveto(GLYPH*,int,int);
	void			g_rlineto(GLYPH*,int,int);
	void			g_rrcurveto(GLYPH*, int,int,int,int,int,int);
	void			g_closepath(GLYPH*);
	void			fixcvends(GENTRY*);
	void			flattencurves(GLYPH*);
	void			fixcvdir(GENTRY*,int);
	int				getcvdir(GENTRY *ge);
	void 			testfixcvdir(GLYPH *g);
	int 			checkcv(GENTRY *ge, int dx, int dy);
	void 			closepaths(GLYPH *g);
	void 			smoothjoints(GLYPH *g);
	void 			debugstems(char *name, STEM *hstems, int nhs, STEM *vstems, int nvs);
	int 			addbluestems(STEM *s, int n);
	void 			sortstems(STEM *s, int n);
	int 			stemoverlap(STEM *s1, STEM *s2);
	int 			steminblue(STEM *s);
	int 			joinstems(STEM *s, int nold, int useblues);
	void 			buildstems(GLYPH *g);
	void 			straighten(GLYPH *g, int zigonly);
	double 			curvelen(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3);
	void 			splitzigzags(GLYPH *g);
	void 			forceconcise(GLYPH *g);
	void 			alignwidths(void);
	int 			besthyst(short *hyst, int base, int *best, int nbest, int width, int *bestindp);
	int 			bestblue(short *zhyst, short *physt, short *ozhyst, int *bluetab);
	void 			findblues(void);
	void 			stemstatistics(void);
	void 			reversepathsfromto(GENTRY *from, GENTRY *to);
	void 			reversepaths(GLYPH *g);
	void 			WriteNameAttr(const char *path, const char *name);
	void 			handle_name(const char *pfa_path);
	void 			handle_cmap(void);
	void 			handle_head(void);
	void 			draw_glyf(int glyphno, int parent, short *xoff, short *yoff, double *matrix);
	double 			f2dot14(short x);
	void 			print_glyf(int glyphno);
	void 			convert_glyf(int glyphno);
	int 			handle_hmtx(void);
	void 			handle_post(void);
	void 			handle_kern(void);


	int		(*uni_lang_converter)(int unival);

	int      stdhw, stdvw;	/* dominant stems widths */
	int      stemsnaph[12], stemsnapv[12];	/* most typical stem width */

 int      bluevalues[14];
 int      nblues;
 int      otherblues[10];
 int      notherb;
 int      bbox[4];	/* the FontBBox array */
 double   italic_angle;

 GLYPH   *glyph_list;
 short    encoding[256];	/* inverse of glyph[].char_no */

 int      optimize;	/* enables space optimization */
 int      smooth;	/* enable smoothing of outlines */
 int      transform;	/* enables transformation to 1000x1000 matrix */
 int      hints;	/* enables autogeneration of hints */
 int      absolute;	/* print out in absolute values */
 int      debug;	/* debugging flag */
 int      trybold;	/* try to guess whether the font is bold */
 int      reverse;	/* reverse font to Type1 path directions */
 int      encode;	/* encode the resulting file */
 int      pfbflag;	/* produce compressed file */
 int      wantafm;	/* want to see .afm instead of .t1a on stdout */

 FILE    *pfa_file, *afm_file;
 TTF_DIRECTORY *directory;
 TTF_DIR_ENTRY *dir_entry;
 char    *filebuffer;
 TTF_NAME *name_table;
 TTF_NAME_REC *name_record;
 TTF_HEAD *head_table;
 TTF_HHEA *hhea_table;
 TTF_KERN *kern_table;
 TTF_CMAP *cmap_table;
 LONGHORMETRIC *hmtx_table;
 TTF_GLYF *glyf_table;
 BYTE    *glyf_start;
 TTF_MAXP *maxp_table;
 TTF_POST_HEAD *post_table;
 USHORT  *short_loca_table;
 ULONG   *long_loca_table;
 int      ttf_file, numglyphs, long_offsets, ncurves;

 char psfontname[256];

 short    cmap_n_segs;
 USHORT  *cmap_seg_start, *cmap_seg_end;
 short   *cmap_idDelta, *cmap_idRangeOffset;
 int      ps_fmt_3, unicode;
 double   scale_factor;

 char    *Unknown_glyph;

 char     name_buffer[2000];
 char    *name_fields[8];


};

#endif
