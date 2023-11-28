#include "utils.h"
#include "ToolPicker.h"
#include "IconBits.h"

extern	uchar	select_tool[], pencil_tool[], bucket_tool[], line_tool[],
				fill_rect_tool[], fill_round_tool[], fill_oval_tool[],
				fill_arc_tool[], fill_triangle_tool[], lasso_tool[],
				erasure_tool[], dropper_tool[], magnify_tool[], rect_tool[],
				round_tool[], oval_tool[], arc_tool[], triangle_tool[];

static const char* tool_tips[kToolCount] = {
	"Rectangle Selection", "Eraser", "Pencil", "Eye Dropper",
	"Bucket Fill", "Draw Line", "Draw Rectangle", "Fill Rectangle",
	"Draw Round Rectangle", "Fill Round Rectangle",
	"Draw Oval", "Fill Oval",
	#if 0
	"Draw Arc", "Fill Arc", "Draw Triangle", "Fill Triangle"
	"Lasso Selection"
	#endif
};

TToolButton::TToolButton(BRect frame, const BBitmap *icon,
						 int32 index, const char* tip)
	: BView( frame, "tool", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW),
	  BToolTipable(*this, tip)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	fIndex = index;
	fSelected = false;
	fIcon = icon;	
}

TToolButton::~TToolButton()
{
}

void 
TToolButton::Draw(BRect)
{
	BRect r = Bounds();

	DrawBitmap(fIcon,r);	
	if (fSelected) {
		r.InsetBy(3,3);
		InvertRect(r);	
	}	

	AddRaisedBevel(this, Bounds(), false);
	
	// notch the four corners
	rgb_color c = Parent()->ViewColor();
	BeginLineArray(4);
	AddLine(Bounds().LeftTop(),Bounds().LeftTop(),c);
	AddLine(Bounds().RightBottom(),Bounds().RightBottom(),c);
	AddLine(Bounds().LeftBottom(),Bounds().LeftBottom(),c);
	AddLine(Bounds().RightTop(),Bounds().RightTop(),c);
	EndLineArray();
}

void 
TToolButton::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}

void 
TToolButton::MouseDown(BPoint pt)
{
	BView::MouseDown(pt);
	if (fSelected)
		return;
	fSelected = true;
	((TToolPicker*)Parent())->ChangeSelection(fIndex);
}

bool
TToolButton::Selected()
{
	return fSelected;
}

void
TToolButton::SetSelected(bool state)
{
	fSelected = state;
	BRect r(Bounds());
	r.InsetBy(3,3);
	Invalidate(r);
}

//
//
//

TToolPicker::TToolPicker(BPoint loc, uint32 what, orientation o,
						 int32 toolsPerLine, int32 tool)
	: BControl(BRect(loc.x,loc.y,loc.x+1,loc.y+1), "", "icon group", NULL,
			   B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE),
	  fToolsPerLine(toolsPerLine), fOrientation(o), fCurrentTool(tool)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	for (int32 i=0 ; i<kToolCount ; i++)
		AddTool(i,what);
	
	float pw=0, ph=0;
	GetPreferredSize(&pw, &ph);
	
	ResizeTo(pw, ph);
}

TToolPicker::~TToolPicker()
{
}

void
TToolPicker::AttachedToWindow()
{
	BView::AttachedToWindow();
	ChangeSelection(fCurrentTool);
}

void
TToolPicker::GetPreferredSize(float *width, float *height)
{
	if( fOrientation == B_HORIZONTAL ) {
		*width = 2 + fToolsPerLine*kToolWidth + (fToolsPerLine-1)*2;
		const int32 toolsHigh = (kToolCount+1)/fToolsPerLine;
		*height = 2 + toolsHigh*kToolHeight + (toolsHigh-1)*2;
	} else {
		*height = 2 + fToolsPerLine*kToolHeight + (fToolsPerLine-1)*2;
		const int32 toolsWide = (kToolCount+1)/fToolsPerLine;
		*width = 2 + toolsWide*kToolWidth + (toolsWide-1)*2;
	}
}

void
TToolPicker::Draw(BRect)
{
	//DrawFancyBorder(this);

	BRect b(Bounds());
	rgb_color highColor;

	if (Window()->IsActive() && IsFocus())
		// view active and focused, draw focus rect
		highColor = keyboard_navigation_color();
	else
		// remove the focus mark if not focus or if window is not frontmost
		highColor = ViewColor();

	SetHighColor(highColor);
	StrokeRect(b);
}

void 
TToolPicker::KeyDown(const char *bytes, int32 n)
{
	if (!IsFocus())
		return;
		
	switch (bytes[0]) {
		case B_DOWN_ARROW:
		case B_LEFT_ARROW:
			PrevTool();
			break;
		case B_UP_ARROW:
		case B_RIGHT_ARROW:
			NextTool();
			break;
		case B_ENTER:
		case B_SPACE:
			break;
		default:
			BControl::KeyDown(bytes, n);
			break;
	}
}

void
TToolPicker::AddTool(int32 which, uint32)
{
	const BBitmap* icon = 0;
	
	switch (which) {
		case kSelectionTool:		icon = Resources().FindBitmap(R_SELECT_ICON);		break;
		case kEraserTool:			icon = Resources().FindBitmap(R_ERASE_ICON);		break;
		case kPencilTool:			icon = Resources().FindBitmap(R_PENCIL_ICON);		break;
		case kEyeDropperTool:		icon = Resources().FindBitmap(R_DROPPER_ICON);		break;
		case kBucketTool:			icon = Resources().FindBitmap(R_BUCKET_ICON);		break;
		case kLineTool:				icon = Resources().FindBitmap(R_LINE_ICON);			break;
		case kRectTool:				icon = Resources().FindBitmap(R_RECT_ICON);			break;
		case kFilledRectTool:		icon = Resources().FindBitmap(R_FILL_RECT_ICON);	break;
		case kRoundRectTool:		icon = Resources().FindBitmap(R_ROUND_ICON);		break;
		case kFilledRoundRectTool:	icon = Resources().FindBitmap(R_FILL_ROUND_ICON);	break;
		case kOvalTool:				icon = Resources().FindBitmap(R_OVAL_ICON);			break;
		case kFilledOvalTool:		icon = Resources().FindBitmap(R_FILL_OVAL_ICON);	break;
		default:					icon = Resources().FindBitmap(R_PENCIL_ICON);		break;
	}
	
	BRect r = GetToolFrame(which);
	fToolBtns[which] = new TToolButton(r, icon, which, tool_tips[which]);
	AddChild(fToolBtns[which]);
}

BRect
TToolPicker::GetToolFrame(int32 which)
{
//	int32 x,y;
	BRect r(0,0,0,0);

	if (fOrientation == B_HORIZONTAL) {
		const int32 x = which%fToolsPerLine;
		const int32 y = which/fToolsPerLine;
		r.left = 1 + (x * (kToolWidth + 2));
		r.right = r.left + (kToolWidth - 1);
		r.top = 1 + (y * (kToolHeight +2));
		r.bottom = r.top + (kToolHeight - 1);
	} else {
		const int32 x = which/fToolsPerLine;
		const int32 y = which%fToolsPerLine;
		r.left = 1 + (x * (kToolWidth + 2));
		r.right = r.left + (kToolWidth - 1);
		r.top = 1 + (y * (kToolHeight +2));
		r.bottom = r.top + (kToolHeight - 1);
	}

	return r;
}

void
TToolPicker::ChangeSelection(int32 index)
{
	if( fCurrentTool < kToolCount ) {
		fToolBtns[fCurrentTool]->SetSelected(false);
	}
	
	fCurrentTool = index;
	if( fCurrentTool < kToolCount ) {
		fToolBtns[fCurrentTool]->SetSelected(true);
	}
	
	BMessage* msg = new BMessage(msg_new_tool);
	if (msg) {
		msg->AddInt32("tool", fCurrentTool);
		SetMessage(msg);
		Invoke();
	}
}

void
TToolPicker::NextTool()
{
	int32 t = (fCurrentTool + 1 >= kToolCount) ? 0 : fCurrentTool + 1;
	ChangeSelection(t);
}

void
TToolPicker::PrevTool()
{
	int32 t = (fCurrentTool - 1 < 0) ? kToolCount - 1 : fCurrentTool - 1;
	ChangeSelection(t);
}

int32
TToolPicker::CurrentTool() const
{
	return fCurrentTool;
}

//

TToolPickerPalette::TToolPickerPalette(BPoint loc, int32)
	: BWindow(BRect(loc.x,loc.y,loc.x+1,loc.y+1), "Tools",
		B_FLOATING_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
		B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE
			| B_WILL_DRAW | B_FRAME_EVENTS | B_AVOID_FOCUS)
{
	fToolPicker = new TToolPicker(BPoint(0,0), msg_new_tool, B_HORIZONTAL, kToolCount);

	ResizeTo(fToolPicker->Bounds().Width(), fToolPicker->Bounds().Height());

	AddChild(fToolPicker);
}


TToolPickerPalette::~TToolPickerPalette()
{
}

int32 
TToolPickerPalette::CurrentTool() const
{
	return 0;
}

void 
TToolPickerPalette::SetTool(int32)
{
}

