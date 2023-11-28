//	ShowApp.cpp

#include "prefix.h"
#include "ShowApp.h"
#include <TranslationKit.h>
#include <support/DataIO.h>
#include "AppMakeView.h"
#include "ShowWindow.h"
#include "prefs.h"

#include <interface/Window.h>
#include <interface/ScrollBar.h>
#include <File.h>
#include <interface/Alert.h>
#include <interface/Box.h>
#include <interface/StringView.h>
#include <interface/Screen.h>
#include <interface/MenuBar.h>
#include <interface/MenuItem.h>
#include <AppFileInfo.h>
#include <FilePanel.h>
#include <Roster.h>
#include <interface/Bitmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Resources.h>
#include <ResourceStrings.h>
extern BResourceStrings g_strings;


#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define DITHER_ITEM 'ditr'

float MENU_BAR_HEIGHT = 20.0;
bool firstTry = true;
extern bool inhibit_first_panel;

int32 ShowApp::sWindowCount = 0;


ShowApp::ShowApp(
	const char *		signature) :
	BApplication(signature)
{
/* figure out the size of the menu bar */
	BWindow * w = new BWindow(BRect(0,0,100,100), "Temp Window", B_TITLED_WINDOW, 0);
	BMenuBar * mb = new BMenuBar(BRect(0,0,100,MENU_BAR_HEIGHT), "Main Bar");
	BMenu * mn = new BMenu("File");
	mb->AddItem(mn);
	w->AddChild(mb);
	MENU_BAR_HEIGHT = mb->Frame().bottom+1;
	w->Run();
	w->PostMessage(B_QUIT_REQUESTED);

	fOpen = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), (const entry_ref *)NULL, 0, true, 
		new BMessage(B_REFS_RECEIVED), NULL, false, true);

	SetPulseRate(50000);
	if (DATA == NULL) {
		BAlert *alrt = new BAlert("", g_strings.FindString(5), g_strings.FindString(6));
		alrt->Go();
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	long curv = 0, minv = 0;
	DATA->Version(&curv, &minv);
	if (curv < B_TRANSLATION_MIN_VERSION) {
		BAlert *alrt = new BAlert("", g_strings.FindString(7), g_strings.FindString(6));
		alrt->Go();
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	if (minv > B_TRANSLATION_CURRENT_VERSION) {
		BAlert *alrt = new BAlert("", g_strings.FindString(8), g_strings.FindString(6));
		alrt->Go();
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
}


ShowApp::~ShowApp()
{
	delete fOpen;
}


void
ShowApp::RefsReceived(
	BMessage *message)
{
	BScreen screen;
	BRect winR = screen.Frame();
	winR.left = 5;
	winR.right -= 5;
	winR.top = 24;
	winR.bottom -= 5;

	for (int ix=0; message->HasRef("refs", ix); ix++)
	{
		/*	get the file out of the message */

		entry_ref ref;
		if (message->FindRef("refs", ix, &ref))
			continue;
		BFile *file = new BFile(&ref, O_RDONLY);
		struct stat stbuf;
		char str[B_FILE_NAME_LENGTH+30];
		char file_name[B_FILE_NAME_LENGTH];
		BEntry entry(&ref);
		if (entry.GetName(file_name) || entry.GetStat(&stbuf) || S_ISDIR(stbuf.st_mode))
		{
			sprintf(str, g_strings.FindString(9), file_name);
			BAlert *error = new BAlert("", str, g_strings.FindString(10));
			error->Go();
			continue;
		}

		/*	try to create a view */

		BView *outView = NULL;
		BString dataInfo;
		BRect extent;
		translator_id translator;
		uint32 format;
		const char *mimeStr = NULL;
		if (B_OK <= file->ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, str, B_FILE_NAME_LENGTH))
		{
			mimeStr = str;
		}
		long err = AppMakeView(file, outView, extent, mimeStr, dataInfo, translator, format);

		if (err || !outView)
		{
			char msg[1024];
			sprintf(msg, g_strings.FindString(11), file_name);
			BAlert *error = new BAlert("Missing Handler", msg, g_strings.FindString(10));
			error->Go();
			continue;
		}

		/* Put this view in a window with scroll bars */

		BRect winSize = winR;
		if (winSize.Width() > extent.Width()+B_V_SCROLL_BAR_WIDTH)
			winSize.right = winSize.left+extent.Width()+B_V_SCROLL_BAR_WIDTH;
		if (winSize.Height() > extent.Height()+B_H_SCROLL_BAR_HEIGHT+
			MENU_BAR_HEIGHT)
			winSize.bottom = winSize.top+extent.Height()+B_H_SCROLL_BAR_HEIGHT+
			MENU_BAR_HEIGHT;
		ShowWindow *window = new ShowWindow(winSize, file_name, ref, translator, format,
			B_DOCUMENT_WINDOW, B_WILL_ACCEPT_FIRST_CLICK|B_ASYNCHRONOUS_CONTROLS);
		BRect viewR = window->Bounds();
		viewR.right -= B_V_SCROLL_BAR_WIDTH;
		viewR.bottom -= B_H_SCROLL_BAR_HEIGHT;
		viewR.top = MENU_BAR_HEIGHT;

		outView->MoveTo(0,MENU_BAR_HEIGHT);
		outView->ResizeTo(viewR.Width(), viewR.Height());
		window->AddChild(outView);
		window->LoadSlices();
		
		BRect tR = viewR;
		tR.top = tR.bottom+1;
		tR.bottom = tR.top+B_H_SCROLL_BAR_HEIGHT;
		tR.right++;
		tR.left += 160;
		BScrollBar *hBar = new BScrollBar(tR, "horiz", outView,
			0, max(extent.Width()-viewR.Width(), 0), B_HORIZONTAL);
		hBar->SetSteps(8, 240);
		window->AddChild(hBar);

		tR.right = tR.left;
		tR.left = -1;
		BBox *box = new BBox(tR, NULL, B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
		window->AddChild(box);
		rgb_color dkGray = { 150, 150, 150, 0 };
		box->SetHighColor(dkGray);
		tR.InsetBy(1, 1);
		tR.OffsetTo(BPoint(1,1));
		dataInfo	<< " (" << (int32)(extent.Width()+1.5)
					<< "x" << (int32)(extent.Height()+1.5) << ")";
		BStringView *strv = new BStringView(tR, "info", dataInfo.String(), B_FOLLOW_ALL);
		box->AddChild(strv);
		rgb_color gray = { 222, 222, 222, 0 };
		strv->SetViewColor(gray);
		strv->SetDrawingMode(B_OP_COPY);
		strv->SetFont(be_plain_font);
		strv->SetFontSize(9);

		tR = viewR;
		tR.left = tR.right+1;
		tR.right = tR.left+B_V_SCROLL_BAR_WIDTH;
		tR.bottom++;
		tR.top--;
		BScrollBar *vBar = new BScrollBar(tR, "vert", outView,
			0, max(extent.Height()-viewR.Height(), 0), B_VERTICAL);
		vBar->SetSteps(8, 160);
		window->AddChild(vBar);

		window->Show();
		winR.OffsetBy(5, 15);

		sWindowCount++;

	}
}


void
ShowApp::ReadyToRun()
{
/*
	if (sWindowCount < 1) {
		(new ShowWindow(BRect(40,30,200,100), "ShowImage", B_TITLED_WINDOW, 0))->Show();
		sWindowCount++;
	}
*/
	BApplication::ReadyToRun();
	if (!fOpen->IsShowing() && !inhibit_first_panel && (sWindowCount < 1)) {
		fOpen->Show();
	}
}


void 
ShowApp::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
	case B_CANCEL: {
			BFilePanel * panel;
			if (!message->FindPointer("source", (void **)&panel)) {
				if ((panel == fOpen) && (sWindowCount < 1)) {
					PostMessage(B_QUIT_REQUESTED);
				}
			}
		} break;
	case msg_WindowDeleted:
		if (--sWindowCount < 1)
			PostMessage(B_QUIT_REQUESTED);
		break;
	case msg_ShowOpenPanel:
		if (!fOpen->IsShowing())
			fOpen->Show();
		else
			fOpen->Window()->Activate(true);
		break;
	case DITHER_ITEM:
		g_prefs.dither = !g_prefs.dither;
		for (int ix=0; ix<CountWindows(); ix++) {
			ShowWindow * w = dynamic_cast<ShowWindow *>(WindowAt(ix));
			if (w->Lock()) {
				BView * v = w->FindView("bitmap");
				if (v != NULL)
					v->Invalidate();
				w->Unlock();
			}
		}
		break;
	default:
		BApplication::MessageReceived(message);
		break;
	}
}


class AboutView :
	public BView
{
public:
		enum {
			N_STARS = 20
		};
		AboutView(
				BRect area,
				const char * title,
				BList * bitmaps) :
			BView(area, title, B_FOLLOW_ALL, B_WILL_DRAW|B_PULSE_NEEDED)
			{
				SetLowColor(0,0,0,255);
				SetViewColor(0,0,0,255);
				SetHighColor(255,0,128,255);
				fList = bitmaps;
				fItem = 0;
				srand(system_time()/1000);
				for (int ix=0; ix<N_STARS; ix++) {
					rgb_color star = { 255, 255, 255, 255 };
					if (ix < (N_STARS/2)) {
						star.red = 128 + (127/(N_STARS/2))*ix;
					}
					else {
						star.blue = 255 - (127/(N_STARS/2))*(ix-(N_STARS/2)+1);
					}
					fStars[ix].x = rand()*area.Width()/RAND_MAX;
					fStars[ix].y = rand()*area.Height()/RAND_MAX;
					fStarColors[ix] = star;
				}
			}
		~AboutView()
			{
				if (fList) for (int ix=0; ix<fList->CountItems(); ix++)
					delete (BBitmap *)fList->ItemAt(ix);
				delete fList;
			}
		void Draw(
				BRect /* area */)
			{
				SetDrawingMode(B_OP_COPY);
				BeginLineArray(N_STARS);
				for (int ix=0; ix<N_STARS; ix++) {
					AddLine(fStars[ix], fStars[ix], fStarColors[ix]);
				}
				EndLineArray();
				SetFont(be_bold_font);
				DrawString(Name(), BPoint(10,20));
				SetFont(be_plain_font);
				app_info info;
				be_app->GetAppInfo(&info);
				BFile appFile(&info.ref, O_RDONLY);
				BAppFileInfo appInfo(&appFile);
				version_info vinfo;
				memset(&vinfo, 0, sizeof(vinfo));
				appInfo.GetVersionInfo(&vinfo, B_APP_VERSION_KIND);
				DrawString(vinfo.short_info, BPoint(10,60));
				DrawString(vinfo.long_info, BPoint(10,90));
				DrawString(g_strings.FindString(12), BPoint(10,120));
				if (fList) {
					SetDrawingMode(B_OP_OVER);
					DrawBitmapAsync((BBitmap *)fList->ItemAt(fItem), BPoint(200,40));
				}
			}
		void MouseDown(
			BPoint pt)
			{
				if (!fList) return;
				BRect r(((BBitmap *)fList->FirstItem())->Bounds());
				r.OffsetBy(200,40);
				float dx = (r.right+r.left)/2-pt.x;
				float dy = (r.top+r.bottom)/2-pt.y;
				float dist = sqrt(dx*dx+dy*dy);
				if (dist < r.Height()/2) {
					Window()->PostMessage(B_QUIT_REQUESTED);
				}
			}
		void Pulse()
			{
				if (!fList) return;
				fItem++;
				if (fItem >= fList->CountItems())
					fItem = 0;
				SetDrawingMode(B_OP_OVER);
				DrawBitmapAsync((BBitmap *)fList->ItemAt(fItem), BPoint(200,40));
				Sync();
			}
private:
		BList * fList;
		int fItem;
		BPoint fStars[N_STARS];
		rgb_color fStarColors[N_STARS];
};

static BBitmap *
read_bits(
	FILE * f)
{
	TranslatorBitmap hdr;
	if (!f) {
		return NULL;
	}
	hdr.magic = 0;
	fread(&hdr, 1, sizeof(hdr), f);
	if (B_TRANSLATOR_BITMAP != B_BENDIAN_TO_HOST_INT32(hdr.magic)) {
		return NULL;
	}
	swap_data(B_INT32_TYPE, &hdr, sizeof(hdr), B_SWAP_BENDIAN_TO_HOST);
	BBitmap * m = new BBitmap(hdr.bounds, hdr.colors);
	if (!m || !m->Bits()) {
		delete m;
		return NULL;
	}
	if (fread(m->Bits(), 1, m->BitsLength(), f) < 1) {
		delete m;
		return NULL;
	}
	if (m->BitsLength() != hdr.dataSize) {
		fseek(f, hdr.dataSize-m->BitsLength(), SEEK_CUR);
	}
	return m;
}

class ShowAboutWindow :
	public BWindow
{
public:
		ShowAboutWindow(
				const char * title) :
			BWindow(BRect(100,100,400,240), title, B_TITLED_WINDOW,
				B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
			{
				app_info info;
				be_app->GetAppInfo(&info);
				BEntry ent(&info.ref);
				char name[256];
				ent.GetName(name);

				BList * list = new BList;;
#if 0
				{
					BFile file(&ent, O_RDONLY);
					BResources res(&file, false);
					size_t size;
					void * data = res.FindResource('abut', "spin", &size);
					if (data != NULL) {
						int fd = open(g_strings.FindString(13), O_RDWR | O_CREAT | O_TRUNC, 0644);
						if (fd >= 0) {
							write(fd, data, size);
							close(fd);
						}
						free(data);
					}
				}
				system(g_strings.FindString(14));
				FILE * f = fopen(g_strings.FindString(15), "r");
				if (f != NULL) {
					BBitmap * map;
					while ((map = read_bits(f)) != NULL) {
						list->AddItem(map);
					}
					fclose(f);
				}
#endif
				if (!list->CountItems()) {
					delete list;
					list = NULL;
				}
				else {
					SetPulseRate(100000);
				}
				view = new AboutView(Bounds(), name, list);
				AddChild(view);
			}
		~ShowAboutWindow()
			{
				ShowApp::sAboutWindow = NULL;
				be_app->PostMessage(msg_WindowDeleted);
				system(g_strings.FindString(16));
			}
private:
		AboutView * view;
};

ShowAboutWindow * ShowApp::sAboutWindow = NULL;

void
ShowApp::AboutRequested()
{
	if (sAboutWindow) {
		sAboutWindow->Show();
		sAboutWindow->Activate(true);
		return;
	}
	sAboutWindow = new ShowAboutWindow(g_strings.FindString(17));
	sWindowCount++;
	sAboutWindow->Show();
}
