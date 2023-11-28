//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef _PREVIEW_DRIVER_H
#define _PREVIEW_DRIVER_H

#include <ListItem.h>
#include <Node.h>
#include <Directory.h>
#include <Shape.h>

#include <print/PrinterConfigAddOn.h>
#include <print/PrinterVectorAddOn.h>
#include <print/PrintPaper.h>


// /////////////////////////////////////////////////////////////////////////////////////

class BDebugDriver : public BPrinterVectorAddOn
{
public:
			BDebugDriver(BTransportIO* transport, BNode *printer_file);
	virtual ~BDebugDriver();
private:
	virtual	status_t BeginJob();
	virtual	status_t EndJob();
	virtual	status_t BeginPage();
	virtual	status_t EndPage();
	virtual	status_t Cancel();

	virtual status_t MovePen(const BPoint& p);
	virtual status_t StrokeLine(const BPoint& p0, const BPoint& p1);
	virtual status_t StrokeRect(const BRect& r);
	virtual status_t FillRect(const BRect& r);
	virtual status_t StrokeRoundRect(const BRect& r, const BPoint& p);
	virtual status_t FillRoundRect(const BRect& r, const BPoint& p);
	virtual status_t StrokeBezier(const BPoint* p);
	virtual status_t FillBezier(const BPoint* p);
	virtual status_t StrokeArc(const BPoint& rx, const BPoint& ry, float alpha, float beta);
	virtual status_t FillArc(const BPoint& rx, const BPoint& ry, float alpha, float beta);
	virtual status_t StrokeEllipse(const BPoint& rx, const BPoint& ry);
	virtual status_t FillEllipse(const BPoint& rx, const BPoint& ry);
	virtual status_t StrokePolygon(int32 count, const BPoint* p, bool closed);
	virtual status_t FillPolygon(int32 count, const BPoint* p);
	virtual status_t StrokeShape(const BShape& shape);
	virtual status_t FillShape(const BShape& shape);
	virtual status_t DrawString(const char *s, float e0, float e1);
	virtual status_t DrawPixels(const BRect& src, const BRect& dst, int32 w, int32 h, int32 rowByte, color_space format, int32 flags, const void *buffer);
	virtual status_t ClipToRects(int32 count, const BRect *r);
	virtual status_t ClipToPicture(BPicture& picture, const BPoint& p, uint32 inverse);
	virtual status_t PushState();
	virtual status_t PopState();
	virtual status_t EnterStateChange();
	virtual status_t ExitStateChange();
	virtual status_t EnterFontState();
	virtual status_t ExitFontState();
	virtual status_t SetOrigin(const BPoint& p);
	virtual status_t SetPenLocation(const BPoint& p);
	virtual status_t SetDrawOp(drawing_mode mode);
	virtual status_t SetLineMode(int32, int32, float);
	virtual status_t SetPenSize(float size);
	virtual status_t SetForeColor(const rgb_color& c);
	virtual status_t SetBackColor(const rgb_color& c);
	virtual status_t SetStipplePattern(const pattern& p);
	virtual status_t SetScale(float scale);
	virtual status_t SetFontFamily(const char *family);
	virtual status_t SetFontStyle(const char *style);
	virtual status_t SetFontSpacing(int32 spacing_mode);
	virtual status_t SetFontSize(float size);
	virtual status_t SetFontRotate(float alpha);
	virtual status_t SetFontEncoding(int32);
	virtual status_t SetFontFlags(int32 flags);
	virtual status_t SetFontShear(float shear);
	virtual status_t SetFontBPP(int32 bpp);
	virtual status_t SetFontFaces(int32 face);
	virtual status_t DrawPicture(const BPicture& picture, const BPoint& p);

	const char*	IndentForLevel(int32 level) const {
		static const char space[] =
		"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
		int32 off = sizeof(space) - level - 1;
		if (off < 0) off = 0;
		return &space[off];
	}

	BDataIO& Out() {
		return *(Transport()->DataIO()) << IndentForLevel(fLevel);
	}
	
	class DebugShapeIterator : public BShapeIterator
	{
	public:
		DebugShapeIterator(BDebugDriver& driver);
		virtual	status_t IterateMoveTo(BPoint *point);
		virtual	status_t IterateLineTo(int32 lineCount, BPoint *linePts);
		virtual	status_t IterateBezierTo(int32 bezierCount, BPoint *bezierPts);
		virtual	status_t IterateClose();
	private:
		BDebugDriver& Driver() { return fDriver; }
		BDebugDriver& fDriver;
	};
	
	friend class DebugShapeIterator;
	
	int fLevel;
};


class BDebugConfig : public BPrinterConfigAddOn
{
public:
			BDebugConfig(BTransportIO* transport, BNode *printer_file);
	virtual ~BDebugConfig();
private:
	virtual int32 PrinterModes(printer_mode_t const **modes);
	virtual status_t PrinterModeSelected(int32 index);
	virtual int32 PaperFormats(int32 tray, BPrintPaper const **papers);
	virtual status_t Save();

	printer_mode_t fPrinterModes[5];
	BPrintPaper fPaperFormats[BPrintPaper::B_NB_PAPER_FORMATS];
};

#endif
