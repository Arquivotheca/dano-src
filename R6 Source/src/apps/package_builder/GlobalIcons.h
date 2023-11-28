#ifndef _GLOBALICONS_H
#define _GLOBALICONS_H


#define S_ICON_SIZE 16

extern BBitmap *gGenericFileIcon;
extern BBitmap *gGenericFolderIcon;
extern BBitmap *gGenericLinkIcon;
extern BBitmap *gTopLevelIcon;
extern BBitmap *gCDIcon;
extern BBitmap *gAutoIcon;
extern BBitmap *gPatchIcon;
extern BBitmap *gScriptIcon;

void	SetGlobalIcons();
// BBitmap		*SIconOfVolume(BVolume *vol);
void		*IconByType(ulong type, const char *kind = "smallBits");

#endif
