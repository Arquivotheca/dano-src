#include <Be.h>
#include "IconCache.h"
#include "MyDebug.h"

IconCache::IconCache(int size)
	:	fSize(size)
{
	fList = new IconCacheEntry[fSize];
}

IconCache::~IconCache()
{
	delete[] fList;
}

BBitmap	*IconCache::Icon(const char *mimetype, const char *signature)
{
	bool isApp = false;
	const char *key = mimetype;
	
	if (!strcmp(mimetype, B_APP_MIME_TYPE)) {
		if (signature) key = signature;
		else key = "application/x-vnd.Be-SCPT";
	}
		
	const char *c;
	uint ix = 0;	// important to have uint since we use this as index
	for (c = key; *c; c++) {
		ix += *c;
	}
	ix = ix % fSize;
	
	IconCacheEntry	*f = fList + ix;
	if (f->mime && !strcmp(key, f->mime))
		return f->icon;
	else {
		f->SetTo(key,isApp);
		return f->icon;	
	}
}


IconCacheEntry::IconCacheEntry()
	:	mime(NULL),
		icon(NULL)
{
}

IconCacheEntry::~IconCacheEntry()
{
	if (mime) free(mime);
	if (icon) delete icon;
}

void IconCacheEntry::SetTo(const char *type, bool isApp)
{
	isApp;
	
	status_t err;
	if (mime) {
		free(mime);
		PRINT(("Hash conflict!!!\n"));
	}
#if DEBUG
	else {
		PRINT(("new icon\n"));
	}
#endif
	mime = strdup(type);
	if (icon) delete icon;
	icon = new BBitmap(BRect(0,0,B_MINI_ICON-1,B_MINI_ICON-1),B_COLOR_8_BIT);
	

	BMimeType mt(mime);	
//		if (isApp) {
//			err = mt.GetIcon(icon,B_MINI_ICON);
//		}
//		else {
		err = mt.GetIcon(icon,B_MINI_ICON);
		if (err) {
			char app[B_MIME_TYPE_LENGTH];
			mt.GetPreferredApp(app);
			mt.SetTo(app);
			err = mt.GetIconForType(mime,icon,B_MINI_ICON);
		}
//		}
	if (err) {
		if (icon) delete icon;
		icon = NULL;
	}
}
