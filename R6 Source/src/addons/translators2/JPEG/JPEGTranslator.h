//****************************************************************************************
//
//	File:		JPEGTranslator.h
//
//  Written by:	Ficus Kirkpatrick and Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef JPEG_TRANSLATOR_H
#define JPEG_TRANSLATOR_H

#include <ProductFeatures.h>

#if _SUPPORTS_READ_WRITE_TRANSLATORS

#include "Prefs.h"
#include <Window.h>
#include <View.h>

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
};

#endif

extern "C" {
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"
}

#include <setjmp.h>

/* error handling */

typedef struct {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
} be_error_mgr;

jpeg_error_mgr *be_std_error(jpeg_error_mgr *err);

#endif
