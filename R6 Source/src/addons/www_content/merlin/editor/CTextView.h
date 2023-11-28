// ============================================================
//  CTextView.h	©1996 Hiroshi Lockheimer
// ============================================================



#include <TextView.h>


class KeyFilter;


class CTextView : public BTextView {
public:
						CTextView(BRect frame, const char *name, 
								  BRect textRect);

	virtual void		KeyDown(const char *bytes, int32 numBytes);
	virtual void		WindowActivated(bool state);
	virtual void		MessageReceived(BMessage *message);
	virtual void		FrameResized(float width, float height);
	virtual void		Paste(BClipboard *clipboard);
	virtual void		AttachedToWindow();
	virtual void		DetachedFromWindow();

	void				Search(const char *text, bool forward, bool wrap, 
							   bool sensitive);
	void				Replace(bool once, const char *text, 
								const char *newText, bool forward, 
								bool wrap, bool sensitive);

protected:
	bool				SearchForward(const char *text, bool wrap, 
									  bool sensitive);
	bool				SearchBackward(const char *text, bool wrap,
									   bool sensitive);

	bool				ReplaceForward(bool once, const char *text,
									   const char *newText, bool wrap, 
									   bool sensitive);
	bool				ReplaceBackward(bool once, const char *text,
										const char *newText, bool wrap, 
										bool sensitive);

	virtual void		InsertText(const char				*inText, 
								   int32					inLength, 
								   int32					inOffset,
								   const text_run_array		*inRuns);
	virtual void		DeleteText(int32 fromOffset, int32 toOffset);

	// Not used in BeIA
#if 0
	virtual void		GetDragParameters(BMessage	*drag,
										  BBitmap	**bitmap,
										  BPoint	*point,
										  BHandler	**handler);
#endif

};


