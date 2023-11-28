/*
	HColorPicker2.h
	
	Copyright Hekkelman Programmatuur
	
	Created: 10/10/97 11:35:51
*/

#ifndef HCOLORPICKER2_H
#define HCOLORPICKER2_H

class HColorSquare;
class HColorSlider;
class HColorDemo;

#include "HDialog.h"

#include <Message.h>
#include <Messenger.h>

class HColorPicker2 : public HDialog {
public:
		HColorPicker2(BRect frame, const char *name, window_type type, int flags,
			BWindow *owner, BPositionIO& data);
		
		enum { sResID = 100 };
		
static	void CreateField(int kind, BPositionIO& data, BView*& inside);
static	void RegisterFields();

virtual	void MessageReceived(BMessage *msg);
		void Connect(BMessage& msg, BHandler *target);
		
//virtual void UpdateFields();
virtual bool OKClicked();
//virtual bool CancelClicked();

private:
		HColorSquare *fSquare;
		HColorSlider *fSlider;
		HColorDemo *fDemo;
		BMessage fMessage;
		BMessenger fTarget;
};

#endif
