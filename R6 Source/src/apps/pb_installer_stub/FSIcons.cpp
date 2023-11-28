// FSIcons.cpp
#include "FSIcons.h"
#include "Util.h"
#include <NodeInfo.h>
#include <sys/types.h>
#include "MyDebug.h"
#include <fs_info.h>
#include <Screen.h>
#include <math.h>
#include <Resources.h>




BBitmap *gGenericFileSIcon;
BBitmap *gGenericFolderSIcon;
BBitmap *gTopLevelIcon;
BBitmap *gYellowWarnIcon;

#define doError(x)


#ifdef __cplusplus
	extern "C" {
#endif
		int32 _kstatfs_(dev_t device, int32, int32, int32, fs_info *);
#ifdef __cplusplus
	}
#endif

BBitmap *SIconOfVolume(BVolume *vol)
{
	BBitmap *smallIcon;
	BRect rect(0,0,B_MINI_ICON-1,B_MINI_ICON-1);
	smallIcon = new BBitmap(rect,B_COLOR_8_BIT);
	
	fs_info info;
	if (!_kstatfs_(vol->Device(),0/*NULL*/,-1,0/*NULL*/,&info)) {
		get_device_icon(info.device_name,smallIcon->Bits(),B_MINI_ICON);
	}
	return smallIcon;
}

void		MakeIconBitmap(	BBitmap **theBitmap,
						BRect rect,
						void *theData,
						color_space depth = B_COLOR_8_BIT);
						
void		MakeTransparent(void *theData, long length, rgb_color transColor);

// 8 bit only!
void	MakeTransparent(void *theData, long length, rgb_color transColor)
{
	BScreen screen;
	uchar	*data = (uchar *)theData;
	uchar	*max = data + length;
	uchar	tColor = screen.IndexForColor(transColor);
	while(data < max){
		if (*data == tColor)
			*data = B_TRANSPARENT_8_BIT;
		data++;
	}
}

void	MakeIconBitmap(BBitmap **theBitmap,
						BRect rect,
						void *theData,
						color_space depth)
{
	if (theData) {
		long imgSize = (long)((1+fabs(rect.right - rect.left))*(1+fabs(rect.bottom - rect.top)));
		color_space rdepth = depth;
		if (depth == B_RGB_32_BIT || depth == B_BIG_RGB_32_BIT) {
			// rdepth = B_BIG_RGB_32_BIT;
			imgSize = imgSize * 3;
		}
	
		*theBitmap = new BBitmap(rect,depth);
		ASSERT(*theBitmap);
		(*theBitmap)->SetBits(theData,imgSize,0,rdepth);
		
		#if 0
		#if B_HOST_IS_LENDIAN
			if (depth == B_RGB_32_BIT) {
				uint32	*data = (uint32 *)(*theBitmap)->Bits();
				uint32	*max = data + (*theBitmap)->BitsLength() / 4;
				while(data < max){
					*data = B_BENDIAN_TO_HOST_INT32(*data);
					data++;
				}
			}
		#endif
		#endif
		free(theData);
	}
	else {
		*theBitmap = NULL;
	}
}


void	SetGlobalIcons(entry_ref appRef)
{
	void *iconBits;
	BRect iRect(0,0,S_ICON_SIZE - 1,S_ICON_SIZE - 1);
	
	//iconBits = IconByType(0,"smallBits");
	//MakeIconBitmap(&gGenericFileIcon,iRect,iconBits);

	//iconBits = IconByType(-1L,"smallBits");
	//MakeIconBitmap(&gGenericFolderIcon,iRect,iconBits);
	//BMimeType	mim(B_APP_MIME_TYPE);
	
	//gGenericFolderIcon = new BBitmap(iRect,B_COLOR_8_BIT);
	//mim.GetIcon(gGenericFolderIcon,B_MINI_ICON);

	
	//mim.SetType("application/Folder");
	//mim.GetIcon(gGenericFolderIcon,B_MINI_ICON);

/***
	BNode	folder("/boot/system/");
	BNodeInfo	ninf(&folder);
	char mStr[B_MIME_TYPE_LENGTH];

	gGenericFolderIcon = new BBitmap(iRect, B_COLOR_8_BIT);
	ninf.GetIcon(gGenericFolderIcon,B_MINI_ICON);
	ninf.GetType(mStr);
	PRINT(("mime type is %s\n",mStr));
***/

	uint32 length;
	BFile			resFile(&appRef,O_RDONLY);
	BResources		appRes(&resFile);

	iconBits = appRes.FindResource('8BMP',"yellowicon",&length);
	if (!iconBits) {
		PRINT(("icon bits is null\n"));
		iconBits = malloc(32*32);
	}
	rgb_color whiteColor = {255,255,255,0};

	MakeTransparent(iconBits,length,whiteColor);
	MakeIconBitmap(&gYellowWarnIcon,BRect(0,0,31,31),iconBits);
	
	iconBits = appRes.FindResource('8BMP',"filesmall",&length);
	MakeTransparent(iconBits,length,whiteColor);
	MakeIconBitmap(&gGenericFileSIcon,iRect,iconBits);
	
	iconBits = appRes.FindResource('8BMP',"foldersmall",&length);
	MakeTransparent(iconBits,length,whiteColor);
	MakeIconBitmap(&gGenericFolderSIcon,iRect,iconBits);
}
