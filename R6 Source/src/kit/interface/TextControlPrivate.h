//
// TextControlPrivate.h
//
//   BTextListControl has a private class that inherits from _BTextInput_, so
//   _BTextInput_ must be defined in a common place.  It would be nice to
//   make _BTextInput_ public (and change its name) so that outside
//   developers could provide their own BTextViews for a BTextControl.
//   There are things that you cannot currently do by subclassing BTextControl
//   because you cannot override the BTextView inside it.  It is often useful
//   to override BTextView::InsertText() and DeleteText(), and you can't do
//   that the way things are structured now.

#ifndef TEXT_CONTROL_PRIVATE_H
#define TEXT_CONTROL_PRIVATE_H

#include <TextView.h>

/* -------------------------------------------------------------------- */

class _BTextInput_ : public BTextView {

public:
						_BTextInput_(BRect rect, BRect trect, ulong rMask, ulong flags);
						_BTextInput_(BMessage *data);
	virtual				~_BTextInput_();
	static	BArchivable	*Instantiate(BMessage *data);
	virtual	status_t	Archive(BMessage *data, bool deep = true) const;

	virtual void		KeyDown(const char *bytes, int32 numBytes);
	virtual void		MakeFocus(bool state);
	virtual void		FrameResized(float x, float y);
	virtual	void		Select(int32 startOffset, int32 endOffset);
	virtual void		Paste(BClipboard *clipboard);

	void				AlignTextRect();
	void				SetInitialText();

protected:

	virtual void		InsertText(const char				*inText, 
								   int32					inLength, 
								   int32					inOffset,
								   const text_run_array		*inRuns);
	virtual void		DeleteText(int32 fromOffset, int32 toOffset);


	char			*fInitialText;
	bool			fClean;
};

/* -------------------------------------------------------------------- */

#endif // TEXT_CONTROL_PRIVATE_H
