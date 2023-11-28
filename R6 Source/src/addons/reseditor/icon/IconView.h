// (c) 1997, Be Incorporated
//
//

#ifndef __ICON_VIEW__
#define __ICON_VIEW__

#include <Bitmap.h>
#include <Mime.h>
#include <Rect.h>
#include <View.h>

extern const BRect kLargeIconRect;
extern const BRect kMiniIconRect;

extern const rgb_color kLightGray;

class BMessage;

class IconView : public BView {
public:
	IconView(BRect, const char *, BBitmap *largeIcon, BBitmap *miniIcon,
		bool owning = true,
		uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
		// if <owning> is true the destructor and setters destroy the current
		// icon bitmaps
		
	virtual ~IconView();

	virtual void KeyDown(const char *, int32);
	
	// you may override these getters and setters if you 
	// are only interrested in the large or small icon respecitvely
	virtual const BBitmap *LargeIcon() const;
	virtual const BBitmap *MiniIcon() const;

	virtual BBitmap *LargeIcon();
	virtual BBitmap *MiniIcon();

	virtual void SetLargeIcon(BBitmap *);
	virtual void SetMiniIcon(BBitmap *);

	virtual void Draw(BRect);
	virtual void MakeFocus(bool focusState = false);
		// force focus to get updated
	virtual void MouseDown(BPoint );
		// to get focused and start drags
	virtual void MessageReceived(BMessage *);

	// Copy/Paste
	virtual void Cut();
	virtual void Paste();
	virtual void Copy();
	virtual void Clear();
	virtual bool AcceptsPaste() const;
	
	virtual bool InitiateDrag(bool wasFocused);
		// default rule initiates a drag if view clicked after it had
		// already been focused
		// override to return false if you do not want dragging out
		// of the icon view
	
	virtual bool AcceptsDrop(const BMessage *) const;
		// override to prevent dropping 

	virtual bool QuitRequested()
		{ return true; }

protected:
	// these setters are used only by cut/paste/clear calls
	// may be overriden to support undo buffering; also, for
	// non-owning icons, these should be overriden so that the owner
	// of the data can acquire the new bitmaps
	virtual void CutPasteSetLargeIcon(BBitmap *);
	virtual void CutPasteSetMiniIcon(BBitmap *);
	
private:
	typedef BView inherited;

	void GetLargeIcon(BMessage *);
		// embeds a large icon into the message
	void GetMiniIcon(BMessage *);
		// embeds a mini icon into the message
	
	// craw-style import
	bool AcceptsRawBits(const BMessage *) const;
	void ImportRawBits(BBitmap **large, BBitmap **mini, const BMessage *);

	bool SetIcons(BMessage *);
		// retrieves both icons from a message
		// common routine for decoding pastes and drops
	bool AcceptsCommon(const BMessage *) const;
	
	void DrawIconItem(BRect, const BBitmap *);
	
	BBitmap *largeBitmap;
	BBitmap *miniBitmap;
	bool owning;
};

#endif
