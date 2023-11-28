
#include "Gizmos.h"
#include <Bitmap.h>
#include <stdio.h>



#define FPRINTF(x) (void)0

extern const unsigned char g_graphicsBits[];
extern const size_t g_graphicsBitsLength;

Gizmo::Gizmo(const BRect &area) :
	m_frame(area)
{
}


Gizmo::~Gizmo()
{
}

void 
Gizmo::Draw(BView *parent)
{
	parent->FillRect(m_frame, B_SOLID_LOW);
}

void 
Gizmo::Click(BView * /* parent */, BPoint /* where */)
{
}


PlayGizmo::PlayGizmo(const BRect &area)
	: Gizmo(area)
{
	FPRINTF((stderr, "PlayGizmo(%g,%g,%g,%g)\n", area.left, area.top, area.right, area.bottom));
	m_state = kOff;
	BRect r(area);
	r.bottom = (r.bottom+1)*3-1;
	m_graphics = new BBitmap(r, B_RGB32);
	m_graphics->LockBits();
	int len = g_graphicsBitsLength;
	if (len > m_graphics->BitsLength()) len = m_graphics->BitsLength();
	memcpy(m_graphics->Bits(), g_graphicsBits, len);
	m_graphics->UnlockBits();
}


PlayGizmo::~PlayGizmo()
{
	delete m_graphics;
}

void 
PlayGizmo::Draw(BView *parent)
{
	BRect r(Frame());
	FPRINTF((stderr, "PlayGizmo::Draw(%g,%g,%g,%g)\n", r.left, r.top, r.right, r.bottom));
	r.OffsetTo(0,0);
	r.OffsetBy(0,(r.Height()+1)*m_state);
	parent->DrawBitmapAsync(m_graphics, r, Frame());
}

void 
PlayGizmo::Click(BView *parent, BPoint where)
{
	FPRINTF((stderr, "PlayGizmo::Click(%g,%g)\n", where.x, where.y));
	int32 oldState = State();
	SetState(kClicked);
	Draw(parent);
	parent->Flush();
	snooze(100000LL);
	SetState(oldState == kOn ? kOff : kOn);
	Draw(parent);
	parent->Flush();
}


PositionGizmo::PositionGizmo(const BRect &area) :
	Gizmo(area)
{
	m_position = 0;
}

void 
PositionGizmo::Draw(BView * parent)
{
	BPoint where(Frame().left+6, Frame().bottom-5);
	FPRINTF((stderr, "PositionGizmo::Draw(%g,%g)\n", where.x, where.y));
	char n[30];
	parent->SetHighColor(32,32,32);
	parent->SetLowColor(0xd8, 0xd8, 0xd8);
	parent->SetDrawingMode(B_OP_COPY);
	parent->FillRect(Frame(), B_SOLID_LOW);
	sprintf(n, "%.1f s", m_position);
	parent->SetFont(be_plain_font);
	parent->DrawString(n, where);
}

