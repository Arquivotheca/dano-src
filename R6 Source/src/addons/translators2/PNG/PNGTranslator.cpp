//****************************************************************************************
//
//	File:		PNGTranslator.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include <ProductFeatures.h>

#if _SUPPORTS_READ_WRITE_TRANSLATORS
#include "PNGTranslator.h"
#include "Encode.h"
#include <Application.h>
#include <Alert.h>
#include <StringView.h>
#include <CheckBox.h>
#endif

#include "Decode.h"
#include <TranslationKit.h>
#include <TranslatorAddOn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// !!! Set these five accordingly
#define NATIVE_TRANSLATOR_ACRONYM "PNG"
#define NATIVE_TRANSLATOR_FORMAT 'PNG '
#define NATIVE_TRANSLATOR_MIME_STRING "image/png"
#define NATIVE_TRANSLATOR_DESCRIPTION "PNG image"
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

// MATT BAGOSIAN SCARY STUFF HERE
static const size_t k_in_png1_ind(0);
static const size_t k_in_png2_ind(k_in_png1_ind + 1);
static const size_t k_in_bitmap_ind(k_in_png2_ind + 1);
static const size_t k_in_null_ind(k_in_bitmap_ind + 1);

static const size_t k_out_png1_ind(0);
static const size_t k_out_bitmap_ind(k_out_png1_ind + 1);
static const size_t k_out_null_ind(k_out_bitmap_ind + 1);
// MATT BAGOSIAN SCARY STUFF END

bool debug = 0;

#if _SUPPORTS_READ_WRITE_TRANSLATORS
status_t CopyInToOut(BPositionIO *in, BPositionIO *out);
status_t TranslatorBitmapToNativeBitmap(BPositionIO *in, BPositionIO *out);
#endif

status_t NativeBitmapToTranslatorBitmap(BPositionIO *in, BPositionIO *out);

// Initialize the above
class InitTranslator {
	public:
		InitTranslator() {
			sprintf(translatorName, "%s Images", NATIVE_TRANSLATOR_ACRONYM);
			sprintf(translatorInfo, "%s image translator v%d.%d.%d, %s", NATIVE_TRANSLATOR_ACRONYM,
				(int)(translatorVersion >> 8), (int)((translatorVersion >> 4) & 0xf),
				(int)(translatorVersion & 0xf), "" /*__DATE__*/);
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
	
	prefs = new Prefs("PNGTranslatorSettings");

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
	
	// !!! Add your controls here
	r.top = copyright->Frame().bottom + 10;
	r.bottom = r.top + 10;
	r.right = r.left + be_plain_font->StringWidth("Write interlaced images") + 20;
	interlaced = new BCheckBox(r, "Interlaced", "Write interlaced images",
		new BMessage(INTERLACED_CHECKBOX));
	AddChild(interlaced);
	if (prefs->interlacing == PNG_INTERLACE_ADAM7) interlaced->SetValue(true);
	else interlaced->SetValue(false);
}

void TranslatorView::AttachedToWindow() {
	BView::AttachedToWindow();
	interlaced->SetTarget(BMessenger(this));
}

void TranslatorView::MessageReceived(BMessage *message) {
	switch (message->what) {
		case INTERLACED_CHECKBOX:
			if (interlaced->Value()) prefs->interlacing = PNG_INTERLACE_ADAM7;
			else prefs->interlacing = PNG_INTERLACE_NONE;
			prefs->Save();
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}

TranslatorView::~TranslatorView() {
	prefs->Save();
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

	const char *debug_text = getenv("PNG_TRANSLATOR_DEBUG");
	if ((debug_text != NULL) && (atoi(debug_text) != 0)) debug = true;

	if ((outType != 0) && (outType != B_TRANSLATOR_BITMAP) && (outType != NATIVE_TRANSLATOR_FORMAT)) {
		if (debug) printf("Identify(): outType %x is unknown\n", (int)outType);
		return B_NO_TRANSLATOR;
	}
	
#if _SUPPORTS_READ_WRITE_TRANSLATORS
	
	char header[sizeof(TranslatorBitmap)];
	status_t err = inSource->Read(header, sizeof(TranslatorBitmap));
	if (err < B_OK) return err;
	
	if (B_BENDIAN_TO_HOST_INT32(((TranslatorBitmap *)header)->magic) == B_TRANSLATOR_BITMAP) {
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
	
	inSource->Seek(0, SEEK_SET);
	PNGToBitmapTranslator *translator = new PNGToBitmapTranslator(inSource, NULL);
	if (translator != NULL && translator->Status() == B_OK) {
		if (debug) printf("Identify(): Found native bitmap\n");
		outInfo->type = inputFormats[0].type;
		outInfo->translator = 0;
		outInfo->group = inputFormats[0].group;
		outInfo->quality = inputFormats[0].quality;
		outInfo->capability = inputFormats[0].capability;
		strcpy(outInfo->name, inputFormats[0].name);
		strcpy(outInfo->MIME, inputFormats[0].MIME);
		delete translator;
		return B_OK;
	}
	
	if (debug) printf("Identify(): Did not find a known magic number or header\n");
	delete translator;
	return B_NO_TRANSLATOR;
}

// Arguably the most important method in the add-on
status_t Translate(BPositionIO *inSource, const translator_info *inInfo, BMessage *ioExtension,
	uint32 outType, BPositionIO *outDestination) {

	const char *debug_text = getenv("PNG_TRANSLATOR_DEBUG");
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
	status_t err = B_OK;
	BitmapToPNGTranslator *translator = new BitmapToPNGTranslator(in, out);
	if (translator != NULL && translator->Status(&err) == B_OK) {
		err = translator->Translate();
	}
	delete translator;
	return err;
}

#endif

// Decode the native format
status_t NativeBitmapToTranslatorBitmap(BPositionIO *in, BPositionIO *out) {
	status_t err = B_OK;
	PNGToBitmapTranslator *translator = new PNGToBitmapTranslator(in, out);
	if (translator != NULL && translator->Status(&err) == B_OK) {
		err = translator->Translate();
	}
	delete translator;
	return err;
}
