// PackListView.h
#include "FListView.h"

#ifndef _PACKLISTVIEW_H
#define _PACKLISTVIEW_H


// for labelling rows in the view
enum {
	ICON_TAG,
	NAME_TAG,
	SIZE_TAG,
	COMPRESSED_SIZE_TAG,
	PERCENTAGE_TAG,
	FILES_TAG
};

// make this a template class!
// fix up public and private stuff
class ArchiveItem;
class ArchiveFolderItem;
class ArchivePatchItem;
class PackArc;

class IconCache;
class IconMenuItem;


class PackList : public FListView {
public:
					PackList(BRect frame,
							const char *name,
							ulong resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							ulong flags = B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);	
virtual				~PackList();
virtual 	void	MessageReceived(BMessage *msg);
virtual 	bool 	MessageDropped(BMessage *msg, BPoint pt, BPoint offset);
virtual		void	InvalidateItem(long index);
virtual 	void 	AttachedToWindow();
virtual 	float 	ItemHeight();
// invokes an item (as in double click)
virtual 	void	Invoke(long index);
//virtual 	void	Show();

void				ChangeName(BMessage *msg);

void 				ReadFromDisk(PackArc *archiveFile);
void				ChangeFolderLevel(BMessage *msg);
void				RefsReceived(BMessage *msg);
void 				RemoveItems(BMessage *msg);
void 				NewFolder(BMessage *msg);
void				AddPatch(BMessage *msg);
void				AddScript(BMessage *msg);
ArchivePatchItem	*SelectedPatch();

inline ArchiveFolderItem *CurrentFolder() {
						return currentlyViewing;
					};
inline ArchiveFolderItem *TopLevel() {
						return toplevel;
					};
		void	SetCurrentFolder(ArchiveFolderItem *fi);
		void	AddEntry(ArchiveItem *addItem);
		void	RemoveEntry(long index);
					
virtual void		SelectionSet();
virtual void 		ReorderItem(long prevItem, long curItem);

ArchiveFolderItem	*toplevel;
ArchiveFolderItem	*currentlyViewing;
BPopUpMenu			*foldersMenu;
IconMenuItem		*superItem;
BLocker				treeLock;
BLocker				deletePoolLock;
RList<ArchiveItem *>	*deletePool;

protected:
virtual 	void 	DrawItem(BRect update, long index, BRect *itemFrame = NULL);
		BBitmap		*gtBitmap;
		
private:
	IconCache		*iconCache;
};
#endif
