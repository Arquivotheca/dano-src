#include <Be.h>
#include <stdlib.h>
// PackageSetWind.cpp

#include "PackageSetWind.h"
#include "Attributes.h"
#include "LabelView.h"

#include "MyDebug.h"
#include "Util.h"
#include "SerialNum.h"

#include <ctype.h>

bool GetBoolControlValue(BMessage *m);


PackageSetWind::PackageSetWind(	const char *title,
								BWindow *parW,
								AttribData *_attrib,
								bool *_dirtyFlag)
	: 	ChildWindow(BRect(0,0,330,380),
					title,
					B_TITLED_WINDOW,
					B_NOT_RESIZABLE | B_NOT_ZOOMABLE,
					parW)
{
	Lock();
	attrib = _attrib;
	dirtyFlag = _dirtyFlag;
	lastDate = NULL;
	
	AddChild(new PackageSetView(Bounds()));
	
	//parentList = (PackList *)(pw->FindView("listing"));
	
	Show();
	Unlock();
}

PackageSetWind::~PackageSetWind()
{
	free(lastDate);
}

bool PackageSetWind::QuitRequested()
{
	/** if we are closing and a text view is in focus then
	we must make sure the text is saved first!!! ***/
	BTextView *f = cast_as(CurrentFocus(),BTextView);
	if (f) {
		f->MakeFocus(false);
		PostMessage(B_QUIT_REQUESTED,this);
		return false;	// could be bad
	}
	else {
		return ChildWindow::QuitRequested();
	}
}

void PackageSetWind::WindowActivated(bool state)
{
	if (state == FALSE) {
		BTextView *f = cast_as(CurrentFocus(),BTextView);
		if (f) {
			f->MakeFocus(FALSE);
			f->MakeFocus(TRUE);
		}
	}
	ChildWindow::WindowActivated(state);
}

void PackageSetWind::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case S_PACK_NAME:
			SetName();
			break;
		case S_PACK_VERS:
			SetVers();
			break;
		case S_PACK_DEV:
			SetDev();
			break;
		case S_PACK_DATE:
			SetDate();
			break;
		case S_PACK_DESC:
			SetDesc();
			break;
		case S_DEPOT_SERIAL:
			SetSerial();
			break;
		case S_SOFT_TYPE:
			BMenuItem	*it;
			msg->FindPointer("source",(void **)&it);
			if (it) {
				it->SetMarked(true);
				attrib->softType = msg->FindInt32("index");
				if (attrib->softType == 5)
					attrib->softType = 255;
				*dirtyFlag = TRUE;
			}
			break;
		case S_VERSION_ID: {
			BTextControl *tc = cast_as(FindView("vid"),BTextControl);
			free(attrib->versionID);
			attrib->versionID = strdup(tc->Text());
			*dirtyFlag = TRUE;
			break;
		}
		case S_PREFIX_ID: {
			BTextControl *tc = cast_as(FindView("pid"),BTextControl);
			free(attrib->prefixID);
			attrib->prefixID = strdup(tc->Text());
			*dirtyFlag = TRUE;
			break;
		}
		case S_DO_REG: {
			attrib->doReg = GetBoolControlValue(msg);
			*dirtyFlag = TRUE;
			break;
		}
		case S_DO_UPDATE: {
			attrib->doUpdate = GetBoolControlValue(msg);
			*dirtyFlag = TRUE;
			break;
		}
		default:
			ChildWindow::MessageReceived(msg);
			break;
	}
}

void PackageSetWind::SetName()
{
	BTextControl *tc = cast_as(FindView("pname"),BTextControl);
	free(attrib->name);
	attrib->name = strdup(tc->Text());
	*dirtyFlag = TRUE;
}

void PackageSetWind::SetVers()
{
	BTextControl *tc = cast_as(FindView("pvers"),BTextControl);
	free(attrib->version);
	attrib->version = strdup(tc->Text());
	*dirtyFlag = TRUE;
}

void PackageSetWind::SetDev()
{
	BTextControl *tc = cast_as(FindView("pdev"),BTextControl);
	free(attrib->developer);
	attrib->developer = strdup(tc->Text());
	*dirtyFlag = TRUE;
}

void PackageSetWind::SetDate()
{
	BTextControl *tc = cast_as(FindView("pdate"),BTextControl);
	const char *d = tc->Text();
	long len = strlen(d);

	
	if ((len == 8 || len == 10) && isdigit(d[0]) && isdigit(d[1]) &&
			d[2] == '/' && isdigit(d[3]) && isdigit(d[4]) &&
			d[5] == '/' && isdigit(d[6]) && isdigit(d[7]))		
	{
		strcpy(lastDate,d);
		
		char *tmp = strdup(d);
		
		struct tm ts;
		memset(&ts,0,sizeof(ts));
		tmp[2] = '\0';
		ts.tm_mon = atoi(tmp) - 1;
		tmp[5] = '\0';
		ts.tm_mday = atoi(tmp+3);
		
		int yr = atoi(tmp+6);

		// full year spec		
		if (len == 10)
			yr -= 1900;
		
		ts.tm_year = yr;
		attrib->releaseDate = mktime(&ts);
	}
	else {
		doError("Invalid date format");
		tc->SetText(lastDate);
		tc->MakeFocus(TRUE);
	}
	*dirtyFlag = TRUE;
}

void PackageSetWind::SetDesc()
{
	BTextControl *tc = cast_as(FindView("pdesc"),BTextControl);
	free(attrib->description);
	attrib->description = strdup(tc->Text());
	*dirtyFlag = TRUE;
}

bool IsValidSerial(const char *can);

bool IsValidSerial(const char *can)
{
	bool valid = false;
	
	if (can && *can) {
		const int kSnLen = 16;
		char serial[kSnLen+1];
		
		// copy only valid characters (digits)
		// break if invalid character found
		int len = strlen(can);
		int j = 0;
		for (int i = 0; j < kSnLen && i < len; i++) {
			if (isdigit(can[i]))
				serial[j++] = can[i];
			else if (can[i] != '-')
				return valid;
		}
		if (j == kSnLen) {
			serial[kSnLen] = 0;		// null terminate
			valid = SNcheck(serial);
		}
	}
	return valid;
}

void PackageSetWind::SetSerial()
{
	BTextControl *tc = cast_as(FindView("dserial"),BTextControl);
	
	const char *can = tc->Text();
	bool valid = false;
	
	if (*can) {
		valid = IsValidSerial(can);
		if (!valid) {
			beep();
			doError("Invalid SoftwareValet Enable Code\n\n\
	A valid code is required to activate SoftwareValet updating and registration\
	for this package.\n\n\
	BeDepot users should have received serial numbers from Be, Inc. If not, contact\
	devsupport@be.com to obtain a serial number.\n\n\
	Non BeDepot users can contact sales@be.com to sign-up.\n\
	See the PackageBuilder User‘s Guide for more details.");
			tc->SetText(B_EMPTY_STRING);
			tc->MakeFocus(TRUE);
			
			if (attrib->serialno) attrib->serialno[0] = 0;
		}
		else {
			free(attrib->serialno);
			attrib->serialno = strdup(can);
			*dirtyFlag = true;
		}
	}
	BControl *c;
	if (c = cast_as(FindView("doreg"),BControl)) c->SetEnabled(valid);
	if (c = cast_as(FindView("doupdate"),BControl)) c->SetEnabled(valid);
}

/////////////////////////////////////////////////////////////////////


PackageSetView::PackageSetView(BRect r)
	:	BView(	r,
				"psett",
				B_FOLLOW_ALL,
				B_WILL_DRAW)
{
	SetViewColor(light_gray_background);
}

class KeyFilter : public BMessageFilter
{
public:
	KeyFilter()
		: BMessageFilter(B_KEY_DOWN)
	{
	}
	virtual filter_result Filter(BMessage *m, BHandler **target)
	{
		//m->PrintToStream();
		//int8 b;
		//m->FindInt8("byte",&b);
		//if (isalnum(b) && !isdigit(b))
		//	return B_SKIP_MESSAGE;	
		return B_DISPATCH_MESSAGE;
	}
};


void PackageSetView::AttachedToWindow()
{
	BView::AttachedToWindow();

	const long tcOffset = 22;
	const long tcDivider = 160;	
	
	PackageSetWind *wind = (PackageSetWind *)Window();
	AttribData *attrib = wind->attrib;
		
	BRect r = Bounds();	

	r.InsetBy(10,6);
	
	r.bottom = r.top + 28;
	
	AddChild(new LabelView(r,"The following information is displayed to the user in\
 SoftwareValet:"));
	
/***	
	The following settings allow installed packages\
 to be tracked, registered, compared and updated using the SoftwareValet. Leaving\
 the Name, Version and Developer fields blank will disable SoftwareValet\
 functions."));
**/ 
 	BTextControl *tc;
 	
 	r.top = r.bottom + 4;
 	r.bottom = r.top + 16;
 	
 	tc = new BTextControl(r,"pname",
 				"Package Name:",
 				attrib->name,
 				new BMessage(S_PACK_NAME));
 	tc->SetDivider(tcDivider);
 	//tc->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	AddChild(tc);
	
	r.OffsetBy(0,tcOffset);
	
	tc = new BTextControl(r,"pvers",
 				"Version:",
 				attrib->version,
 				new BMessage(S_PACK_VERS));
 	tc->SetDivider(tcDivider);
 	//tc->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	AddChild(tc);
	
	r.OffsetBy(0,tcOffset);
	
	tc = new BTextControl(r,"pdev",
 				"Developer:",
 				attrib->developer,
 				new BMessage(S_PACK_DEV));
 	tc->SetDivider(tcDivider);
 	//tc->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	AddChild(tc);
	
	r.OffsetBy(0,tcOffset);
	
	char dateStr[20];
	struct tm *tp;
	tp = localtime(&attrib->releaseDate);
	
	//if (tp->tm_year >= 100)
	
	strftime(dateStr,20,"%m/%d/%Y",tp);
	//else
	//	strftime(dateStr,8,"%m/%d/%y",tp);
	
	wind->lastDate = strdup(dateStr);

	r.right -= 80;
	tc = new BTextControl(r,"pdate",
 				"Release Date (mm/dd/yr):",
 				dateStr,
 				new BMessage(S_PACK_DATE));
 	tc->SetDivider(tcDivider);
 	//tc->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	AddChild(tc);
	r.right += 80;
	
	r.OffsetBy(0,tcOffset);
	
	tc = new BTextControl(r,"pdesc",
 				"Description:",
 				attrib->description,
 				new BMessage(S_PACK_DESC));
 	tc->SetDivider(tcDivider - 80);
 	//tc->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	AddChild(tc);
	
	r.OffsetBy(0,tcOffset);

	BPopUpMenu *menu = new BPopUpMenu("Software Type");
	menu->AddItem(new BMenuItem("Commercial",new BMessage(S_SOFT_TYPE)));
	menu->AddItem(new BMenuItem("Trialware",new BMessage(S_SOFT_TYPE)));
	menu->AddItem(new BMenuItem("Freeware",new BMessage(S_SOFT_TYPE)));
	menu->AddItem(new BMenuItem("Shareware",new BMessage(S_SOFT_TYPE)));
	menu->AddItem(new BMenuItem("Beta",new BMessage(S_SOFT_TYPE)));
	menu->AddItem(new BMenuItem("Other",new BMessage(S_SOFT_TYPE)));
	BMenuField *mf = new BMenuField(r,"stype","Software Type:",menu);
	
	if (attrib->softType != 255)
		menu->ItemAt(attrib->softType)->SetMarked(true);
	else
		menu->ItemAt(5)->SetMarked(true);
	
	AddChild(mf);
		
	//r.OffsetBy(12,tcOffset + 8);
	
	//BCheckBox *cb = new BCheckBox(r,"updtc","New major version release",
	//						new BMessage(S_IS_UPDATE));
	//AddChild(cb);
	//cb->SetValue(!attrib->isUpdate);
	
	/// divider drawn here
	
	r.OffsetBy(0, tcOffset + 8);
	BRect rr = r;
	rr.bottom = rr.top + 28;
	AddChild(new LabelView(rr,"The following information should be filled in\
 to enable SoftwareValet features such as registration and updating:"));
	
	r.OffsetTo(rr.left, rr.bottom + 6);
	
	tc = new BTextControl(r,"dserial",
 				"SoftwareValet Enable Code:",
 				attrib->serialno,
 				new BMessage(S_DEPOT_SERIAL));
 	tc->SetDivider(tcDivider);
 	//tc->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	AddChild(tc);
	
	r.OffsetBy(20, tcOffset);
	
	BCheckBox *cb = new BCheckBox(r,"doreg","Allow user to send registration",
							new BMessage(S_DO_REG));
	AddChild(cb);
	cb->SetEnabled((attrib->serialno && *attrib->serialno));
	if (cb->IsEnabled()) cb->SetValue(attrib->doReg);
	
	r.OffsetBy(0, tcOffset);
	cb = new BCheckBox(r,"doupdate","Allow user to check for updates",
							new BMessage(S_DO_UPDATE));
	AddChild(cb);
	cb->SetEnabled((attrib->serialno && *attrib->serialno));
	if (cb->IsEnabled()) cb->SetValue(attrib->doUpdate);
	
	r.OffsetBy(-20, tcOffset + 8);
	rr = r;
	rr.bottom = rr.top + 42;
	
	AddChild(new LabelView(rr,"To distribute the package via means \
other than BeDepot ESD but with support for registration and updating, \
the following must be filled in. Contact Be, Inc. before launching."));
	
	r.OffsetTo(rr.left, rr.bottom + 6);
	
	tc = new BTextControl(r,"vid",
 				"Package Version ID:",
 				B_EMPTY_STRING,
 				new BMessage(S_VERSION_ID));
 	tc->SetDivider(tcDivider);
 	//tc->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	AddChild(tc);
	tc->SetText(attrib->versionID);
	tc->TextView()->AddFilter(new KeyFilter());
	
	r.OffsetBy(0, tcOffset);
	
	tc = new BTextControl(r,"pid",
 				"Serial Schema ID:",
 				B_EMPTY_STRING,
 				new BMessage(S_PREFIX_ID));
 	tc->SetDivider(tcDivider);
 	//tc->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	AddChild(tc);
	tc->SetText(attrib->prefixID);
	tc->TextView()->AddFilter(new KeyFilter());
}

void PackageSetView::Draw(BRect u)
{
	BView *v = FindView("stype");
	DrawHSeparator(2,Bounds().right - 2 ,v->Frame().bottom + 6,this);
	v = FindView("doupdate");
	DrawHSeparator(2,Bounds().right - 2 ,v->Frame().bottom + 6,this);
	
	BView::Draw(u);
}

