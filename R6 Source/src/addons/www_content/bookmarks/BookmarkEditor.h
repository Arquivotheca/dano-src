#ifndef _T_BOOKMARK_EDITOR_H
#define _T_BOOKMARK_EDITOR_H

#ifndef _CONTROL_H
#include <interface/Control.h>
#endif

#include <support/String.h>

class TBookmarkList;
class BScrollView;
class TBookmarkFile;

enum {
	T_BOOKMARK_SET_FRAME		= 'Tbsf'
	/** Fields:
	*** (BRect) "frame" New frame for view.
	**/
};

class TBookmarkEditor : public BControl
{
public:
	TBookmarkEditor(BRect frame, const char* name,
					const BMessage& attrs,
					TBookmarkFile* bookmarks = 0,
					uint32 resizeFlags = B_FOLLOW_ALL,
					uint32 flags = B_WILL_DRAW | B_NAVIGABLE_JUMP | B_FRAME_EVENTS);
	virtual ~TBookmarkEditor();

	void SetBookmarks(TBookmarkFile* bookmarks);
	TBookmarkFile* Bookmarks() const;
	
	virtual void AttachedToWindow();
	virtual void AllAttached();
	virtual void DetachedFromWindow();
	virtual void MessageReceived(BMessage* msg);
	virtual void FrameMoved(BPoint new_position);
	virtual void FrameResized(float width, float height);
	virtual void GetPreferredSize(float* width, float* height);
	
	virtual void SetViewColor(rgb_color color);
	virtual void SetHighColor(rgb_color color);
	
	virtual void OpenURL(TBookmarkItem* it);
	
private:
	typedef BView inherited;
	
	void LayoutViews(float width, float height);
	void UpdateBackgrounds();
	void UpdateControlState();
	
	void UpdateBookmarks();
	
	TBookmarkFile* fBookmarks;
	bool fOwnBookmarks;
	
	const BBitmap* fBackground;
	
	bool fHaveViewColor;
	bool fHaveHighColor;
	
	TBookmarkList* fList;
	BScrollView* fScroller;
};

#endif
