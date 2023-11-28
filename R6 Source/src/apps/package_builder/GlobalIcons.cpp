#include <Be.h>
#include "FArchiveItem.h"

#include "GlobalIcons.h"

#include <NodeInfo.h>
#include "MyDebug.h"
#include "Util.h"

BBitmap *gGenericFileIcon;
BBitmap *gGenericFolderIcon;
BBitmap *gGenericLinkIcon;
BBitmap *gTopLevelIcon;
BBitmap *gCDIcon;
BBitmap *gAutoIcon;
BBitmap *gPatchIcon;
BBitmap *gScriptIcon;

#define _dr9_

// grabs it from volume id 0...
void *IconByType(ulong type, const char *kind) 
{
	type;
#ifdef _dr9_
	if (strcmp(kind,"largeBits") == 0) {
		return malloc(32*32);
	}
	else if (strcmp(kind,"smallBits") == 0) {
		return malloc(16*16);	
	}
	else {
		PRINT(("ICON REQUEST ERROR!!!\n"));
		return NULL;
	}
#else
	BQuery iQuery;
	BVolume vol = boot_volume();
	BDatabase *db = vol.Database();
	record_id iconID;
	void *iconData;
			
	iQuery.AddTable(db->FindTable("Icon"));
	iQuery.PushField("type");
	iQuery.PushLong(type);
	iQuery.PushOp(B_EQ);
	iQuery.FetchOne();
	if (iQuery.CountRecordIDs() < 1) {
		doError("Icon by type query failed");
		return NULL;
	}
	else {
		iconID = iQuery.RecordIDAt(0);
		BRecord qRecord(db,iconID);
		long siz;
		void *data = qRecord.FindRaw(kind,&siz);
		iconData = malloc(siz);
		memcpy(iconData,data,siz);
		if (qRecord.Error() != B_NO_ERROR || iconData == NULL) {
			doError("Could not get icon data from query");
			return NULL;
		}
	}
	return iconData;
#endif
}

void	MakeIconBitmap(BBitmap **theBitmap,BRect rect,void *theData);

void	MakeIconBitmap(BBitmap **theBitmap,BRect rect,void *theData)
{
	if (theData) {
		*theBitmap = new BBitmap(rect,B_COLOR_8_BIT);
		(*theBitmap)->SetBits(theData,
						(*theBitmap)->BitsLength(),0,B_COLOR_8_BIT);
		free(theData);
	}
	else {
		*theBitmap = NULL;
	}
}
void	MakeTransparent(void *theData, long length, rgb_color transColor);

// 8 bit only!
void	MakeTransparent(void *theData, long length, rgb_color transColor)
{
	uchar	*data = (uchar *)theData;
	uchar	*max = data + length;
	uchar	tColor;
	{
		BScreen		screen;
		tColor = screen.IndexForColor(transColor);
	}
	while(data < max){
		if (*data == tColor)
			*data = B_TRANSPARENT_8_BIT;
		data++;
	}
}


void	SetGlobalIcons()
{
	void *iconBits;
	BRect iRect(0,0,S_ICON_SIZE - 1,S_ICON_SIZE - 1);
	
	/**
	iconBits = IconByType(0);
	MakeIconBitmap(&gGenericFileIcon,iRect,iconBits);

	iconBits = IconByType(-1L);
	MakeIconBitmap(&gGenericFolderIcon,iRect,iconBits);

	**/
	//iconBits = IconByType('PArc');
	//MakeIconBitmap(&gTopLevelIcon,iRect,iconBits);
	
	iconBits = IconByType('CDRM');
	MakeIconBitmap(&gCDIcon,iRect,iconBits);
	
	iconBits = IconByType('BAPP'/**,'3DZO' **/);
	MakeIconBitmap(&gAutoIcon,iRect,iconBits);
	
	size_t sz;
	app_info info;
	be_app->GetAppInfo(&info);
	
	BEntry			appEnt(&info.ref);
	BNode			node(&appEnt);
	BNodeInfo		nodeInf(&node);
	
	gTopLevelIcon = new BBitmap(BRect(0,0,15,15),B_COLOR_8_BIT);
	nodeInf.GetIcon(gTopLevelIcon,B_MINI_ICON);
	
	BFile			appFile(&info.ref,O_RDWR);
	BResources		appRes(&appFile);
	
	iconBits = appRes.FindResource('MICN',1000L,&sz);
	MakeIconBitmap(&gPatchIcon,iRect,iconBits);

	rgb_color tcolor = {255,255,255,0};
	iconBits = appRes.FindResource('8BMP',"foldersmall",&sz);
	MakeTransparent(iconBits,sz,tcolor);
	MakeIconBitmap(&gGenericFolderIcon,iRect,iconBits);
	
	iconBits = appRes.FindResource('8BMP',"filesmall",&sz);
	MakeTransparent(iconBits,sz,tcolor);
	MakeIconBitmap(&gGenericFileIcon,iRect,iconBits);
	
	iconBits = appRes.FindResource('8BMP',"scriptsmall",&sz);
	MakeTransparent(iconBits,sz,tcolor);
	MakeIconBitmap(&gScriptIcon,iRect,iconBits);
	
	BMimeType	mime("application/x-vnd.Be-symlink");
	
	gGenericLinkIcon = new BBitmap(BRect(0,0,B_MINI_ICON-1,B_MINI_ICON-1),B_COLOR_8_BIT);
	mime.GetIcon(gGenericLinkIcon,B_MINI_ICON);
}
