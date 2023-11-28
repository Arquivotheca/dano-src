// ============================================================
//  STE.h	©1996 Hiroshi Lockheimer
// ============================================================
// 	STE Version 1.0a5

#ifndef STE_H
#define STE_H

#include <Font.h>

const ulong	STE_STYLE_TYPE = 'STEs';


enum {
	doFont			= B_FONT_FAMILY_AND_STYLE,	// set font
	doSize			= B_FONT_SIZE,				// set size
	doShear			= B_FONT_SHEAR,				// set shear
//	doUnderline		= 0x00000008,				// set underline
	doColor			= 0x00001000,				// set color
	doExtra			= 0x00002000,				// set the extra field
	doAll			= 0x00003FFF,				// set everything
	addSize			= 0x00010000				// add size value
};

struct STEStyle : public BFont
{
	rgb_color	color;
	long		extra;
	
	STEStyle();
	STEStyle(const STEStyle&);
	STEStyle(const BFont &inFont, rgb_color inColor, long inExtra);

	STEStyle&		operator=(const STEStyle &inStyle); 
	STEStyle&		operator=(const BFont &inFont); 

	bool operator==(const STEStyle &inStyle) const;
	bool operator!=(const STEStyle &inStyle) const;
	void PrintToStream() const;
};
typedef STEStyle* STEStylePtr;
typedef const STEStyle* ConstSTEStylePtr;



struct STEStyleRun {
	int32			offset;		// byte offset of first character of run
	STEStyle		style;		// style info

	STEStyleRun();
	STEStyleRun(const STEStyle& inStyle, int32 inOffset);
	STEStyleRun(const STEStyleRun&);
	STEStyleRun&		operator=(const STEStyleRun &inStyle); 
};

typedef STEStyleRun* STEStyleRunPtr;
typedef const STEStyleRun* ConstSTEStyleRunPtr;



typedef struct STEStyleRange {
	long			count;		// number of style runs
	STEStyleRun		runs[1];	// array of count number of runs
} STEStyleRange, *STEStyleRangePtr;

typedef const STEStyleRange* ConstSTEStyleRangePtr;


typedef struct STELine {
	long			offset;		// offset of first character of line
	float			origin;		// pixel position of top of line
	float			ascent;		// maximum ascent for line
} STELine, *STELinePtr;

typedef const STELine* ConstSTELinePtr;


struct STEStyleRecord {
	long			refs;		// reference count for this style
	float			ascent;		// ascent for this style
	float			descent;	// descent for this style
	STEStyle		style;		// style info

	STEStyleRecord();
	STEStyleRecord&		operator=(const STEStyleRecord &inREcord); 
};

typedef STEStyleRecord* STEStyleRecordPtr;
typedef const STEStyleRecord* ConstSTEStyleRecordPtr;



typedef struct STEStyleRunDesc {
	long			offset;		// byte offset of first character of run
	long			index;		// index of corresponding style record
} STEStyleRunDesc, *STEStyleRunDescPtr;

typedef const STEStyleRunDesc* ConstSTEStyleRunDescPtr;

#endif



