//****************************************************************************************
//
//	File:		PNGTranslator.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef PNG_TRANSLATOR_H
#define PNG_TRANSLATOR_H

#include <Window.h>
#include <View.h>
class Prefs;
class BCheckBox;

#define INTERLACED_CHECKBOX		'picb'

class TranslatorWindow : public BWindow {
	public:
		TranslatorWindow(BRect rect, const char *name);
		bool QuitRequested();
};

class TranslatorView : public BView {
	public:
		TranslatorView(BRect rect, const char *name);
		void AttachedToWindow();
		void MessageReceived(BMessage *message);
		~TranslatorView();

	private:
		Prefs *prefs;
		BCheckBox *interlaced;
};

#endif
