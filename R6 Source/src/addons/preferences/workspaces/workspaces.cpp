#include "workspaces.h"
#include <View.h>
#include <Control.h>
#include <Region.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <Screen.h>
#include <stdio.h>

class WSview : public BControl
{
public:
			WSview(BRect frame, const char *name, BMessage *message, uint32 resizeMask, uint32 flags);
	void	Draw(BRect r);
	void	MouseDown(BPoint p);
	void	get_counts(long *h, long *v);
	void	space_rect(BRect *r, long space, long inset);
};

WSview::WSview(BRect frame, const char *name, BMessage *message, uint32 resizeMask, uint32 flags)
 : BControl(frame, name, B_EMPTY_STRING, message, resizeMask, flags)
{
}

void WSview::Draw(BRect r)
{
	BRegion	clip;
	BRect	curr;
	clip.Include(Bounds());
	BFont	f;
	font_height	height;
	GetFont(&f);
	f.GetHeight(&height);

	SetLowColor(255, 255, 255);

	int32 max = count_workspaces();
	for(int32 i = 0; i < max; i++)
	{
		BRect r;
		BRegion	localclip;

		space_rect(&r, i, 1);
		SetHighColor(BScreen().DesktopColor(i));
		FillRect(r, B_SOLID_HIGH);
		clip.Exclude(r);
		if(i == current_workspace())
			curr = r;

		localclip.Include(r);
		ConstrainClippingRegion(&localclip);

		char tmp[20];
		sprintf(tmp, "%ld", i + 1);
#define PADX 3
#define PADY 3
		BRect	box;
		BPoint	text;
		float	textw = StringWidth(tmp);
		box.left = r.left + ((r.right - r.left) - textw) / 2 - PADX;
		box.right = box.left + PADX + textw + PADX;
		box.top = r.top + ((r.bottom - r.top) - height.ascent - height.descent) / 2 - PADY;
		box.bottom = box.top + PADY + height.ascent + height.descent + PADY;
		text.x = box.left + PADX + 1;
		text.y = box.top + PADY + height.ascent;
		SetHighColor(0, 0, 0);
		FillRect(box, B_SOLID_LOW);
		DrawString(tmp, text);
		box.InsetBy(-1, -1);
		SetHighColor(216, 216, 216);
		StrokeRect(box, B_SOLID_HIGH);

		ConstrainClippingRegion(0);
	}

	ConstrainClippingRegion(&clip);
	SetHighColor(255, 255, 255);
	FillRect(r, B_SOLID_HIGH);
	ConstrainClippingRegion(0);

	SetHighColor(0, 0, 0);
	curr.InsetBy(-1, -1);
	StrokeRect(curr, B_SOLID_HIGH);
}

void WSview::MouseDown(BPoint p)
{
	int32	ws;
	long	h, v;
	BRect	r = Bounds();
	get_counts(&h, &v);
	ws = int32(p.x * h / r.right) + int32(p.y * v / r.bottom) * h;
	BMessage	copy(*Message());
	copy.AddInt32("workspace", ws);
	Invoke(&copy);
}

// the following code was ripped from the app_server

//---------------------------------------------------------------
// Return an h and v pair for the defined number of workspaces.
//---------------------------------------------------------------

void WSview::get_counts(long *h, long *v)
{
	switch (count_workspaces()) {
		case 1:
			*h = 1; *v = 1; break;
		case 2:
			*h = 2; *v = 1; break;
		case 3:
			*h = 3; *v = 1; break;
		case 4:
			*h = 2; *v = 2; break;
		case 5:
			*h = 5; *v = 1; break;
		case 6:
			*h = 3; *v = 2; break;
		case 7:
			*h = 7; *v = 1; break;
		case 8:
			*h = 4; *v = 2; break;
		case 9:
			*h = 3; *v = 3; break;
		case 10:
			*h = 5; *v = 2; break;
		case 11:
			*h = 11; *v = 1; break;
		case 12:
			*h = 4; *v = 3; break;
		case 13:
			*h = 13; *v = 1; break;
		case 14:
			*h = 7; *v = 2; break;
		case 15:
			*h = 5; *v = 3; break;
		case 16:
			*h = 4; *v = 4; break;
		case 17:
			*h = 17; *v = 1; break;
		case 18:
			*h = 6; *v = 3; break;
		case 19:
			*h = 19; *v = 1; break;
		case 20:
			*h = 5; *v = 4; break;
		case 21:
			*h = 7; *v = 3; break;
		case 22:
			*h = 11; *v = 2; break;
		case 23:
			*h = 23; *v = 1; break;
		case 24:
			*h = 6; *v = 4; break;
		case 25:
			*h = 5; *v = 5; break;
		case 26:
			*h = 13; *v = 2; break;
		case 27:
			*h = 9; *v = 3; break;
		case 28:
			*h = 7; *v = 4; break;
		case 29:
			*h = 29; *v = 1; break;
		case 30:
			*h = 6; *v = 5; break;
		case 31:
			*h = 31; *v = 1; break;
		case 32:
			*h = 8; *v = 4; break;
	}
}

//---------------------------------------------------------------
// Return the workspace rect within the workspace preference
// panel window for a given workspace.			
//---------------------------------------------------------------

void WSview::space_rect(BRect *r, long space, long inset)
{
	long	i, j, h, v;
	float	width, height;
	BRect	bound;

	bound = Bounds();
	get_counts(&h, &v);
	i = space / h;
	j = space % h;
	width = (bound.right - bound.left) / h;
	height = (bound.bottom - bound.top) / v;
	r->top = (float)((int32)(i * height + inset));
	r->left = (float)((int32)(j * width + inset));

	// workspaces on the right or bottom edge actually go all the way to
	// the edge.
	if (i != (v - 1))
		r->bottom = (float)((int32)((i + 1) * height - inset));
	else
		r->bottom = (float)((int32)((bound.bottom - bound.top) - inset));
	if (j != (h - 1))
		r->right = (float)((int32)((j + 1) * width - inset));
	else
		r->right = (float)((int32)((bound.right - bound.left) - inset));
}

class LineView : public BView
{
public:
	LineView(BRect frame)
	 : BView(frame, "line", 0, B_WILL_DRAW)
	{
	}

	void AttachedToWindow()
	{
		SetViewColor(Parent()->ViewColor());
	}

	void Draw(BRect)
	{
		BRect r = Bounds();
		SetHighColor(156, 156, 156);
		StrokeLine(BPoint(r.left, 10), BPoint(r.right, 10), B_SOLID_HIGH);
		SetHighColor(156, 156, 156);
		StrokeLine(BPoint(r.left, 11), BPoint(r.right, 11), B_SOLID_HIGH);
		SetHighColor(Parent()->ViewColor());
		FillRect(BRect(r.left, 0, r.right, 9), B_SOLID_HIGH);
		FillRect(BRect(r.left, 12, r.right, r.bottom), B_SOLID_HIGH);
	}
};


static const int32 kMaxWorkspaceCount = 32;

#define WX	400
#define WY	330

WorkspacesView::WorkspacesView()
 : BView(BRect(0, 0, WX, WY), "workspaces", B_FOLLOW_TOP | B_FOLLOW_LEFT, 0)
{
	LineView	*topbar = new LineView(BRect(0, 0, WX, 24));
	AddChild(topbar);

	wsmenu = new BPopUpMenu("workspace  count");

	char 		str[32];
	BMessage 	*msg=NULL;
	int32		count = count_workspaces();
	int32		max_ws = (count >= kMaxWorkspaceCount)
					? count : kMaxWorkspaceCount;
	BMenuItem*	mi;
	for (int32 i=1 ; i<=max_ws ; i++)
	{
		msg = new BMessage('conf');
		msg->AddInt32("count", i);
		sprintf(str, "%ld", i);
		mi = new BMenuItem(str, msg);
		wsmenu->AddItem(mi);		
	}

	//	check the currect workspace count
	mi = wsmenu->ItemAt(count - 1);
	if(mi)
		mi->SetMarked(true);

	float w = StringWidth("Workspace count:") + 5;

	BRect rect;
	rect.bottom = 60;
	rect.top = rect.bottom - 20;
	rect.left = 0;
	rect.right = w + 50;
	BMenuField *mf = new BMenuField(rect, "", "Workspace count:", wsmenu, true);
	AddChild(mf);
	mf->SetAlignment(B_ALIGN_RIGHT);
	mf->SetDivider(w);

	ws = new WSview(BRect(70, 70, 310, 250), "wsview", new BMessage('wsel'), B_FOLLOW_NONE, B_WILL_DRAW);
	AddChild(ws);
}

void WorkspacesView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	wsmenu->SetTargetForItems(this);
	ws->SetTarget(this);
}

void WorkspacesView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case 'conf' :
			{
				int32 wsc;
				msg->FindInt32("count", &wsc);
				if (wsc != count_workspaces())
				{
					set_workspace_count(wsc);
					ws->Invalidate();
				}
			}
			break;

		case 'wsel' :
			{
				int32 nws;
				msg->FindInt32("workspace", &nws);
				if(nws != current_workspace())
				{
					activate_workspace(nws);
					ws->Draw(ws->Bounds());
				}
			}
			break;

		case B_WORKSPACE_ACTIVATED :
			ws->Draw(ws->Bounds());
			break;

		default :
			BView::MessageReceived(msg);
			break;
	}
}

