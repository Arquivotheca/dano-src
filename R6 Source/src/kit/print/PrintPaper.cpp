// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <stdio.h>
#include <strings.h>

#include <print/PrintPaper.h>

namespace BPrivate
{
	static void swap(float &f0, float &f1);
} using namespace BPrivate;

static void BPrivate::swap(float &f0, float &f1)
{
	const float t = f0;
	f0 = f1;
	f1 = t;
}

BPrintPaper::BPrintPaper()
	: 	id(B_UNKNOWN),
		name(NULL),
		width(0), height(0),
		leftMargin(0), topMargin(0), rightMargin(0), bottomMargin(0),
		attributes(B_PORTRAIT)
{
	memset(_reserved, 0, sizeof(_reserved));
}

status_t BPrintPaper::SetTo(int32 paper_id, const char *paper_name)
{
	float w,h;
	switch (paper_id)
	{
		case B_LEGAL:
			w = inch_to_milimeter(8.5);
			h = inch_to_milimeter(14.0);
			break;
		case B_LETTER:
			w = inch_to_milimeter(8.5);
			h = inch_to_milimeter(11.0);
			break;
		case B_EXECUTIVE:
			w = inch_to_milimeter(7.25);
			h = inch_to_milimeter(10.5);
			break;
		case B_A0:		w = 840.0f;		h = 1188.0f;		break;
		case B_A1:		w = 594.0f;		h = 840.0f;			break;
		case B_A2:		w = 420.0f;		h = 594.0f;			break;
		case B_A3:		w = 297.0f;		h = 420.0f;			break;
		case B_A4:		w = 210.0f;		h = 297.0f;			break;
		case B_A5:		w = 148.0f;		h = 210.0f;			break;
		case B_A6:		w = 105.0f;		h = 148.0f;			break;
		case B_B:		w = 279.0f;		h = 432.0f;			break;
		case B_B4:		w = 257.0f;		h = 364.0f;			break;
		case B_B5:		w = 182.0f;		h = 257.0f;			break;
		case B_ENVELOPE_10:
			w = inch_to_milimeter(9.5);
			h = inch_to_milimeter(4.125);
			break;
		case B_ENVELOPE_DL:		w = 220.0f;		h = 110.0f;		break;
		case B_ENVELOPE_C6:		w = 162.0f;		h = 114.0f;		break;
		default:
			return B_BAD_VALUE;
	};

	return SetTo(paper_id, paper_name, w, h);
}

status_t BPrintPaper::SetTo(int32 paper_id, const char *paper_name, float w, float h)
{
	id = paper_id;
	name = paper_name;
	width = w;
	height = h;
	return B_OK;
}

status_t BPrintPaper::SetTo(int32 paper_id, const char *paper_name, float w, float h, float left, float top, float right, float bottom)
{
	SetTo(paper_id, paper_name, w, h);
	return SetMargins(left, top, right, bottom);
}

status_t BPrintPaper::SetMargins(float left, float top, float right, float bottom)
{
	leftMargin = left;
	topMargin = top;
	rightMargin = right;
	bottomMargin = bottom;
	return B_OK;
}

void BPrintPaper::SetPortrait()
{
	attributes &= ~(B_PORTRAIT | B_LANDSCAPE);
	attributes |= B_PORTRAIT;
}

void BPrintPaper::SetLandscape()
{
	attributes &= ~(B_PORTRAIT | B_LANDSCAPE);
	attributes |= B_LANDSCAPE;
}

void BPrintPaper::SetVMirror(bool m)
{
	if (m == false)		attributes &= ~B_V_MIRROR;
	else				attributes |= B_V_MIRROR;
}

void BPrintPaper::SetHMirror(bool m)
{
	if (m == false)		attributes &= ~B_H_MIRROR;
	else				attributes |= B_H_MIRROR;
}

const char *BPrintPaper::Name() const
{
	const char *names[] = {"Legal", "Letter", "Executive", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "B", "B4", "B5", "Envelope 10", "Envelope DL", "Envelope C6"};
	if ((name == NULL) && (id >= B_LEGAL) && (id <= B_ENVELOPE_C6))
		return names[id];
	return name;
}

const char *BPrintPaper::PrettyName() const
{
	return Name();
}

bool BPrintPaper::Portrait() const
{
	return ((attributes & (B_PORTRAIT | B_LANDSCAPE)) == B_PORTRAIT);
}

bool BPrintPaper::Landscape() const
{
	return ((attributes & (B_PORTRAIT | B_LANDSCAPE)) == B_LANDSCAPE);
}

bool BPrintPaper::VMirror() const
{
	return ((attributes & B_V_MIRROR) == B_V_MIRROR);
}

bool BPrintPaper::HMirror() const
{
	return ((attributes & B_H_MIRROR) == B_H_MIRROR);
}

float BPrintPaper::PaperWidth() const
{
	if (Portrait())	return width;
	else			return height;
}

float BPrintPaper::PaperHeight() const
{
	if (Portrait())	return height;
	else 			return width;
}

float BPrintPaper::AspectRatio() const
{
	if ((width == 0.0f) || (height == 0.0f))
		return 0.0f;
	return PaperHeight()/PaperWidth();
}

BRect BPrintPaper::PrintableRect() const
{
	float l = leftMargin;
	float t = topMargin;
	float r = rightMargin;
	float b = bottomMargin;
	if (VMirror())	swap(l, r);
	if (HMirror())	swap(t, b);
	if (Portrait())	return BRect(l, t, width-r, height-b);
	else			return BRect(t, r, height-b, width-l);
}

float BPrintPaper::PrintableWidth() const
{
	return PrintableRect().Width();
}

float BPrintPaper::PrintableHeight() const
{
	return PrintableRect().Height();
}


BPoint BPrintPaper::ConvertToPrintable(const BPoint& paperPoint) const
{
	 return paperPoint - PrintableRect().LeftTop();
}

BPoint BPrintPaper::ConvertToPaper(const BPoint& printablePoint) const
{
	 return printablePoint + PrintableRect().LeftTop();
}


float BPrintPaper::milimeter_to_inch(const float mm)
{
	return mm / 25.4f;
}

float BPrintPaper::milimeter_to_pixel(const float mm, const float resolutionDpi = 72.0f)
{
	return milimeter_to_inch(mm)*resolutionDpi;
}

float BPrintPaper::inch_to_milimeter(const float in)
{
	return in * 25.4f;
}

float BPrintPaper::inch_to_pixel(const float in, const float resolutionDpi = 72.0f)
{
	return in*resolutionDpi;
}

float BPrintPaper::pixel_to_milimeter(const float pixel, const float resolutionDpi = 72.0f)
{
	return inch_to_milimeter(pixel/resolutionDpi);
}

float BPrintPaper::pixel_to_inch(const float pixel, const float resolutionDpi = 72.0f)
{
	return pixel/resolutionDpi;
}

BPrintPaper::paper_id_t BPrintPaper::DefaultFormat()
{
	return B_LETTER;	// TODO: We want localisation here
}

BPrintPaper::paper_id_t BPrintPaper::FindPaperID(const BPrintPaper& paper)
{
	const float tolerence = 2;	// 2mm tolerence
	for (int32 i=0 ; i<B_NB_PAPER_FORMATS ; i++)
	{
		BPrintPaper reference;
		reference.SetTo(i);
		
		if (paper.PaperWidth() <= reference.PaperWidth()-tolerence)
			continue;
		if (paper.PaperWidth() >= reference.PaperWidth()+tolerence)
			continue;
		if (paper.PaperHeight() <= reference.PaperHeight()-tolerence)
			continue;
		if (paper.PaperHeight() >= reference.PaperHeight()+tolerence)
			continue;
		
		return (paper_id_t)reference.id;
	}
	return (paper_id_t)paper.id;
}


