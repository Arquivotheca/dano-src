//--------------------------------------------------------------------
//	
//	TrackView.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifndef TRACK_VIEW_H
#include "TrackView.h"
#endif


//====================================================================

TTrackView::TTrackView(BRect rect, char *title, scsi_toc *toc, BList *list,
					   BWindow *wind, int cd_id)
	   :BView(rect, title, B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED)
{
	fTOC = toc;
	fList = list;
	fWindow = wind;
	fCDID = cd_id;
}

//--------------------------------------------------------------------

TTrackView::~TTrackView(void)
{
	if (fTextView)
		KillEditField(true);
}

//--------------------------------------------------------------------

void TTrackView::AttachedToWindow(void)
{
	fTextView = NULL;
	fTrack = 0;
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetFont(be_plain_font);
}

//--------------------------------------------------------------------

void TTrackView::Draw(BRect where)
{
	int32		index = 1;
	BRect		r;
	track_info	*track_data;

	r = Bounds();
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top));
	StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.bottom));
	SetHighColor(184, 184, 184);
	StrokeLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.left + 1, r.bottom), BPoint(r.right, r.bottom));

	SetHighColor(0, 0, 0);
	SetDrawingMode(B_OP_OVER);
	MovePenTo(TITLE_POS + 3, HEADER - 4);
	DrawString("Title");
	MovePenTo(TIME_POS + 3, HEADER - 4);
	DrawString("Time");
	SetDrawingMode(B_OP_COPY);
	fTracks = 0;
	while (track_data = (track_info*)fList->ItemAt(index)) {
		fTracks++;
		SetHighColor(128, 128, 128);
		StrokeLine(BPoint(MARGIN, HEADER + ((index - 1) * CELL_HEIGHT)),
				   BPoint(MARGIN + CELL_WIDTH, HEADER + ((index - 1) * CELL_HEIGHT)));
		StrokeLine(BPoint(MARGIN, HEADER + ((index - 1) * CELL_HEIGHT)),
				   BPoint(MARGIN, HEADER + index * CELL_HEIGHT));
		SetHighColor(255, 255, 255);
		StrokeLine(BPoint(MARGIN + 1, HEADER + index * CELL_HEIGHT - 1),
				   BPoint(MARGIN + CELL_WIDTH, HEADER + index * CELL_HEIGHT - 1));
		StrokeLine(BPoint(MARGIN + CELL_WIDTH, HEADER + ((index - 1) * CELL_HEIGHT + 1)),
				   BPoint(MARGIN + CELL_WIDTH, HEADER + index * CELL_HEIGHT - 1));
		if (fTracks == fTrack)
			DrawTrack(index, true);
		else
			DrawTrack(index, false);
		index++;
	}

	if (0) { //fTextView) {
		r.Set(TITLE_POS - 1, HEADER + (CELL_HEIGHT * fField) + 1,
			  TIME_POS - 8, HEADER + (CELL_HEIGHT * (fField + 1) - 3));
		if (r.Intersects(where)) {
			r.Set(where.left - (TITLE_POS - 1),
				  where.top - (HEADER + (CELL_HEIGHT * fField) + 1),
				  where.right - (TIME_POS - 8),
				  where.bottom - (HEADER + (CELL_HEIGHT * (fField + 1) - 3)));
			fTextView->FillRect(r, B_SOLID_LOW);
			fTextView->Draw(r);
		}
	}
}

//--------------------------------------------------------------------

void TTrackView::MouseDown(BPoint thePoint)
{
	int32	index;
	BRect	r;
	BRect	r1;

	r1 = Bounds();
	r.Set(TITLE_POS, HEADER, TIME_POS - 8, r1.bottom - TRAILER);
	if (r.Contains(thePoint)) {
		fWindow->PostMessage(KILL_TITLE_SAVE);
		index = (thePoint.y - HEADER) / CELL_HEIGHT;
		if (fTextView)
			KillEditField(true);
		OpenEditField(index);
	}
}

//--------------------------------------------------------------------

void TTrackView::Pulse(void)
{
	int32				position;
	BRect				r;
	scsi_position		pos;

	if (fCDID) {
		ioctl(fCDID, B_SCSI_GET_POSITION, &pos);
		if ((pos.position[1] == 0x11) || (pos.position[1] == 0x12)) {
			if ((fTrack) && (fTrack != pos.position[6]))
				DrawTrack(fTrack, false);
			if ((pos.position[6]) && (fTrack != pos.position[6]))
				DrawTrack(pos.position[6], true);
		}
		else if (fTrack) {
			DrawTrack(fTrack, false);
			fTrack = 0;
		}
	}
}

//--------------------------------------------------------------------

void TTrackView::DrawTrack(int32 track, bool selected)
{
	char		str[256];
	char		*string;
	const char	*src[1];
	char		*result[1];
	int32		length;
	BFont		font;
	BRect		r;
	track_info	*track_data;

	track_data = (track_info*)fList->ItemAt(track);
	r.Set(MARGIN + 1, HEADER + ((track - 1) * CELL_HEIGHT) + 1,
		  MARGIN + CELL_WIDTH - 1, HEADER + (track * CELL_HEIGHT) - 2);
	if (selected) {
		fTrack = track;
		SetHighColor(136, 136, 136);
	}
	else
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(r);
	SetDrawingMode(B_OP_OVER);
	SetHighColor(0, 0, 0);
	MovePenTo(TRACK_POS, HEADER + track * CELL_HEIGHT - 5);
	sprintf(str, "%d", track);
	DrawString(str);
	MovePenTo(TITLE_POS, HEADER + track * CELL_HEIGHT - 5);
	GetFont(&font);

	src[0] = track_data->title;
	string = (char *)malloc(strlen(track_data->title) + 16);
	result[0] = string;
	font.GetTruncatedStrings(src, 1, B_TRUNCATE_END, (TIME_POS - TITLE_POS) - 8, result);

	DrawString(string);
	free(string);

	length = (fTOC->toc_data[4 + (track * 8) + 5] * 60) +
			 (fTOC->toc_data[4 + (track * 8) + 6]);
	length -= ((fTOC->toc_data[4 + ((track - 1) * 8) + 5] * 60) +
			   (fTOC->toc_data[4 + ((track - 1) * 8) + 6]));
	sprintf(str, "%.2d:%.2d", length / 60, length % 60);
	MovePenTo(TIME_POS, HEADER + track * CELL_HEIGHT - 5);
	DrawString(str);
	SetDrawingMode(B_OP_COPY);
}

//--------------------------------------------------------------------

void TTrackView::KillEditField(bool save)
{
	const char	*text;
	int32		length;
	int32		loop;
	track_info	*track_data;

	if (fTextView) {
		RemoveChild(fTextView);
		if (save) {
			track_data = (track_info*)(fList->ItemAt(fField + 1));
			if (track_data) {
				text = fTextView->Text();
				length = fTextView->TextLength();
				for (loop = 0; loop < length; loop++)
					track_data->title[loop] = *text++;
				track_data->title[loop] = 0;
				track_data->flags |= DIRTY;
				fWindow->PostMessage(SAVE_DATA);
			}
		}
		delete fTextView;
		fTextView = NULL;
	}
}

//--------------------------------------------------------------------

void TTrackView::NextField(bool prev)
{
	int32	field;

	field = fField;
	if (fTextView)
		KillEditField(true);
	if (prev) {
		field--;
		if (field < 0)
			field = fTracks - 1;
	}
	else {
		field++;
		if (field == fTracks)
			field = 0;
	}
	OpenEditField(field);
}

//--------------------------------------------------------------------

void TTrackView::OpenEditField(int32 field)
{
	BRect		r;
	BRect		dr;
	track_info	*track_data;

	fField = field;
	r.Set(TITLE_POS - 1, HEADER + (CELL_HEIGHT * fField) + 1,
		  TIME_POS - 8, HEADER + (CELL_HEIGHT * (fField + 1) - 3));
	dr = r;
	dr.OffsetTo(0, 0);
	fTextView = new TEditTextView(r, dr);
	AddChild(fTextView);
	track_data = (track_info*)(fList->ItemAt(fField + 1));
	if (track_data)
		fTextView->SetText(track_data->title);
	fTextView->SelectAll();
	fTextView->SetMaxBytes(sizeof(track_data->title) - 1);
}
