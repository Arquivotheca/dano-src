#include "SetupWindow.h"
#include "LabelView.h"
#include "Util.h"
#include "SettingsManager.h"
#include "PackageDB.h"
#include "NameDialog.h"
#include "MyDebug.h"
#include "ValetVersion.h"
#include <time.h>
#include <CheckBox.h>
#include <ClassInfo.h>
#include <Control.h>
#include <Path.h>
#include <FindDirectory.h>
#include <SymLink.h>
#include <AppFileInfo.h>
#include <Query.h>
#include <Box.h>
#include <Resources.h>
#include <Bitmap.h>
#include <Roster.h>
#include <StringView.h>
#include <String.h>
#include <Button.h>

// NOTE: As of the 4.5 release, the SetupWindow is much different than previous
//		 releases.  If SoftwareValet is being run for the first time, the package
//       database will be updated, and the setup window will ask the user if they
//       wish to register the BeOS.  If they choose to register, SoftwareValet
//       will open the registration window for the BeOS package.  If not, SoftwareValet
//       will not ask again.

extern SettingsManager *gSettings;
void	WaitForWindow(BWindow *w);

SetupWindow::SetupWindow(bool forceShow)
	:	BWindow(BRect(0,0,480,346), "SoftwareValet Setup", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	// find the pref for the last version installed on the drive
	const char *lastVers = gSettings->data.FindString("firstlaunch/version");
	if (!lastVers) {
#if __INTEL__
			if (gSettings->data.FindBool("firstlaunch/done"))
				lastVers = "010500";
			else
#endif
				lastVers = B_EMPTY_STRING;
	}
	int c = strcmp(lastVers, kInternalVersionNumberString);
//	if (!strcmp(lastVers, "010501")) {
//		// if 1.51, don't pop up settings window because nothing
//		// has really changed in terms of setup since 1.51
//		c = 0;
//	}
	if (!forceShow && c >= 0) {
		delete this;
		return;
	} else {
		Lock();
		PositionWindow(this,0.5,0.3);
		AddChild(new SetupView(Bounds()));
		Unlock();
		Show();
	}	
}

void		SetupWindow::GetButtons()
{
	BView *v = FindView("register");
	while (v) {
		BCheckBox *c = cast_as(v,BCheckBox);
		if (c != NULL) {
			cnfg.AddBool(c->Name(),c->Value() == B_CONTROL_ON);
		}
		v = v->NextSibling();
	}
}


PackageItem *SetupWindow::InitializePackageDB()
{
	// create a registry item for SoftwareValet
	PackageDB	db;
	PackageItem	*it = new PackageItem();
	
	bool found(false);
	
	// set time to June 1, 1999
	struct tm tm; 
	memset(&tm,0,sizeof(struct tm));
	tm.tm_year = 1999 - 1900;	// 1999
	tm.tm_mon = 5;				// June
	tm.tm_mday = 1;				// 1st
	
	time_t	releaseDate = mktime(&tm);
	
	if (db.FindPackage(it,kProductNameString,kVersionNumberString,NULL) >= B_OK ||
		db.FindNextOlderVersion(it, kProductNameString, NULL, releaseDate) >= B_OK)
	{
		// found the closest version for an update
		found = true;
	}
	BMessage	&data = it->data;

	ReplaceString(&data,"package", kPackageNameString);
	ReplaceString(&data,"version", kVersionNumberString);
	ReplaceString(&data,"developer", kPackageDevelString);
	ReplaceString(&data,"description",kPackageDescString);
	ReplaceInt32(&data,"softtype", PackageItem::SOFT_TYPE_COMMERCIAL);
	ReplaceInt32(&data,"mediatype", PackageItem::MEDIA_TYPE_PHYSICAL);	
	ReplaceInt32(&data,"releasedate", releaseDate);
	ReplaceInt32(&data,"installdate", time(0));
	ReplaceInt64(&data,"installsize", 0); // is this correct?
	// registered (don't touch)
	//ReplaceString(&data,"sid",attr->downloadInfo.FindString("sid"));
	ReplaceString(&data,"pid","10000");
#if __INTEL__
	ReplaceString(&data,"vid","1749637");	// Valet 4.5 x86
#else
	ReplaceString(&data,"vid","1749689");	// Valet 4.5 PPC
#endif
	ReplaceString(&data,"depotsn","3030819070327364");
	ReplaceBool(&data,"regservice", true);	// enable registration
	ReplaceBool(&data,"upservice", true);	// enable update

	if (found) {
		PRINT(("calling db write package\n"));
		db.WritePackage(it,PackageDB::BASIC_DATA);
	}
	else {
		PRINT(("calling db add package\n"));
		db.AddPackage(it);
	}
	
	return it;
}


//void		SetTransceiverAuto(bool autoStart);

bool		SetupWindow::QuitRequested()
{
	Hide();	
	BMessage	*msg = CurrentMessage();
	if (msg && msg->HasPointer("source")) {
		BControl	*c;
		msg->FindPointer("source",(void **)&c);
		if (strcmp(c->Name(),"now") == 0) {
			// create package item and save it in the DB
			PackageItem *it = InitializePackageDB();

			//// perform setup
			GetButtons(); // populates cnfg
			
			BEntry		appEnt;
			app_info	info;
			be_app->GetAppInfo(&info);
			appEnt.SetTo(&info.ref);
			
//			if (cnfg.FindBool("troll")) {
//				SetTransceiverAuto(true);
//				
//				ReplaceBool(&gSettings->data,"comm/listenerauto",true);
//			}
			ReplaceString(&gSettings->data,"firstlaunch/version", kInternalVersionNumberString);
			gSettings->SaveSettings();
			
			RemoveOldVersions(&appEnt);			
			
			if (cnfg.FindBool("register"))
			{
				BMessage reg('IReg');
				reg.AddPointer("package",it);
				reg.AddBool("quit_after_reg", false);
				be_app->PostMessage(&reg);
			}
			else {
				// delete the package item
				delete it;
			}
		}
	}
	return true;
}

int verscompare(int amaj, int amid, int amin, int bmaj, int bmid, int bmin);

void		SetupWindow::RemoveOldVersions(BEntry *appEnt)
{
	// remove old versions
	// criteria
	// name = SoftwareValet
	// size < XXXX
	// BEOS:APP_SIG = "application/x-scode-SValet"
	
	// 1.51
	version_info	cur;
	cur.major = kVersionInfo.major;
	cur.middle = kVersionInfo.middle;
	cur.minor = kVersionInfo.minor;
	{
		BFile			f(appEnt,O_RDONLY);
		BAppFileInfo	info(&f);
		info.GetVersionInfo(&cur, B_APP_VERSION_KIND);
	}
	const char *valetQuery =
		"(name = SoftwareValet && BEOS:APP_SIG = application/x-scode-SValet) || \
(name = 'SoftwareValet Transceiver' && BEOS:APP_SIG = application/x-scode-VList)";
	
	BQuery	query;
	
	query.Clear();
	BVolume	vol;
	appEnt->GetVolume(&vol);
	query.SetVolume(&vol);
	query.SetPredicate(valetQuery);
	query.Fetch();
	
	bool prompted = false;
	
	entry_ref	ref;
	while (query.GetNextRef(&ref) >= B_OK) {
		
		
		BFile	file(&ref,O_RDONLY);
		BAppFileInfo	info(&file);
		if (info.InitCheck() >= B_OK)
		{
			version_info	vers;
			info.GetVersionInfo(&vers, B_APP_VERSION_KIND);
			
			if (verscompare(vers.major, vers.middle, vers.minor,
					cur.major, cur.middle, cur.minor) < 0)
			{
				if (!prompted) {
					int res = doError("One or more older versions of SoftwareValet were found on your drive.\n\n\
It is recommended that you not use older versions of SoftwareValet. Would you like to remove the old \
versions?",NULL,"Don't Remove","Remove");
					
					if (res == 0)
						break;					
					prompted = true;
				}
				BEntry	rm(&ref);
				rm.Remove();
			}
		}
	}
}

void		MakeIconBitmap(	BBitmap **theBitmap,
						BRect rect,
						void *theData,
						color_space depth = B_COLOR_8_BIT);
						
class GraphicBox : public BBox
{
public:
	GraphicBox(BRect r)
		:	BBox(r),
			splashGraphic(NULL)
	{
		void *iconBits;
		BRect bRect(0,0,176-1,288-1);

		app_info	info;
		be_app->GetAppInfo(&info);
		BFile		resFile(&info.ref,O_RDONLY);
		BResources	appRes(&resFile);
		size_t siz;
		iconBits = appRes.FindResource('BMAP',12L,&siz);
		//SwapComponents(iconBits,siz);
		MakeIconBitmap(&splashGraphic,bRect,iconBits,B_RGB_32_BIT);
	}
	virtual			~GraphicBox()
	{
		delete splashGraphic;
	}
	
	virtual void	Draw(BRect r)
	{
		DrawBitmapAsync(splashGraphic);
		//rgb_color color = HighColor();
		//SetHighColor(220,180,180);
		//FillRect(r);
		//SetHighColor(color);
		BBox::Draw(r);
	}
private:
	BBitmap	*splashGraphic;
};


SetupView::SetupView(BRect f)
	:	BView(f,B_EMPTY_STRING,B_FOLLOW_ALL,B_WILL_DRAW)
{
	BPath		appPath;
	BEntry		appEnt;
	app_info	info;
	be_app->GetAppInfo(&info);
	appEnt.SetTo(&info.ref);
	appEnt.GetPath(&appPath);

	SetViewColor(light_gray_background);
	
	BRect r = f;
	r.OffsetTo(0,0);
	r.InsetBy(8,8);
	
	BRect boxr = r;
	r.right -= 8 + 166;
	boxr.left = r.right;
	
	BStringView *sv;
	LabelView	*lv;
	BFont		pFont;
	pFont.SetSize(18.0);
	pFont.SetFamilyAndStyle("Baskerville","Bold");
	
	BFont		dFont(*be_plain_font);
	dFont.SetSize(10.0);
	
	r.bottom = r.top + 20;
	sv = new BStringView(r,B_EMPTY_STRING,"Welcome to SoftwareValet!");
	AddChild(sv);
	sv->SetFont(&pFont);

	r.OffsetBy(0,r.Height() + 10);
	r.bottom = r.top + 14*8;
	
	BString label;
	label << "SoftwareValet" << B_UTF8_TRADEMARK << " is a utility that allows you "
		  << "to install many popular software packages, and to easily download "
		  << "updates for those packages and for the BeOS" << B_UTF8_TRADEMARK
		  << " from the Internet.\n\nThis setup window is being shown because "
		  << "this version of SoftwareValet" << B_UTF8_TRADEMARK << " has not "
		  << "been run previously on this computer or the SoftwareValet"
		  << B_UTF8_TRADEMARK << " settings file has been deleted.";
		
	lv = new LabelView(r, label.String());
	AddChild(lv);
	lv->SetFontAndColor(&dFont);
	r.OffsetBy(0, r.Height() + 10);
	r.bottom = r.top + 14;
	BRect dr;
	const int lvind = 32;

	////////////
	label.SetTo("Register the BeOS");
	label << B_UTF8_TRADEMARK << " via the Internet";
	BCheckBox *cb = new BCheckBox(r,"register",label.String(), new BMessage('CbSt'));
	AddChild(cb);
	cb->SetValue(true);
	dr = r;
	/////////
	
	r.bottom = boxr.bottom - 6;
	r.top = r.bottom - 24;
//	r.left = boxr.left;
//	r.right = r.left + 80;
//	AddChild(new BButton(r,"later","Setup Later",new BMessage(B_QUIT_REQUESTED)));
	
	r.right = boxr.right - 4;
	r.left = r.right - 80;
	BButton *btn = new BButton(r,"now","Continue",new BMessage(B_QUIT_REQUESTED));
	AddChild(btn);
	btn->MakeDefault(true);
	
	////////
		
	boxr.bottom = r.top - 12;
	
	AddChild(new GraphicBox(boxr));
	boxr.OffsetTo(0,0);
}
