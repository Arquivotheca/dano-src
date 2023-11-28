// ===========================================================================
//	Form.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "Form.h"
#include "HTMLTags.h"
#include "InputGlyph.h"
#include "MessageWindow.h"
#include "HTMLDoc.h"

#include <UTF8.h>
#include <List.h>
// ===========================================================================

Form::Form(Document *doc) : mMethod(AV_GET), mSelect(NULL), mTextArea(NULL), mWaitingForOption(false)
{
	mEncoding = B_MS_WINDOWS_CONVERSION;
	mHTMLDoc = doc;
	mInputList = new BList;
#ifdef JAVASCRIPT
	mContainer = NULL;
#endif
}

Form::~Form()
{
	delete(mInputList);
}

void
Form::SetEncoding(
	uint32	encoding)
{
	mEncoding = encoding;
}

uint32
Form::Encoding()
{
	return (mEncoding);
}

void Form::OpenForm(const char* action, short method, const char* targetFrame, const char *name, const char *enctype)
{
	mAction = action;
	mMethod = method;
	mTargetFrame = targetFrame;
	mName = name;
	mEnctype = enctype;
}

void Form::AddInput(InputGlyph *glyph)
{
	mInputList->AddItem(glyph);
	glyph->SetForm(this);
	//glyph->Reset(0);
	glyph->Init();
}

InputGlyph* Form::GetNextElement(InputGlyph *previousElement)
{
	if (mInputList->CountItems() == 0)
		return NULL;
	if (previousElement == NULL)
		return (InputGlyph *)mInputList->ItemAt(0);
	int32 index = mInputList->IndexOf(previousElement);
	if (index < 0)
		return (InputGlyph *)mInputList->ItemAt(0);
	if (index == mInputList->CountItems())
		return NULL;
	return (InputGlyph *)mInputList->ItemAt(index + 1);
}



//	Let the form have the first crack at the input text

bool Form::AddText(const char* text, long textCount)
{
	if (mSelect && mWaitingForOption) {
		mOptionText += text;
		return true;
	}
	return false;
}

void Form::CloseOption()
{
	mWaitingForOption = false;
	if (!mSelect)
		return;

	CropTrailingSpace(mOptionText);
	
	mSelect->AddOption(mOptionText.String(),mHasValue,mSelectValue.String(),mSelected);
}

void Form::OpenSelect(InputGlyph* select)
{
	if (mWaitingForOption)
		CloseOption();
	
	mSelect = select;
	AddInput(select);
}

void Form::OpenOptGroup(const char *label)
{
	if (mWaitingForOption)
		CloseOption();
	mSelect->OpenOptGroup(label);
}

void Form::CloseOptGroup()
{
	if (mWaitingForOption)
		CloseOption();
	mSelect->CloseOptGroup();
}

void Form::Option(bool selected, char* value)
{
	if (!mSelect)
		return;
		
//	Two consecutive <OPTION>'s result in null option text

	if (mWaitingForOption)
		CloseOption();
	
//	Interpret value

	mSelectValue = value;
	// Be careful here.  If value is NULL, then there was no value attribute.
	// If it's an empty string, then there was a blank value attribute.  We have
	// to handle these cases differently.
	mHasValue = (value);
//	mHasValue = (value && *value);
	mSelected = selected;
	mWaitingForOption = true;
	mOptionText = "";
}

InputGlyph* Form::CloseSelect()
{
	if (mWaitingForOption)
		CloseOption();
	InputGlyph *g = mSelect;
	mSelect = NULL;
	return g;
}

void Form::OpenTextArea(InputGlyph* text)
{
	if (mWaitingForOption)
		CloseOption();
	
	mTextArea = text;
	mTextAreaText = "";
}

void Form::AddTextAreaText(const char *text)
{
	if (mTextArea == 0)
		return;
	mTextAreaText += text;
}

void	Form::CloseTextArea()
{
	if (mTextArea) {
		mTextArea->SetAttributeStr(A_VALUE,mTextAreaText.String());
		mTextAreaText = "";
		AddInput(mTextArea);
	}
	mTextArea = NULL;
}

//	Handy for counting nad getting input items

short Form::CountInput()
{
	return mInputList->CountItems();
}

InputGlyph *Form::GetInput(short itemIndex)
{
	return (InputGlyph *)mInputList->ItemAt(itemIndex);
}

short Form::GetInputIndex(InputGlyph *item)
{
	for (short i = 0; i < CountInput(); i++)
		if (GetInput(i) == item)
			return i;
//	NP_ASSERT(false);
	return -1;
}

//	Returns the current targeted text glyph, if any

InputGlyph* Form::GetTarget()
{
	InputGlyph *g;
	for (short i = 0; (bool)(g = GetInput(i)); i++)
		if (g->IsTextField()) {
			//if (((InputText *)g)->IsTarget())
				return g;
		}
	return 0;
}

// Is form the target for keys?

bool Form::IsTarget()
{
	return GetTarget() != 0;
}

// Tell Form not to be target

void Form::Untarget(DrawPort *drawPort)
{
	InputGlyph *g = GetTarget();
	if (g)
		g->SetTarget(drawPort,false);	// Untarget current
}

//	Make this inputText the target for keystrokes

void Form::TargetText(InputGlyph *inputText, DrawPort *drawPort)
{
	InputGlyph *target = GetTarget();
	if (!inputText) {					//	Tabbing case
		short count = CountInput();
		short j = target ? GetInputIndex(target) + 1 : 0;	// Start after last field
		for (short i = 0; i < count; i++) {
			inputText = GetInput((i + j) % count);
			if (inputText->IsTextField() || inputText->IsPassword()) {
				if (inputText == target)
					return;				// Tabbed to myself
				break;
			}
		}
		if (!inputText) return;
	}

//	Set a new target for keystrokes

	if (target)
		target->SetTarget(drawPort,false);	// Untarget current
	inputText->SetTarget(drawPort,true);		// Target new
}

//	Deselect any other radio button in group....
//	.. when a radio button is selected

void Form::RadioGroup(InputGlyph *radio, DrawPort *drawPort)
{
	const char* s = radio->GetName();
	for (short i = 0; i < CountInput(); i++) {
		InputGlyph *g = GetInput(i);
		if (g->IsRadio() && !strcmp(s,g->GetName()))
			g->SetCheck(g == radio,drawPort);
	}
}

//	Reset the form, redraw if changed

void Form::Reset(DrawPort *drawPort)
{
#ifdef JAVASCRIPT
	if (mOnResetScript.Length()) {
		mHTMLDoc->ExecuteHandler(mContainer, mOnReset);
		return;
	}
#endif
	for (short j = 0; j < CountInput(); j++)
		GetInput(j)->Reset(drawPort);
}

//	Called when a user clicks in a button
 
void Form::ButtonPushed(InputGlyph *button, bool checked, DrawPort *drawPort)
{
	if (button->IsRadio())
		RadioGroup(button,drawPort);
    
    if (button->IsCheckBox())
    	button->SetCheck(!checked,drawPort);
    	
	if (button->IsReset())
		Reset(drawPort);

	if (button->IsSubmit())
		Submit(button);
}

//	Key

bool Form::Key(DrawPort *drawPort, short key)
{
	short i;
	InputGlyph *target = GetTarget();
	if (!target) return false;
	
	switch (key) {
		case 0x09:						// Tab key
			TargetText(0,drawPort);
			target = GetTarget();
			if (target != 0)
				target->SelectAll(drawPort);
			break;
		case 0x03:
		case 0x0D:						// Return or enter
			for (i = 0; i < CountInput(); i++) {
				InputGlyph *g = GetInput(i);
				if (g->IsSubmit()) {
					target->SelectAll(drawPort);	// Give some feedback
					g->Hilite(1,drawPort);
					//drawPort->Wait(20);
					g->Hilite(0,drawPort);
					return Submit(g);	// Send form on Return or enter
				}
			}
			break;
		default:
			target->Key(drawPort,key);
	}
	return 0;
}

//	Build the form data

bool	Form::Submit(InputGlyph* submitGlyph)
{
	mFormData = "";
	
#if 0
//	Only add the value of the submitting glyph if there is more than one submit button
	int submitCount = 0;
	for (short i = 0; i < CountInput(); i++)
		if (GetInput(i)->IsSubmit())
			submitCount++;
#endif
//	Don't add value of submitting glyph if it is a text field, it will get added below

	//	Submit values from everybody else
	BString submission;				// Add all the submissions

	InputGlyph *g;
	for (short i = 0; i < CountInput(); i++) {
		g = GetInput(i);

		if (g && !(g->IsSubmit() || g->IsReset())) {
			submission = "";
			g->Submit(mEncoding, submission);
			if (submission.Length() != 0) {
				if (mFormData.Length() != 0)
					mFormData += "&";
				mFormData += submission;
			}
		}
	}

	if (submitGlyph && submitGlyph->IsTextField() == false) {
		submission = "";
		submitGlyph->Submit(mEncoding, submission);
		if (submission.Length() != 0) {
			if (mFormData.Length() != 0)
				mFormData += "&";
			mFormData += submission;
		}
	}
	if (mFormData.Length())
		pprintBig("%s", mFormData.String());
	return mFormData.Length() != 0;
}

// Atomic click for the mac
// All useful responses are passed thru PushButton

void Form::Click(float, float, DrawPort *)
{
#ifdef MAC
	InputGlyph *g;
	long value = 0;
	for (short i = 0; i < CountInput(); i++) {
		g = GetInput(i);
		bool on = 0;
		if (g->Clicked(h,v)) {
			if (g->IsTextField()) {
    			TargetText(g,drawPort);						// Text Target
    			g->TrackClick(&value,drawPort);
			} else {
				if (g->TrackClick(&value,drawPort))
					ButtonPushed(g,g->GetCheck(),drawPort);	// Push a button!
			}
		}
	}
#endif
}

//	An enter key in a text glyph may trigger a submission
//	Return submit glyph if there is one submit and one text field.

InputGlyph* Form::SubmitTextField(InputGlyph *textGlyph)
{
	//If there are more than one text items and no submit item, don't submit
	//otherwise, submit on either a submit input or hitting enter.  'Enter' submission
	//will include the first submit button value in the form.
	
	InputGlyph *workingItem = NULL;
	InputGlyph *submitItem = NULL;
	int numTextItems = 0;
	int numSubmitItems = 0;
	
	for (int i = 0; i < CountInput(); i++) {
		workingItem = GetInput(i);
		if (workingItem->IsSubmit()) {
			numSubmitItems++;
			if (numSubmitItems == 1) //this finds the first submit, if any
				submitItem = workingItem;
			continue;
		}
		if (workingItem->IsTextField())
			numTextItems++;
	}
	
	if(numSubmitItems > 0)
		return submitItem;

	if (numTextItems == 1)
		return textGlyph;

	return NULL;	
}

//	Make this inputText the target for keystrokes

InputGlyph* Form::NextTextField(InputGlyph *inputText)
{
	InputGlyph *t = 0;
	int count = CountInput();
	int j = inputText ? GetInputIndex(inputText) + 1 : 0;	// Start after last field
	for (int i = 0; i < count; i++) {
		t = GetInput((i + j) % count);
		if (t->IsTextField() || t->IsPassword()) {
			if (t == inputText)
				return NULL;				// Tabbed to myself
			break;
		}
	}
	return t;
}

const char* Form::Method()
{
	return mMethod == AV_GET ? "get" : "post";
}

void Form::SetMethod(const char *method)
{
	if (strcasecmp(method, "post") == 0)
		mMethod = AV_POST;
	else
		mMethod = AV_GET;
}

//	Get form submission, return post data if any

BString* Form::GetSubmission(InputGlyph* submitGlyph, BString& url)
{
	Submit(submitGlyph);						// Build submission
	
	if (mMethod == AV_GET) {
		url = mAction;							// Get based request	
		url += "?";
		url += mFormData;		
		return NULL;
	} else {
		url = mAction;							// Post based request
		return new BString(mFormData);
	}
}

#ifdef JAVASCRIPT
void Form::SetOnSubmitScript(const char *script)
{
	if (!mHTMLDoc->LockDocAndWindow())
		return;
	mHTMLDoc->InitJavaScript();
	mHTMLDoc->UnlockDocAndWindow();
	mOnSubmitScript = script;
}

void Form::SetOnResetScript(const char *script)
{
	if (!mHTMLDoc->LockDocAndWindow())
		return;
	mHTMLDoc->InitJavaScript();
	mHTMLDoc->UnlockDocAndWindow();
	mOnResetScript = script;
}

void Form::SetContainer(jseVariable container)
{
	if (!mContainer) {
		if (!mHTMLDoc->LockDocAndWindow())
			return;
		mContainer = container;
		if (mOnSubmitScript.Length())
			mOnSubmit = mHTMLDoc->AddHandler(container, "onSubmit", mOnSubmitScript.String());
		else
			mOnSubmit = 0;
		if (mOnResetScript.Length())
			mOnReset = mHTMLDoc->AddHandler(container, "onReset", mOnResetScript.String());
		else
			mOnReset = 0;
		mHTMLDoc->UnlockDocAndWindow();
	}
}

bool Form::ExecuteSubmitScript()
{
	if (!mOnSubmitScript.Length())
		return false;
		
	mHTMLDoc->LockDocAndWindow();
	mHTMLDoc->ExecuteHandler(mContainer, mOnSubmit);
	mHTMLDoc->UnlockDocAndWindow();
	return true;
}
#endif

