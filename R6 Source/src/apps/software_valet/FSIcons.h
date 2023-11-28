#ifndef _FSICONS_H_
#define _FSICONS_H_

#include <Bitmap.h>
#include <Entry.h>
#include <Volume.h>

#define S_ICON_SIZE 16

extern BBitmap *gGenericFileSIcon;
extern BBitmap *gGenericFolderSIcon;

extern BBitmap *gTopLevelIcon;
extern BBitmap *gYellowWarnIcon;

void		SetGlobalIcons(entry_ref appRef);

BBitmap		*SIconOfVolume(BVolume *vol);
void		*IconByType(ulong type, const char *kind = "smallBits");
//BBitmap		*SIconByRecord(BRecord *r);

#endif

