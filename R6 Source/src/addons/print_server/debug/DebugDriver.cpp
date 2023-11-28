//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include <BeBuild.h>
#include <OS.h>
#include <StreamIO.h>

#include "DebugDriver.h"

extern "C" BPrinterAddOn *instantiate_printer_addon(int32 index, BTransportIO* transport, BNode *printer_file)
{
	if (index != 0)	return NULL;
	return static_cast<BPrinterAddOn *>(new BDebugDriver(transport, printer_file));
}

extern "C" BPrinterConfigAddOn *instantiate_printer_config_addon(BTransportIO* transport, BNode *printer_file)
{
	return static_cast<BPrinterConfigAddOn *>(new BDebugConfig(transport, printer_file));
}


// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BDebugDriver::BDebugDriver(BTransportIO* transport, BNode *printer_file)
	:	BPrinterVectorAddOn(transport, printer_file)
{
}

BDebugDriver::~BDebugDriver()
{
}

status_t BDebugDriver::BeginJob()
{
	fLevel = 0;
	Out() << "Debug printer driver. Be Incorporated (c) 2001." << endl;
	Out() << "-----------------------------------------------" << endl << endl;
	Out() << Settings().Message() << endl << endl;
	Out() << "BeginJob" << endl;
	fLevel++;
	return BPrinterVectorAddOn::BeginJob();
}

status_t BDebugDriver::EndJob()
{
	fLevel--;
	Out() << "EndJob" << endl;
	return BPrinterVectorAddOn::EndJob();
}

status_t BDebugDriver::BeginPage()
{
	Out() << "BeginPage" << endl;
	fLevel++;
	return BPrinterVectorAddOn::BeginPage();
}

status_t BDebugDriver::EndPage()
{
	fLevel--;
	Out() << "EndPage" << endl;
	return BPrinterVectorAddOn::EndPage();
}

status_t BDebugDriver::Cancel()
{
	fLevel = 0;
	Out() << "*** Cancel" << endl;
	return B_OK;
}

// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

status_t BDebugDriver::MovePen(const BPoint& p) {
	Out() << "MovePen(" << p << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::StrokeLine(const BPoint& p0, const BPoint& p1) {
	Out() << "StrokeLine(" << p0 << ", " << p1 << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::StrokeRect(const BRect& r) {
	Out() << "StrokeRect(" << r << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::FillRect(const BRect& r) {
	Out() << "FillRect(" << r << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::StrokeRoundRect(const BRect& r, const BPoint& p) {
	Out() << "StrokeLine(" << r << ", " << p << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::FillRoundRect(const BRect& r, const BPoint& p) {
	Out() << "FillRoundRect(" << r << ", " << p << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::StrokeBezier(const BPoint* p) {
	Out()	<< "StrokeBezier("
			<< p[0] << ", "
			<< p[1] << ", "
			<< p[2] << ", "
			<< p[3] << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::FillBezier(const BPoint* p) {
	Out()	<< "FillBezier("
			<< p[0] << ", "
			<< p[1] << ", "
			<< p[2] << ", "
			<< p[3] << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::StrokeArc(const BPoint& rx, const BPoint& ry, float alpha, float beta) {
	Out()	<< "StrokeArc("
			<< rx << ", "
			<< ry << ", "
			<< alpha << ", "
			<< beta << ", "
			<< ")" << endl;
	return B_OK;
}
status_t BDebugDriver::FillArc(const BPoint& rx, const BPoint& ry, float alpha, float beta) {
	Out()	<< "FillArc("
			<< rx << ", "
			<< ry << ", "
			<< alpha << ", "
			<< beta << ", "
			<< ")" << endl;
	return B_OK;
}
status_t BDebugDriver::StrokeEllipse(const BPoint& rx, const BPoint& ry) {
	Out() << "StrokeEllipse(" << rx << ", " << ry << ", " << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::FillEllipse(const BPoint& rx, const BPoint& ry) {
	Out() << "FillEllipse(" << rx << ", " << ry << ", " << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::StrokePolygon(int32 count, const BPoint* p, bool closed) {
	if (count) {
		Out() << "StrokePolygon(" << count << ", closed = " << closed << ", " << endl;
		for (int i=0 ; i<count-1 ; i++)
			Out() << "\t" << p[i] << ", " << endl;
		Out() << "\t" << p[count-1] << ")" << endl;
	} else {
		Out() << "StrokePolygon(" << count << ", closed = " << closed << ")" << endl;
	}
	return B_OK;
}
status_t BDebugDriver::FillPolygon(int32 count, const BPoint* p) {
	if (count) {
		Out() << "FillPolygon(" << count << ", " << endl;
		for (int i=0 ; i<count-1 ; i++)
			Out() << "\t" << p[i] << ", " << endl;
		Out() << "\t" << p[count-1] << ")" << endl;
	} else {
		Out() << "FillPolygon(" << count << ")" << endl;
	}
	return B_OK;
}
status_t BDebugDriver::StrokeShape(const BShape& shape) {
	Out() << "StrokeShape()" << endl;
	Out() << "{" << endl;
	DebugShapeIterator shapeIterator(*this);
	fLevel++;
	status_t result = shapeIterator.Iterate(&shape);
	fLevel--;
	Out() << "}" << endl;
	return result;
}
status_t BDebugDriver::FillShape(const BShape& shape) {
	Out() << "FillShape()" << endl;
	Out() << "{" << endl;
	DebugShapeIterator shapeIterator(*this);
	fLevel++;
	status_t result = shapeIterator.Iterate(&shape);
	fLevel--;
	Out() << "}" << endl;
	return result;
}
status_t BDebugDriver::DrawString(const char *str, float e0, float e1) {
	Out() << "DrawString(\"" << str <<"\", " << e0 << ", " << e1 << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::DrawPixels(const BRect& src, const BRect& dst, int32 w, int32 h, int32 rowByte, color_space format, int32 flags, const void *buffer) {
	Out()
		<< "DrawPixels("
		<< src << ", "
		<< dst << ", "
		<< w << ", "
		<< h << ", "
		<< rowByte << ", "
		<< format << ", "
		<< flags << ", "
		<< buffer
		<< ")" << endl;
	return B_OK;
}
status_t BDebugDriver::ClipToRects(int32 count, const BRect *r) {
	if (count) {
		Out() << "ClipToRects(" << count << ", " << endl;
		for (int i=0 ; i<count-1 ; i++)
			Out() << "\t" << r[i] << ", " << endl;
		Out() << "\t" << r[count-1] << ")" << endl;
	} else {
		Out() << "ClipToRects(" << count << ")" << endl;
	}
}
status_t BDebugDriver::ClipToPicture(BPicture& picture, const BPoint& p, uint32 inverse) {
	Out() << "ClipToPicture(" << p << ", inverse = " << inverse << ")" << endl;
	fLevel++;
	status_t result = Iterate(&picture);
	fLevel--;
	return result;	
}
status_t BDebugDriver::PushState() {
	Out() << "PushState" << endl;
	fLevel++;
	return B_OK;
}
status_t BDebugDriver::PopState() {
	fLevel--;
	Out() << "PopState" << endl;
	return B_OK;
}
status_t BDebugDriver::EnterStateChange() {
	Out() << "EnterStateChange" << endl;
	fLevel++;
	return B_OK;
}
status_t BDebugDriver::ExitStateChange() {
	fLevel--;
	Out() << "ExitStateChange" << endl;
	return B_OK;
}
status_t BDebugDriver::EnterFontState() {
	Out() << "EnterFontState" << endl;
	fLevel++;
	return B_OK;
}
status_t BDebugDriver::ExitFontState() {
	fLevel--;
	Out() << "ExitFontState" << endl;
	return B_OK;
}
status_t BDebugDriver::SetOrigin(const BPoint& p) {
	Out() << "SetOrigin(" << p << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetPenLocation(const BPoint& p) {
	Out() << "SetPenLocation(" << p << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetDrawOp(drawing_mode mode) {
	Out() << "SetDrawOp(" << mode << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetLineMode(int32 a, int32 b, float f) {
	Out() << "SetLineMode(" << a <<", " << b << ", " << f << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetPenSize(float size) {
	Out() << "SetPenSize(" << size << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetForeColor(const rgb_color& c) {
	Out() << "SetForeColor(" << c << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetBackColor(const rgb_color& c) {
	Out() << "SetBackColor(" << c << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetStipplePattern(const pattern& p) {
	Out() << "SetStipplePattern(" << BHexDump(p.data, 8, 8) << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetScale(float scale) {
	Out() << "SetScale(" << scale << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetFontFamily(const char *family) {
	Out() << "SetFontFamily(\"" << family << "\")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetFontStyle(const char *style) {
	Out() << "SetFontStyle(\"" << style << "\")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetFontSpacing(int32 spacing_mode) {
	Out() << "SetFontSpacing(" << spacing_mode << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetFontSize(float size) {
	Out() << "SetFontSize(" << size << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetFontRotate(float alpha) {
	Out() << "SetFontRotate(" << alpha << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetFontEncoding(int32 e) {
	Out() << "SetFontEncoding(" << e << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetFontFlags(int32 flags) {
	Out() << "SetFontFlags(" << flags << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetFontShear(float shear) {
	Out() << "SetFontShear(" << shear << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetFontBPP(int32 bpp) {
	Out() << "SetFontBPP(" << bpp << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::SetFontFaces(int32 face) {
	Out() << "SetFontFaces(" << face << ")" << endl;
	return B_OK;
}
status_t BDebugDriver::DrawPicture(const BPicture& picture, const BPoint& p) {
	Out() << "DrawPicture(" << p << ")" << endl;
	fLevel++;
	status_t result = Iterate(&picture);
	fLevel--;
	return result;	
}

// ----------------------------------------------------------------

BDebugDriver::DebugShapeIterator::DebugShapeIterator(BDebugDriver& driver)
	: fDriver(driver)
{
}

status_t BDebugDriver::DebugShapeIterator::IterateMoveTo(BPoint *point)
{
	Driver().Out() << "BShape::MoveTo(" << *point << ")" << endl;
	return B_OK;
}

status_t BDebugDriver::DebugShapeIterator::IterateLineTo(int32 count, BPoint *linePts)
{
	if (count) {
		Driver().Out() << "BShape::LineTo(" << count << ", " << endl;
		for (int i=0 ; i<count-1 ; i++)
			Driver().Out() << "\t" << linePts[i] << ", " << endl;
		Driver().Out() << "\t" << linePts[count-1] << ")" << endl;
	} else {
		Driver().Out() << "BShape::LineTo(" << count << ")" << endl;
	}
	return B_OK;
}

status_t BDebugDriver::DebugShapeIterator::IterateBezierTo(int32 count, BPoint *bezierPts)
{
	if (count) {
		Driver().Out() << "BShape::BezierTo(" << count << ", " << endl;
		for (int i=0 ; i<count-1 ; i++) {
			Driver().Out() << "\t" << bezierPts[i*3+0] << ", " << bezierPts[i*3+1] << ", " << bezierPts[i*3+2] << ", " << endl;
		}
		Driver().Out() << "\t" << bezierPts[(count-1)*3+0] << ", " << bezierPts[(count-1)*3+1] << ", " << bezierPts[(count-1)*3+2] << ")" << endl;
	} else {
		Driver().Out() << "BShape::BezierTo(" << count << ")" << endl;
	}
	return B_OK;
}

status_t BDebugDriver::DebugShapeIterator::IterateClose()
{
	Driver().Out() << "BShape::Close()" << endl;
	return B_OK;
}

// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BDebugConfig::BDebugConfig(BTransportIO* transport, BNode *printer_file)
	:	BPrinterConfigAddOn(transport, printer_file)
{
	// Just to set some default values
	Settings().SetSettings(BMessage());	

	for (int i=0 ; i<BPrintPaper::B_NB_PAPER_FORMATS ; i++)
	{
		fPaperFormats[i].SetTo(i);
		fPaperFormats[i].SetMargins(3,3,3,5);
		if (i == BPrintPaper::DefaultFormat())
			PaperSelected(0, 1);
	}

	PrinterModeSelected(1);
}

BDebugConfig::~BDebugConfig()
{
}
		
int32 BDebugConfig::PrinterModes(printer_mode_t const **modes)
{
	static const char *paper = "Plain paper";	
	fPrinterModes[0].paper = paper;
	fPrinterModes[0].quality = "72 DPI";
	fPrinterModes[0].attributes = printer_mode_t::B_COLOR_ONLY;
	fPrinterModes[1].paper = paper;
	fPrinterModes[1].quality = "300 DPI";
	fPrinterModes[1].attributes = printer_mode_t::B_COLOR_ONLY | printer_mode_t::B_IS_DEFAULT_PAPER | printer_mode_t::B_IS_DEFAULT_QUALITY;
	fPrinterModes[2].paper = paper;
	fPrinterModes[2].quality = "360 DPI";
	fPrinterModes[2].attributes = printer_mode_t::B_COLOR_ONLY;
	fPrinterModes[3].paper = paper;
	fPrinterModes[3].quality = "600 DPI";
	fPrinterModes[3].attributes = printer_mode_t::B_COLOR_ONLY;
	fPrinterModes[4].paper = paper;
	fPrinterModes[4].quality = "720 DPI";
	fPrinterModes[4].attributes = printer_mode_t::B_COLOR_ONLY;
 	*modes = fPrinterModes;
	return sizeof(fPrinterModes)/sizeof(fPrinterModes[0]);
}

status_t BDebugConfig::PrinterModeSelected(int32 index)
{
	static int32 resolution[] = {72, 300, 360, 600, 720};
	const int c = sizeof(fPrinterModes)/sizeof(fPrinterModes[0]);
	if ((index < 0) || (index >= c))
		return B_BAD_VALUE;
	Settings().SetDeviceXdpi(resolution[index]);
	Settings().SetDeviceYdpi(resolution[index]);
	return B_OK;
}

int32 BDebugConfig::PaperFormats(int32 tray, BPrintPaper const **papers)
{
	if (tray != 0)
		return 0;
	*papers = fPaperFormats;
	return sizeof(fPaperFormats)/sizeof(fPaperFormats[0]);
}

status_t BDebugConfig::Save()
{
	Settings().SetNbCopies(1);
	return B_OK;
}

