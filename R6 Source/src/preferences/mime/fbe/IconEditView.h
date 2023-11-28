#ifndef ICON_EDIT_VIEW_H
#define ICON_EDIT_VIEW_H

#include "IconView.h"

class TIconWindow;

const int32 kEditorClosing = 'FBCL';

class IconEditView : public IconView {
public:
	IconEditView(BRect, const char *,
		BBitmap *largeIcon, BBitmap *miniIcon,
		bool owning = true,
		uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
	
	virtual ~IconEditView();
	
	virtual void MouseDown(BPoint);
	
	void SetIconMimeType(char *mimeType);
	
	virtual void ApplyChange(BMimeType *mime=NULL);
	
	virtual void ShowIconEditor();
	
	virtual void CutPasteSetLargeIcon(BBitmap *);
	virtual void CutPasteSetMiniIcon(BBitmap *);

	virtual bool QuitRequested();
	void MessageReceived(BMessage *);

protected:
	virtual bool IconEditorQuitRequested();
	bool dirty;

private:
	bigtime_t fLastTime;
	TIconWindow *fIconWindow;	
	char fSelectedMimeType[B_MIME_TYPE_LENGTH + 1];
};

#endif
