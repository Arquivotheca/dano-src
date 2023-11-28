//****************************************************************************************
//
//	File:		TGATranslator.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef TGA_TRANSLATOR_H
#define TGA_TRANSLATOR_H

#include <SupportDefs.h>

struct TargaHeader {
	uchar		cmtlen;
	uchar		maptype;
	uchar		version;
	uchar		cmap_origin;
	uchar		cmap_origin_hi;
	uchar		cmap_size;
	uchar		cmap_size_hi;
	uchar		cmap_bits;
	ushort		left;
	ushort		top;
	ushort		width;
	ushort		height;
	uchar		pixsize;
	uchar		descriptor;
};

#if _SUPPORTS_READ_WRITE_TRANSLATORS

#include <Window.h>
#include <View.h>
class Prefs;
class BMenuItem;
class BMenuField;
class BCheckBox;

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
		BMenuField *menufield;
		BMenuItem *eightbits, *sixteenbits, *twentyfourbits, *thirtytwobits;
		BCheckBox *compressed, *greyscale;
};

#endif

#endif
