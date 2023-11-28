//****************************************************************************************
//
//	File:		TGATranslator.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include <ProductFeatures.h>

#if _SUPPORTS_READ_WRITE_TRANSLATORS
#include "Encode.h"
#include "Prefs.h"
#include <Application.h>
#include <Alert.h>
#include <StringView.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <CheckBox.h>
#endif

#include "TGATranslator.h"
#include "Decode.h"
#include <TranslationKit.h>
#include <TranslatorAddOn.h>
#include <ByteOrder.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// !!! Set these five accordingly
#define NATIVE_TRANSLATOR_ACRONYM "TGA"
#define NATIVE_TRANSLATOR_FORMAT 'TGA '
#define NATIVE_TRANSLATOR_MIME_STRING "image/x-targa"
#define NATIVE_TRANSLATOR_DESCRIPTION "TGA image"
#define copyright_string "© 1999 Be Incorporated"

// The translation kit's native file type
#define B_TRANSLATOR_BITMAP_MIME_STRING "image/x-be-bitmap"
#define B_TRANSLATOR_BITMAP_DESCRIPTION "Be Bitmap image"

// Translation Kit required globals
char translatorName[32];
char translatorInfo[100];
int32 translatorVersion = B_BEOS_VERSION;

// A couple other useful variables
char native_translator_file_name[32];
char native_translator_window_title[32];

bool debug = 0;

#if _SUPPORTS_READ_WRITE_TRANSLATORS
status_t CopyInToOut(BPositionIO *in, BPositionIO *out);
status_t TranslatorBitmapToNativeBitmap(BPositionIO *in, BPositionIO *out);

#define BITS_PER_PIXEL_8			'8bpp'
#define BITS_PER_PIXEL_16			'16bp'
#define BITS_PER_PIXEL_24			'24bp'
#define BITS_PER_PIXEL_32			'32bp'
#define COMPRESSED					'comp'
#define GREYSCALE					'grey'
#endif

status_t NativeBitmapToTranslatorBitmap(BPositionIO *in, BPositionIO *out);

// Initialize the above
class InitTranslator {
	public:
		InitTranslator() {
			sprintf(translatorName, "%s Images", NATIVE_TRANSLATOR_ACRONYM);
			sprintf(translatorInfo, "%s image translator v%d.%d.%d, %s", NATIVE_TRANSLATOR_ACRONYM,
				(int)(translatorVersion >> 8), (int)((translatorVersion >> 4) & 0xf),
				(int)(translatorVersion & 0xf), __DATE__);
			sprintf(native_translator_file_name, "%sTranslator", NATIVE_TRANSLATOR_ACRONYM);
			sprintf(native_translator_window_title, "%s Settings", NATIVE_TRANSLATOR_ACRONYM);
		}
};
	
static InitTranslator it;

// Define the formats we know how to read
translation_format inputFormats[] = {
	{ NATIVE_TRANSLATOR_FORMAT, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		NATIVE_TRANSLATOR_MIME_STRING, NATIVE_TRANSLATOR_DESCRIPTION },

#if _SUPPORTS_READ_WRITE_TRANSLATORS
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		B_TRANSLATOR_BITMAP_MIME_STRING, B_TRANSLATOR_BITMAP_DESCRIPTION },
#endif

	{ 0, 0, 0, 0, 0, 0 },
};

// Define the formats we know how to write
translation_format outputFormats[] = {

#if _SUPPORTS_READ_WRITE_TRANSLATORS
	{ NATIVE_TRANSLATOR_FORMAT, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		NATIVE_TRANSLATOR_MIME_STRING, NATIVE_TRANSLATOR_DESCRIPTION },
#endif

	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		B_TRANSLATOR_BITMAP_MIME_STRING, B_TRANSLATOR_BITMAP_DESCRIPTION },
	{ 0, 0, 0, 0, 0, 0 },
};

#if _SUPPORTS_READ_WRITE_TRANSLATORS

// Try to add a configuration view, if it doesn't exist display a message and exit
TranslatorWindow::TranslatorWindow(BRect rect, const char *name) :
	BWindow(rect, name, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE) {

	BRect extent(0, 0, 239, 239);
	BView *config = NULL;
	status_t err = MakeConfig(NULL, &config, &extent);
	if ((err < B_OK) || (config == NULL)) {
		char error_message[255];
		sprintf(error_message, "%s does not currently allow user configuration.", native_translator_file_name);
		BAlert *alert = new BAlert("No Config", error_message, "Quit");
		alert->Go();
		exit(1);
	}
	
	ResizeTo(extent.Width(), extent.Height());
	AddChild(config);
}

// We're the only window so quit the app too
bool TranslatorWindow::QuitRequested() {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

// Build the configuration view, making it font sensitive
TranslatorView::TranslatorView(BRect rect, const char *name) :
	BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW) {
	
	prefs = new Prefs("TGATranslatorSettings");

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	font_height fh;
	be_bold_font->GetHeight(&fh);
	BRect r(10, 15, 10 + be_bold_font->StringWidth(translatorName), 15 + fh.ascent + fh.descent);
	
	BStringView *title = new BStringView(r, "Title", translatorName);
	title->SetFont(be_bold_font);
	AddChild(title);
	
	char version_string[100];
	sprintf(version_string, "v%d.%d.%d %s", (int)(translatorVersion >> 8), (int)((translatorVersion >> 4) & 0xf),
		(int)(translatorVersion & 0xf), __DATE__);
	r.top = r.bottom + 20;
	be_plain_font->GetHeight(&fh);
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(version_string);
	
	BStringView *version = new BStringView(r, "Version", version_string);
	version->SetFont(be_plain_font);
	AddChild(version);
	
	r.top = r.bottom + 10;
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(copyright_string);
	
	BStringView *copyright = new BStringView(r, "Copyright", copyright_string);
	copyright->SetFont(be_plain_font);
	AddChild(copyright);
	
	BPopUpMenu *popupmenu = new BPopUpMenu("ColorSpacePopUp");
	eightbits = new BMenuItem("8 bits per pixel", new BMessage(BITS_PER_PIXEL_8));
	sixteenbits = new BMenuItem("16 bits per pixel", new BMessage(BITS_PER_PIXEL_16));
	twentyfourbits = new BMenuItem("24 bits per pixel", new BMessage(BITS_PER_PIXEL_24));
	thirtytwobits = new BMenuItem("32 bits per pixel", new BMessage(BITS_PER_PIXEL_32));
	popupmenu->AddItem(eightbits);
	popupmenu->AddItem(sixteenbits);
	popupmenu->AddItem(twentyfourbits);
	popupmenu->AddItem(thirtytwobits);
	
	r.top = r.bottom + 10;
	r.bottom = r.top + 10;
	r.right = r.left + be_plain_font->StringWidth("Write images with:") + 7 +
		be_plain_font->StringWidth("32 bits per pixel") + 30;
	menufield = new BMenuField(r, "ColorSpaceMenuField", "Write images with:", popupmenu);
	menufield->SetDivider(be_plain_font->StringWidth("Write images with:") + 7);
	AddChild(menufield);
	
	if (prefs->bits_per_pixel == 8) eightbits->SetMarked(true);
	else if (prefs->bits_per_pixel == 16) sixteenbits->SetMarked(true);
	else if (prefs->bits_per_pixel == 24) twentyfourbits->SetMarked(true);
	else thirtytwobits->SetMarked(true);
	
	r.top = menufield->Frame().bottom + 15;
	r.bottom = r.top + 10;
	r.right = r.left + be_plain_font->StringWidth("Write compressed images") + 20;
	compressed = new BCheckBox(r, "Compressed", "Write compressed images", new BMessage(COMPRESSED));
	AddChild(compressed);
	compressed->SetValue(prefs->compressed);
	compressed->SetEnabled(false);
	
	r.top = compressed->Frame().bottom + 5;
	r.bottom = r.top + 10;
	r.right = r.left + be_plain_font->StringWidth("Write greyscale images") + 20;
	greyscale = new BCheckBox(r, "Greyscale", "Write greyscale images", new BMessage(GREYSCALE));
	AddChild(greyscale);
	greyscale->SetValue(prefs->greyscale);
	if (prefs->greyscale) {
		eightbits->SetMarked(true);
		prefs->bits_per_pixel = 8;
		prefs->Save();
	}
}

void TranslatorView::AttachedToWindow() {
	BMessenger messenger(this);
	eightbits->SetTarget(messenger);
	sixteenbits->SetTarget(messenger);
	twentyfourbits->SetTarget(messenger);
	thirtytwobits->SetTarget(messenger);
	compressed->SetTarget(messenger);
	greyscale->SetTarget(messenger);
}

void TranslatorView::MessageReceived(BMessage *message) {
	switch (message->what) {
		case BITS_PER_PIXEL_8:
			prefs->bits_per_pixel = 8;
			break;
		case BITS_PER_PIXEL_16:
			prefs->bits_per_pixel = 16;
			greyscale->SetValue(false);
			break;
		case BITS_PER_PIXEL_24:
			prefs->bits_per_pixel = 24;
			greyscale->SetValue(false);
			break;
		case BITS_PER_PIXEL_32:
			prefs->bits_per_pixel = 32;
			greyscale->SetValue(false);
			break;
		case COMPRESSED:
			prefs->compressed = compressed->Value();
			break;
		case GREYSCALE:
			prefs->greyscale = greyscale->Value();
			if (prefs->greyscale) {
				eightbits->SetMarked(true);
				prefs->bits_per_pixel = 8;
			}
			break;
		default:
			BView::MessageReceived(message);
			return;
	}
	prefs->Save();
}

TranslatorView::~TranslatorView() {
	delete prefs;
}

// Hook to create and return our configuration view
status_t MakeConfig(BMessage *ioExtension, BView **outView, BRect *outExtent) {
	outExtent->Set(0, 0, 239, 239);
	*outView = new TranslatorView(*outExtent, "TranslatorView");
	return B_OK;
}

// Application entry point
int main() {
	char app_signature[255];
	sprintf(app_signature, "application/x-vnd.Be-%s", native_translator_file_name);
	BApplication app(app_signature);
	
	BRect window_rect(100, 100, 339, 339);
	TranslatorWindow *window = new TranslatorWindow(window_rect, native_translator_window_title);
	window->Show();
	
	app.Run();
	return 0;
}

#else

// Hook to create and return our configuration view
status_t MakeConfig(BMessage *ioExtension, BView **outView, BRect *outExtent) {
	*outView = NULL;
	return B_ERROR;
}

#endif

// Determine whether or not we can handle this data
status_t Identify(BPositionIO *inSource, const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType) {
	
	const char *debug_text = getenv("TGA_TRANSLATOR_DEBUG");
	if ((debug_text != NULL) && (atoi(debug_text) != 0)) debug = true;

	status_t err;
	if ((outType != 0) && (outType != B_TRANSLATOR_BITMAP) && (outType != NATIVE_TRANSLATOR_FORMAT)) {
		if (debug) printf("Identify(): outType %x is unknown\n", (int)outType);
		return B_NO_TRANSLATOR;
	}
	
#if _SUPPORTS_READ_WRITE_TRANSLATORS

	TranslatorBitmap tbheader;
	err = inSource->Read(&tbheader, sizeof(TranslatorBitmap));
	if (err != sizeof(TranslatorBitmap)) return (err < 0) ? err : B_ERROR;
	
	if (B_BENDIAN_TO_HOST_INT32(tbheader.magic) == B_TRANSLATOR_BITMAP) {
		if (debug) printf("Identify(): Found a translator bitmap\n");
		outInfo->type = inputFormats[1].type;
		outInfo->translator = 0;
		outInfo->group = inputFormats[1].group;
		outInfo->quality = inputFormats[1].quality;
		outInfo->capability = inputFormats[1].capability;
		strcpy(outInfo->name, inputFormats[1].name);
		strcpy(outInfo->MIME, inputFormats[1].MIME);
		return B_OK;
	}
	
#endif

	if (inSource->Seek(0, SEEK_SET) != 0) return B_ERROR;
	TargaHeader tgaheader;
	err = inSource->Read(&tgaheader, sizeof(TargaHeader));
	if (err != sizeof(TargaHeader)) return (err < 0) ? err : B_ERROR;

	if (((tgaheader.version == 1) && (tgaheader.pixsize == 8)) || // paletted 8 bit uncompressed
			((tgaheader.version == 2) && ((tgaheader.pixsize == 16) ||
				(tgaheader.pixsize == 24) || (tgaheader.pixsize == 32))) || // true color uncompressed
			((tgaheader.version == 3) && (tgaheader.pixsize == 8)) || // greyscale uncompressed
			((tgaheader.version == 9) && (tgaheader.pixsize == 8)) || // paletted 8 bit RLE
			((tgaheader.version == 10) && ((tgaheader.pixsize == 16) ||
				(tgaheader.pixsize == 24) || (tgaheader.pixsize == 32)))) { // true color RLE
		
		// Add header checks here

		if (debug) printf("Identify(): Found native bitmap\n");
		outInfo->type = inputFormats[0].type;
		outInfo->translator = 0;
		outInfo->group = inputFormats[0].group;
		outInfo->quality = inputFormats[0].quality;
		outInfo->capability = inputFormats[0].capability;
		strcpy(outInfo->name, inputFormats[0].name);
		strcpy(outInfo->MIME, inputFormats[0].MIME);
		return B_OK;
	}
	
	if (debug) printf("Identify(): Did not find a known magic number or header\n");
	return B_NO_TRANSLATOR;
}

// Arguably the most important method in the add-on
status_t Translate(BPositionIO *inSource, const translator_info *inInfo, BMessage *ioExtension,
	uint32 outType, BPositionIO *outDestination) {

	const char *debug_text = getenv("TGA_TRANSLATOR_DEBUG");
	if ((debug_text != NULL) && (atoi(debug_text) != 0)) debug = true;

	// If no specific type was requested, convert to the interchange format
	if (outType == 0) outType = B_TRANSLATOR_BITMAP;

#if _SUPPORTS_READ_WRITE_TRANSLATORS

	// What action to take, based on the findings of Identify()
	if (outType == inInfo->type) {
		return CopyInToOut(inSource, outDestination);
	}
	
	if (inInfo->type == B_TRANSLATOR_BITMAP && outType == NATIVE_TRANSLATOR_FORMAT) {
		return TranslatorBitmapToNativeBitmap(inSource, outDestination);
	}

#endif
	
	if (inInfo->type == NATIVE_TRANSLATOR_FORMAT && outType == B_TRANSLATOR_BITMAP) {
		return NativeBitmapToTranslatorBitmap(inSource, outDestination);
	}

	return B_NO_TRANSLATOR;
}

#if _SUPPORTS_READ_WRITE_TRANSLATORS

// The user has requested the same format for input and output, so just copy
status_t CopyInToOut(BPositionIO *in, BPositionIO *out) {
	int block_size = 65536;
	void *buffer = malloc(block_size);
	char temp[1024];
	if (buffer == NULL) {
		buffer = temp;
		block_size = 1024;
	}
	status_t err = B_OK;
	
	// Read until end of file or error
	while (1) {
		ssize_t to_read = block_size;
		err = in->Read(buffer, to_read);
		// Explicit check for EOF
		if (err == -1) {
			if (buffer != temp) free(buffer);
			return B_OK;
		}
		if (err <= B_OK) break;
		to_read = err;
		if (debug) printf("Wrote %d\n", (int)to_read);
		err = out->Write(buffer, to_read);
		if (err != to_read) if (err >= 0) err = B_DEVICE_FULL;
		if (err < B_OK) break;
	}
	
	if (buffer != temp) free(buffer);
	return (err >= 0) ? B_OK : err;
}

// Encode into the native format
status_t TranslatorBitmapToNativeBitmap(BPositionIO *in, BPositionIO *out) {
	return WriteTarga(*in, *out);
}

#endif

// Decode the native format
status_t NativeBitmapToTranslatorBitmap(BPositionIO *in, BPositionIO *out) {
	return WriteBitmap(*in, *out, NULL);
}
