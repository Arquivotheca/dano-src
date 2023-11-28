//--------------------------------------------------------------------
//	
//	SaveWindow.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <byteorder.h>
#include <MenuField.h>
#include <MediaDefs.h>
#include <ParameterWeb.h>
#include <String.h>
#include <MediaTrack.h>

#ifndef SAVE_WINDOW_H
#include "SaveWindow.h"
#endif

extern bool		changed;
extern int32	track;
extern bool		mute;
extern int32	format;
extern bool		quitting;

#define min(a,b) (((a)<(b))?(a):(b))

class ConfigWindow: public BWindow
{
	public:
		ConfigWindow(BView *view, BWindow *parent)
			: BWindow(view->Bounds(),"Encoder settings",
				B_FLOATING_WINDOW_LOOK,B_MODAL_SUBSET_WINDOW_FEEL,
				B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
		{
			AddChild(view);
			AddToSubset(parent);
			MoveTo(parent->Frame().LeftTop());
			Show();
		}
};


TSaveWindow::TSaveWindow(BRect rect, char *title, int32 cd_id,
						 scsi_toc *toc, int32 track, BList* list)
			:BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect		r;

	fTOC = toc;
	fTrack = track;
	fTitleList = list;
	fCDID = cd_id;

	r.Set(PLAY_BUTTON_H, PLAY_BUTTON_V,
		  PLAY_BUTTON_H + SAVE_BUTTON_WIDTH, PLAY_BUTTON_V + SAVE_BUTTON_HEIGHT);
	fPlayButton = new BButton(r, "play", "Preview", new BMessage(PLAY_BUT));
	AddChild(fPlayButton);

	r.Set(STOP_BUTTON_H, STOP_BUTTON_V,
		  STOP_BUTTON_H + SAVE_BUTTON_WIDTH, STOP_BUTTON_V + SAVE_BUTTON_HEIGHT);
	fStopButton = new BButton(r, "stop", "Stop", new BMessage(STOP_BUT));
	AddChild(fStopButton);
	Lock();
	fStopButton->SetEnabled(false);
	Unlock();

	r.Set(SAVE_BUTTON_H, SAVE_BUTTON_V,
		  SAVE_BUTTON_H + SAVE_BUTTON_WIDTH, SAVE_BUTTON_V + SAVE_BUTTON_HEIGHT);
	fSaveButton = new BButton(r, "save", "Save...", new BMessage(SAVE_BUT));
	AddChild(fSaveButton);

	r.Set(0, 0, SAVE_WIDTH, SAVE_HEIGHT);
	fSaveView = new TSaveView(r, "save", this, fCDID);
	AddChild(fSaveView);
	SetPulseRate(100000);
	InitSaveFormats();
}

TSaveWindow::~TSaveWindow(void)
{
	// Delete cached save formats
	for (int32 i = 0; i < saveFormats->CountItems(); i++) {
		delete (save_format *)saveFormats->ItemAt(i);
	}
	delete saveFormats;
}

status_t TSaveWindow::InitSaveFormats(void)
{
	saveFile = NULL;
	saveTrack = NULL;
	saveFormats = new BList();
	
	media_file_format mff;
	media_codec_info mci;
	media_format in_f, out_f;
	int32 cookie = 0, cookie2;

	in_f.type = B_MEDIA_RAW_AUDIO;
	in_f.u.raw_audio = media_raw_audio_format::wildcard;
	while (get_next_file_format(&cookie, &mff) == B_OK) {
		// Skip video formats like AVI & QuickTime
		if (mff.capabilities & (media_file_format::B_KNOWS_RAW_VIDEO |
								media_file_format::B_KNOWS_ENCODED_VIDEO)) {
			continue;
		}
		
		cookie2 = 0;
		while (get_next_encoder(&cookie2, &mff, &in_f, &out_f, &mci) == B_OK) {
			save_format *sf = new save_format;
			sf->name << mff.short_name << "/" << mci.pretty_name;
			sf->file_format = mff;
			sf->codec_info = mci;
			saveFormats->AddItem(sf);
			if (mff.family == B_WAV_FORMAT_FAMILY && out_f.type == B_MEDIA_RAW_AUDIO)
				defaultSaveFormat = sf;
		}
	}
}

void TSaveWindow::ConfigureCodec()
{
	BMenuItem *mi = panelFormatMenu->FindMarked();
	if(!mi)
		return;
	static int32 lastindex=-1;
	int32 index=panelFormatMenu->IndexOf(mi);

	if(index!=lastindex || !saveFile)
	{
		lastindex=index;
		if(saveFile)
			delete saveFile;
		save_format *sf = (save_format *)saveFormats->ItemAt(index);
		saveFile=new BMediaFile(&sf->file_format);
		media_format format;
		saveTrack=saveFile->CreateTrack(&format, &sf->codec_info);
	}

	if(saveTrack)
	{
		BView *webview = saveTrack->GetParameterView();
		if(webview)
		{
			// show view in window, for user-configuration
			new ConfigWindow(webview,filePanel->Window());
		}
		return; // don't close file!!
	}
	delete saveFile;
	saveFile=NULL;
	saveTrack=NULL;
}


void TSaveWindow::MessageReceived(BMessage* message)
{
	char				name[B_FILE_NAME_LENGTH];
	int32				i;
	int32				time;
	BMessenger			window(this);
	scsi_play_position	audio;
	track_info			*track_data;

	switch(message->what) {
		case 'conf':
			ConfigureCodec();
			break;

		case PLAY_BUT:
			time = fSaveView->fTrackStart + fSaveView->fStart;
			audio.start_m = time / (60 * 75);
			time %= (60 * 75);
			audio.start_s = time / 75;
			audio.start_f = time % 75;

			time = fSaveView->fTrackStart + fSaveView->fStop;
			audio.end_m = time / (60 * 75);
			time %= (60 * 75);
			audio.end_s = time / 75;
			audio.end_f = time % 75;

			if ((audio.start_m == audio.end_m) &&
				(audio.start_s == audio.end_s) &&
				(audio.start_f == audio.end_f))
				ioctl(fCDID, B_SCSI_STOP_AUDIO);
			else
				ioctl(fCDID, B_SCSI_PLAY_POSITION, &audio);
			fSaveView->Pulse();
			break;

		case SAVE_BUT:
		{
			track_data = (track_info*)fTitleList->ItemAt(fTrack);
			filePanel = new BFilePanel(B_SAVE_PANEL, &window);
			strncpy(name, track_data->title, B_FILE_NAME_LENGTH);
			i = 0;
			while(name[i]) {
				if (name[i] == '/')
					name[i] = '-';
				i++;
			}
			filePanel->SetSaveText(name);

			// Add a menu to the file panel to allow the user to select
			// the file format to save in.
			panelFormatMenu = new BMenu("File Format");
			panelFormatMenu->SetRadioMode(true);
			for (int32 i = 0; i < saveFormats->CountItems(); i++) {
				save_format *sf = (save_format *)saveFormats->ItemAt(i);
				BString label(sf->name);
		
				BMediaFile file(&sf->file_format);
				media_format format;
				BMediaTrack *track=file.CreateTrack(&format, &sf->codec_info);
				if(track)
				{
					BView *webview = track->GetParameterView();
					if(webview)
					{
						label << B_UTF8_ELLIPSIS;
						delete webview;
					}
					file.ReleaseTrack(track);
				}
				BMenuItem *menuitem;
				panelFormatMenu->AddItem(menuitem=new BMenuItem(label.String(), new BMessage('conf')));
				if	(defaultSaveFormat->file_format.id==sf->file_format.id &&
					 defaultSaveFormat->codec_info.id==sf->codec_info.id)
					menuitem->SetMarked(true);
			}
			panelFormatMenu->SetTargetForItems(this);
			
			BMenuBar *mb = dynamic_cast<BMenuBar *>(filePanel->Window()->FindView("MenuBar"));
			if (mb)
				mb->AddItem(panelFormatMenu);

			filePanel->Window()->Show();
		}
		break;

		case STOP_BUT:
			if (fCDID) {
				ioctl(fCDID, B_SCSI_STOP_AUDIO);
				fSaveView->Pulse();
			}
			break;

		case B_SAVE_REQUESTED:
			SaveRequested(message);
			break;

		default:
			if ((message->what > 99) && (message->what < 255)) {
				i = message->what - 100;
				fSaveView->fTrackStart = (fTOC->toc_data[4 + (i * 8) + 5] * 60 * 75) +
										 (fTOC->toc_data[4 + (i * 8) + 6] * 75) +
										 (fTOC->toc_data[4 + (i * 8) + 7]);
				fSaveView->fStart = 0;
				fSaveView->fTrackLength = ((fTOC->toc_data[4 + ((i + 1) * 8) + 5] * 60 * 75) +
										   (fTOC->toc_data[4 + ((i + 1) * 8) + 6] * 75) +
										   (fTOC->toc_data[4 + ((i + 1) * 8) + 7])) -
										   fSaveView->fTrackStart;
				fSaveView->fStop = fSaveView->fTrackLength;
				
				fSaveView->DrawSelector();
				fTrack = i + 1;
			}
			else
				inherited::MessageReceived(message);
	}
}

bool TSaveWindow::QuitRequested(void)
{
	if (quitting)
		return true;
	else {
		Hide();
		return false;
	}
}

void TSaveWindow::SaveRequested(BMessage *msg)
{
	bool			error = false;
	const char		*name = NULL;
	int32			length;
	BDirectory		directory;
	BFile			*file;
	BNodeInfo		*node;
	BRect			r;
	BRect			wind_rect;
	entry_ref		dir;
	TStatusWindow	*statusWindow;

	if (msg->FindRef("directory", &dir) == B_NO_ERROR) {
		msg->FindString("name", &name);
		directory.SetTo(&dir);
		if (directory.InitCheck() == B_NO_ERROR) 
		{
			file = new BFile();
			directory.CreateFile(name, file);
			if (file->InitCheck() == B_NO_ERROR) 
			{
				media_file_format		fileFormat;
				memset(&fileFormat,0,sizeof(fileFormat));
				media_codec_info		codecInfo;
				memset(&codecInfo,0,sizeof(codecInfo));
				if (saveFile) {
					// copy format of preconfigured file
					if(B_OK!=saveFile->GetFileFormatInfo(&fileFormat))
						printf("GetFileFormatInfo failed\n");
					if(B_OK!=saveTrack->GetCodecInfo(&codecInfo))
						printf("GetCodecInfo failed\n");
				} else {
					fileFormat = defaultSaveFormat->file_format;
					codecInfo = defaultSaveFormat->codec_info;
				}
				// Set the MIME type of the target file
				BNodeInfo 	info(file);
				info.SetType(fileFormat.mime_type);
				
				BMediaFile *dstFile = NULL;
				BMediaTrack	*dstTrack = NULL;
				if( (dstFile = new BMediaFile( file, &fileFormat ))->InitCheck() != B_OK )
				{
					printf("mediafile error %08x (%s)\n",dstFile->InitCheck(),strerror(dstFile->InitCheck()));
					throw "Dest file failed InitCheck";
				}

				media_format format;
				memset(&format,0,sizeof(format));
				format.type = B_MEDIA_RAW_AUDIO;
				format.u.raw_audio.frame_rate = 44100;
				format.u.raw_audio.channel_count = 2;
				format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
				format.u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	
				if( !(dstTrack = dstFile->CreateTrack( &format, &codecInfo )) )
				{
					printf("mediatrack error %08x (%s)\n",dstTrack->InitCheck(),strerror(dstTrack->InitCheck()));
					throw "Could not create dest track";
				}

				if (saveFile)
				{
					// copy parameters from template BMediaFile
					BParameterWeb *srcweb;
					if(	B_OK==saveTrack->GetParameterWeb(&srcweb) && srcweb)
					{
						BParameterWeb *dstweb=NULL;
						if(B_OK==dstTrack->GetParameterWeb(&dstweb))
						{
							char buffer[10000];
							size_t howmuch=sizeof(buffer);
							if(B_OK==srcweb->MakeParameterData(buffer,&howmuch))
								dstweb->ApplyParameterData(buffer,howmuch);
							else
								printf("error making paramaterdata\n");
							delete dstweb;
						}
						else
							printf("no destination web\n");
						delete srcweb;
					}
					else
						printf("no source web\n");
				}

				wind_rect = Frame();
				r.left = wind_rect.left + ((wind_rect.Width() - STATUS_WIDTH) / 2);
				r.right = r.left + STATUS_WIDTH;
				r.top = wind_rect.top + ((wind_rect.Height() - STATUS_HEIGHT) / 2);
				r.bottom = r.top + STATUS_HEIGHT;

				dstFile->AddCopyright("Copyright 1999 Be Incorporated");
				dstFile->CommitHeader();
				statusWindow = new TStatusWindow(r, "Save Progress", dstFile, dstTrack,
									fSaveView->fTrackStart, fSaveView->fStart,
									fSaveView->fStop, fCDID);
			}
			else {
				error = true;
				delete file;
			}
		}
		else
			error = true;
	}
	else
		error = true;

	if (error)
		(new BAlert("", "Error creating file.", "Sorry"))->Go();
}

void TSaveWindow::ShowTrack(int32 track)
{
	fTrack = track;
	ResetMenu();
	BWindow::Show();
}

void TSaveWindow::ResetMenu(void)
{
	char		text[100];
	int32		i = 0;
	int32		track_num;
	BMenuItem	*mitem;
	track_info	*track_data;

	if (fSaveView->fTrackMenu) {
		while (mitem = fSaveView->fTrackMenu->RemoveItem((int32)0)) {
			delete mitem;
		}
		while (fTOC->toc_data[4 + (i * 8) + 2] != 0xaa) {
			track_num = fTOC->toc_data[4 + (i * 8) + 2];
			track_data = (track_info*)fTitleList->ItemAt(track_num);
			if (track_data)
				fSaveView->fTrackMenu->AddItem(mitem = new BMenuItem(
					track_data->title, new BMessage(100 + i)));
			else {
				sprintf(text, "Track %d", track_num);
				fSaveView->fTrackMenu->AddItem(mitem = new BMenuItem(text, new BMessage(100 + i)));
			}
			if (fTrack == fTOC->toc_data[4 + (i * 8) + 2]) {
				fSaveView->fTrackStart = (fTOC->toc_data[4 + (i * 8) + 5] * 60 * 75) +
										 (fTOC->toc_data[4 + (i * 8) + 6] * 75) +
										 (fTOC->toc_data[4 + (i * 8) + 7]);
				fSaveView->fStart = 0;
				fSaveView->fTrackLength = ((fTOC->toc_data[4 + ((i + 1) * 8) + 5] * 60 * 75) +
										   (fTOC->toc_data[4 + ((i + 1) * 8) + 6] * 75) +
										   (fTOC->toc_data[4 + ((i + 1) * 8) + 7])) -
										   fSaveView->fTrackStart;
				fSaveView->fStop = fSaveView->fTrackLength;
				mitem->SetMarked(true);
			}
			i++;
		}
	}
}


TSaveView::TSaveView(BRect rect, char *title, TSaveWindow* wind, int32 cd_id)
		  :BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	BMenuField	*mf;
	BMenuItem	*mitem;
	BRect		r;
	rgb_color	c;

	fLastx = -1;
	fWindow = wind;
	fCDID = cd_id;
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);

	r = Bounds();
	r.left += SELECTOR_H;
	r.right -= SELECTOR_H;
	r.top = SELECTOR_V;
	r.bottom = SELECTOR_V + SELECTOR_HEIGHT;
	r.OffsetTo(0, 0);
	fOffScreen = new BBitmap(r, B_COLOR_8_BIT, true);
	fOffScreen->AddChild(fOffView = new BView(r, "", B_FOLLOW_ALL, B_WILL_DRAW));

	r.Set(TRACK_MENU_H, TRACK_MENU_V,
		  TRACK_MENU_H + TRACK_MENU_WIDTH, TRACK_MENU_V + TRACK_MENU_HEIGHT);
	fTrackMenu = new BPopUpMenu("Tracks");
	fTrackMenu->SetRadioMode(true);
	fTrackMenu->SetLabelFromMarked(true);
	mf = new BMenuField(r, "tracks", "Track:", fTrackMenu);
	mf->SetDivider(mf->StringWidth(mf->Label()) + 6.0);
	AddChild(mf);
}

TSaveView::~TSaveView(void)
{
	delete fOffScreen;
}

void TSaveView::AttachedToWindow(void)
{
}

void TSaveView::Draw(BRect where)
{
	BRect	r;
	BRect	r1;
	BRect	r2;

	r1.Set(0, 0, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT);
	r = Frame();

	r2.Set(PLAY_BUTTON_H, PLAY_BUTTON_V,
		   PLAY_BUTTON_H + SAVE_BUTTON_WIDTH, PLAY_BUTTON_V + SAVE_BUTTON_HEIGHT);
	if (r2.Intersects(where))
		fWindow->fPlayButton->Draw(r1);

	r2.Set(STOP_BUTTON_H, STOP_BUTTON_V,
		   STOP_BUTTON_H + SAVE_BUTTON_WIDTH, STOP_BUTTON_V + SAVE_BUTTON_HEIGHT);
	if (r2.Intersects(where))
		fWindow->fStopButton->Draw(r1);

	r2.Set(SAVE_BUTTON_H, SAVE_BUTTON_V,
		   SAVE_BUTTON_H + SAVE_BUTTON_WIDTH, SAVE_BUTTON_V + SAVE_BUTTON_HEIGHT);
	if (r2.Intersects(where))
		fWindow->fSaveButton->Draw(r1);

	// Window shading
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top));
	StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.bottom));
	StrokeLine(BPoint(r.left + 8, DIVIDER + 1), BPoint(r.right - 8, DIVIDER + 1));
	SetHighColor(152, 152, 152);
	StrokeLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.left + 1, r.bottom), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.left + 8, DIVIDER), BPoint(r.right - 8, DIVIDER));

	SetHighColor(0, 0, 0);
	SetFont(be_plain_font);
	SetDrawingMode(B_OP_OVER);

    const rgb_color kWhite  = {255, 255, 255, 255};
    const rgb_color kDark   = {100, 100, 100, 255};
	r.Set(STOP_BUTTON_H + SAVE_BUTTON_WIDTH + 10, STOP_BUTTON_V + 1,
		  STOP_BUTTON_H + SAVE_BUTTON_WIDTH + 10 + 1, STOP_BUTTON_V + SAVE_BUTTON_HEIGHT);
	BeginLineArray(2);
	AddLine(r.LeftTop(), r.LeftBottom(), kDark);
	AddLine(r.RightTop(), r.RightBottom(), kWhite);
	EndLineArray();

	r = Bounds();
	r.left += 3;
	r.right -= 3;
	r.top = SELECTOR_V;
	r.bottom = SELECTOR_V + SELECTOR_HEIGHT;
	if (r.Intersects(where))
		DrawSelector();
}

void TSaveView::MouseDown(BPoint thePoint)
{
	bool	tracking_start = false;
	uint32	buttons;
	float	temp;
	float	unit;
	float	width;
	float	offset;
	BPoint	where;
	BRect	back;
	BRect	track;

	back = Bounds();
	back.left += 8;
	back.right -= 8;
	back.top = SELECTOR_V + 15;
	back.bottom = back.top + 7;

	unit = (back.right - back.left) / fTrackLength;
	track = back;
	track.left += (fStart * unit) - 5;
	track.right = track.left + 10;
	track.bottom += 14;
	if ((track.Contains(thePoint)) || ((back.Contains(thePoint)) &&
		(thePoint.x < track.left))) {	
		tracking_start = true;
		goto track_mouse;
	}

	track.left = back.left + (fStop * unit) - 5;
	track.right = track.left + 10;
	if ((track.Contains(thePoint)) || ((back.Contains(thePoint)) &&
		(thePoint.x > track.right)))
		goto track_mouse;
	else
		return;

track_mouse:
	if (track.Contains(thePoint))
		offset = thePoint.x - (track.left + 5);
	else
		offset = 0.0;
	do {
		GetMouse(&where, &buttons);
		where.x -= offset;
		if (where.x < back.left)
			where.x = back.left;
		if (where.x > back.right)
			where.x = back.right;
		temp = (where.x - back.left) * (1 / unit);
		if (tracking_start) {
			if (temp > fStop)
				temp = fStop;
			if ((int32)temp != (int32)fStart) {
				fStart = (int32)temp;
				DrawSelector();
			}
		}
		else {
			if (temp < fStart)
				temp = fStart;
			if ((int32)temp != (int32)fStop) {
				fStop = (int32)temp;
				DrawSelector();
			}
		}
		Pulse();
	} while (buttons);
}

void TSaveView::Pulse(void)
{
	int32				position;
	float				unit;
	BRect				back;
	scsi_position		pos;

	if ((fCDID) && (!Window()->IsHidden())) {
		if (changed) {
			changed = false;
			fWindow->fTrack = track;
			fWindow->ResetMenu();
			DrawSelector();
		}

		ioctl(fCDID, B_SCSI_GET_POSITION, &pos);
		if (pos.position[1] == 0x11) {
			if (!fWindow->fStopButton->IsEnabled()) {
				fWindow->Lock();
				fWindow->fStopButton->SetEnabled(true);
				fWindow->Unlock();
			}
		}
		else if (fWindow->fStopButton->IsEnabled()) {
			fWindow->Lock();
			fWindow->fStopButton->SetEnabled(false);
			fWindow->Unlock();
		}

		if (((pos.position[1] == 0x11) ||
			((pos.position[1] == 0x12) && (pos.position[6]))) &&
			 (pos.position[6] == fWindow->fTrack)) {
			back = Bounds();
			back.left += 8;
			back.right -= 8;

			unit = (back.right - back.left) / fTrackLength;
			position = (pos.position[13] * 60 * 75) + (pos.position[14] * 75) +
					pos.position[15];
			if (position != fPos) {
				fPos = position;
				position = (int32)(position * unit);
				fLastx = (int32)position;
				DrawSelector();
			}
		}
		else if (fLastx >= 0) {
			fLastx = -1;
			DrawSelector();
		}
	}
}

void TSaveView::DrawSelector(void)
{
	char	text[256];
	int32	min;
	int32	sec;
	int32	frame;
	int32	time;
	float	start_width;
	float	stop_width;
	float	unit;
	float	l;
	BRect	sr;
	BRect	dr;

	fOffScreen->Lock();
	fOffView->SetFont(be_plain_font);
	fOffView->SetFontSize(9);
	sr = fOffView->Bounds();
	fOffView->SetHighColor(216, 216, 216);
	fOffView->FillRect(sr);

	fOffView->SetHighColor(128, 128, 128);
	fOffView->StrokeLine(BPoint(sr.left + 4, sr.top + 14),
						 BPoint(sr.right - 5, sr.top + 14));
	fOffView->StrokeLine(BPoint(sr.left + 4, sr.top + 14),
						 BPoint(sr.left + 4, sr.top + 23));
	fOffView->SetHighColor(255, 255, 255);
	fOffView->StrokeLine(BPoint(sr.left + 5, sr.top + 23),
						 BPoint(sr.right - 4, sr.top + 23));
	fOffView->StrokeLine(BPoint(sr.right - 4, sr.top + 23),
						 BPoint(sr.right - 4, sr.top + 14));
	dr.left = sr.left + 5;
	dr.top = sr.top + 15;
	dr.right = sr.right - 5;
	dr.bottom = dr.top + 7;
	fOffView->SetHighColor(184, 184, 184);
	fOffView->FillRect(dr);

	unit = (dr.right - dr.left) / fTrackLength;
	dr.right = dr.left + (fStop * unit);
	dr.left += fStart * unit;
	fOffView->SetHighColor(51, 0, 203);
	fOffView->SetHighColor(102, 152, 203);
	fOffView->FillRect(dr);

	fOffView->SetHighColor(80, 80, 80);
	// Draw start indicator outline
	fOffView->StrokeLine(BPoint(dr.left, dr.top + 6),
						 BPoint(dr.left - 5, dr.top + 11));
	fOffView->StrokeLine(BPoint(dr.left - 5, dr.top + 11),
						 BPoint(dr.left + 5, dr.top + 11));
	fOffView->StrokeLine(BPoint(dr.left + 5, dr.top + 11),
						 BPoint(dr.left, dr.top + 6));

	// Draw stop indicator outline
	fOffView->StrokeLine(BPoint(dr.right, dr.top + 6),
						 BPoint(dr.right - 5, dr.top + 11));
	fOffView->StrokeLine(BPoint(dr.right - 5, dr.top + 11),
						 BPoint(dr.right + 5, dr.top + 11));
	fOffView->StrokeLine(BPoint(dr.right + 5, dr.top + 11),
						 BPoint(dr.right, dr.top + 6));

	// Draw current position indicator outline
	if (fLastx >= 0) {
		fOffView->StrokeLine(BPoint(5 + fLastx, dr.top + 1),
							 BPoint(5 + fLastx - 5, dr.top - 4));
		fOffView->StrokeLine(BPoint(5 + fLastx - 5, dr.top - 4),
							 BPoint(5 + fLastx + 5, dr.top - 4));
		fOffView->StrokeLine(BPoint(5 + fLastx + 5, dr.top - 4),
							 BPoint(5 + fLastx, dr.top + 1));
	}

	// Fill start indicator
	fOffView->SetHighColor(0, 255, 0);
	fOffView->StrokeLine(BPoint(dr.left, dr.top + 7),
						 BPoint(dr.left, dr.top + 7));
	fOffView->StrokeLine(BPoint(dr.left - 1, dr.top + 8),
						 BPoint(dr.left + 1, dr.top + 8));
	fOffView->StrokeLine(BPoint(dr.left - 2, dr.top + 9),
						 BPoint(dr.left + 2, dr.top + 9));
	fOffView->StrokeLine(BPoint(dr.left - 3, dr.top + 10),
						 BPoint(dr.left + 3, dr.top + 10));

	// Draw start time
	fOffView->SetHighColor(0, 105, 0);
	time = fStart;
	min = time / (60 * 75);
	time %= (60 * 75);
	sec = time / 75;
	time %= 75;
	frame = (int32) ((time / 75.0) * 1000.0);
	sprintf(text, "%.2d:%.2d.%.3d", min, sec, frame);
	start_width = fOffView->StringWidth(text);
	if (((dr.left - 4) + start_width) > sr.right)
		l = sr.right - start_width;
	else
		l = dr.left - 4;
	fOffView->MovePenTo(l, dr.top + 23);
	fOffView->SetDrawingMode(B_OP_OVER);
	fOffView->DrawString(text);
	fOffView->SetDrawingMode(B_OP_COPY);

	// Fill stop indicator
	fOffView->SetHighColor(255, 0, 0);
	fOffView->StrokeLine(BPoint(dr.right, dr.top + 7),
						 BPoint(dr.right, dr.top + 7));
	fOffView->StrokeLine(BPoint(dr.right - 1, dr.top + 8),
						 BPoint(dr.right + 1, dr.top + 8));
	fOffView->StrokeLine(BPoint(dr.right - 2, dr.top + 9),
						 BPoint(dr.right + 2, dr.top + 9));
	fOffView->StrokeLine(BPoint(dr.right - 3, dr.top + 10),
						 BPoint(dr.right + 3, dr.top + 10));

	// Draw stop time
	time = fStop;
	min = time / (60 * 75);
	time %= (60 * 75);
	sec = time / 75;
	time %= 75;
	frame = (int32)((time / 75.0) * 1000.0);
	sprintf(text, "%.2d:%.2d.%.3d", min, sec, frame);
	stop_width = fOffView->StringWidth(text);
	if (((dr.right - 4) + stop_width) > sr.right)
		l = sr.right - stop_width;
	else
		l = dr.right - 4;
	if ((dr.left + start_width + 4) > l)
		fOffView->MovePenTo(l, dr.top + 34);
	else
		fOffView->MovePenTo(l, dr.top + 23);
	fOffView->SetDrawingMode(B_OP_OVER);
	fOffView->DrawString(text);
	fOffView->SetDrawingMode(B_OP_COPY);

	// Fill current position indicator
	if (fLastx >= 0) {
		fOffView->SetHighColor(0, 0, 255);
		fOffView->StrokeLine(BPoint(5 + fLastx, dr.top),
							 BPoint(5 + fLastx, dr.top));
		fOffView->StrokeLine(BPoint(5 + fLastx - 1, dr.top - 1),
							 BPoint(5 + fLastx + 1, dr.top - 1));
		fOffView->StrokeLine(BPoint(5 + fLastx - 2, dr.top - 2),
							 BPoint(5 + fLastx + 2, dr.top - 2));
		fOffView->StrokeLine(BPoint(5 + fLastx - 3, dr.top - 3),
							 BPoint(5 + fLastx + 3, dr.top - 3));

		// Draw current position time
		fOffView->SetHighColor(51, 51, 102);
		time = fPos;
		min = time / (60 * 75);
		time %= (60 * 75);
		sec = time / 75;
		time %= 75;
		frame = (int32)((time / 75.0) * 1000.0);
		sprintf(text, "%.2d:%.2d.%.3d", min, sec, frame);
		start_width = fOffView->StringWidth(text);
		if (((5 + fLastx - 4) + start_width) > sr.right)
			l = sr.right - start_width;
		else
			l = 5 + fLastx - 4;
		fOffView->MovePenTo(l, dr.top - 8);
		fOffView->SetDrawingMode(B_OP_OVER);
		fOffView->DrawString(text);
		fOffView->SetDrawingMode(B_OP_COPY);
	}

	fOffView->Sync();
	DrawBitmap(fOffScreen, BPoint(SELECTOR_H, SELECTOR_V));
	fOffScreen->Unlock();
}

TStatusWindow::TStatusWindow(BRect rect, char *title, BMediaFile *theFile, BMediaTrack *theTrack,
				int32 trackStart, int32 start, int32 stop, int32 cd_id)
			  :BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE |
													 B_NOT_ZOOMABLE |
													 B_NOT_CLOSABLE)
{
	BButton			*but;
	BCheckBox		*box;
	BRect			r;
	scsi_inquiry	info;

	fStop = false;
	fCDID = cd_id;

	r.left = (STATUS_WIDTH - STATUS_BUTTON_WIDTH) - 10;
	r.right = r.left + STATUS_BUTTON_WIDTH;
	r.top = STATUS_HEIGHT - (STATUS_BUTTON_HEIGHT + 9);
	r.bottom = r.top + STATUS_BUTTON_HEIGHT;
	but = new BButton(r, "status", "Stop", new BMessage(STATUS_BUTTON));
	AddChild(but);

	r.top -= STATUS_MUTE_HEIGHT + 9;
	r.bottom = r.top + STATUS_MUTE_HEIGHT;
	r.right = r.left + STATUS_MUTE_WIDTH;
	box = new BCheckBox(r, "status", "Mute", new BMessage(STATUS_MUTE));
	AddChild(box);

	// If it's not a Toshiba 3501 or 3601 always mute
	if ((ioctl(fCDID, B_SCSI_INQUIRY, &info) != B_NO_ERROR) || 
		(info.inquiry_data[26] != '3') ||
		((info.inquiry_data[27] != '6') && (info.inquiry_data[27] != '5') &&
		 (info.inquiry_data[27] != '7'))) {
		Lock();
		mute = true;
		box->SetEnabled(false);
		Unlock();
	}
	if (mute) {
		Lock();
		box->SetValue(1);
		Unlock();
	}

	r.Set(0, 0, STATUS_WIDTH, STATUS_HEIGHT);
	fView = new TStatusView(r, "Status", theFile, theTrack, trackStart,
							start, stop, this, fCDID);
	AddChild(fView);
	Show();
}

void TStatusWindow::MessageReceived(BMessage* message)
{
	if (message->what == STATUS_BUTTON)
		fStop = true;
	else
		if (message->what == STATUS_MUTE) {
			if (mute)
				mute = false;
			else
				mute = true;
		}
}

TStatusView::TStatusView(BRect rect, char *title, BMediaFile *theFile, BMediaTrack *theTrack,
						 int32 trackStart, int32 start, int32 stop,
						 TStatusWindow *wind, int32 cd_id)
			:BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW)
{
	fFile = theFile;
	fTrack = theTrack;
	fTrackStart = trackStart;
	fStart = start;
	fStop = stop;
	fWind = wind;
	fCDID = cd_id;
}

void TStatusView::AttachedToWindow(void)
{
	thread_id	thread;

	thread = spawn_thread(_SaveThreadEntry, "write_cd_data", B_NORMAL_PRIORITY, this);
	resume_thread(thread);
}

void TStatusView::Draw(BRect where)
{
	BRect	r;

	SetHighColor(0, 0, 0);
	SetFont(be_bold_font);
	SetFontSize(12);
	MovePenTo(STATUS_TEXT_H, STATUS_TEXT_V);
	DrawString("Saving Track");

	r.Set(PROGRESS_H, PROGRESS_V, PROGRESS_H + PROGRESS_WIDTH,
			PROGRESS_V + PROGRESS_HEIGHT);
	StrokeRect(r);
}

int32 TStatusView::_SaveThreadEntry(void *arg)
{
	((TStatusView*)arg)->SaveThread();
	return 0;
}

void TStatusView::SaveThread()
{
	int16*			data;
	int32			time;
	uint32			buf_size;
	uint32			length;
	uint32			start;
	uint32			frames;
	float			orig_length;
	float			temp;
	float			width;
	BRect			r;
	scsi_read_cd	audio;

	r.Set(PROGRESS_H + 1, PROGRESS_V + 1, 0,
		 (PROGRESS_V + 1) + (PROGRESS_HEIGHT - 2));
	width = PROGRESS_WIDTH - 2;

	length = fStop - fStart;
	orig_length = length;

	BMediaTrack *audioTrack=fTrack;

	// grab 1 seconds of audio (or less) each pass
	frames = min(1 * 75, (int)length);
	buf_size = frames * 2352;
	data = (int16 *)malloc(buf_size);
	audio.buffer = (char *)data;
	start = fStart + fTrackStart;

	while ((length > 0) && (!fWind->fStop)) {
		time = start;
		audio.start_m = time / (60 * 75);
		time %= (60 * 75);
		audio.start_s = time / 75;
		audio.start_f = time % 75;

		time = min(frames, length);
		audio.length_m = time / (60 * 75);
		time %= (60 * 75);
		audio.length_s = time / 75;
		audio.length_f = time % 75;

		audio.buffer_length = min(frames, length) * 2352;

		if (mute)
			audio.play = false;
		else
			audio.play = true;

		// Read raw data from the CD
		if (ioctl(fCDID, B_SCSI_READ_CD, &audio) != B_NO_ERROR) {
			(new BAlert("", "An error occurred reading the track.", "Sorry"))->Go();
			break;
		}

		// Write out to the file
		status_t error = audioTrack->WriteFrames(data, audio.buffer_length / sizeof(short) / 2);
		if (error) {
			(new BAlert("", "Error writing data", "Sorry"))->Go();
			break;
		}

		start += min(frames, length);
		length -= min(frames, length);
		temp = orig_length - length;

		// Update progress bar
		r.right = r.left + ((temp / orig_length) * width);
		if (!fWind->Lock())
			break;

		SetHighColor(128, 128, 128);
		FillRect(r);
		fWind->Flush();
		fWind->Unlock();
	}

	free(data);
			
	fFile->CloseFile();
	delete fFile;
	if (fWind->Lock())
		fWind->Quit();
}
