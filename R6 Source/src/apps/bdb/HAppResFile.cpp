/*
	HAppResFile.cpp
*/

#include "lib.h"
#include "HAppResFile.h"

#include <Application.h>
#include <Roster.h>
#include <Resources.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

using namespace HResources;

#if __BEOS__

BResources *gAppResFile = NULL;

static void InitAppResFile()
{
	app_info ai;
	be_app->GetAppInfo(&ai);
	
	BEntry entry(&ai.ref);
	
	BFile *file = new BFile;
	FailOSErr(file->SetTo(&entry, B_READ_ONLY));
	
	gAppResFile = new BResources;
	FailOSErr(gAppResFile->SetTo(file));
} // InitAppResFile

BMenuBar* HResources::GetMenuBar(BRect r, int id)
{
	BMenuBar *mbar = new BMenuBar(r, "mbar");
	FailNil(mbar);
	
	size_t size;
	const short *lst = (short *)GetResource('MBAR', id, size);
	FailNilRes(lst);
	
	for (uint32 i = 0; i < (size / 2); i++)
		mbar->AddItem(GetMenu(lst[i]));
	
	return mbar;
} /* GetMenuBar */

BMenu* HResources::GetMenu(int id, bool popup)
{
	size_t size;
	const char *m = (char *)GetResource('MENU', id, size);
	if (!m) throw HErr("Could not find resource!");
	
	BMemoryIO buf(m, size);
	BPositionIO& data = buf;
	
	char s[256];
	data >> s;
	
	BMenu *menu = popup ? new BPopUpMenu(s) : new BMenu(s);
	char type, key;
	long l;
	short modifiers;
	
	buf >> type;
	while (type)
	{
		switch (type)
		{
			case 1:
				data >> s >> l >> modifiers >> key;
				menu->AddItem(new BMenuItem(s, new BMessage(l), key, modifiers));
				break;
//			case 2:
//				break;
			case 3:
				menu->AddSeparatorItem();
				break;
			case 4:
				buf >> l;
				menu->AddItem(GetMenu(l));
				break;
			default:
				break;
		}
		buf >> type;
	}
	
	return menu;
} /* GetMenu */

const void* HResources::GetResource(unsigned long type, int id)
{
	if (gAppResFile == NULL)
		InitAppResFile();
	size_t size;
	return gAppResFile->LoadResource(type, id, &size);
} // HGetResource

const void* HResources::GetResource(unsigned long type, int id, size_t& size)
{
	if (gAppResFile == NULL)
		InitAppResFile();
	return gAppResFile->LoadResource(type, id, &size);
} // HGetResource

const void* HResources::GetResource(unsigned long type, int id, size_t& size, const char **name)
{
	if (gAppResFile == NULL)
		InitAppResFile();
	
	gAppResFile->GetResourceInfo(type, id, name, &size);
	return gAppResFile->LoadResource(type, id, &size);
} // HGetResource

const void* HResources::GetNamedResource(unsigned long type, const char *name)
{
	if (gAppResFile == NULL)
		InitAppResFile();
	
	size_t size;
	return gAppResFile->LoadResource(type, name, &size);
} // HResources::GetNamedResource

#else

const void* HGetResource(unsigned long type, int id)
{
	Handle h = ::GetResource(type, id);
	if (h)
	{
		::HLockHi(h);
		return *h;
	}
	else
		return NULL;
} // HGetResource

#endif
