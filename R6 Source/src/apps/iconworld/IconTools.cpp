//--------------------------------------------------------------------
//	
//	IconTools.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef ICON_TOOLS_H
#include "IconTools.h"
#endif
#ifndef ICON_WORLD_H
#include "IconWorld.h"
#endif
#ifndef ICON_VIEW_H
#include "IconView.h"
#endif
#ifndef ICON_COLOR_H
#include "IconColor.h"
#endif
#ifndef ICON_PAT_H
#include "IconPat.h"
#endif
#ifndef ICON_WINDOW_H
#include "IconWindow.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <OS.h>
#include <Screen.h>

extern	uchar	select_tool[], pencil_tool[], bucket_tool[], line_tool[],
				fill_rect_tool[], fill_round_tool[], fill_oval_tool[],
				fill_arc_tool[], fill_triangle_tool[], lasso_tool[],
				erasure_tool[], dropper_tool[], magnify_tool[], rect_tool[],
				round_tool[], oval_tool[], arc_tool[], triangle_tool[];


//====================================================================

TToolView::TToolView(BRect rect)
		  :BView(rect, "", B_NOT_RESIZABLE, B_WILL_DRAW)
{
	fArcAngle = 90.0;
	fStartAngle = 45.0;
	fXRadius = 8.0;
	fYRadius = 8.0;
}

//--------------------------------------------------------------------

TToolView::~TToolView(void)
{
	for (short counter = 0; counter < NUMTOOLS; counter++)
		delete fToolIcons[counter];
}

//--------------------------------------------------------------------

void TToolView::AttachedToWindow(void)
{
	BRect			r;
	unsigned char	*bits;

	r.Set(0, 0, TOOLSIZE - 1, TOOLSIZE - 1);

	if (TOOLWIDTH > TOOLHEIGHT) {
		bits = select_tool;
		fToolIcons[0] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[0]->SetBits((char*)bits, fToolIcons[0]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = pencil_tool;
		fToolIcons[1] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[1]->SetBits((char*)bits, fToolIcons[1]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = bucket_tool;
		fToolIcons[2] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[2]->SetBits((char*)bits, fToolIcons[2]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = line_tool;
		fToolIcons[3] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[3]->SetBits((char*)bits, fToolIcons[3]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = fill_rect_tool;
		fToolIcons[4] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[4]->SetBits((char*)bits, fToolIcons[4]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = fill_round_tool;
		fToolIcons[5] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[5]->SetBits((char*)bits, fToolIcons[5]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = fill_oval_tool;
		fToolIcons[6] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[6]->SetBits((char*)bits, fToolIcons[6]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = fill_arc_tool;
		fToolIcons[7] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[7]->SetBits((char*)bits, fToolIcons[7]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = fill_triangle_tool;
		fToolIcons[8] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[8]->SetBits((char*)bits, fToolIcons[8]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = lasso_tool;
		fToolIcons[9] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[9]->SetBits((char*)bits, fToolIcons[9]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = erasure_tool;
		fToolIcons[10] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[10]->SetBits((char*)bits, fToolIcons[10]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = dropper_tool;
		fToolIcons[11] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[11]->SetBits((char*)bits, fToolIcons[11]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = magnify_tool;
		fToolIcons[12] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[12]->SetBits((char*)bits, fToolIcons[12]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = rect_tool;
		fToolIcons[13] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[13]->SetBits((char*)bits, fToolIcons[13]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = round_tool;
		fToolIcons[14] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[14]->SetBits((char*)bits, fToolIcons[14]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = oval_tool;
		fToolIcons[15] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[15]->SetBits((char*)bits, fToolIcons[15]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = arc_tool;
		fToolIcons[16] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[16]->SetBits((char*)bits, fToolIcons[16]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = triangle_tool;
		fToolIcons[17] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[17]->SetBits((char*)bits, fToolIcons[17]->BitsLength(), 0, B_COLOR_8_BIT);
		fTool = 1;
	}
	else {
		bits = select_tool;
		fToolIcons[0] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[0]->SetBits((char*)bits, fToolIcons[0]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = lasso_tool;
		fToolIcons[1] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[1]->SetBits((char*)bits, fToolIcons[1]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = erasure_tool;
		fToolIcons[2] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[2]->SetBits((char*)bits, fToolIcons[2]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = pencil_tool;
		fToolIcons[3] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[3]->SetBits((char*)bits, fToolIcons[3]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = dropper_tool;
		fToolIcons[4] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[4]->SetBits((char*)bits, fToolIcons[4]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = bucket_tool;
		fToolIcons[5] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[5]->SetBits((char*)bits, fToolIcons[5]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = magnify_tool;
		fToolIcons[6] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[6]->SetBits((char*)bits, fToolIcons[6]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = line_tool;
		fToolIcons[7] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[7]->SetBits((char*)bits, fToolIcons[7]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = fill_rect_tool;
		fToolIcons[8] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[8]->SetBits((char*)bits, fToolIcons[8]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = rect_tool;
		fToolIcons[9] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[9]->SetBits((char*)bits, fToolIcons[9]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = fill_round_tool;
		fToolIcons[10] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[10]->SetBits((char*)bits, fToolIcons[10]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = round_tool;
		fToolIcons[11] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[11]->SetBits((char*)bits, fToolIcons[11]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = fill_oval_tool;
		fToolIcons[12] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[12]->SetBits((char*)bits, fToolIcons[12]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = oval_tool;
		fToolIcons[13] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[13]->SetBits((char*)bits, fToolIcons[13]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = fill_arc_tool;
		fToolIcons[14] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[14]->SetBits((char*)bits, fToolIcons[14]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = arc_tool;
		fToolIcons[15] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[15]->SetBits((char*)bits, fToolIcons[15]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = fill_triangle_tool;
		fToolIcons[16] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[16]->SetBits((char*)bits, fToolIcons[16]->BitsLength(), 0, B_COLOR_8_BIT);

		bits = triangle_tool;
		fToolIcons[17] = new BBitmap(r, B_COLOR_8_BIT);
		fToolIcons[17]->SetBits((char*)bits, fToolIcons[17]->BitsLength(), 0, B_COLOR_8_BIT);
		fTool = T_PENCIL;
	}
}

//--------------------------------------------------------------------

void TToolView::Draw(BRect where)
{
	short		tool;
	short		counter;
	long		x = 0, y = 0;
	long		i;
	BRect		r;
	rgb_color	c;

	for (counter = 0; counter < NUMTOOLS; counter++) {
		r.Set(x * TOOLSIZE + WINDBORDER + 1,
			  y * TOOLSIZE + WINDBORDER + 1,
			  x * TOOLSIZE + WINDBORDER - 1 + TOOLSIZE,
			  y * TOOLSIZE + WINDBORDER - 1 + TOOLSIZE);
		if (where.Intersects(r)) 
			DrawBitmap(GetToolMap(counter), BPoint(x * TOOLSIZE + WINDBORDER,
												   y * TOOLSIZE + WINDBORDER));
		x += 1;
		if (x == TOOLWIDTH) {
			x = 0;
			y += 1;
		}
	}

	c.red = 0;
	c.green = 0;
	c.blue = 0;
	BeginLineArray(TOOLWIDTH + TOOLHEIGHT + 2);
	for (i = 0; i <= TOOLWIDTH; i += 1)
		AddLine(BPoint(i * TOOLSIZE + WINDBORDER, WINDBORDER),
			    BPoint(i * TOOLSIZE + WINDBORDER, TOOLHEIGHT * TOOLSIZE + WINDBORDER),c);
	for (i = 0; i <= TOOLHEIGHT; i += 1)
		AddLine(BPoint(WINDBORDER, i * TOOLSIZE + WINDBORDER),
			    BPoint(TOOLWIDTH * TOOLSIZE + WINDBORDER, i * TOOLSIZE + WINDBORDER),c);
	EndLineArray();
	
	tool = GetTool();
	x = tool % TOOLWIDTH;
	y = tool / TOOLWIDTH;
	r.Set(x * TOOLSIZE + WINDBORDER + 1,
		  y * TOOLSIZE + WINDBORDER + 1,
		  x * TOOLSIZE + WINDBORDER - 1 + TOOLSIZE,
		  y * TOOLSIZE + WINDBORDER - 1 + TOOLSIZE);
	InvertRect(r);
}

//--------------------------------------------------------------------

void TToolView::MouseDown(BPoint thePoint)
{
	bool	first_pass = TRUE;
	short	current, new_sel, last_sel;
	short	tool;
	long	clicks;
	long	x, y;
	ulong	buttons;
	BPoint	where;
	BRect	r;

	Window()->CurrentMessage()->FindInt32("clicks", &clicks);
	current = last_sel = GetTool();
	where = thePoint;
	do {
		x = (where.x - WINDBORDER) / TOOLSIZE;
		y = (where.y - WINDBORDER) / TOOLSIZE;
		if (((x >= 0) && (x < TOOLWIDTH)) && ((y >= 0) && (y < TOOLHEIGHT)) &&
			((where.x > WINDBORDER) && (where.y > WINDBORDER))) {
			new_sel = (y * TOOLWIDTH) + x;
			if (new_sel >= NUMTOOLS)
				new_sel = current;
		}
		else
			new_sel = current;
		if ((first_pass) && (new_sel == last_sel) && (clicks > 1)) {
			tool = GetToolType();
			if ((tool == T_ARC) || (tool == T_FILL_ARC)) {
				GetParameters(T_ARC);
				return;
			}
			else if ((tool == T_RRECT) || (tool == T_FILL_RRECT)) {
				GetParameters(T_RRECT);
				return;
			}
		}
		SetTool(new_sel);
		last_sel = new_sel;
		snooze(50000);
		first_pass = FALSE;
		GetMouse(&where, &buttons);
	} while(buttons);
}

//--------------------------------------------------------------------

void TToolView::GetParameters(short tool)
{
	float	*param1, *param2;
	BRect	r;

	r = Window()->Frame();
	r.left += ((r.Width() - PARAM_WIDTH) / 2);
	r.right = r.left + PARAM_WIDTH;
	r.top += ((r.Height() - PARAM_HEIGHT) / 2);
	r.bottom = r.top + PARAM_HEIGHT;
	if (tool == T_ARC) {
		param1 = &fStartAngle;
		param2 = &fArcAngle;
	}
	else {
		param1 = &fXRadius;
		param2 = &fYRadius;
	}
	new TParamWindow(r, tool, param1, param2);
}

//--------------------------------------------------------------------

short TToolView::GetTool(void)
{
	return	fTool;
}

//--------------------------------------------------------------------

short TToolView::GetToolType(void)
{
	if (TOOLHEIGHT > TOOLWIDTH)
		return	fTool;
	else {
		switch (fTool) {
			case  0: return T_MARQUEE;
			case  1: return T_PENCIL;
			case  2: return T_BUCKET;
			case  3: return T_LINE;
			case  4: return T_FILL_RECT;
			case  5: return T_FILL_RRECT;
			case  6: return T_FILL_OVAL;
			case  7: return T_FILL_ARC;
			case  8: return T_FILL_TRIANGLE;
			case  9: return T_LASSO;
			case 10: return T_ERASER;
			case 11: return T_DROPPER;
			case 12: return T_MAGNIFY;
			case 13: return T_RECT;
			case 14: return T_RRECT;
			case 15: return T_OVAL;
			case 16: return T_ARC;
			case 17: return T_TRIANGLE;
			default: return T_PENCIL;
		}
	}
}

//--------------------------------------------------------------------

void TToolView::SetTool(short toolID)
{
	long	x;
	long	y;
	BRect	r;

	if ((toolID != fTool) && (Window()->Lock())) {
		x = fTool % TOOLWIDTH;
		y = fTool / TOOLWIDTH;
		r.Set(x * TOOLSIZE + WINDBORDER + 1,
			  y * TOOLSIZE + WINDBORDER + 1,
			  x * TOOLSIZE + WINDBORDER - 1 + TOOLSIZE,
			  y * TOOLSIZE + WINDBORDER - 1 + TOOLSIZE);
		InvertRect(r);
		x = toolID % TOOLWIDTH;
		y = toolID / TOOLWIDTH;
		r.Set(x * TOOLSIZE + WINDBORDER + 1,
			  y * TOOLSIZE + WINDBORDER + 1,
			  x * TOOLSIZE + WINDBORDER - 1 + TOOLSIZE,
			  y * TOOLSIZE + WINDBORDER - 1 + TOOLSIZE);
		InvertRect(r);
		fTool = toolID;
		Sync();
		Window()->Unlock();
	}
}

//--------------------------------------------------------------------

BBitmap* TToolView::GetToolMap(short tool_id)
{
	return fToolIcons[tool_id];
}

//--------------------------------------------------------------------

void TToolView::Dropper(short x, short y, BBitmap *bitmap)
{
	unsigned char	*bits;
	BRect			r;
	TColorView		*color_view;

	bits = (unsigned char *)bitmap->Bits();
	r = bitmap->Bounds();
	r.right += 1;
	color_view = ((TIconApp *)be_app)->fColorWind->fView;
	if (color_view->Window()->Lock()) {
		if (modifiers() & B_COMMAND_KEY) {
			if (color_view->GetTheLowColor() != short(bits[(long)(y * r.right + x)]))
				color_view->SetTheLowColor(short(bits[(long)(y * r.right + x)]));
		}
		else {
			if (color_view->GetTheHighColor() != short(bits[(long)(y * r.right + x)]))
				color_view->SetTheHighColor(short(bits[(long)(y * r.right + x)]));
		}
		color_view->Window()->Unlock();
	}
}

//--------------------------------------------------------------------

void TToolView::Fill(short x, short y, short mode, BView *view, 
								uchar *dst_bits, uchar *src_bits)
{
	short		size;
	uchar		color;
	const color_map*	cmap;

	BScreen screen( Window() );
	cmap = screen.ColorMap();
	
	if (view->Window()->Lock()) {
		size = view->Bounds().right + 1;
		memset(src_bits, 0, size * size);

		color = ((TIconApp *)be_app)->fColorWind->fView->GetTheHighColor();
		if (color == B_TRANSPARENT_8_BIT)
			view->SetHighColor(0x77, 0x74, 0x73, 0x72);
		else
			view->SetHighColor(cmap->color_list[color]);
		color = ((TIconApp *)be_app)->fColorWind->fView->GetTheLowColor();
		if (color == B_TRANSPARENT_8_BIT)
			view->SetLowColor(0x77, 0x74, 0x73, 0x72);
		else
			view->SetLowColor(cmap->color_list[color]);
		view->SetPenSize(1);
		switch(mode) {
			case M_MODE_COPY:
				view->SetDrawingMode(B_OP_COPY);
				break;
			case M_MODE_OVER:
				view->SetDrawingMode(B_OP_OVER);
				break;
			case M_MODE_ERASE:
				view->SetDrawingMode(B_OP_ERASE);
				break;
			case M_MODE_INVERT:
				view->SetDrawingMode(B_OP_INVERT);
				break;
			case M_MODE_ADD:
				view->SetDrawingMode(B_OP_ADD);
				break;
			case M_MODE_SUBTRACT:
				view->SetDrawingMode(B_OP_SUBTRACT);
				break;
			case M_MODE_BLEND:
				view->SetDrawingMode(B_OP_BLEND);
				break;
			case M_MODE_MIN:
				view->SetDrawingMode(B_OP_MIN);
				break;
			case M_MODE_MAX:
				view->SetDrawingMode(B_OP_MAX);
				break;
		}
		color = dst_bits[y * size + x];
		RecursiveFill(x, y, color, view, dst_bits, src_bits);
		memcpy(src_bits, dst_bits, size * size);
		view->Window()->Unlock();
	}
}

//--------------------------------------------------------------------

void TToolView::RecursiveFill(short x, short y, uchar color, BView *view,
								uchar *dst_bits, uchar *src_bits)
{
	bool		next;
	short		index = 0;
	short		size;
	last_pos	*pos;
	pattern		*pat;

	size = view->Bounds().right + 1;
	pos = (last_pos *)malloc(size * size * sizeof(last_pos));
	pat = ((TIconApp *)be_app)->fPatWind->fView->GetPatMap(
		  ((TIconApp *)be_app)->fPatWind->fView->GetPat());
	pos[index].x = x;
	pos[index].y = y;
	pos[0].dir = 0;
	for (;;) {
		next = FALSE;
		if (dst_bits[y * size + x] == color) {
			src_bits[y * size + x] = 0xff;
			view->StrokeLine(BPoint(x, y), BPoint(x, y), *pat);
			next = TRUE;
			if ((y > 0) && (!src_bits[(y - 1) * size + x])) {
				pos[index].dir = 1;
				y--;
			}
			else if ((x > 0) && (!src_bits[y * size + x - 1])) {
				pos[index].dir = 2;
				x--;
			}
			else if ((y < (size - 1)) && (!src_bits[(y + 1) * size + x])) {
				pos[index].dir = 3;
				y++;
			}
			else if ((x < (size - 1)) && (!src_bits[y * size + x + 1])) {
				pos[index].dir = 4;
				x++;
			}
			else
				next = FALSE;
		}
		else
			pos[index].dir = 4;
		while (!next) {
			next = TRUE;
			while (pos[index].dir == 4) {
				index--;
				if (index < 0) {
					free(pos);
					view->Sync();
					return;
				}
				x = pos[index].x;
				y = pos[index].y;
			}
			switch (pos[index].dir) {
				case 0:
					pos[index].dir = 1;
					if ((y > 0) && (!src_bits[(y - 1) * size + x])) {
						y--;
						break;
					}

				case 1:
					pos[index].dir = 2;
					if ((x > 0) && (!src_bits[y * size + x - 1])) {
						x--;
						break;
					}

				case 2:
					pos[index].dir = 3;
					if ((y < (size - 1)) && (!src_bits[(y + 1) * size + x])) {
						y++;
						break;
					}

				case 3:
					pos[index].dir = 4;
					if ((x < (size - 1)) && (!src_bits[y * size + x + 1])) {
						x++;
						break;
					}
					else
						next = FALSE;
			}
		}
		index++;
		pos[index].x = x;
		pos[index].y = y;
		pos[index].dir = 0;
	}
}

//--------------------------------------------------------------------

void TToolView::Pen(short x, short y, short last_x, short last_y,
					BView *view, short shape)
{
	const color_map* cmap;
	long			color;

	BScreen screen( Window() );
	cmap = screen.ColorMap();

	if (view->Window()->Lock()) {
		view->SetDrawingMode(B_OP_COPY);
		if (shape == T_PENCIL) {
			color = ((TIconApp *)be_app)->fColorWind->fView->GetTheHighColor();
			if (color == B_TRANSPARENT_8_BIT)
				view->SetHighColor(0x77, 0x74, 0x73, 0x72);
			else
				view->SetHighColor(cmap->color_list[color]);
			view->SetPenSize(1);
		}
		else {
			view->SetHighColor(0x77, 0x74, 0x73, 0x72);
			view->SetPenSize(2);
		}
		view->StrokeLine(BPoint(x, y), BPoint(last_x, last_y));
		view->Sync();
		view->Window()->Unlock();
	}
}


//--------------------------------------------------------------------

void TToolView::Shape(short x, short y, short last_x, short last_y, BView *view,
					  short shape, short pen, short mode, bool frame_it)
{
	bool		frame;
	short		left, top, right, bottom;
	long		color;
	BPoint		leg1, leg2, leg3;
	BRect		r;
	const color_map* cmap;
	pattern*	pat;
	TColorView	*color_view;

	BScreen screen( Window() );
	cmap = screen.ColorMap();
	
	color_view = ((TIconApp *)be_app)->fColorWind->fView;
	frame = TRUE;
  	pat = ((TIconApp *)be_app)->fPatWind->fView->GetPatMap(
			  ((TIconApp *)be_app)->fPatWind->fView->GetPat());
	if (view->Window()->Lock()) {
		color = color_view->GetTheHighColor();
		if (color == B_TRANSPARENT_8_BIT)
			view->SetHighColor(0x77, 0x74, 0x73, 0x72);
		else
			view->SetHighColor(cmap->color_list[color]);
		color = color_view->GetTheLowColor();
		if (color == B_TRANSPARENT_8_BIT)
			view->SetLowColor(0x77, 0x74, 0x73, 0x72);
		else
			view->SetLowColor(cmap->color_list[color]);
		view->SetPenSize(pen);
		switch(mode) {
			case M_MODE_COPY:
				view->SetDrawingMode(B_OP_COPY);
				break;
			case M_MODE_OVER:
				view->SetDrawingMode(B_OP_OVER);
				break;
			case M_MODE_ERASE:
				view->SetDrawingMode(B_OP_ERASE);
				break;
			case M_MODE_INVERT:
				view->SetDrawingMode(B_OP_INVERT);
				break;
			case M_MODE_ADD:
				view->SetDrawingMode(B_OP_ADD);
				break;
			case M_MODE_SUBTRACT:
				view->SetDrawingMode(B_OP_SUBTRACT);
				break;
			case M_MODE_BLEND:
				view->SetDrawingMode(B_OP_BLEND);
				break;
			case M_MODE_MIN:
				view->SetDrawingMode(B_OP_MIN);
				break;
			case M_MODE_MAX:
				view->SetDrawingMode(B_OP_MAX);
				break;
		}

		if (shape == T_LINE)
			if (frame_it)
				view->StrokeLine(BPoint(x, y), BPoint(last_x, last_y));
			else
				view->StrokeLine(BPoint(x, y), BPoint(last_x, last_y), *pat);
		else {
			if (x < last_x) {
				left = x;
				right = last_x;
			}
			else {
				left = last_x;
				right = x;
			}
			if (y < last_y) {
				top = y;
				bottom = last_y;
			}
			else {
				top = last_y;
				bottom = y;
			}
			r.Set(left, top, right, bottom);
	
			if ((shape == T_TRIANGLE) || (shape == T_FILL_TRIANGLE)) {
				leg1 = BPoint(x, y);
				leg2 = BPoint(last_x, y);
				if (((x <= last_x) && (y >= last_y)) || ((x < last_x) && (y < last_y)))
					leg3 = BPoint((last_x - x) / 2 + x, last_y);
				else
					leg3 = BPoint((x - last_x) / 2 + last_x, last_y);
			}
			frame = frame_it;
			switch(shape) {
				case T_FILL_RECT:
					view->FillRect(r, *pat);
					break;
				case T_FILL_RRECT:
					view->FillRoundRect(r, fXRadius, fYRadius, *pat);
					break;
				case T_FILL_OVAL:
					view->FillEllipse(r, *pat);
					break;
				case T_FILL_ARC:
					view->FillArc(r, fStartAngle, fArcAngle, *pat);
					break;
				case T_FILL_TRIANGLE:
					view->FillTriangle(leg1, leg2, leg3, r, *pat);
					break;
			}
	
			if (frame) {
				switch(shape) {
					case T_RECT:
					case T_FILL_RECT:
						if (frame_it)
							view->StrokeRect(r);
						else
							view->StrokeRect(r, *pat);
						break;
					case T_RRECT:
					case T_FILL_RRECT:
						if (frame_it)
							view->StrokeRoundRect(r, fXRadius, fYRadius);
						else
							view->StrokeRoundRect(r, fXRadius, fYRadius, *pat);
						break;
					case T_OVAL:
					case T_FILL_OVAL:
						if (frame_it)
							view->StrokeEllipse(r);
						else
							view->StrokeEllipse(r, *pat);
						break;
					case T_ARC:
					case T_FILL_ARC:
						if (frame_it)
							view->StrokeArc(r, fStartAngle, fArcAngle);
						else
							view->StrokeArc(r, fStartAngle, fArcAngle, *pat);
						break;
					case T_TRIANGLE:
					case T_FILL_TRIANGLE:
						if (frame_it)
							view->StrokeTriangle(leg1, leg2, leg3, r);
						else
							view->StrokeTriangle(leg1, leg2, leg3, r, *pat);
						break;
				}
			}
		}
		view->Sync();
		view->Window()->Unlock();
	}
}


//====================================================================

TParamWindow::TParamWindow(BRect rect, short tool, float *param1, float *param2)
			 :BWindow(rect, "", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	BRect		r;

	r.Set(0, 0, PARAM_WIDTH, PARAM_HEIGHT);
	fView = new TParamView(r, tool, param1, param2);
	Lock();
	AddChild(fView);
	Unlock();
	Show();
}

//--------------------------------------------------------------------

void TParamWindow::MessageReceived(BMessage *msg)
{
	PostMessage(msg, fView);
}


//====================================================================

TParamView::TParamView(BRect rect, short tool, float *param1, float *param2)
		   :BView(rect, "", B_FOLLOW_ALL, B_WILL_DRAW)
{
	rgb_color	c;

	fTool = tool;
	fParam1Value = param1;
	fParam2Value = param2;
	c.red = c.green = c.blue = VIEW_COLOR;
	SetViewColor(c);
	SetFont(be_plain_font);
	SetFontSize(9);
	SetDrawingMode(B_OP_OVER);
}

//--------------------------------------------------------------------

void TParamView::AttachedToWindow(void)
{
	char		string[256];
	BRect		r;

	sprintf(string, "%f", *fParam1Value);
	r.Set(PARAM1_H, PARAM1_V, PARAM1_WIDTH, PARAM1_HEIGHT);
	if (fTool == T_ARC)
		fParam1 = new BTextControl(r, "", PARAM1_TEXT_ARC, string,
							new BMessage(PARAM1));
	else
		fParam1 = new BTextControl(r, "", PARAM1_TEXT_RRECT, string,
							new BMessage(PARAM1));
	AddChild(fParam1);
	fParam1->SetFont(be_plain_font);
	fParam1->SetFontSize(9);
	fParam1->BTextControl::MakeFocus(TRUE);

	sprintf(string, "%f", *fParam2Value);
	r.Set(PARAM2_H, PARAM2_V, PARAM2_WIDTH, PARAM2_HEIGHT);
	if (fTool == T_ARC)
		fParam2 = new BTextControl(r, "", PARAM2_TEXT_ARC, string,
							new BMessage(PARAM2));
	else
		fParam2 = new BTextControl(r, "", PARAM2_TEXT_RRECT, string,
							new BMessage(PARAM2));
	AddChild(fParam2);
	fParam2->SetFont(be_plain_font);
	fParam2->SetFontSize(9);

	r.Set(BUTTON1_H, BUTTON1_V,
		  BUTTON1_H + BUTTON1_WIDTH,
		  BUTTON1_V + BUTTON1_HEIGHT);
	fOK = new BButton(r, "", BUTTON1_TEXT, new BMessage(OK));
	AddChild(fOK);

	r.Set(BUTTON2_H, BUTTON2_V,
		  BUTTON2_H + BUTTON2_WIDTH,
		  BUTTON2_V + BUTTON2_HEIGHT);
	fCancel = new BButton(r, "", BUTTON2_TEXT, new BMessage(OK));
	AddChild(fCancel);

	fOK->MakeDefault(TRUE);
}

//--------------------------------------------------------------------

void TParamView::Draw(BRect where)
{
	BRect	r;

	r = Bounds();

	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top));
	StrokeLine(BPoint(r.left, r.top + 1), BPoint(r.left, r.bottom - 1));
	StrokeLine(BPoint(BREAK_H, BREAK_V + 1), BPoint(BREAK_WIDTH, BREAK_V + 1));
	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.right - 1, r.bottom - 1), BPoint(r.left, r.bottom - 1));
	StrokeLine(BPoint(BREAK_H, BREAK_V), BPoint(BREAK_WIDTH, BREAK_V));

	SetHighColor(0, 0, 0);
	MovePenTo(TEXT_H, TEXT_V);
	if (fTool == T_ARC)
		DrawString(TEXT_ARC);
	else
		DrawString(TEXT_RRECT);
}

//--------------------------------------------------------------------

void TParamView::MessageReceived(BMessage *msg)
{
	BMessage	*message;

	switch (msg->what) {
		case PARAM1:
		case PARAM2:
			break;

		case OK:
			*fParam1Value = atof(fParam1->Text());
			*fParam2Value = atof(fParam2->Text());
			// will fall through
		case CANCEL:
			Window()->Quit();
			break;
	}
}
