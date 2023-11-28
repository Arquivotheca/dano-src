#include <Be.h>
// PatchWindow.cpp

#include "PatchWindow.h"
#include "PackWindow.h"
#include "PackMessages.h"
#include "MFilePanel.h"

#include "Util.h"
#include "MyDebug.h"

const rgb_color dark_red = {200,80,80,0};


PatchWindow::PatchWindow(const char *title,PackWindow *_pw)
	: ChildWindow(BRect(0,0,400,180),title,B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE, _pw),
	  pw(_pw),
	  parentList(NULL),
	  busy(FALSE),
	  filePanelW(NULL)
{
	Lock();
	
	parentList = pw->FindView("listing");
	
	oldFileRef.device = oldFileRef.directory = -1;
	newFileRef.device = newFileRef.directory = -1;
	
	AddChild(new PatchView(Bounds(),"patchview",pw));
	
	Show();
	Unlock();
}

void PatchWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_ADD_PATCH: {
			if (! busy) {
				busy = TRUE;

				BMessage ptchMsg(M_ADD_PATCH);
				ptchMsg.AddRef("oldref",&oldFileRef);
				ptchMsg.AddRef("newref",&newFileRef);

				pw->PostMessage(&ptchMsg,parentList);
				
				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		case M_SEL_OLDPATCH:
		case M_SEL_NEWPATCH: {
			if (msg->HasRef("refs")) {
				if (filePanelW->Lock()) {
					filePanelW->PostMessage(B_QUIT_REQUESTED);
					filePanelW->Unlock();
					filePanelW = NULL;
				}
				PRINT(("got file\n"));
				entry_ref curRef;
				msg->FindRef("refs",0,&curRef);
				BEntry		curFile(&curRef);
				
				if (curFile.InitCheck() < B_NO_ERROR)
					return;
				
				BStringView	*sView;
				char *prefix;
				
				if (msg->what == M_SEL_OLDPATCH) {
					oldFileRef = curRef;
					prefix = "old";
				}
				else {
					newFileRef = curRef;
					prefix = "new";
				}
				BString	viewName;
				BString	bufStr;
				char buf[B_FILE_NAME_LENGTH];
				
				viewName << prefix << "filename";
				sView = (BStringView *)FindView(viewName.String());
				ASSERT(sView);
				curFile.GetName(buf);
				sView->SetText(buf);
				
				viewName.Truncate(0);
				viewName << prefix << "size";
				sView = (BStringView *)FindView(viewName.String());
				ASSERT(sView);
				off_t fsize;
				curFile.GetSize(&fsize);
				//if (fsize < 1024)
					bufStr << fsize << " bytes";
				//else
				//	sprintf(buf,"%d K",(long)(fsize/1024));
				sView->SetText(bufStr.String());
				
				viewName.Truncate(0);
				viewName << prefix << "version";
				sView = (BStringView *)FindView(viewName.String());
				ASSERT(sView);

				version_info vi;
				BFile file(&curFile, B_READ_ONLY);
				status_t code = GetVersionInfo(&file, &vi);

				if (code == B_OK) {
//					BuildVersionString(&vi, &bufStr);
					bufStr.SetTo(vi.short_info);
				}					
				else {
					bufStr.SetTo("N/A");
				}
				sView->SetText(bufStr.String());

				time_t ftime;
				struct tm *tp;
				viewName.Truncate(0);
				viewName << prefix << "created";
				sView = (BStringView *)FindView(viewName.String());
				ASSERT(sView);
				
				curFile.GetCreationTime(&ftime);
				tp = localtime(&ftime);
				strftime(buf, B_FILE_NAME_LENGTH, "%b %d %Y, %I:%M %p",tp);
				sView->SetText(buf);

				viewName.Truncate(0);
				viewName << prefix << "modified";
				sView = (BStringView *)FindView(viewName.String());
				ASSERT(sView);
				
				curFile.GetModificationTime(&ftime);
				tp = localtime(&ftime);
				strftime(buf, B_FILE_NAME_LENGTH,"%b %d %Y, %I:%M %p",tp);
				sView->SetText(buf);
				
				BButton *addBtn = (BButton *)FindView("addpatch");
				
				if (oldFileRef.directory >= 0 && oldFileRef.device >= 0 &&
					newFileRef.directory >= 0 && newFileRef.device >= 0)
					addBtn->SetEnabled(TRUE);	
				else	
					addBtn->SetEnabled(FALSE);
			}
			else {
				if (msg->what == M_SEL_OLDPATCH)
					MakeFilePanel(msg->what,"Select Old Version");
				else
					MakeFilePanel(msg->what,"Select New Version");
			}
			break;
		}
		default:
			ChildWindow::MessageReceived(msg);
	}
}


void PatchWindow::MakeFilePanel(long msgCode, const char *prompt )
{
	// run the open panel with all files
	if (filePanelW->Lock()) {
		filePanelW->Activate(TRUE);		
		filePanelW->Unlock();
	}
	else {
		BFilePanel *pan = new MFilePanel(B_OPEN_PANEL,
										new BMessenger(FindView("patchview")),
										NULL,
										false,
										true,
										msgCode);
		filePanelW = pan->Window();
		BString buf;
		buf << "PackageBuilder : " << prompt;
		filePanelW->SetTitle(buf.String());
		pan->Show();
	}
}

void PatchView::AllAttached()
{
	BButton *addBtn = (BButton *)FindView("addpatch");
	addBtn->MakeDefault(TRUE);
	addBtn->SetEnabled(FALSE);
}

///////////////////////////////////////////////////////////////////////////////


PatchView::PatchView(BRect frame,const char *name,PackWindow *_pw)
	: BView(frame,name,B_FOLLOW_ALL,B_WILL_DRAW),
	  pw(_pw)
{

	SetViewColor(light_gray_background);
	
	BRect r = Bounds();
	BRect cr, fr;
	r.InsetBy(12,12);
	cr = r;
	
	//////////////////////////////
	
	cr.right = cr.left + 110;
	cr.bottom = cr.top + 24;
	
	BButton *btn;
	btn = new BButton(cr,"seloldfile","Select Old File...",new BMessage(M_SEL_OLDPATCH));
	AddChild(btn);
	
	AddChildLabels(cr,"old");
	
	cr.left = (Bounds().right + r.left)/2.0;
	cr.right = cr.left + 110;
	btn = new BButton(cr,"selnewfile","Select New File...",new BMessage(M_SEL_NEWPATCH));
	AddChild(btn);
	
	AddChildLabels(cr,"new");
	
	cr = r;

	cr.left = cr.right - 80;
	cr.top = cr.bottom - 24;
	//cr.OffsetBy(-12,0);
	
	btn = new BButton(cr,"addpatch","Add",new BMessage(M_ADD_PATCH));
	AddChild(btn);
	
	cr.OffsetBy(-110,0);
	btn = new BButton(cr,"addpatch","Cancel",new BMessage(B_QUIT_REQUESTED));
	AddChild(btn);
}

void PatchView::AttachedToWindow()
{
	BView::AttachedToWindow();
}

#define PATCHVIEW_BOTTOM_SIZE 54

void PatchView::MessageReceived(BMessage *msg)
{
	BPoint dropLoc;
	float midpoint;
	switch (msg->what) {
		case 'DATA':
			PRINT(("view got data message\n"));
		case B_REFS_RECEIVED:
			PRINT(("view got refs message\n"));
			if (!msg->WasDropped())
				return;
			if (!msg->HasRef("refs"))
				return;			
			dropLoc = msg->DropPoint();
			ConvertFromScreen(&dropLoc);
			
			if (dropLoc.y > Bounds().bottom - PATCHVIEW_BOTTOM_SIZE)
				return;
				
			midpoint = Bounds().Width() / 2.0;
			
			if (dropLoc.x < midpoint)
				msg->what = M_SEL_OLDPATCH;
			else
				msg->what = M_SEL_NEWPATCH;
			
			Looper()->DetachCurrentMessage();
			Looper()->PostMessage(msg,Window());
			break;
		default:
			BView::MessageReceived(msg);
	}
}

void PatchView::AddChildLabels(BRect btn,const char *prefix)
{
	BStringView	*sv;
	char buf[80];
	
	BRect fr = btn;
	fr.top = fr.bottom + 12;
	fr.bottom = fr.top + 12;
	fr.right = fr.left + 50;
	
	BRect rr;
	rr = fr;
	rr.left = rr.right+2;
	rr.right = rr.left + 120;

	AddChild(sv = new BStringView(fr,B_EMPTY_STRING,"Filename:"));
	sv->SetHighColor(220,0,0);
	sprintf(buf,"%sfilename",prefix);
	AddChild(new BStringView(rr,buf,B_EMPTY_STRING));
	
	fr.OffsetBy(0,14);
	AddChild(sv = new BStringView(fr,B_EMPTY_STRING,"Version:"));
	sv->SetHighColor(220,0,0);
	rr.OffsetBy(0,14);
	sprintf(buf,"%sversion",prefix);
	AddChild(new BStringView(rr,buf,"-"));
	
	fr.OffsetBy(0,14);
	AddChild(sv = new BStringView(fr,B_EMPTY_STRING,"Size:"));
	sv->SetHighColor(220,0,0);
	rr.OffsetBy(0,14);
	sprintf(buf,"%ssize",prefix);
	AddChild(new BStringView(rr,buf,B_EMPTY_STRING));

	fr.OffsetBy(0,14);
	AddChild(sv = new BStringView(fr,B_EMPTY_STRING,"Created:"));
	sv->SetHighColor(220,0,0);
	rr.OffsetBy(0,14);
	sprintf(buf,"%screated",prefix);
	AddChild(new BStringView(rr,buf,B_EMPTY_STRING));
	
	fr.OffsetBy(0,14);
	AddChild(sv = new BStringView(fr,B_EMPTY_STRING,"Modified:"));
	sv->SetHighColor(220,0,0);
	rr.OffsetBy(0,14);
	sprintf(buf,"%smodified",prefix);
	AddChild(new BStringView(rr,buf,B_EMPTY_STRING));
}

void PatchView::Draw(BRect updt)
{
	BView::Draw(updt);
	
	float left = Bounds().left;
	float right = Bounds().right;
	float v = Bounds().bottom - PATCHVIEW_BOTTOM_SIZE;
	DrawHSeparator(left,right,v,this);
}

status_t GetVersionInfo(BFile *file, version_info *info)
{
	BAppFileInfo appinfo(file);
	status_t code = appinfo.GetVersionInfo(info, B_APP_VERSION_KIND);
	return (code == B_OK) ? B_OK : B_ERROR;
}

void BuildVersionString(version_info *info, BString *str)
{
	str->Truncate(0);
	*str << info->major << "." << info->middle << "." << info->minor << ".";
	*str << info->variety << "." << info->internal;
}
