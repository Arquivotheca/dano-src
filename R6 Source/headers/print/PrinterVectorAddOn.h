// ***********************************************************************
// libprint.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINTER_VECTOR_ADDON_H_
#define _PRINTER_VECTOR_ADDON_H_

#include <interface/GraphicsDefs.h>
#include <print/PrinterAddOn.h>

namespace BPrivate
{
	struct _printer_vector_addon_data;	
}

class BPrinterVectorAddOn;
class BShape;

class BPrintPictureIterator
{
public:
		BPrintPictureIterator(BPrinterVectorAddOn& driver);
	virtual ~BPrintPictureIterator();
	status_t Iterate(const BPicture *picture);
	BPrinterVectorAddOn& Driver();

public:
	// Vector Hooks
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
	virtual status_t _Reserved_BPrintPictureIterator_0(int32 arg, ...);
	virtual status_t _Reserved_BPrintPictureIterator_1(int32 arg, ...);
	virtual status_t _Reserved_BPrintPictureIterator_2(int32 arg, ...);
	virtual status_t _Reserved_BPrintPictureIterator_3(int32 arg, ...);
	virtual status_t _Reserved_BPrintPictureIterator_4(int32 arg, ...);
	virtual status_t _Reserved_BPrintPictureIterator_5(int32 arg, ...);
	virtual status_t _Reserved_BPrintPictureIterator_6(int32 arg, ...);
	virtual status_t _Reserved_BPrintPictureIterator_7(int32 arg, ...);
	virtual status_t Perform(int32 arg, ...);

private:
	BPrintPictureIterator(const BPrintPictureIterator &);
	BPrintPictureIterator& operator = (const BPrintPictureIterator &);
	BPrinterVectorAddOn& fDriver;
	BPicture *fPicture;
	uint32 _reserved_BPrintPictureIterator_[4];
};


// ---------------------------------------------------------------


class BPrinterVectorAddOn : public BPrinterAddOn, public BPrintPictureIterator
{
public:
			BPrinterVectorAddOn(BTransportIO* transport, BNode *printer_file);
	virtual	~BPrinterVectorAddOn();

protected:
	// Note: folowing hook must be called by the derived class
	virtual	status_t BeginJob();
	virtual status_t EndJob();
	virtual	status_t BeginPage();
	virtual status_t EndPage();
	virtual status_t Cancel();

	// For implementation, you shouldn't need to override this method.
	virtual status_t	Print(const page_t& page, const int nbCopies = 1);

private:
	BPrinterVectorAddOn(const BPrinterVectorAddOn &);
	BPrinterVectorAddOn& operator = (const BPrinterVectorAddOn &);

	virtual status_t _Reserved_BPrinterVectorAddOn_0(int32 arg, ...);
	virtual status_t _Reserved_BPrinterVectorAddOn_1(int32 arg, ...);
	virtual status_t _Reserved_BPrinterVectorAddOn_2(int32 arg, ...);
	virtual status_t _Reserved_BPrinterVectorAddOn_3(int32 arg, ...);
	virtual status_t Perform(int32 arg, ...);

private:
	BPrivate::_printer_vector_addon_data *_fPrivate;
	BPrivate::_printer_vector_addon_data& _rPrivate;
	uint32 _reserved_BPrinterVectorAddOn_[4];
};

#endif
