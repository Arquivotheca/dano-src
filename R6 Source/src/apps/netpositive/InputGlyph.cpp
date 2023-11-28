// ===========================================================================
//	InputGlyph.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include <UTF8.h>

#include "InputGlyph.h"
#include "HTMLTags.h"
#include "Builder.h"
#include "Parser.h"
#include "Form.h"
#include "ImageGlyph.h"
#include "HTMLWindow.h"
#include "BeInput.h"
#include "HTMLDoc.h"

#include <stdio.h>

//===================================================================================
//	SelectOption: Simple class to remember options and values for select

Option::Option() : mSelected(false), mOn(false)
{
}

Option::Option(
	Option	*o)
{
	mOption = o->mOption;
	mValue = o->mValue;
	mSelected = o->mSelected;
	mOn = o->mOn;
	mHasValue = o->mHasValue;
	mOptGroupLabel = o->mOptGroupLabel;
}

void Option::Set(const char *optionStr,bool hasValue,const char *valueStr, bool selected, const char *optGroupLabel)
{
	mOption = optionStr;
	mValue = valueStr;
	mSelected = selected;
	mHasValue = hasValue;
	mOptGroupLabel = optGroupLabel;
}

// ===========================================================================

InputGlyph::InputGlyph(Document* htmlDoc, bool hidden) :
	CompositeGlyph(htmlDoc), mHidden(hidden)
{
	mType = AV_TEXT;	// Text is default input
	mRows = 1;
	mCols = 20;
	mMaxLength = 0;

	mMultiple = 0;		// For select
	mSize = 1;

	mAlign = AV_ABSMIDDLE;
	mForm = 0;
	mChecked = mOrigChecked = false;
	mWrapType = AV_OFF;

	mUserValuesSet = false;
#ifdef JAVASCRIPT
	mContainer = 0;
#endif
}

InputGlyph::~InputGlyph()
{
}

#ifdef JAVASCRIPT
void InputGlyph::SetContainer(jseVariable container)
{
	DocAndWindowAutolock lock(mHTMLDoc);
	mContainer = container;
	if (mOnClickScript.Length())
		mOnClick = mHTMLDoc->AddHandler(container, "onClick", mOnClickScript.String());
	else
		mOnClick = 0;
}

void InputGlyph::ExecuteOnClickScript()
{
	if (mOnClick) {
		DocAndWindowAutolock lock(mHTMLDoc);
		mHTMLDoc->ExecuteHandler(mContainer, mOnClick);
	}
}

void InputGlyph::ValueChanged()
{
	if (mContainer) {
		DocAndWindowAutolock lock(mHTMLDoc);
		mHTMLDoc->UpdateObject(mContainer);
	}
}
#endif

void InputGlyph::Blur()
{
}


void InputGlyph::Focus()
{
}


void InputGlyph::Select()
{
}


void InputGlyph::Click()
{
}


//	Form that input glyph lives inside

void InputGlyph::SetForm(Form *form)
{
	mForm = form;
}
 
Form *InputGlyph::GetForm()
{
	return mForm;
}

void InputGlyph::SetCheck(bool, DrawPort *)
{
}

bool InputGlyph::GetCheck()
{
	return 0;
}
	
// Track a click in a mac "control"

bool InputGlyph::TrackClick(long *value,DrawPort *)
{
	*value = 0;
#ifdef MAC
	do {
		Point where;
		::GetMouse(&where);
		if (*value != Clicked(where.h,where.v)) {
			*value ^= 1;
			Hilite(*value,drawPort);
		}
	} while (::StillDown());
	if (*value)
		Hilite(0,drawPort);
#endif
	return *value;
}

bool InputGlyph::IsImage()
{
	return mType == AV_IMAGE;
}

bool InputGlyph::IsSelect()
{
	return dynamic_cast<BeSelect*>(this);
}

bool InputGlyph::IsHidden()
{
	return mType == AV_HIDDEN;
}

bool InputGlyph::IsTextField()
{
	return mType == AV_TEXT;
}

bool InputGlyph::IsRadio()
{
	return mType == AV_RADIO;
}

bool InputGlyph::IsCheckBox()
{
	return mType == AV_CHECKBOX;
}

bool InputGlyph::IsReset()
{
	return mType == AV_RESET;
}

bool InputGlyph::IsSubmit()
{
	return mType == AV_SUBMIT;
}

bool	InputGlyph::IsPassword()
{
	return mType == AV_PASSWORD;
}


//	The set attribute stuff is probably too general

void InputGlyph::SetAttribute(long attributeID, long value, bool)
{
	switch (attributeID) {
		case A_TYPE:		mType = value;		break;
		case A_CHECKED:		
			mChecked = mOrigChecked = 1;		
			break;
		case A_MAXLENGTH:	mMaxLength = value;	break;	// Buffer Length for AV_TEXT or AV_PASSWORD
		case A_MULTIPLE:	mMultiple = true;	break;	// Multiple selection for SELECT
		case A_COLS:		mCols = value;		break;
		case A_ROWS:		mRows = value;		break;

		case A_SIZE:
			if (mType == T_SELECT)
				mSize = value;	// Number of items visible in selection list
			else
				mCols = value;	// Width of Field for AV_TEXT or AV_PASSWORD
			break;

		case A_WRAP:
			mWrapType = value;
			break;
	}
}

//	Remember name and value

void InputGlyph::SetAttributeStr(long attributeID, const char* value)
{
	switch (attributeID) {
		case A_NAME:	mName = (char*)value;		break;
		case A_VALUE:	
			ParseValue(value,mOrigValue);	
			mValue = mOrigValue;
			break;
#ifdef JAVASCRIPT
		case A_ONCLICK:
			DocAndWindowAutolock lock(mHTMLDoc);
			mHTMLDoc->InitJavaScript();
			mOnClickScript = value;
			break;
#endif
	}
}

void
InputGlyph::SetUserValues(
	const char	*value,
	bool		checked,
	CLinkedList	*list)
{
	if (!mHidden) {
		mValue = value;
		mChecked = checked;
		if (list != NULL) {
			mOption.DeleteAll();
			for (Option *o = (Option *)list->First(); o != NULL; o = (Option *)o->Next()) 
				mOption.Add(new Option(o));
		}
	
		mUserValuesSet = true;
	}
}

InputValueItem*
InputGlyph::UserValues()
{
	CLinkedList *list = NULL;

	if (mOption.First() != NULL) {
		list = new CLinkedList();
		for (Option *o = (Option *)mOption.First(); o != NULL; o = (Option *)o->Next()) 
			list->Add(new Option(o));
	}

	return (new InputValueItem(mValue.String(), mChecked, list));
}

void InputGlyph::AddOption(const char *itemStr, bool hasValue, const char *valueStr, bool selected)
{
	Option *origopt = new(Option);
	origopt->Set(itemStr,hasValue,valueStr,selected, mOptGroupLabel.String());
	mOrigOption.Add(origopt);

	if (!mUserValuesSet) {
		Option *opt = new(Option);
		opt->Set(itemStr,hasValue,valueStr,selected, mOptGroupLabel.String());
		mOption.Add(opt);
	}
}

void InputGlyph::OpenOptGroup(const char *label)
{
	if (label)
		mOptGroupLabel = label;
	else
		mOptGroupLabel = "";
}

void InputGlyph::CloseOptGroup()
{
	mOptGroupLabel = "";
}

bool InputGlyph::Idle(DrawPort *)
{
	return 0;
}

void InputGlyph::SetTarget(DrawPort *, bool)
{
}

void InputGlyph::SelectAll(DrawPort *)
{
}

ImageGlyph *InputGlyph::GetImage()
{
	return 0;
}

bool InputGlyph::Key(DrawPort *, short)
{
	return 0;
}

void InputGlyph::SetType(short type)
{
	mType = type;
}

const char* InputGlyph::GetName()
{
	return mName.String();
}

void
InputGlyph::Init()
{
}

//	Reset to default values

void InputGlyph::Reset(DrawPort *)
{
	mValue = mOrigValue;
	mChecked = mOrigChecked;
	mOption.DeleteAll();
	for (Option *o = (Option *)mOrigOption.First(); o != NULL; o = (Option *)o->Next())
		mOption.Add(new Option(o));
}

// Submit current values

void InputGlyph::Submit(uint32 encoding, BString& submission)
{
	submission = "";
	if (!mHidden) {
		CropTrailingSpace(mName);
		CropTrailingSpace(mValue);
	}

	if (mName.Length() > 0) {
		BString str;
		Encode(encoding, mName,str);
		submission = str;
		submission += "=";
		Encode(encoding, mValue,str, !mHidden);
		submission += str;
	}
}

//	Parse values for &amp; encoded characters

void InputGlyph::ParseValue(const char *value, BString& parsedValue)
{
	BigStr str;
	char c;
						
	while ((bool)(c = value[0])) {
	
		if (c == '&') {
			int i = 0;
			char amps[8];					// Look for ; or if the amp string is 6 chars
			while (value[i+1]) {
				if (value[i+1] == ';')
					break;
				if (i == 6)					// Max amp sequence is six chars
					break;
				amps[i] = value[i+1];
				i++;
			}
			amps[i] = 0;
			
			bool cantBe;

			uint32 enc = (mForm != NULL) ? mForm->Encoding() : 
										   B_MS_WINDOWS_CONVERSION;
			c = AmpersandCharacter(amps,&cantBe, enc);	// Lookup amp sequence
			if (c != 0)
				value += (1 + i);			// & + string + ;
			else
				c = '&';
		}
		
		str.Add(c);
		value++;
	}
	
	str.Finish();
	parsedValue = str.Chars();
}

//	See if this char needs to be encoded

bool NeedsEncode(char c)
{
	if (c < 0x20 || c == 0x7F)
		return true;
//	 return strchr("%+&=#;/?:$!,'()",c) != NULL;
	return strchr("!\"#$%&'()*+,-/:;<=>?@[\\]^`{|}~",c) != NULL;
}
	
//	Encode the data stored in the name/value string

void InputGlyph::Encode(uint32 encoding, BString& raw,BString& encoded, bool stripSpaces)
{
	static char hex[] = "0123456789ABCDEF";
	
	encoded = "";
	if (raw.Length() == 0)
		return;
		
	BString		convertedText = "";
	const char	*text = raw.String();
	int32		length = raw.Length();
	int32		i = 0;
	int32		convState = 0;

	if (encoding == N_AUTOJ_CONVERSION)
		encoding = B_MS_WINDOWS_CONVERSION;

	if (encoding == N_NO_CONVERSION)
		convertedText.SetTo(text, length);
	else {

	while (i < length) {
		int32	srcLen = length - i;
		int32	dstLen = 256;
		char	dst[256 + 1];
		
		if ( convert_from_utf8(encoding, 
							   text + i, 
							   &srcLen, 
							   dst, 
							   &dstLen,
							   &convState) != B_NO_ERROR )
			break;

		dst[dstLen] = '\0';
		convertedText += dst;
	
		i += srcLen;		
	}

	}

//	Strip trailing spaces

	if (stripSpaces)
		CropTrailingSpace(convertedText);
	
	const char *r = convertedText.String();
			
//	Encode all chars that need it, convert spaces to '+'

	BigStr bigStr;

	while (r[0]) {
		if (NeedsEncode(r[0])) {
			if (r[0] == 0x0A)
				bigStr.AddStr("%0D%0A");				// Newlines are lf/cr
			else {
				bigStr.Add('%');
				bigStr.Add(hex[((uchar)r[0]) >> 4]);
				bigStr.Add(hex[((uchar)r[0]) & 0xF]);
			}
		} else {
			if (r[0] == ' ')
				bigStr.Add('+');	// Convert spaces to pluses
			else
				bigStr.Add(r[0]);
		}
		r++;
	}
	bigStr.Finish();
	
	encoded = bigStr.Chars();
}

// ===========================================================================
//	InputImage - Uses a regular image..
//	It does not draw itself, it lets the image glyph do the work...

InputImage::InputImage(ConnectionManager *mgr, bool forceCache, BMessenger *listener, Document* htmlDoc) :
	InputGlyph(htmlDoc)
{
	mImage = new ImageGlyph(mgr, forceCache, listener, htmlDoc);
	mImage->SetAttribute(A_ISMAP,0,0);
}

//	Input image pretends to be a submit button

bool InputImage::IsSubmit()
{
	return true;
}

ImageGlyph* InputImage::GetImage()
{
	return mImage;
}

void InputImage::SetAttribute(long attributeID, long value, bool isPercentage)
{
	if (attributeID != A_ALIGN || value > 0)
		mImage->SetAttribute(attributeID,value,isPercentage);
	InputGlyph::SetAttribute(attributeID,value,isPercentage);
}

void InputImage::SetAttributeStr(long attributeID, const char* value)
{
	mImage->SetAttributeStr(attributeID,value);
	InputGlyph::SetAttributeStr(attributeID,value);
}

void InputImage::Submit(uint32 encoding, BString& submission)
{
	float h,v;
	mImage->GetClickCoord(&h,&v);
	
	BString name;
	Encode(encoding, mName,name);
	const char *namestr = name.String();
	if (!namestr)
		namestr = "";
		
	char s[1024];
	if (*namestr)
		sprintf(s,"%s.x=%1.0f&%s.y=%1.0f",namestr,h,namestr,v);
	else
		sprintf(s,"x=%1.0f&y=%1.0f",h,v);
	submission = s;
}

bool InputImage::Clicked(float h, float v)
{
	return mImage->Clicked(h,v);
}
