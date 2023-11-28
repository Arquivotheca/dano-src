//  ===========================================================================
//	Form.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
//  ===========================================================================

#ifndef __FORM__
#define __FORM__

#include <SupportDefs.h>
#include <String.h>
class BList;

#ifdef JAVASCRIPT
#include "jseopt.h"
#include "seall.h"
#include "HTMLDoc.h"
#endif

class InputGlyph;
class DrawPort;

// ===========================================================================

class Document;

#ifdef JAVASCRIPT
class Form : public ContainerUser
#else
class Form  /*: public NPObject*/
#endif
{
public:
					Form(Document *doc);
		virtual		~Form();
		
		void			SetEncoding(uint32 encoding);
		uint32			Encoding();

		InputGlyph*		SubmitTextField(InputGlyph *textGlyph);
		InputGlyph*		NextTextField(InputGlyph *inputText);
		
		BString* 		GetSubmission(InputGlyph* submitGlyph, BString& url);
		void			Reset(DrawPort *drawPort);
		const char*		GetTargetFrame() {return mTargetFrame.String();}
		
		void			OpenForm(const char* action, short method, const char *targetFrame, const char *name, const char *enctype);
		void			AddInput(InputGlyph* glyph);

		bool			AddText(const char* text, long textCount);

		void			OpenSelect(InputGlyph* select);
		void			Option(bool selected, char* value);
		void			OpenOptGroup(const char *label);
		void			CloseOption();
		InputGlyph*		CloseSelect();
		void			CloseOptGroup();

		void			OpenTextArea(InputGlyph* text);
		void			AddTextAreaText(const char *text);
		void			CloseTextArea();

		bool			IsTarget();
		void			Untarget(DrawPort *drawPort);
        
        void			ButtonPushed(InputGlyph *button, bool checked, DrawPort *drawPort);
        
		bool			Key(DrawPort *drawPort, short key);
		bool			Click(DrawPort *drawPort);
		bool			Idle(DrawPort *drawPort);

		void			Click(float h, float v, DrawPort *drawPort);

		short			CountInput();
		InputGlyph*		GetInput(short itemIndex);
		short			GetInputIndex(InputGlyph *item);
		
		const char*		Method();
		void			SetMethod(const char *method);
		const char*		Target() {return mTargetFrame.String();}
		void			SetTarget(const char *target) {mTargetFrame = target;}
		const char*		Action() {return mAction.String();}
		void			SetAction(const char *action) {mAction = action;}
		const char*		Name() {return mName.String();}
		void			SetName(const char *name) {mName = name;}
		const char*		Enctype() {return mEnctype.String();}
		void			SetEnctype(const char *enctype) {mEnctype = enctype;}
		
		InputGlyph*		GetNextElement(InputGlyph *previousElement);
		bool			Submit(InputGlyph* submitGlyph);
		
#ifdef JAVASCRIPT
		void			SetOnSubmitScript(const char *script);
		void			SetOnResetScript(const char *script);
		const char*		GetOnSubmitScript() {return mOnSubmitScript.String();}
		void			SetContainer(jseVariable container);
		bool			ExecuteSubmitScript();
#endif

protected:
		InputGlyph*		GetTarget();

		void			TargetText(InputGlyph *inputText, DrawPort *drawPort);
		void			RadioGroup(InputGlyph *radio, DrawPort *drawPort);

		uint32			mEncoding;

		BString			mAction;
		int				mMethod;
		BString			mFormData;

		InputGlyph*		mSelect;
		InputGlyph*		mTextArea;
		
		BString			mSelectValue;
		BString			mTextAreaText;
		BString			mOptionText;
		bool			mSelected;
		bool			mWaitingForOption;
		bool			mHasValue;

		BList*			mInputList;
		
		BString			mTargetFrame;
		BString			mName;
		BString			mEnctype;
		Document*		mHTMLDoc;
#ifdef JAVASCRIPT
		BString			mOnSubmitScript;
		jseVariable		mOnSubmit;
		BString			mOnResetScript;
		jseVariable		mOnReset;
#endif
};

#endif
