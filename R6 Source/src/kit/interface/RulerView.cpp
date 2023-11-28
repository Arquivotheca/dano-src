//******************************************************************************
//
//	File:		RulerView.cpp
//
//	Description:	scrollable container with rulers view class implementation
//
//	Written By:	Robert Polic
//
//	Copyright 2001, Be Incorporated
//
//******************************************************************************

#include <Debug.h>

#include <RulerView.h>

#include <ClassInfo.h>
#include <MenuItem.h>
#include <Message.h>
#include <Window.h>
#include <scroll_view_misc.h>

#define kRULER_WIDTH						16
#define kRULER_HEIGHT						16

#define kINCHES								"Inches"
#define kCENTIMETERS						"Centimeters"
#define kMILLIMETERS						"Millimeters"
#define kPICAS								"Picas"
#define kPOINTS								"Points"

#define kDEFAULT_FONT						be_plain_font
#define kDEFAULT_SIZE						9
#define kDEFAULT_FILL_COLOR					0xffffffff
#define kDEFAULT_TEXT_COLOR					0x000000ff
#define kDEFAULT_CORNER_FRAME_COLOR			0xc0c0c0ff

struct ruler_defintion
{
	ruler_units		units;			// enum (from header)
	char*			label;			// menu label
	float			conversion;		// from inches
	int32			divisions;		// default divisions
};

static ruler_defintion	ruler[] = {{B_INCHES,		kINCHES,		 1.00,  8},
								   {B_CENTIMETERS,	kCENTIMETERS,	 2.54, 10},
								   {B_MILLIMETERS,	kMILLIMETERS,	25.40, 10},
								   {B_PICAS,		kPICAS,			 6.00, 10},
								   {B_POINTS,		kPOINTS,		72.00,  1}};


/* ---------------------------------------------------------------- */

static rgb_color _long_to_color_(uint32 color)
{
	rgb_color	rgb;

	rgb.red = (color >> 24);
	rgb.green = (color >> 16);
	rgb.blue = (color >> 8);
	rgb.alpha = color;

	return rgb;
}


/* ---------------------------------------------------------------- */

static ulong _color_to_long_(rgb_color rgb)
{
	return (rgb.red << 24) | (rgb.green << 16) | (rgb.blue << 8) | rgb.alpha;
}


/* ================================================================ */

BRulerView::BRulerView(const char* name, BView* a_view, uint32 resizeMask,
					   uint32 flags, bool h_scroll, bool h_ruler,
					   bool v_scroll, bool v_ruler, bool track, ruler_units units,
					   int32 divisions, float scale, border_style border)
  :	BScrollView(name, a_view, resizeMask, flags, h_scroll, v_scroll, border),
	fHorizontalRuler(h_ruler),
	fVerticalRuler(v_ruler),
	fTrackMouse(track),
	fScale(scale),
	fDivisions(divisions),
	fUnits(units)
{
	Initialize();
}


/* ---------------------------------------------------------------- */

BRulerView::BRulerView(BMessage* data)
	: BScrollView(data)
{
	Initialize();

	(data->HasBool("h_ruler")) ?
		fHorizontalRuler = data->FindBool("h_ruler") :
		fHorizontalRuler = false;
	(data->HasBool("v_ruler")) ?
		fVerticalRuler = data->FindBool("v_ruler") :
		fVerticalRuler = false;
	(data->HasBool("tracking")) ?
		fTrackMouse = data->FindBool("tracking") :
		fTrackMouse = false;
	if (data->HasInt32("horizontal_ruler_offset"))
		fHorizontalRulerOffset = data->HasInt32("horizontal_ruler_offset");
	if (data->HasInt32("vertical_ruler_offset"))
		fVerticalRulerOffset = data->HasInt32("vertical_ruler_offset");
	if (data->HasFloat("horizontal_origin_offset"))
		fHorizontalOriginOffset = data->FindFloat("horizontal_origin_offset");
	if (data->HasFloat("vertical_origin_offset"))
		fVerticalOriginOffset = data->FindFloat("vertical_origin_offset");
	(data->HasFloat("scale")) ?
		fScale = data->HasFloat("scale") :
		fScale = 100.0;
	(data->HasInt32("divisions")) ?
		fDivisions = data->FindInt32("divisions") :
		fDivisions = 8;
	(data->HasInt32("ruler_units")) ?
		fUnits = (ruler_units)(data->HasInt32("ruler_units")) :
		fUnits = B_INCHES;
	if (data->HasString("font_family"))
	{
		const char* family = NULL;
		const char* style = NULL;

		data->FindString("font_family", &family);
		data->FindString("font_style", &style);
		fFont.SetFamilyAndStyle(family, style);
	}
	if (data->HasFloat("font_size"))
		fFont.SetSize(data->FindFloat("font_size"));
	if (data->HasInt32("fill_color"))
		fFillColor = _long_to_color_(data->FindInt32("fill_color"));
	if (data->HasInt32("text_color"))
		fTextColor = _long_to_color_(data->FindInt32("text_color"));
	if (data->HasInt32("corner_frame_color"))
		fCornerFrameColor = _long_to_color_(data->FindInt32("corner_frame_color"));
}


/* ---------------------------------------------------------------- */

BRulerView::~BRulerView()
{
	delete fMenu;
}


/* ---------------------------------------------------------------- */

status_t BRulerView::Archive(BMessage* data, bool deep) const
{
	font_family	family;
	font_style	style;

	BScrollView::Archive(data, deep);

	data->AddBool("h_ruler", fHorizontalRuler);
	data->AddBool("v_ruler", fVerticalRuler);
	data->AddBool("tracking", fTrackMouse);
	data->AddInt32("horizontal_ruler_offset", fHorizontalRulerOffset);
	data->AddInt32("vertical_ruler_offset", fVerticalRulerOffset);
	data->AddFloat("horizontal_origin_offset", fHorizontalOriginOffset);
	data->AddFloat("vertical_origin_offset", fVerticalOriginOffset);
	data->AddFloat("scale", fScale);
	data->AddInt32("divisions", fDivisions);
	data->AddInt32("ruler_units", (int32)fUnits);

	fFont.GetFamilyAndStyle(&family, &style);
	data->AddString("font_family", family);
	data->AddString("font_style", style);
	data->AddFloat("font_size", fFont.Size());

	data->AddInt32("fill_color", _color_to_long_(fFillColor));
	data->AddInt32("text_color", _color_to_long_(fTextColor));
	data->AddInt32("corner_frame_color", _color_to_long_(fCornerFrameColor));

	return B_NO_ERROR;
}


/* ---------------------------------------------------------------- */

BArchivable* BRulerView::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, "BRulerView"))
		return NULL;
	return new BRulerView(data);
}


/* ---------------------------------------------------------------- */

void BRulerView::AllAttached()
{
	if (fHorizontalRuler)
		ShowRuler(B_HORIZONTAL);

	if (fVerticalRuler)
		ShowRuler(B_VERTICAL);

	BScrollView::AllAttached();
}


/* ---------------------------------------------------------------- */

void BRulerView::AllDetached()
{
	if (fHorizontalRuler)
		HideRuler(B_HORIZONTAL);

	if (fVerticalRuler)
		HideRuler(B_VERTICAL);

	BScrollView::AllDetached();
}


/* ---------------------------------------------------------------- */

void BRulerView::DrawCorner(BDeadCorner* corner, BRect rect)
{
	corner->DrawCorner(rect);
}


/* ---------------------------------------------------------------- */

void BRulerView::DrawRuler(BRuler* ruler, BRect rect)
{
	ruler->DrawRuler(rect);
}


/* ---------------------------------------------------------------- */

void BRulerView::MouseMoved(BPoint pt, uint32 code, const BMessage* msg)
{
	if (fTrackMouse)
	{
		if ((Target()) && (Target()->Frame().Contains(pt)))
		{
			BPoint	p = Target()->ConvertFromParent(pt);

			if (fHorizontalRulerView)
				fHorizontalRulerView->SetMouseMark(p.x);
			if (fVerticalRulerView)
				fVerticalRulerView->SetMouseMark(p.y);
		}
		else
		{
			if (fHorizontalRulerView)
				fHorizontalRulerView->SetMouseMark(-1.0);
			if (fVerticalRulerView)
				fVerticalRulerView->SetMouseMark(-1.0);
		}
	}
	BView::MouseMoved(pt, code, msg);
}


/* ---------------------------------------------------------------- */

void BRulerView::RulerMenu(BPoint p, uint32 buttons)
{
	if (buttons == B_SECONDARY_MOUSE_BUTTON)
	{
		uint32		index = 0;
		BMenuItem*	item;

		// mark the current item
		while (ruler[index].units != fUnits)
			index++;
		item = fMenu->ItemAt(index);
		item->SetMarked(true);

		item = fMenu->Go(p);
		if (item)
		{
			const char* label;

			label = item->Label();
			for (index = 0; index < sizeof(ruler) / sizeof(ruler_defintion); index++)
			{
				if (strcmp(label, ruler[index].label) == 0)
				{
					SetRulerUnits(ruler[index].units);
					break;
				}
			}
		}
	}
}


/* ---------------------------------------------------------------- */

BDeadCorner* BRulerView::Corner()
{
	return fCornerView;
}


/* ---------------------------------------------------------------- */

BRuler* BRulerView::Ruler(orientation ruler)
{
	if (ruler == B_HORIZONTAL)
		return fHorizontalRulerView;
	else if (ruler == B_VERTICAL)
		return fVerticalRulerView;
	return NULL;
}


/* ---------------------------------------------------------------- */

void BRulerView::HideRuler(orientation ruler)
{
	TScrollBar*	scrollbar;

	if (Target())
	{
		if ((ruler == B_HORIZONTAL) && (fHorizontalRulerView))
		{
			fHorizontalRuler = false;

			RemoveChild(fHorizontalRulerView);
			delete fHorizontalRulerView;
			fHorizontalRulerView = NULL;

			if ((scrollbar = dynamic_cast<TScrollBar*>(ScrollBar(B_HORIZONTAL))) != NULL)
				scrollbar->SetRulerView(NULL);

			Target()->MoveBy(0, -(kRULER_HEIGHT + 1));
			Target()->ResizeBy(0, kRULER_HEIGHT + 1);

			if ((scrollbar = dynamic_cast<TScrollBar*>(ScrollBar(B_VERTICAL))) != NULL)
			{
				scrollbar->MoveBy(0, -(kRULER_HEIGHT + 1));
				scrollbar->ResizeBy(0, kRULER_HEIGHT + 1);
			}

			if (fVerticalRulerView)
			{
				fVerticalRulerView->MoveBy(0, -(kRULER_HEIGHT + 1));
				fVerticalRulerView->ResizeBy(0, kRULER_HEIGHT + 1);
			}
		}
		else if ((ruler == B_VERTICAL) && (fVerticalRulerView))
		{
			fVerticalRuler = false;

			RemoveChild(fVerticalRulerView);
			delete fVerticalRulerView;
			fVerticalRulerView = NULL;

			if ((scrollbar = dynamic_cast<TScrollBar*>(ScrollBar(B_VERTICAL))) != NULL)
				scrollbar->SetRulerView(NULL);

			Target()->MoveBy(-(kRULER_WIDTH + 1), 0);
			Target()->ResizeBy(kRULER_WIDTH + 1, 0);

			if ((scrollbar = dynamic_cast<TScrollBar*>(ScrollBar(B_HORIZONTAL))) != NULL)
			{
				scrollbar->MoveBy(-(kRULER_WIDTH + 1), 0);
				scrollbar->ResizeBy(kRULER_WIDTH + 1, 0);
			}

			if (fHorizontalRulerView)
			{
				fHorizontalRulerView->MoveBy(-(kRULER_WIDTH + 1), 0);
				fHorizontalRulerView->ResizeBy(kRULER_WIDTH + 1, 0);
			}
		}

		if ((fCornerView) && ((!fHorizontalRulerView) || (!fVerticalRulerView)))
		{
			RemoveChild(fCornerView);
			delete fCornerView;
			fCornerView = NULL;
		}
	}
}


/* ---------------------------------------------------------------- */

void BRulerView::ShowRuler(orientation ruler)
{
	BRect		frame;
	TScrollBar*	scrollbar;

	if (Target())
	{
		if ((ruler == B_HORIZONTAL) && (!fHorizontalRulerView))
		{
			fHorizontalRuler = true;
	
			frame = Target()->Frame();
			Target()->MoveBy(0, kRULER_HEIGHT + 1);
			Target()->ResizeBy(0, -(kRULER_HEIGHT + 1));

			if (fVerticalRulerView)
			{
				fVerticalRulerView->MoveBy(0, kRULER_HEIGHT + 1);
				fVerticalRulerView->ResizeBy(0, -(kRULER_HEIGHT + 1));
			}
	
			frame.bottom = frame.top + kRULER_HEIGHT;
			if ((scrollbar = dynamic_cast<TScrollBar*>(ScrollBar(B_VERTICAL))) != NULL)
			{
				frame.right += B_V_SCROLL_BAR_WIDTH;
				scrollbar->MoveBy(0, kRULER_HEIGHT + 1);
				scrollbar->ResizeBy(0, -(kRULER_HEIGHT + 1));
			}

			fHorizontalRulerView = new BRuler(frame, "h_ruler", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW, B_HORIZONTAL, this);
			AddChild(fHorizontalRulerView);
			fHorizontalRulerView->ScrollBy(Target()->Bounds().left, 0);
			if ((scrollbar = dynamic_cast<TScrollBar*>(ScrollBar(B_HORIZONTAL))) != NULL)
				scrollbar->SetRulerView(fHorizontalRulerView);
		}
		else if ((ruler == B_VERTICAL) && (!fVerticalRulerView))
		{
			fVerticalRuler = true;
	
			frame = Target()->Frame();
			Target()->MoveBy(kRULER_WIDTH + 1, 0);
			Target()->ResizeBy(-(kRULER_WIDTH + 1), 0);

			if (fHorizontalRulerView)
			{
				fHorizontalRulerView->MoveBy(kRULER_WIDTH + 1, 0);
				fHorizontalRulerView->ResizeBy(-(kRULER_WIDTH + 1), 0);
			}
	
			frame.right = frame.left + kRULER_WIDTH;
			if ((scrollbar = dynamic_cast<TScrollBar*>(ScrollBar(B_HORIZONTAL))) != NULL)
			{
				frame.bottom += B_H_SCROLL_BAR_HEIGHT;
				scrollbar->MoveBy(kRULER_WIDTH + 1, 0);
				scrollbar->ResizeBy(-(kRULER_WIDTH + 1), 0);
			}

			fVerticalRulerView = new BRuler(frame, "v_ruler", B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW, B_VERTICAL, this);
			AddChild(fVerticalRulerView);
			fVerticalRulerView->ScrollBy(0, Target()->Bounds().top);
			if ((scrollbar = dynamic_cast<TScrollBar*>(ScrollBar(B_VERTICAL))) != NULL)
				scrollbar->SetRulerView(fVerticalRulerView);
		}

		if ((!fCornerView) && (fHorizontalRulerView) && (fVerticalRulerView))
		{
			frame = Target()->Frame();
	
			frame.left -= kRULER_WIDTH + 1;
			frame.right = frame.left + kRULER_WIDTH;
			frame.top -= kRULER_HEIGHT + 1;
			frame.bottom = frame.top + kRULER_HEIGHT;
			fCornerView = new BDeadCorner(frame, "dead_spot", B_FOLLOW_NONE, B_WILL_DRAW, this);
			AddChild(fCornerView);
			fCornerView->SetViewColor(fFillColor);
		}
	}
}


/* ---------------------------------------------------------------- */

int32 BRulerView::Divisions()
{
	return fDivisions;
}


/* ---------------------------------------------------------------- */

void BRulerView::SetDivisions(int32 divisions)
{
	if (divisions != fDivisions)
	{
		fDivisions = divisions;
		UpdateRulers();
	}
}


/* ---------------------------------------------------------------- */

const BFont* BRulerView::Font() const
{
	return &fFont;
}


/* ---------------------------------------------------------------- */

void BRulerView::SetFont(const BFont* font)
{
	fFont = *font;
	UpdateRulers();
}


/* ---------------------------------------------------------------- */

bool BRulerView::MouseTracking()
{
	return fTrackMouse;
}


/* ---------------------------------------------------------------- */

void BRulerView::SetMouseTracking(bool track)
{
	if (track != fTrackMouse)
	{
		uint32	buttons;
		BPoint	p;

		if (Target())
			Target()->GetMouse(&p, &buttons);
		else
			p.Set(-1, -1);

		if (track)
			fTrackMouse = track;

		if (fHorizontalRulerView)
			fHorizontalRulerView->SetMouseMark((track) ? p.x : -1.0);
		if (fVerticalRulerView)
			fVerticalRulerView->SetMouseMark((track) ? p.y : -1.0);

		if (!track)
			fTrackMouse = track;
	}
}


/* ---------------------------------------------------------------- */

float BRulerView::OriginOffset(orientation ruler)
{
	if (ruler == B_HORIZONTAL)
		return fHorizontalOriginOffset;
	else if (ruler == B_VERTICAL)
		return fVerticalOriginOffset;
	return 0.0;
}


/* ---------------------------------------------------------------- */

void BRulerView::SetOriginOffset(orientation ruler, float offset)
{
	bool update = false;

	if ((ruler == B_HORIZONTAL) && (fHorizontalOriginOffset != offset))
	{
		fHorizontalOriginOffset = offset;
		update = true;
	}
	else if ((ruler == B_VERTICAL) && (fVerticalOriginOffset != offset))
	{
		fVerticalOriginOffset = offset;
		update = true;
	}
	if (update)
		UpdateRulers();
}


/* ---------------------------------------------------------------- */

float BRulerView::RulerOffset(orientation ruler)
{
	if (ruler == B_HORIZONTAL)
		return fHorizontalRulerOffset;
	else if (ruler == B_VERTICAL)
		return fVerticalRulerOffset;
	return 0;
}


/* ---------------------------------------------------------------- */

void BRulerView::SetRulerOffset(orientation ruler, float offset)
{
	bool update = false;

	if ((ruler == B_HORIZONTAL) && (fHorizontalRulerOffset != offset))
	{
		fHorizontalRulerOffset = offset;
		update = true;
	}
	else if ((ruler == B_VERTICAL) && (fVerticalRulerOffset != offset))
	{
		fVerticalRulerOffset = offset;
		update = true;
	}
	if (update)
		UpdateRulers();
}


/* ---------------------------------------------------------------- */

ruler_units BRulerView::RulerUnits()
{
	return fUnits;
}


/* ---------------------------------------------------------------- */

void BRulerView::SetRulerUnits(ruler_units units)
{
	if (units != fUnits)
	{
		uint32	index;

		for (index = 0; index < sizeof(ruler) / sizeof(ruler_defintion); index++)
		{
			if (ruler[index].units == fUnits)
			{
				ruler[index].divisions = fDivisions;
				break;
			}
		}

		fUnits = units;
		for (index = 0; index < sizeof(ruler) / sizeof(ruler_defintion); index++)
		{
			if (ruler[index].units == fUnits)
			{
				fDivisions = ruler[index].divisions;
				break;
			}
		}
		UpdateRulers();
	}
}


/* ---------------------------------------------------------------- */

float BRulerView::Scale()
{
	return fScale;
}


/* ---------------------------------------------------------------- */

void BRulerView::SetScale(float scale)
{
	if (scale != fScale)
	{
		fScale = scale;
		UpdateRulers();
	}
}


/* ---------------------------------------------------------------- */

rgb_color BRulerView::CornerFrameColor() const
{
	return fCornerFrameColor;
}


/* ---------------------------------------------------------------- */

void BRulerView::SetCornerFrameColor(rgb_color color)
{
	fCornerFrameColor = color;
	if (fCornerView)
		fCornerView->Invalidate(fCornerView->Bounds());
}


/* ---------------------------------------------------------------- */

rgb_color BRulerView::FillColor() const
{
	return fFillColor;
}


/* ---------------------------------------------------------------- */

void BRulerView::SetFillColor(rgb_color color)
{
	fFillColor = color;
	UpdateRulers();
	if (fCornerView)
	{
		fCornerView->SetViewColor(fFillColor);
		fCornerView->Invalidate(fCornerView->Bounds());
	}
}


/* ---------------------------------------------------------------- */

rgb_color BRulerView::TextColor() const
{
	return fTextColor;
}


/* ---------------------------------------------------------------- */

void BRulerView::SetTextColor(rgb_color color)
{
	fTextColor = color;
	UpdateRulers();
}


/* ---------------------------------------------------------------- */

void BRulerView::Initialize()
{
	BMenuItem*	item;

	fHorizontalRulerOffset = 0.0;
	fVerticalRulerOffset = 0.0;
	fHorizontalOriginOffset = 0.0;
	fVerticalOriginOffset = 0.0;
	fCornerView = NULL;
	fHorizontalRulerView = NULL;
	fVerticalRulerView = NULL;
	fFont = kDEFAULT_FONT;
	fFont.SetSize(kDEFAULT_SIZE);
	fFillColor = _long_to_color_(kDEFAULT_FILL_COLOR);
	fTextColor = _long_to_color_(kDEFAULT_TEXT_COLOR);
	fCornerFrameColor = _long_to_color_(kDEFAULT_CORNER_FRAME_COLOR);

	fMenu = new BPopUpMenu("unit menu");
	fMenu->SetRadioMode(true);
	for (uint32 index = 0; index < sizeof(ruler) / sizeof(ruler_defintion); index++)
		fMenu->AddItem(item = new BMenuItem(ruler[index].label,
							  new BMessage((int32)ruler[index].units)));

	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}


/* ---------------------------------------------------------------- */

void BRulerView::UpdateRulers()
{
	if (fHorizontalRulerView)
		DrawRuler(fHorizontalRulerView, fHorizontalRulerView->Bounds());
	if (fVerticalRulerView)
		DrawRuler(fVerticalRulerView, fVerticalRulerView->Bounds());
}


/* ---------------------------------------------------------------- */

void BRulerView::_ReservedRulerView01() {}
void BRulerView::_ReservedRulerView02() {}
void BRulerView::_ReservedRulerView03() {}
void BRulerView::_ReservedRulerView04() {}
void BRulerView::_ReservedRulerView05() {}
void BRulerView::_ReservedRulerView06() {}
void BRulerView::_ReservedRulerView07() {}
void BRulerView::_ReservedRulerView08() {}
void BRulerView::_ReservedRulerView09() {}
void BRulerView::_ReservedRulerView10() {}
void BRulerView::_ReservedRulerView11() {}
void BRulerView::_ReservedRulerView12() {}
void BRulerView::_ReservedRulerView13() {}
void BRulerView::_ReservedRulerView14() {}
void BRulerView::_ReservedRulerView15() {}
void BRulerView::_ReservedRulerView16() {}


/* ================================================================ */

BRuler::BRuler(BRect rect, const char* name, uint32 resizeMask, uint32 flags,
			   orientation direction, BRulerView* ruler_view)
  :	BView(rect, name, resizeMask, flags),
	fMark(-1.0),
	ScreenDPI(72.0),
	fRulerView(ruler_view),
	fOrientation(direction)
{
}


/* ---------------------------------------------------------------- */

void BRuler::Draw(BRect rect)
{
	// so ruler drawing can be easily overridden
	fRulerView->DrawRuler(this, rect);
}


/* ---------------------------------------------------------------- */

void BRuler::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case kVALUE_MESSAGE:
		{
			if (fRulerView->Target())
			{
				uint32	buttons;
				BPoint	p;

				SetMouseMark(-1.0);
				if ((fOrientation == B_HORIZONTAL) &&
					(Bounds().left != fRulerView->Target()->Bounds().left))
					ScrollTo(fRulerView->Target()->Bounds().left, Bounds().top);
				else if ((fOrientation == B_VERTICAL) &&
					(Bounds().top != fRulerView->Target()->Bounds().top))
					ScrollTo(Bounds().left, fRulerView->Target()->Bounds().top);

				fRulerView->Target()->GetMouse(&p, &buttons);
				if (fRulerView->Target()->Bounds().Contains(p))
				{
					if (fOrientation == B_HORIZONTAL)
						SetMouseMark(p.x);
					else
						SetMouseMark(p.y);
				}
			}
		}
		break;

		default:
			BView::MessageReceived(msg);
	}
}


/* ---------------------------------------------------------------- */

void BRuler::MouseDown(BPoint pt)
{
	BPoint	p = ConvertToScreen(pt);
	uint32	buttons = (uint32)Window()->CurrentMessage()->FindInt32("buttons");

	fRulerView->RulerMenu(p, buttons);
	BView::MouseDown(pt);
}


/* ---------------------------------------------------------------- */

void BRuler::WindowActivated(bool state)
{
	if (fRulerView->MouseTracking())
		SetMouseMark(-1.0);
	fActive = state;
	BView::WindowActivated(state);
}


/* ---------------------------------------------------------------- */

void BRuler::DrawRuler(BRect rect)
{
	float		unit_pixels = (ScreenDPI * (fRulerView->Scale() / 100.0)) /
							  ruler[fRulerView->RulerUnits()].conversion;
	int32		divisions;
	int32		numeral_marks;
	int32		unit_marks;
	int32		loop;
	int32		start;
	BFont		font = fRulerView->Font();

	CalculateMarks(unit_pixels, &divisions, &unit_marks, &numeral_marks);

	SetHighColor(fRulerView->FillColor());
	FillRect(rect);
	Sync();
	SetHighColor(fRulerView->TextColor());
	if (fOrientation == B_VERTICAL)
		font.SetRotation(90.0);
	SetFont(&font);

	if (fOrientation == B_HORIZONTAL)
	{
		float	offset = fRulerView->OriginOffset(B_HORIZONTAL);
		float	x;

		start = (int32)(max_c(0, rect.left - offset) / unit_pixels);
		x = start * unit_pixels + offset;

		while (x < rect.right + 1)
		{
			if (start % unit_marks == 0)
			{
				float	top = Bounds().top;

				if (start % numeral_marks != 0)
					top = Bounds().bottom - 3;
				StrokeLine(BPoint(x, top), BPoint(x, Bounds().bottom));
			}

			if (start % numeral_marks == 0)
			{
				char	digit[16];

				sprintf(digit, "%d", (int)start + (int)fRulerView->RulerOffset(B_HORIZONTAL));
				MovePenTo(x + 4, Bounds().bottom - 5);
				DrawString(digit);
				Sync();
			}

			if (divisions)
			{
				for (loop = 1; loop < divisions; loop++)
				{
					float	left = unit_pixels / divisions * loop;
					float	top;

					if ((float)loop == (float)divisions / 2.0) top = Bounds().bottom - 7;
					else top = Bounds().bottom - 3;
					StrokeLine(BPoint(x + left, top), BPoint(x + left, Bounds().bottom));
				}
			}
			x += unit_pixels;
			start++;
		}
		StrokeLine(BPoint(rect.left, rect.bottom), BPoint(rect.right, rect.bottom));
		if ((fMark != -1.0) && (fMark >= rect.left) && (fMark <= rect.right))
		{
			float	mark = fMark;

			fMark = -1.0;
			SetMouseMark(mark);
		}
	}
	else
	{
		float	offset = fRulerView->OriginOffset(B_VERTICAL);
		float	y;

		start = (int32)(max_c(0, rect.top - offset) / unit_pixels);
		y = start * unit_pixels + offset;

		while (y < rect.bottom + 1)
		{
			if (start % unit_marks == 0)
			{
				float	left = Bounds().left;

				if (start % numeral_marks != 0)
					left = Bounds().right - 3;
				StrokeLine(BPoint(left, y), BPoint(Bounds().right, y));
			}

			if (start % numeral_marks == 0)
			{
				char	digit[16];

				sprintf(digit, "%d", (int)start + (int)fRulerView->RulerOffset(B_HORIZONTAL));
				MovePenTo(Bounds().right - 5, y + font.StringWidth(digit) + 4);
				DrawString(digit);
				Sync();
			}

			if (divisions)
			{
				for (loop = 1; loop < divisions; loop++)
				{
					float	top = unit_pixels / divisions * loop;
					float	left;

					if ((float)loop == (float)divisions / 2.0) left = Bounds().right - 7;
					else left = Bounds().right - 3;
					StrokeLine(BPoint(left, y + top), BPoint(Bounds().right, y + top));
				}
			}
			y += unit_pixels;
			start++;
		}
		StrokeLine(BPoint(rect.right, rect.top), BPoint(rect.right, rect.bottom));
		if ((fMark != -1.0) && (fMark >= rect.top) && (fMark <= rect.bottom))
		{
			float	mark = fMark;

			fMark = -1.0;
			SetMouseMark(mark);
		}
	}
}


/* ---------------------------------------------------------------- */

orientation BRuler::Orientation()
{
	return fOrientation;
}


/* ---------------------------------------------------------------- */

void BRuler::SetMouseMark(float mark)
{
	if ((fRulerView->MouseTracking()) && (mark != fMark) && (fActive))
	{
		BRect	r = Bounds();

		SetDrawingMode(B_OP_INVERT);
		SetHighColor(fRulerView->TextColor());
		SetLowColor(fRulerView->FillColor());
		if (fMark != -1)
		{
			// erase the current mark
			if (fOrientation == B_HORIZONTAL)
				StrokeLine(BPoint(fMark, r.top), BPoint(fMark, r.bottom - 1), B_MIXED_COLORS);
			else
				StrokeLine(BPoint(r.left, fMark), BPoint(r.right - 1, fMark), B_MIXED_COLORS);
		}
		if (fActive)
		{
			fMark = mark;
			if (fMark != -1)
			{
				// draw the new mark
				if (fOrientation == B_HORIZONTAL)
				{
					if (fMark >= fRulerView->OriginOffset(B_HORIZONTAL))
						StrokeLine(BPoint(fMark, r.top), BPoint(fMark, r.bottom - 1), B_MIXED_COLORS);
					else
						fMark = -1;
				}
				else
				{
					if (fMark >= fRulerView->OriginOffset(B_VERTICAL))
						StrokeLine(BPoint(r.left, fMark), BPoint(r.right - 1, fMark), B_MIXED_COLORS);
					else
						fMark = -1;
				}
			}
		}
		SetDrawingMode(B_OP_COPY);
		Sync();
	}
}


/* ---------------------------------------------------------------- */

void BRuler::CalculateMarks(float unit_pixels, int32* divisions, int32 *unit_marks, int32 *number_marks)
{
	int32	marks = fRulerView->Divisions();

	while ((marks) && ((unit_pixels / (float)marks) < 5))
		marks /= 2;
	*divisions = marks;

	*unit_marks = 1;
	while (unit_pixels * *unit_marks < 5)
		*unit_marks += 1;

	*number_marks = 1;
	while (unit_pixels * *number_marks < 20)
		*number_marks += 1;
}


/* ================================================================ */

BDeadCorner::BDeadCorner(BRect rect, const char* name, uint32 resizeMask, uint32 flags,
						 BRulerView* ruler_view)
  :	BView(rect, name, resizeMask, flags),
	fRulerView(ruler_view)
{
}


/* ---------------------------------------------------------------- */

void BDeadCorner::Draw(BRect rect)
{
	// so corner drawing can be easily overridden
	fRulerView->DrawCorner(this, rect);
}


/* ---------------------------------------------------------------- */

void BDeadCorner::MouseDown(BPoint pt)
{
	BPoint	p = ConvertToScreen(pt);
	uint32	buttons = (uint32)Window()->CurrentMessage()->FindInt32("buttons");

	fRulerView->RulerMenu(p, buttons);
	BView::MouseDown(pt);
}


/* ---------------------------------------------------------------- */

void BDeadCorner::DrawCorner(BRect /* rect */)
{
	SetHighColor(fRulerView->CornerFrameColor());
	StrokeRect(Bounds());
}
