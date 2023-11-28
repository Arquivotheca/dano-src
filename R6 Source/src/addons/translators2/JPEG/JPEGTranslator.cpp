//****************************************************************************************
//
//	File:		JPEGTranslator.cpp
//
//  Written by:	Ficus Kirkpatrick and Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "JPEGTranslator.h"
#include "Decode.h"
#include <ByteOrder.h>
#include <TranslationKit.h>
#include <TranslatorAddOn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if _SUPPORTS_READ_WRITE_TRANSLATORS
#include "Encode.h"
#include <Application.h>
#include <Alert.h>
#include <StringView.h>
#include <Slider.h>
#include <CheckBox.h>
#endif

// !!! Set these five accordingly
#define NATIVE_TRANSLATOR_ACRONYM "JPEG"
#define NATIVE_TRANSLATOR_FORMAT 'JPEG'
#define NATIVE_TRANSLATOR_MIME_STRING "image/jpeg"
#define NATIVE_TRANSLATOR_DESCRIPTION "JPEG image"
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

typedef struct {
	struct jpeg_compress_struct pub;
	TranslatorBitmap info;
} bitmap_info_struct;

#if _SUPPORTS_READ_WRITE_TRANSLATORS
status_t CopyInToOut(BPositionIO *in, BPositionIO *out);
status_t TranslatorBitmapToNativeBitmap(BPositionIO *in, BPositionIO *out);
static status_t read_bitmap_header(struct jpeg_compress_struct *cinfo, BPositionIO *in_data);
static void collapse_scanline(struct jpeg_compress_struct *cinfo, JSAMPROW in, JSAMPROW out);

#define QUALITY_SLIDER			'qusl'
#define PROGRESSIVE_CHECKBOX	'prch'

#endif

status_t NativeBitmapToTranslatorBitmap(BPositionIO *in, BPositionIO *out);
static bool validate_jpeg(BPositionIO *in_data);
static status_t write_bitmap_header(struct jpeg_decompress_struct *cinfo, BPositionIO *out_data);
size_t bytes_per_row(struct jpeg_compress_struct *cinfo);
static int expand_scanline(struct jpeg_decompress_struct *cinfo, JSAMPROW in, JSAMPROW out);
static void be_error_exit(j_common_ptr cinfo);

// Initialize the above
class InitTranslator {
	public:
		InitTranslator() {
			sprintf(translatorName, "%s Images", NATIVE_TRANSLATOR_ACRONYM);
			sprintf(translatorInfo, "%s image translator v%d.%d.%d, %s", NATIVE_TRANSLATOR_ACRONYM,
				(int)(translatorVersion >> 8), (int)((translatorVersion >> 4) & 0xf),
				(int)(translatorVersion & 0xf), "" /*__DATE__ */);
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
	
	r.top = r.bottom + 10;
	r.bottom = r.top + 10;
	r.right = rect.right - 10;
	BSlider *slider = new BSlider(r, "QualitySlider", "Image Quality", new BMessage(QUALITY_SLIDER), 0, 20);
	slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	slider->SetHashMarkCount(11);
	slider->SetLimitLabels("0", "100");
	AddChild(slider);
	
	prefs = new Prefs("JPEGTranslatorSettings");
	slider->SetValue(prefs->quality / 5);
	
	r.top = slider->Frame().bottom + 5;
	r.bottom = r.top + 10;
	r.right = r.left + be_plain_font->StringWidth("Write progressive images") + 20;
	BCheckBox *checkbox = new BCheckBox(r, "ProgressiveCheckbox", "Write progressive images",
		new BMessage(PROGRESSIVE_CHECKBOX));
	AddChild(checkbox);
	
	checkbox->SetValue(prefs->progressive);
}

void TranslatorView::AttachedToWindow() {
	BView::AttachedToWindow();
	BMessenger messenger(this);
	BSlider *slider = (BSlider *)FindView("QualitySlider");
	if (slider != NULL) slider->SetTarget(messenger);
	BCheckBox *checkbox = (BCheckBox *)FindView("ProgressiveCheckbox");
	if (checkbox != NULL) checkbox->SetTarget(messenger);
}

void TranslatorView::MessageReceived(BMessage *message) {
	switch (message->what) {
		case QUALITY_SLIDER: {
			BSlider *slider = (BSlider *)FindView("QualitySlider");
			if (slider != NULL) {
				prefs->quality = slider->Value() * 5;
				prefs->Save();
			}
			break;
		}
		case PROGRESSIVE_CHECKBOX: {
			BCheckBox *checkbox = (BCheckBox *)FindView("ProgressiveCheckbox");
			if (checkbox != NULL) {
				prefs->progressive = checkbox->Value();
				prefs->Save();
			}
			break;
		}
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

static bool validate_jpeg(BPositionIO *in_data) {
	struct jpeg_decompress_struct cinfo;
	be_error_mgr err;

	/* initialize the barbaric error handler */
	cinfo.err = be_std_error(&err.pub);
	if (setjmp(err.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		return false;
	}

	/* set up */
	jpeg_create_decompress(&cinfo);
	be_position_io_src(&cinfo, in_data);

	/* read the header;  no need to check for errors, this will
	 * automagically longjmp back to the error handler if it fails,
	 * at which point we'll return B_NO_TRANSLATOR.  it sucks, but
	 * it works and it's better than having to implement JPEG.
	 */
	jpeg_read_header(&cinfo, TRUE);
	
	/* clean up */
	jpeg_destroy_decompress(&cinfo);
	return true;
}

// Determine whether or not we can handle this data
status_t Identify(BPositionIO *inSource, const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType) {

	const char *debug_text = getenv("JPEG_TRANSLATOR_DEBUG");
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
	if (validate_jpeg(inSource)) {
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

	const char *debug_text = getenv("JPEG_TRANSLATOR_DEBUG");
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
	bitmap_info_struct binfo;
	struct jpeg_compress_struct *cinfo = &binfo.pub;
	be_error_mgr err;
	JSAMPARRAY in_buffer, out_buffer;
	size_t row_stride;
	int n;
	Prefs prefs("JPEGTranslatorSettings");
	
	/* set_ashamed(TRUE) */
	cinfo->err = be_std_error(&err.pub);
	if (setjmp(err.setjmp_buffer)) goto bail;
		
	/* set up */
	jpeg_create_compress(cinfo);
	be_position_io_dst(cinfo, out);

	/* get the image width/height/etc */
	if (read_bitmap_header(cinfo, in) != B_OK) goto bail;

	jpeg_set_defaults(cinfo);

	jpeg_set_quality(cinfo, prefs.quality, TRUE);
	if (prefs.progressive) jpeg_simple_progression(cinfo);
	jpeg_start_compress(cinfo, TRUE);

	row_stride = bytes_per_row(cinfo);
	in_buffer = (*cinfo->mem->alloc_sarray)((j_common_ptr) cinfo, JPOOL_IMAGE, row_stride, 1);
	out_buffer = (*cinfo->mem->alloc_sarray)((j_common_ptr) cinfo, JPOOL_IMAGE,
		cinfo->image_width * cinfo->input_components, 1);

	while (cinfo->next_scanline < cinfo->image_height) {
		if (in->Read(in_buffer[0], row_stride) <= 0) goto bail; /* is this the right thing to do? */
		collapse_scanline(cinfo, in_buffer[0], out_buffer[0]);
		jpeg_write_scanlines(cinfo, out_buffer, 1);
	}	

	jpeg_finish_compress(cinfo);
	jpeg_destroy_compress(cinfo);
	
	return B_OK;
	
bail:
	jpeg_destroy_compress(cinfo);
	return B_NO_TRANSLATOR;
}

static status_t read_bitmap_header(struct jpeg_compress_struct *cinfo, BPositionIO *in_data) {
	TranslatorBitmap *bmp = (TranslatorBitmap*)&cinfo[1];

	if (in_data->Read(bmp, sizeof(*bmp)) != sizeof(*bmp)) return B_ERROR;
	bmp->magic = B_BENDIAN_TO_HOST_INT32(bmp->magic);

	if(bmp->magic != B_TRANSLATOR_BITMAP) {
		if (debug) printf("JPEG:read_bitmap_header: mad bitmap magic\n");
		return B_ERROR;
	}

	bmp->bounds.left = B_BENDIAN_TO_HOST_FLOAT(bmp->bounds.left);
	bmp->bounds.right = B_BENDIAN_TO_HOST_FLOAT(bmp->bounds.right);
	bmp->bounds.top = B_BENDIAN_TO_HOST_FLOAT(bmp->bounds.top);
	bmp->bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(bmp->bounds.bottom);
	bmp->rowBytes = B_BENDIAN_TO_HOST_INT32(bmp->rowBytes);
	bmp->colors = (color_space)B_BENDIAN_TO_HOST_INT32(bmp->colors);
	bmp->dataSize = B_BENDIAN_TO_HOST_INT32(bmp->dataSize);

	cinfo->image_width = (JDIMENSION)(bmp->bounds.right - bmp->bounds.left + 1);
	cinfo->image_height = (JDIMENSION)(bmp->bounds.bottom - bmp->bounds.top + 1);
	cinfo->input_components = 3;
	cinfo->in_color_space = JCS_RGB;

	return B_OK;
}

static void collapse_scanline(struct jpeg_compress_struct *cinfo, JSAMPROW in, JSAMPROW out) {
	int ctr, ctr2, ctr3, ctr4;
	uint16 val;

	TranslatorBitmap *bmp = (TranslatorBitmap*)&cinfo[1];
	if(bmp->magic != B_TRANSLATOR_BITMAP) {
		if (debug) printf("JPEG:collapse_scanline: where's the magic, baby?\n");
		return;
	}

	switch(bmp->colors) {
		case B_CMAP8: {
			// in: [Y]
			// out: [R][G][B]
			const color_map *map = system_colors();
			rgb_color c;
			for (ctr = 0; ctr < cinfo->image_width; ctr++) {
				ctr3 = ctr*3;
	
				c = map->color_list[in[ctr]];
				out[ctr3] = c.red;
				out[ctr3+1] = c.green;
				out[ctr3+2] = c.blue;
			}
		}	break;
		case B_RGBA15:
			// in: [GB][ARG]	(arrrgh.)
		case B_RGB15:
			// in: [GB][-RG]
			// out: [R][G][B]
			for (ctr = 0; ctr < cinfo->image_width; ctr++) {
				ctr2 = ctr*2;
				ctr3 = ctr*3;
	
				val = in[ctr2]+(in[ctr2+1]<<8);
				out[ctr3] = ((val&0x7c00)>>7)|((val&0x7c00)>>12);
				out[ctr3+1] = ((val&0x3e0)>>2)|((val&0x3e0)>>7);
				out[ctr3+2] = ((val&0x1f)<<3)|((val&0x1f)>>2);
			}
			break;
		case B_RGB16:
			// in: [GB][RG]
			// out: [R][G][B]
			for (ctr = 0; ctr < cinfo->image_width; ctr++) {
				ctr2 = ctr*2;
				ctr3 = ctr*3;
	
				val = in[ctr2]+(in[ctr2+1]<<8);
				out[ctr3] = ((val&0xf800)>>8)|((val&0xf800)>>13);
				out[ctr3+1] = ((val&0x7e0)>>3)|((val&0x7e0)>>9);
				out[ctr3+2] = ((val&0x1f)<<3)|((val&0x1f)>>2);
			}
			break;
		case B_RGB32:
		case B_RGBA32:
			// in: [B][G][R][A]
			// out: [R][G][B]
			for (ctr = 0; ctr < cinfo->image_width; ctr++) {
				ctr3 = ctr*3;
				ctr4 = ctr*4;
	
				out[ctr3] = in[ctr4+2];
				out[ctr3+1] = in[ctr4+1];
				out[ctr3+2] = in[ctr4];
			}
			break;
		default:
			if (debug) printf("JPEG:collapse_scanline: can't translate from this "
				"colorspace (0x%x)\n", bmp->colors);
			break;
	}
}

#endif

// Decode the native format
status_t NativeBitmapToTranslatorBitmap(BPositionIO *in, BPositionIO *out) {
	struct jpeg_decompress_struct cinfo;
	be_error_mgr err;
	JSAMPARRAY in_buffer, out_buffer;
	int row_stride;
	int n;
	status_t status = B_OK;
	
	/* initialize barbaric error handler */
	cinfo.err = be_std_error(&err.pub);
	if (setjmp(err.setjmp_buffer)) goto bail;
	
	/* set up */
	jpeg_create_decompress(&cinfo);
	be_position_io_src(&cinfo, in);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	/* let the reader know the image details */
	if (write_bitmap_header(&cinfo, out) != B_OK) goto bail;
	
	/* decompress the image */
	row_stride = cinfo.output_width * cinfo.output_components;
	in_buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
	out_buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE,
		cinfo.output_width * 4, 1);

	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, in_buffer, 1);
		n = expand_scanline(&cinfo, in_buffer[0], out_buffer[0]);
		status = out->Write(out_buffer[0], n);
		if (status < B_OK) {
			if (debug) printf("JPEG: error writing: %x (%s)\n", status, strerror(status));
			break;
		}
	}
	if (status > B_OK) status = B_OK;

	/* clean up */
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return status;	

bail:
	jpeg_destroy_decompress(&cinfo);
	return B_NO_TRANSLATOR;
}

static status_t write_bitmap_header(struct jpeg_decompress_struct *cinfo, BPositionIO *out_data) {
	TranslatorBitmap bm_header;

	bm_header.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	bm_header.bounds.left = B_HOST_TO_BENDIAN_FLOAT(0.0);
	bm_header.bounds.right = B_HOST_TO_BENDIAN_FLOAT((float)cinfo->output_width-1);
	bm_header.bounds.top = B_HOST_TO_BENDIAN_FLOAT(0.0);
	bm_header.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT((float)cinfo->output_height-1);
	bm_header.rowBytes = B_HOST_TO_BENDIAN_INT32(cinfo->output_width*4);
	bm_header.colors = (color_space)B_BENDIAN_TO_HOST_INT32(B_RGB32);
	bm_header.dataSize = B_BENDIAN_TO_HOST_INT32(cinfo->output_width*4*cinfo->output_height);

	if (out_data->Write(&bm_header, sizeof(bm_header)) != sizeof(bm_header)) return B_ERROR;
	return B_OK;
}

size_t bytes_per_row(struct jpeg_compress_struct *cinfo) {
	TranslatorBitmap *bmp = (TranslatorBitmap*)&cinfo[1];
	if(bmp->magic != B_TRANSLATOR_BITMAP) {
		if (debug) printf("JPEG:bytes_per_row: where's the magic, baby?\n");
		return 0;
	}
	return bmp->rowBytes;
}

static int expand_scanline(struct jpeg_decompress_struct *cinfo, JSAMPROW in, JSAMPROW out) {
	int ctr, ctr3, ctr4;
	
	switch (cinfo->output_components) {
		case 3:
			for (ctr = 0; ctr < cinfo->output_width; ctr++) {
				ctr3 = ctr*3;
				ctr4 = ctr*4;
	
				out[ctr4] = in[ctr3+2];
				out[ctr4+1] = in[ctr3+1];
				out[ctr4+2] = in[ctr3];
				out[ctr4+3] = 255;
			}
			break;
		case 1:
			for (ctr = 0; ctr < cinfo->output_width; ctr++) {
				ctr4 = ctr*4;
				
				out[ctr4] = in[ctr];
				out[ctr4+1] = in[ctr];
				out[ctr4+2] = in[ctr];
				out[ctr4+3] = 255;
			}
			break;
		default:
			if (debug) printf("!!! expand_scanline called with output_components == %d\n",
					cinfo->output_components);
			return 0;
	}
	return cinfo->output_width * 4;
}

/* error handling stuff */

static void be_error_exit(j_common_ptr cinfo) {
	be_error_mgr *mgr = (be_error_mgr *)cinfo->err;
	(*cinfo->err->output_message)(cinfo);
	longjmp(mgr->setjmp_buffer, 1);
}

struct jpeg_error_mgr *be_std_error(struct jpeg_error_mgr *err) {
	struct jpeg_error_mgr *ret;
	
	ret = jpeg_std_error(err);
	ret->error_exit = be_error_exit;
	/* XXXficus probably want to make output_message do a BAlert */

	return err;
}
