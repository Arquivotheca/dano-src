#ifndef _T_BOOKMARK_LIST_H
#define _T_BOOKMARK_LIST_H

#ifndef _LISTVIEW_H
#include <interface/ListView.h>
#endif

#include <support/String.h>

class TBookmarkItem;
class TBookmarkFile;

namespace BExperimental {
class BResourceSet;
class BBitmapButton;
};

using namespace BExperimental;

class BMessage;
class BBitmap;

BResourceSet& Resources();
const char* ResourceString(int32 id);
const BBitmap* ResourceBitmap(const char* name);

BBitmapButton* MakeBitmapButton(const char* name, int32 label,
								BMessage* message,
								const char* normalBM, const char* overBM,
								const char* pressedBM, const char* disabledBM,
								const char* disabledPressedBM);

#define SB_MAIN "strings.txt"

enum {
	SI_ADD_FOLDER,
	SI_ADD_BOOKMARK,
	SI_DELETE,
	SI_FOLDER
};

struct bookmark_attrs
{
	bookmark_attrs(const BMessage& attrs);
	~bookmark_attrs();
	
	rgb_color fBackgroundColor;
	rgb_color fFolderColor;
	rgb_color fOpenFolderColor;
	rgb_color fBookmarkColor;
	rgb_color fSelectedColor;
	rgb_color fActiveColor;
	rgb_color fTextColor;
	
	bool fUnderlineLinks;
	int32 fUnderlineHeight;
	int32 fUnderlineOffset;
	
	BFont fListFont;
	
	const BBitmap* fBackground;
	
	const BBitmap* fInfoBitmap;
	const BBitmap* fInfoOutsideBitmap;
	const BBitmap* fInfoOverBitmap;
	const BBitmap* fInfoOnBitmap;
	const BBitmap* fInfoOnOverBitmap;
	const BBitmap* fDeleteBitmap;
	const BBitmap* fDeleteOutsideBitmap;
	const BBitmap* fDeleteOverBitmap;
	const BBitmap* fFolderBitmap;
	const BBitmap* fFolderClosedBitmap;
	const BBitmap* fBookmarkBitmap;

private:
	const BBitmap* bitmap_from_resource(const BMessage& attrs,
										const char* param,
										const char* def_name,
										const BBitmap* def_bitmap=0);
};

class TBookmarkList : public BListView
{
public:
	TBookmarkList(BRect frame, const char* name,
				  const BMessage& attrs,
				  list_view_type type = B_SINGLE_SELECTION_LIST,
				  uint32 resizeMask = B_FOLLOW_LEFT |
									  B_FOLLOW_TOP,
				  uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS |
								 B_NAVIGABLE);
	virtual ~TBookmarkList();

	virtual	void SetDeletionMessage(BMessage *message);
	BMessage* DeletionMessage() const;
	uint32 DeletionCommand() const;
		
	void SetBookmarks(TBookmarkFile* bookmarks);
	TBookmarkFile* Bookmarks() const;
	
	void SetCurrentFolder(TBookmarkItem* folder);
	TBookmarkItem* CurrentFolder(uint32* level = 0, int32* index = 0) const;
	
	TBookmarkItem* CurrentBookmark() const;
	
	void SetCurrentEdit(int32 idx);
	void SetCurrentEdit(TBookmarkItem* item);
	int32 CurrentEdit() const;
	
	int32 OffsetAtLevel(uint32 level, int32 offset, int32 starting = 0) const;
	
	void AddBookmarks(const BList* bookmarks, TBookmarkItem* parent = 0);
	
	bool CanRemove(int32 item) const;
	int32 RemoveSelected(BList* out_bookmarks);
	
	void UpdateBookmarkList();
	
	void UpdateItem(int32 index);
	void ExpandItem(int32 index);
	void ContractItem(int32 index);
	
	virtual void SetViewColor(rgb_color color);
	
	virtual void MessageReceived(BMessage* msg);
	
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	
	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
	virtual void MouseUp(BPoint pt);
	
	virtual void Draw(BRect updateRect);
	
private:
	typedef BListView inherited;
	
	void start_watching();
	void stop_watching();
	void move_over_item(BPoint where, const BMessage* drag);
	
	bookmark_attrs fAttrs;
	rgb_color fViewColor;
	
	BMessage* fDeletionMessage;
	TBookmarkFile* fBookmarks;
	TBookmarkItem* fFolder;
	int32 fEditItem;
	int32 fHitItem;
	bool fPressingItem;
	bool fCanDrag;
	BPoint fInitPos;
	int32 fOverItem;
};

#endif
