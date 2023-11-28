// ===========================================================================
//	BeInput.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __BEINPUT__
#define __BEINPUT__

#include "InputGlyph.h"

#include <TextView.h>
#include <ScrollView.h>
#include <String.h>
class BControl;
class BMenuItem;
class BPopUpMenu;
class BListView;

class BeInputImage : public InputImage {
public:
				BeInputImage(ConnectionManager *mgr, bool forceCache, BMessenger *listener, Document* htmlDoc);

virtual	void	Reset(DrawPort *drawPort);
				BeInputImage(Document* htmlDoc);
virtual	bool 	Clicked(float h, float v);			// Submit if clicked
virtual	void	Layout(DrawPort *drawPort);
virtual void	Click();
// If you have keyboard navigable input images, implement blur and focus, too.

protected:
		BView*	mView;	// set in layout
		bool	mSentMessage;
};

class BeInputText;

// ===========================================================================

class MonoText : public BTextView {
public:
			MonoText(BRect rect, char *name, BRect textRect, BeInputText* inputText,bool renderAsBullets);
	virtual	~MonoText();
	
	virtual	void	AttachedToWindow();
	virtual	void	KeyDown(const char *bytes, int32 numBytes);
	virtual	void	Draw(BRect updateRect);
	virtual void	MakeFocus(bool focus);
	virtual void	Paste(BClipboard *clipboard);
	virtual	void	InsertText(const char *inText, int32 inLength, int32 inOffset, const text_run_array *inRuns);
	virtual	void	DeleteText(int32 fromOffset, int32 toOffset);
	virtual void	DetachedFromWindow();
	virtual void	MessageReceived(BMessage *msg);
			void	SetTop(float top) {mViewTop = top;}
			void	SetLeft(float left) {mViewLeft = left;}
			void	Position();
		const char *ActualText();
			void	ForgetInputText() {mInputText = 0;}

protected:
			float	mViewTop,mViewLeft;
			float	mFrameTop, mFrameLeft;
			BeInputText* mInputText;
			bool	mRenderAsBullets;
			BString	mData;
};

// ===========================================================================

class BeInputText : public InputGlyph {
public:
						BeInputText(bool isPassword, Document* htmlDoc);
		virtual			~BeInputText();

		virtual	void	Reset(DrawPort *drawPort);
		virtual	void	Layout(DrawPort *drawPort);
		virtual	void	Draw(DrawPort *drawPort);
		virtual	void	SetAttributeStr(long attributeID, const char* value);
		virtual	void	SetTop(float top);
		virtual	void	SetLeft(float left);
		virtual void	OffsetBy(const BPoint& offset);
		virtual void	SetValue(const char *value, bool force);
		virtual InputValueItem 	*UserValues();
	virtual const char *GetValue();
				bool	SpecialKey(ulong aChar);

				void	SetTarget(DrawPort* drawPort,bool target);
				bool	IsTarget();
				void	SelectAll(DrawPort *drawPort);
				
				void	ForgetTextView() {mBTextView = 0;}

		virtual	void	Submit(uint32 encoding, BString& submission);
		virtual void	Init();

		virtual	float		GetMinUsedWidth(DrawPort *drawPort);	// Smallest Glyph width
		virtual float		GetMaxUsedWidth(DrawPort *drawPort);
		virtual void		Blur();
		virtual void		Focus();
		virtual void		Select();
		virtual void		Click();

		BString			mOnChangeScript;
#ifdef JAVASCRIPT
		virtual void		SetContainer(jseVariable container);

		jseVariable		mOnChange;
		virtual void	ValueChanged();
#endif

protected:
		MonoText*		mBTextView;
		bool			mFinal;
		BScrollBar*		mHScrollBar;
		BScrollBar*		mVScrollBar;
};

// ===========================================================================

class BeControl : public InputGlyph {
public:
						BeControl(Document* htmlDoc);
		virtual			~BeControl();
		
			const char*	GetTitle();
				void	Create(BView *view);

		virtual	bool	GetCheck();
		virtual	void	SetCheck(bool on, DrawPort *drawPort);

		virtual	void	Reset(DrawPort *drawPort);
		virtual	void	Layout(DrawPort *drawPort);
//		virtual	void	Draw(DrawPort *drawPort);
		virtual	void	Submit(uint32 encoding, BString& submission);
virtual InputValueItem	*UserValues();
		virtual void	Init();
		virtual	void	SetTop(float top);
		virtual	void	SetLeft(float left);
		virtual void	OffsetBy(const BPoint& offset);
		virtual void	SetValue(const char *value, bool force);
		virtual void		Blur();
		virtual void		Focus();
		virtual void		Click();

		virtual	float		GetMinUsedWidth(DrawPort *drawPort);	// Smallest Glyph width
		virtual float		GetMaxUsedWidth(DrawPort *drawPort);
		
protected:
		BControl*		mBControl;
		bool			mPositionFinal;
		bool			mSizeFinal;
};

// ===========================================================================
//	Class for a popup menu select thingy

class SelectView {
public:
					SelectView(bool multiple);
	virtual			~SelectView();

	virtual	void	GetOptimalSize(float *width, float *height);
	virtual	void	SetOptions(CLinkedList* options);

	virtual	bool	OptionIsMarked(int i);
	
	virtual int32	GetNumOptions();
	virtual int32	GetSelectedIndex();
	virtual void	SetSelectedIndex(int32 index);
	virtual Option*	OptionAt(int32 index);
	virtual void	UpdateOption(Option *o);
			
	virtual void	Submit(uint32 encoding, BString& name,BString& submission, InputGlyph *g);
	virtual	void	Reset();
	InputValueItem	*UserValues(InputValueItem *item);

protected:
		bool			mMultiple;
		CLinkedList*	mOptions;
};

// ===========================================================================
//	Class for a popup menu select thingy
class BeSelect;

class MenuView : public BView, public SelectView {
public:
					MenuView(BRect rect, const char *name, bool multiple, BeSelect* glyph);
	virtual			~MenuView();
	
	virtual	void	AttachedToWindow();
	virtual	void	Draw(BRect updateRect);
	virtual	void	MouseDown(BPoint point);
	virtual void	MakeFocus(bool focus);

			void	GetOptimalSize(float *width, float *height);
	virtual	void	SetOptions(CLinkedList* options);
virtual	void		KeyDown(const char *bytes, int32 numBytes);
	virtual void	SetSelectedIndex(int32 index);
	virtual void	UpdateOption(Option *o);
	BMenuItem*		GetItemAt(int i);
	BMenuItem*		GetMarked();
			
			bool	OptionIsMarked(int i);
			void	Reset();
			void	DoMenu(bool keepOpen);
			void	SetTop(float top) {mViewTop = top;}
			void	SetLeft(float left) {mViewLeft = left;}
			void	Position();

protected:
			float	mViewTop,mViewLeft;
			float	mFrameLeft, mFrameTop;
		BPopUpMenu*		mBPopUpMenu;
		float				mWidth;
		BeSelect*		mGlyph;
};


// ===========================================================================
//	Class for a list select

class ListView : public BScrollView, public SelectView {
public:
					ListView(BListView *target, int size, char *name, bool multiple, BeSelect *glyph);
	virtual			~ListView();

	virtual void	AttachedToWindow();
	
			void	GetOptimalSize(float *width, float *height);
	virtual	void	SetOptions(CLinkedList* options);
	virtual void	MakeFocus(bool focus);
	virtual void	SetSelectedIndex(int32 index);
	virtual void	UpdateOption(Option *o);
	virtual void	MessageReceived(BMessage *msg);

			bool	OptionIsMarked(int i);
			void	Reset();
			void	SetTop(float top) {mViewTop = top;}
			void	SetLeft(float left) {mViewLeft = left;}
			void	Position();

protected:
			float	mViewTop,mViewLeft;
			float	mFrameLeft, mFrameTop;
	BListView*	mListView;
	int			mSize;
	BeSelect*	mGlyph;
};

// ===========================================================================
//	Popup menu or selection thing

class BeSelect : public InputGlyph {
public:
						BeSelect(Document* htmlDoc);
		virtual			~BeSelect();
		
		virtual	void	Reset(DrawPort *drawPort);
		virtual	void	Layout(DrawPort *drawPort);
//		virtual	void	Draw(DrawPort *drawPort);
		virtual	void	Submit(uint32 encoding, BString& submission);
virtual InputValueItem	*UserValues();
		virtual void	Init();
		virtual	void	SetTop(float top);
		virtual	void	SetLeft(float left);
		virtual void	OffsetBy(const BPoint& offset);
		virtual void	SelectViewChanged();
		virtual	void	SetAttributeStr(long attributeID, const char* value);
		virtual int32	GetNumOptions();
		virtual int32	GetSelectedIndex();
		virtual void	SetSelectedIndex(int32 index);
		virtual Option*	OptionAt(int32 index);
		virtual void	UpdateOption(Option *o);
		virtual void		Blur();
		virtual void		Focus();

		virtual	float		GetMinUsedWidth(DrawPort *drawPort);	// Smallest Glyph width
		virtual float		GetMaxUsedWidth(DrawPort *drawPort);
#ifdef JAVASCRIPT
		virtual void		SetContainer(jseVariable container);
#endif
	
protected:
		SelectView*		mSelectView;
		bool			mPositionFinal;
		bool			mIsPopup;
		BString			mOnChangeScript;
#ifdef JAVASCRIPT
		jseVariable		mOnChange;
#endif
};


#endif


