#include <Application.h>
#include <ClassInfo.h>

#include "RegisterWindow.h"
#include "RegisterView.h"
#include "PackageItem.h"
#include "SimpleListView.h"
#include "PackageItem.h"

#include "Util.h"
#define MODULE_DEBUG 0
#include "MyDebug.h"

extern long	gSubCount;

RegisterWindow::RegisterWindow(	PackageItem *pkgItem,
								BMessenger updt,
								bool quitWhenDone/*,
								bool freeP*/)
	:	BWindow(BRect(0,0,280,380),B_EMPTY_STRING,B_TITLED_WINDOW,
					B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	Lock();
	fQuitWhenDone = quitWhenDone;
//	if (freeP) {
//		sel = pkgItem;
//	}
//	else {
		// set subwindow count
		PRINT(("adding to sub count\n"));
		sel = NULL;
		atomic_add(&gSubCount,1);
//	}
	
	// see what we're trying to register
	
	BMessage	curPkg;
	PackageItem *curItem;
	SimpleListView	*lv = NULL;
	long low = 1;
	long hi = 1;
			
	if (!pkgItem) {
		PRINT(("----------pkg Item is NULL\n"));
			
		BLooper *loop;
		lv = cast_as(updt.Target(&loop),SimpleListView);
		ASSERT(lv);
		low = lv->LowSelection();
		hi = lv->HighSelection();
	}
	else {
		curItem = pkgItem;
	}
				
	bool showSerial = true;
	
	int registerCount = 0;
	for (long i = low; i <= hi && low >= 0; i++) {
		if (!pkgItem) {
			// multiple selection
			curItem = (PackageItem *)lv->ItemAt(i);
			if (curItem->selected == FALSE)
				continue;
		}
		if (curItem->ValetSupported() && curItem->data.FindBool("regservice"))
		{
			BMessage response;
			
			const char *sid = curItem->data.FindString("sid");
			const char *pid = curItem->data.FindString("pid");
			const char *vid = curItem->data.FindString("vid");

			bool haveSerialID = (sid && *sid);	
			bool havePrefixID = (pid && *pid);
			bool haveVersionID = (vid && *vid);
			
			if (!haveSerialID) {
			
				if (!haveVersionID) {
					// can't register/update no info to send up
					if (low == hi)
						doError("The package %s does not contain the necessary information for registration.\n\n\
Either the package developer did not enter this information or there was an error when the package \
was downloaded.",curItem->data.FindString("package"));

					continue;
				}
				
				// no prefix id needed, the server will find this from version id
				pid = B_EMPTY_STRING;
				
				// shareware
				if (curItem->data.FindInt32("softtype") == PackageItem::SOFT_TYPE_COMMERCIAL) {
					if (low != hi) {
						doError("The package %s requires you to enter a serial number. You cannot \
register this package as part of a selection of packages until you register it individually and \
enter your product serial number.",
						curItem->data.FindString("package"));
			 			
			 			registerCount = 0;
			 			break;
					}
					else {
						// we will prompt them for serial number in the window
					}
				}
				else {
					showSerial = false;
				}
			}
			else {
				// migration
				if (strcmp(sid,"old") == 0) sid = B_EMPTY_STRING;
				// we have a serial id that we know to be associated with a
				// version id in the database so we don't pass it up
				
				// MKK 3/17/98 we don't want to blank out vid in case serial generation is supported
				// vid = B_EMPTY_STRING;
				// but we do need a prefixID
				if (!havePrefixID) {
					// can't register/update no prefix id
					if (low == hi)
						doError("The package %s does not contain the necessary information for updating.\n\n\
Either the package developer did not enter this information or there was an error when the package \
was downloaded.",curItem->data.FindString("package"));
					continue;
				}
			}
			curPkgs.AddPointer("items",curItem);
			curPkgs.AddString("sid",sid);
			curPkgs.AddString("pid",pid);
			curPkgs.AddString("vid",vid);
			
			registerCount++;
			PRINT(("adding pkgname %s\n",curItem->data.FindString("package")));
		}
		else if (low == hi) {
			doError("Registration is not supported for the package \"%s\".",
				curItem->data.FindString("package"));
 			registerCount = 0;
 			break;
		}
	}

	if (registerCount > 0) {	
		char tilbuf[128];
		if (registerCount > 1) {
			sprintf(tilbuf,"Multiple Registration");
			showSerial = false;
		}
		else {
			sprintf(tilbuf,"Register \"%s\"",pkgItem->data.FindString("package"));				
		}
		SetTitle(tilbuf);			
		PositionWindow(this,0.5,0.5);
	
		BView *v = new RegisterView(Bounds(),curPkgs,updt,showSerial);
		AddChild(v);
		
		Show();
		// make sure we are at the front!
		Activate(true);
	}
	else {
		delete this;
		return;
	}
	
	Unlock();
}

void RegisterWindow::Quit()
{
	PRINT(("REGWINDOW quit\n"));
	if (fQuitWhenDone) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	BWindow::Quit();
}

RegisterWindow::~RegisterWindow()
{
	PRINT(("RegisterWindow::~RegisterWindow"));
	if (sel)
		delete sel;
	else
		atomic_add(&gSubCount,-1);
}
