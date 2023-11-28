#ifndef _DOICONS_H_
#define _DOICONS_H_

#include <Bitmap.h>
#include <Volume.h>

#define S_ICON_SIZE 16
#define L_ICON_SIZE	32

#define P_BTN_WIDTH 160
#define P_BTN_HEIGHT 52

extern BBitmap *gGenericAppIcon;
extern BBitmap *gYellowWarnIcon;

extern BBitmap *gGenericAppSIcon;
extern BBitmap *gGenericFolderSIcon;
extern BBitmap *gGenericFileSIcon;

extern BBitmap *gShopGraphic;
extern BBitmap *gDownloadGraphic;
extern BBitmap *gInstallGraphic;
extern BBitmap *gManageGraphic;
extern BBitmap *gPrefsGraphic;

extern BBitmap *gPkgIcon;

void		SetValetIcons(entry_ref appRef);

BBitmap		*SIconOfVolume(BVolume *vol);
void		*IconByType(ulong type, const char *kind = "smallBits");
void		MakeIconBitmap(	BBitmap **theBitmap,
						BRect rect,
						void *theData,
						color_space depth = B_COLOR_8_BIT);

#endif

