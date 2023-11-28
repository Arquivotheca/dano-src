#include "PackFilePanel.h"
#include "PackData.h"
#include "SettingsManager.h"
#include "Util.h"

#include "ErrHandler.h"
#include "DataID.h"

#include <File.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Autolock.h>
#include <Window.h>
#include <View.h>
#include <StringView.h>
#include <PopUpMenu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>

extern const rgb_color		label_red;
extern SettingsManager		*gSettings;

status_t	verify_magic(BFile *f);
status_t	read_arc_header(PackData &tPackData, BFile *f,
							int32 &vers, off_t &attribOffset);
status_t	read_attrib(PackData *fPackData, BFile *fArcFile,
					char **name , char **version, char **developer);							
							
extern uint16 CUR_VERS;
const int kMagicSize = 8;
extern uchar kPHeader[];


class PRefFilter : public BRefFilter{
public:
				PRefFilter();
virtual	bool	Filter(const entry_ref*, BNode*, struct stat*,
					   const char* mimetype);
	bool on;
};

PRefFilter::PRefFilter()
	:	on(false)
{
}

bool PRefFilter::Filter(const entry_ref *ref, BNode*, struct stat *sb,
					   const char* mimetype)
{
	if (!on)
		return true;
	
	// reject partially downloaded files	
	if (!strcmp(mimetype, "application/x-scode-VPart"))
		return false;
	
	BFile	f;	
	if (S_ISREG(sb->st_mode))
		f.SetTo(ref, O_RDONLY);
	else if (S_ISLNK(sb->st_mode)) {
		// if symlink to file, open the referred file
		BEntry ent(ref,true);
		if (ent.IsFile())
			f.SetTo(&ent,O_RDONLY);		// symlink to file
		else
			return true;	// symlink to non-file
	}
	else
		return true;	// not symlink or file

	// we have a file		
	if (f.InitCheck() == B_NO_ERROR &&	
		verify_magic(&f) == B_NO_ERROR)
		return true;
		
	// wasn't a package or couldn't be opened
	return false;
}

filter_result type_changed(BMessage *msg, BHandler **, BMessageFilter *);

filter_result type_changed(BMessage *msg, BHandler **, BMessageFilter *)
{
	void *p;
	msg->FindPointer("panel",&p);
	BFilePanel	*panel = (BFilePanel *)p;
	
	if (panel) {
		PRefFilter *filter = (PRefFilter *)panel->RefFilter();
		if (msg->FindInt32("index") == 0)
			filter->on = false;
		else
			filter->on = true;
			
		panel->Refresh();
	}
	return B_DISPATCH_MESSAGE;	
}

PackFilePanel::PackFilePanel(BMessenger *target,
						entry_ref	*start_dir,
						uint32		message)
	:	MFilePanel(B_OPEN_PANEL, target, start_dir, false, false, message),
		data(NULL)
{
	BWindow *wind = Window();
	BAutolock	w(wind);
	BRect b;

	BView *v = wind->ChildAt(0);
	
	///// Adjust the window size, for space at top and bottom
	v->SetResizingMode(B_FOLLOW_NONE);
	wind->ResizeBy(0,50 + 19);
	v->MoveBy(0,19);
	v->SetResizingMode(B_FOLLOW_ALL);
	float x1,x2,y1,y2;
	
	wind->GetSizeLimits(&x1,&x2,&y1,&y2);
	wind->SetSizeLimits(x1,x2,y1+50+18,y2);
	
	///// move the menu bar to the root
	BView *emb = v->FindView("MenuBar");
	v->RemoveChild(emb);
	wind->AddChild(emb);
	
	///// Add a descriptive string view
	
	b = v->Bounds();
	b.top++;
	b.bottom = b.top + 18;
	BStringView *sv = new BStringView(b,B_EMPTY_STRING,
		"Please open the package file you wish to install...",
		B_FOLLOW_H_CENTER | B_FOLLOW_TOP);
	v->AddChild(sv);
	sv->SetAlignment(B_ALIGN_CENTER);
	
	///// Add the info view at the bottom
	
	b = wind->Bounds();
	b.top = b.bottom - 50;
	
	BView *info = new BView(b, B_EMPTY_STRING,
							B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
							B_WILL_DRAW /* | B_FULL_UPDATE_ON_RESIZE */);
	wind->AddChild(info);
	info->SetViewColor(v->ViewColor());

	/////	Add the popup menu
	
	b = v->Bounds();
	b.InsetBy(14,14);
	b.right = b.left + 180;
	b.top = b.bottom - 20;
	
	BPopUpMenu *p = new BPopUpMenu(B_EMPTY_STRING);
	
	BMessage model('Type');
	model.AddPointer("panel",this);
	
	p->AddItem(new BMenuItem("All files",new BMessage(model)));
	p->AddItem(new BMenuItem("Package files",new BMessage(model)));
	p->ItemAt(0)->SetMarked(true);
	BMenuField	*f = new BMenuField(b,"types","Display:",p,
						B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	
	f->SetDivider(60);
	f->SetFont(be_bold_font);
	f->Menu()->SetFont(be_bold_font);
	f->MenuBar()->SetFont(be_bold_font);
	
	v->AddChild(f);
	wind->AddFilter(new BMessageFilter('Type',type_changed));

	/////

	char	*labels[] = {"Package:",
						 "Version:",
						 "Developer:"};
	char	*names[] = {"pkg",
						 "vers",
						 "dev"};
	
	BRect br = info->Bounds();
	br.left += 6;
	br.bottom = br.top + 14;
	br.right = br.left + 60;
	BRect lr;
	lr = br;
	lr.left = br.right + 5;
	lr.right = lr.left + 120;
	for (long i = 0; i < nel(labels); i++) {
		BStringView *sv = new BStringView(br,B_EMPTY_STRING,labels[i],
					B_FOLLOW_LEFT | B_FOLLOW_TOP);
		info->AddChild(sv);
		sv->SetAlignment(B_ALIGN_RIGHT);
		sv->SetHighColor(label_red);
		
		sv = new BStringView(lr,names[i],B_EMPTY_STRING,
					B_FOLLOW_LEFT | B_FOLLOW_TOP);
		info->AddChild(sv);
		
		br.OffsetBy(0,15);
		lr.OffsetBy(0,15);
	}
	filter = new PRefFilter();
	SetRefFilter(filter);
	data = new PackData();

	BDirectory tmp(gSettings->data.FindString("download/path"));
	SetPanelDirectory(&tmp);
}

PackFilePanel::~PackFilePanel()
{
	if (Window()->Lock()) {
		Window()->ResizeBy(0,-(50 + 19));
		Window()->Unlock();
	}
	
	delete data;
	delete filter;
}

void PackFilePanel::SelectionChanged()
{
	entry_ref	ref;
	Rewind();
	if (GetNextSelectedRef(&ref) == B_NO_ERROR) {
		BStringView *sv = (BStringView *)Window()->FindView("pkg");
		BStringView *vs = (BStringView *)Window()->FindView("vers");
		BStringView *dv = (BStringView *)Window()->FindView("dev");
		sv->SetText(B_EMPTY_STRING);
		vs->SetText(B_EMPTY_STRING);
		dv->SetText(B_EMPTY_STRING);
		
		// traverse sym links
		BEntry	entry(&ref, true);
		if (!entry.IsFile())
			return;
		BFile	file(&entry,O_RDONLY);
		if (file.InitCheck())
			return;
			
		int32 	vers;
		off_t	attribOff;
		if (verify_magic(&file) == B_NO_ERROR &&
			read_arc_header(*data, &file, vers, attribOff) == B_NO_ERROR &&
			file.Seek(attribOff,SEEK_SET) == attribOff)
		{
			char *name = 0;
			char *vers = 0;
			char *dev = 0;
			read_attrib(data, &file, &name , &vers, &dev);

			if (!name || !*name)
				sv->SetText("Unknown...");			
			else if (name) sv->SetText(name);
			if (vers) vs->SetText(vers);
			if (dev) dv->SetText(dev);
			
			free(name);
			free(vers);
			free(dev);
		}
		else
			sv->SetText("Invalid package...");
	}
}

status_t	verify_magic(BFile *f)
{
	f->Seek(0,SEEK_SET);
	
	uchar header[kMagicSize];
	if (f->Read(header,kMagicSize) == kMagicSize) {
	
		return memcmp(kPHeader,header,kMagicSize);
	}
	return -1;
}

status_t	read_arc_header(PackData &tPackData, BFile *f,
							int32 &vers, off_t &attribOffset)
{
	ErrHandler		err;
	off_t			size;
	
	try {
		record_header	header;
		err = tPackData.ReadRecordHeader(f,&header);
	
		if (header.what != ID_PKGHEADER || header.type != LIS_TYPE)
			err = B_ERROR;
	
		bool readingEntries = true;
		while (readingEntries) {
			err = tPackData.ReadRecordHeader(f,&header);
			switch (header.what) {
				case ID_FORMAT_VERS:
					err = tPackData.ReadInt32(f,&vers);
					break;
				case ID_ATTRIB_OFFSET:
					err = tPackData.ReadInt64(f,&size);
					attribOffset = size;
					break;
				case END_TYPE:
					readingEntries = false;
					break;
				default:
					err = tPackData.SkipData(f,&header);
					break;
			}
		}
	}
	catch (ErrHandler::OSErr e) {
		return e.err;
	}
	return B_NO_ERROR;
}

status_t	read_attrib(PackData *fPackData, BFile *fArcFile,
					char **name , char **version, char **developer)
{
	ErrHandler		err;
	status_t		serr = B_NO_ERROR;
	
	record_header	header;
	try {
		err = fPackData->ReadRecordHeader(fArcFile,&header);
		if (header.what != ID_PKGATTR || header.type != LIS_TYPE) {
			err = B_ERROR;
		}
		bool readingEntries = true;
		while(readingEntries) {
			err = fPackData->ReadRecordHeader(fArcFile,&header);
			switch (header.what) {
				case A_PACK_NAME:
					err = fPackData->ReadString(fArcFile, name);
					break;
				case A_PACK_VERSION:
					err = fPackData->ReadString(fArcFile, version);
					break;
				case A_PACK_DEVELOPER:
					err = fPackData->ReadString(fArcFile, developer);
					break;
			/***
				case A_PACK_REL_DATE:
					err = fPackData->ReadInt32(fArcFile, (int32 *)&att->releaseDate);
					break;
				case A_PACK_DESCRIPTION:
					err = fPackData->ReadString(fArcFile, &att->description);
					break;
				case A_PACK_FLAGS:
					int32 flags;
					err = fPackData->ReadInt32(fArcFile, &flags);
					att->isUpdate = flags & PKGF_ISUPDATE;
					break;
			***/
				case END_TYPE:
					readingEntries = false;
					break;
				default:
					err = fPackData->SkipData(fArcFile,&header);
					break;
			}
		}
	}
	catch (ErrHandler::OSErr e) {
		serr = e.err;	
	}
	return serr;	
}
