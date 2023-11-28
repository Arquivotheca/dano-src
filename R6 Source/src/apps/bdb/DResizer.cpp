/*	$Id: DResizer.cpp,v 1.3 1998/11/17 12:16:39 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

#include "bdb.h"
#include "DResizer.h"

#include <Window.h>

const float
	kResizerWidth = 6;

DResizer::DResizer(BRect frame, const char *name, BView *v1, BView *v2)
	: BView(frame, name, 0, B_WILL_DRAW) 
{
	BView *parent;
	
	parent = v1->Parent();
	ASSERT(parent == v2->Parent());
	
	m_view1 = v1;
	m_view2 = v2;
	
	BRect b1, b2;
	
	b1 = v1->Frame();
	b2 = v2->Frame();
	
	if (b1.top == b2.top)
	{
		v1->ResizeTo(b1.Width() - kResizerWidth / 2, b1.Height());
		v2->ResizeTo(b2.Width() - kResizerWidth / 2 - 1, b2.Height());
		v2->MoveBy(kResizerWidth / 2 + 1, 0);
		
		m_vertical = true;
		SetResizingMode(B_FOLLOW_TOP_BOTTOM);
		
		ResizeTo(kResizerWidth, b1.Height());
		MoveTo(v1->Frame().right + 1, b1.top);
	}
	else
	{
		ASSERT(b1.left == b2.left);

		v1->ResizeTo(b1.Width(), b1.Height() - kResizerWidth / 2);
		v2->ResizeTo(b2.Width(), b2.Height() - kResizerWidth / 2 - 1);
		v2->MoveBy(0, kResizerWidth / 2 + 1);
		
		m_vertical = false;
		SetResizingMode(B_FOLLOW_LEFT_RIGHT);
		
		ResizeTo(b1.Width(), kResizerWidth);
		MoveTo(b1.left, v1->Frame().bottom + 1);
	}
	
	SetViewColor(kViewColor);
} /* DResizer::DResizer */

void DResizer::MouseDown(BPoint where)
{
	ulong btns;
	
	if (m_vertical)
	{
		float x = where.x - Bounds().left;
		
		while (GetMouse(&where, &btns), btns)
		{
			if (where.x - Bounds().left != x)
			{
				Window()->DisableUpdates();
				
				float dx = where.x + Bounds().left - x;
				MoveBy(dx, 0);
				
				m_view1->ResizeBy(dx, 0);
				m_view2->ResizeBy(-dx, 0);
				m_view2->MoveBy(dx, 0);
				
				Window()->EnableUpdates();
			}
		}
	}
	else
	{
		float y = where.y - Bounds().top;
		
		while (GetMouse(&where, &btns), btns)
		{
			if (where.y - Bounds().top != y)
			{
				Window()->DisableUpdates();
				
				float dy = where.y + Bounds().top - y;
				MoveBy(0, dy);
				
				m_view1->ResizeBy(0, dy);
				m_view2->ResizeBy(0, -dy);
				m_view2->MoveBy(0, dy);
				
				Window()->EnableUpdates();
			}
		}
	}
} /* DResizer::MouseDown */

void DResizer::Draw(BRect /*update*/)
{
	BRect bounds(Bounds());

	if (m_vertical)
	{
		bounds.InsetBy(0, 2);
		bounds.left += floor(bounds.Width() / 2) - 1;
		bounds.right = bounds.left + 1;

		SetHighColor(kWhite);
		StrokeLine(bounds.LeftTop(), bounds.LeftBottom());
		SetHighColor(kDarkShadow);
		StrokeLine(bounds.RightTop(), bounds.RightBottom());
		SetHighColor(kBlack);
	}
	else
	{
		bounds.InsetBy(2, 0);
		bounds.top += floor(bounds.Height() / 2) - 1;
		bounds.bottom = bounds.top + 1;

		SetHighColor(kWhite);
		StrokeLine(bounds.LeftTop(), bounds.RightTop());
		SetHighColor(kDarkShadow);
		StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
		SetHighColor(kBlack);
	}
} /* DResizer::Draw */
