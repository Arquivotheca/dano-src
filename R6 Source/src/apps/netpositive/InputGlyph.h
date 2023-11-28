// ===========================================================================
//	InputGlyph.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __INPUT__
#define __INPUT__

#include "Glyph.h"
#ifdef JAVASCRIPT
#include "jseopt.h"
#include "seall.h"
#include "HTMLDoc.h"
#endif

#include <SupportDefs.h>
#include <String.h>
class BMessenger;

class ImageGlyph;
class Image;
class ConnectionManager;

//===================================================================================
//	Simple class to remember options and values for select

class Option : public CLinkable {
public:
					Option();
					Option(Option *o);

			void	Set(const char *optionStr,bool hasValue,const char *valueStr, bool selected, const char *optGroupLabel);

			BString	mOption;
			BString	mValue;
			BString	mOptGroupLabel;
			bool	mSelected;
			bool	mOn;
			bool	mHasValue;
};

// ===========================================================================
 
class Form;
class InputValueItem;

#ifdef JAVASCRIPT
class InputGlyph : public CompositeGlyph, public ContainerUser
#else
class InputGlyph : public CompositeGlyph
#endif
{
public:
						InputGlyph(Document* htmlDoc, bool hidden = false);
	virtual				~InputGlyph();
                        
            void		SetForm(Form *form);
            Form		*GetForm();
            
	virtual	void		SetAttribute(long attributeID, long value, bool isPercentage);
	virtual	void		SetAttributeStr(long attributeID, const char* value);
	virtual void		SetUserValues(const char *value, bool checked, CLinkedList *list);
	virtual InputValueItem	*UserValues();
	//	Used by inputSelect

	virtual	void		AddOption(const char *itemStr, bool hasValue, const char *valueStr, bool selected);
	virtual void		OpenOptGroup(const char *label);
	virtual void		CloseOptGroup();

	virtual	bool		IsTextField();
	virtual	bool		IsRadio();
	virtual	bool		IsCheckBox();
	virtual	bool		IsReset();
	virtual	bool		IsSubmit();
	virtual	bool		IsPassword();
	virtual bool		IsImage();
	virtual bool		IsSelect();
	virtual bool		IsHidden();

			void		SetType(short type);
			const char*	GetName();

	virtual	void		SetCheck(bool on, DrawPort *drawPort);
	virtual	bool		GetCheck();
	virtual bool		GetOrigCheck() {return mOrigChecked;}
	virtual	bool		TrackClick(long *value,DrawPort *drawPort);

	virtual void		Init();
	virtual	void		Reset(DrawPort *drawPort);
	virtual	void		Submit(uint32 encoding, BString& submission);
	
	void		ParseValue(const char *value, BString& parsedValue);	// Input
	void		Encode(uint32 encoding, BString& raw,BString& encoded, bool stripSpaces = true);					// Output

	virtual	bool		Idle(DrawPort *drawport);
	virtual	bool		Key(DrawPort *drawPort, short key);
	virtual	void		SetTarget(DrawPort *drawPort, bool makeTarget);
	virtual	void		SelectAll(DrawPort *drawPort);
	virtual	ImageGlyph*	GetImage();
	virtual const char *GetValue() {return mValue.String();}
	virtual void		SetValue(const char *value, bool force) {mValue = value;}
	virtual const char *GetOrigValue() {return mOrigValue.String();}
	virtual void		Blur();
	virtual void		Focus();
	virtual void		Select();
	virtual void		Click();
#ifdef JAVASCRIPT
	virtual void		SetContainer(jseVariable container);
	virtual void		ExecuteOnClickScript();
	virtual void		ValueChanged();
#endif

protected:
			Form		*mForm;
			short		mType;
			BString		mName;
			BString		mValue;
			BString		mOrigValue;
			BString		mOptGroupLabel;

			short		mWrapType;			
			bool		mChecked;
			bool		mOrigChecked;
			bool		mMultiple;
			short		mRows,mCols;
			long		mMaxLength;
			short		mSize;
			bool		mHilite;
			bool		mHidden;
			
			CLinkedList	mOption;	// Select options ...
			CLinkedList	mOrigOption;

			bool		mUserValuesSet;
#ifdef JAVASCRIPT
			jseVariable	mOnClick;
			BString		mOnClickScript;
#endif
};

// ===========================================================================

class InputImage : public InputGlyph {
public:
					InputImage(ConnectionManager *mgr, bool forceCache, BMessenger *listener, Document *htmlDoc);

	virtual	ImageGlyph*	GetImage();

	virtual	bool		IsSubmit();

	virtual	void		SetAttribute(long attributeID, long value, bool isPercentage);
	virtual	void		SetAttributeStr(long attributeID, const char* value);

	virtual	void		Submit(uint32 encoding, BString& submission);
	virtual	bool		Clicked(float h, float v);

protected:
			ImageGlyph*	mImage;
};

// ===========================================================================

// ===========================================================================

class ImageButton : public InputGlyph {
public:
						ImageButton(Document* htmlDoc);
		virtual			~ImageButton();

				void	SetImages(Image *onImage, Image *offImage, Image *dimImage);
				void	SetDim(bool dim, DrawPort *drawPort);

		virtual	void	Layout(DrawPort *drawPort);
		virtual	void	Draw(DrawPort *drawPort);
		virtual	void 	Hilite(long value, DrawPort *drawPort);

protected:
				Image*	mOnImage;
				Image*	mOffImage;
				Image*	mDimImage;
				short	mOn;
				bool	mDim;
};

#endif
