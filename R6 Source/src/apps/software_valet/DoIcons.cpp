#include "DoIcons.h"
#include <Resources.h>

BBitmap *gShopGraphic;
BBitmap *gDownloadGraphic;
BBitmap *gInstallGraphic;
BBitmap *gManageGraphic;
BBitmap *gPrefsGraphic;
BBitmap *gPkgIcon;

char *gClickSound;
long gClickSoundSize;

char *gClickUpSound;
long gClickUpSoundSize;

#include "colors.h"
#include "Util.h"

#include "MyDebug.h"

void	SetValetIcons(entry_ref appRef)
{
	void *iconBits;
	BRect iRect(0,0,L_ICON_SIZE - 1,L_ICON_SIZE - 1);
	BRect sRect(0,0,S_ICON_SIZE-1,S_ICON_SIZE-1);
	BRect bRect(0,0,P_BTN_WIDTH-1,P_BTN_HEIGHT-1);

	BFile		resFile(&appRef,O_RDONLY);
	BResources	appRes(&resFile);
	size_t siz;
	
	/// button graphics
	
	iconBits = appRes.FindResource('BMAP',1L,&siz);
	//SwapComponents(iconBits,siz);
	MakeIconBitmap(&gShopGraphic,bRect,iconBits,B_RGB_32_BIT);

	iconBits = appRes.FindResource('BMAP',2L,&siz);
	//SwapComponents(iconBits,siz);
	MakeIconBitmap(&gInstallGraphic,bRect,iconBits,B_RGB_32_BIT);

	iconBits = appRes.FindResource('BMAP',3L,&siz);
	//SwapComponents(iconBits,siz);
	MakeIconBitmap(&gManageGraphic,bRect,iconBits,B_RGB_32_BIT);

	iconBits = appRes.FindResource('BMAP',4L,&siz);
	//SwapComponents(iconBits,siz);
	MakeIconBitmap(&gPrefsGraphic,bRect,iconBits,B_RGB_32_BIT);
	
	iconBits = appRes.FindResource('BMAP',5L,&siz);
	//SwapComponents(iconBits,siz);
	MakeIconBitmap(&gDownloadGraphic,bRect,iconBits,B_RGB_32_BIT);
	
	// sounds
	gClickSound = (char *)appRes.FindResource('RSND',0L,&siz);
	gClickSoundSize = siz;
	
	gClickUpSound = (char *)appRes.FindResource('RSND',1L,&siz);
	gClickUpSoundSize = siz;

	/// package icon
	iconBits = appRes.FindResource('8BMP',"pkgicon",&siz);
	MakeIconBitmap(&gPkgIcon,iRect,iconBits,B_COLOR_8_BIT);
}
