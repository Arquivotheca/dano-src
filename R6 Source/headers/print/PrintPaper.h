// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINT_PAPER_H_
#define _PRINT_PAPER_H_

#include <SupportDefs.h>
#include <Point.h>
#include <Rect.h>


struct BPrintPaper
{
	// All values in millimeter
	int32 id;
	const char *name;
	float width;
	float height;
	float leftMargin;
	float topMargin;
	float rightMargin;
	float bottomMargin;
	uint32 attributes;
	uint32 _reserved[3];

	// Some paper ID are defined
	enum paper_id_t
	{
		B_UNKNOWN = -3,
		B_MINIMAL = -2,
		B_MAXIMAL = -1,
		B_LEGAL = 0,
		B_LETTER,
		B_EXECUTIVE,
		B_A0,
		B_A1,
		B_A2,
		B_A3,
		B_A4,
		B_A5,
		B_A6,
		B_B,
		B_B4,
		B_B5,	
		B_ENVELOPE_10,
		B_ENVELOPE_DL,
		B_ENVELOPE_C6,
		B_PAPER_FORMAT_COUNT,
		B_START_USERDEF_PAPER = 1000,

		// Deprecated.
		B_NB_PAPER_FORMATS = B_PAPER_FORMAT_COUNT
	};

	enum
	{
		B_PORTRAIT	= 0x00000000,
		B_LANDSCAPE = 0x00000001,
		B_H_MIRROR	= 0x00000002,
		B_V_MIRROR	= 0x00000004
	};
	
	BPrintPaper();

	// Helper functions
	status_t SetTo(int32 id, const char *name=NULL);
	status_t SetTo(int32 id, const char *name, float w, float h);
	status_t SetTo(int32 id, const char *name, float w, float h, float left, float top, float right, float bottom);
	status_t SetMargins(float left, float top, float right, float bottom);
	void SetPortrait();
	void SetLandscape();
	void SetVMirror(bool = true);
	void SetHMirror(bool = true);

	const char *Name() const;
	const char *PrettyName() const;
	bool Portrait() const;
	bool Landscape() const;
	bool VMirror() const;
	bool HMirror() const;

	float PaperWidth() const;
	float PaperHeight() const;
	float AspectRatio() const;
	BRect PrintableRect() const;
	float PrintableWidth() const;
	float PrintableHeight() const;
	
	BPoint ConvertToPrintable(const BPoint& paperPoint) const;
	BPoint ConvertToPaper(const BPoint& printablePoint) const;

	static float milimeter_to_inch		(const float mm);
	static float milimeter_to_pixel		(const float mm, const float resolutionDpi = 72.0f);
	static float inch_to_milimeter		(const float in);
	static float inch_to_pixel			(const float in, const float resolutionDpi = 72.0f);
	static float pixel_to_milimeter		(const float pixel, const float resolutionDpi = 72.0f);
	static float pixel_to_inch			(const float pixel, const float resolutionDpi = 72.0f);
	static paper_id_t DefaultFormat();
	static paper_id_t FindPaperID		(const BPrintPaper& paper);
};

#endif
