/*	TIFFHandler.cpp	*/
/*	Datatypes translator for the TIFF format	*/

#include <InterfaceDefs.h>
#include <Debug.h>
#include <Rect.h>
#include <View.h>
#include <StringView.h>
#include <Screen.h>
#include <string.h>
#include <stdio.h>
#include <Mime.h>
#include <unistd.h>
#include <stdlib.h>

#include <TranslatorAddOn.h>
#include <TranslatorFormats.h>
#include <DataIO.h>
#include <byteorder.h>
#include "tiffio.h"


char			translatorName[] = "TIFF Images";		//	required, C string, ex "Jon's Sound"
char			translatorInfo[100];
int32			translatorVersion = B_BEOS_VERSION;		//	required, integer, ex 100

namespace BPrivate {
class infoFiller {
public:
	infoFiller()
		{
			sprintf(translatorInfo, "TIFF image translator v%d.%d.%d, %s",
				translatorVersion>>8, (translatorVersion>>4)&0xf, translatorVersion&0xf,
				__DATE__);
		}
};
static infoFiller theFiller;
}

static	int	debug = 0;


/*
	uint32		type;						//	BeOS data type
	uint32		t_cls;						//	class of this format
	float		quality;					//	format quality 0.0-1.0
	float		capability;					//	handler capability 0.0-1.0
	char		MIME[B_MIME_TYPE_LENGTH+11];	//	MIME string
	char		formatName[B_MIME_TYPE_LENGTH+11];	//	only descriptive
*/
enum {
	kTIF = 'TIFF'
};

translation_format inputFormats[] = {
	{ kTIF, B_TRANSLATOR_BITMAP, 0.5, 0.6, "image/tiff", "TIFF image" },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.4, 0.6, "image/x-be-bitmap", "Be Bitmap image" },
	{ 0, 0, 0, 0, 0, 0 },
};		//	optional (else Identify is always called)


translation_format outputFormats[] = {
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.4, 0.6, "image/x-be-bitmap", "Be Bitmap image" },
	{ 0, 0, 0, 0, 0, 0 },
};	//	optional (else Translate is called anyway)



#define S32(x) B_BENDIAN_TO_HOST_INT32(x)
#define SFL(x) B_BENDIAN_TO_HOST_FLOAT(x)

#define INT_HEIGHT(x) (SFL(x.bottom)-SFL(x.top))
#define INT_WIDTH(x) (SFL(x.right)-SFL(x.left))

const uint32 SWAPPED_B_TRANSLATOR_BITMAP = S32(B_TRANSLATOR_BITMAP);


// WAA - A simple utility routine to convert pixels
// if necessary.  This shouldn't be required since the
// image can tell us what endianness is appropriate.

void
convert(char buff[4])
{
	char r = buff[0];	// Save the red
	buff[0] = buff[2];	// Assign blue
	buff[2] = r;		// Assign red
}


	//	Return B_NO_TRANSLATOR if not handling this data.
	//	Even if inputFormats exists, may be called for data without hints
	//	Ff outType is not 0, must be able to export in wanted format

status_t
Identify(	//	required
	BPositionIO *		inSource,
	const translation_format *	inFormat,
	BMessage *			ioExtension,
	translator_info *			outInfo,
	uint32				outType)
{
	//	Check that our assumptions hold water
	ASSERT(sizeof(short) == 2);
	if (sizeof(short) != 2) 
	{
		fprintf(stderr, "TIFFTranslator sees sizeof(short) as %d, should be 2\n", sizeof(short));
		return B_ERROR;
	}
	
	union 
	{
		char c[2];
		short s;
	} t;
#if B_HOST_IS_LENDIAN
	t.s = 0x100;
#else
	t.s=1;
#endif
	ASSERT((t.c[0] == 0) && (t.c[1] == 1));
	if (t.c[1] != 1) 
	{
		fprintf(stderr, "TIFFTranslator compiled with wrong endian-ness!\n");
		return B_ERROR;
	}
	
	if (getenv("TIFF_HANDLER_DEBUG") != NULL) debug = 1;

	//	check that we can handle the output format requested
	if ((outType != 0) && (outType != B_TRANSLATOR_BITMAP) && 
			(outType != kTIF)) 
	{
		if (debug) fprintf(stderr, "outType %c%c%c%c\n", outType>>24, 
			outType>>16, outType>>8, outType);
		return B_NO_TRANSLATOR;
	}

#define BYTES 100

	union 
	{
		unsigned char		data[BYTES];
	} buffer;

	//	read header and see what happens
	uint32 iThink = 0;	//	set to guess of format
	status_t err;
	ssize_t size = sizeof(buffer);
	err = (*inSource).Read(&buffer, size);
	if ((err < B_OK) || (size != sizeof(buffer))) 
		return err;

	//	make a guess based on magic numbers
	if ((buffer.data[0] == 'M' && buffer.data[1] == 'M') ||
		(buffer.data[0] == 'I' && buffer.data[1] == 'I'))
	{
		iThink = kTIF;
		if (debug) 
			printf("TIFF magic\n");
	} else {
		return B_NO_TRANSLATOR;
	}

	// We might want to do some sanity checking in here
	// to make sure we can really decode the image.

	(*outInfo).type = inputFormats[0].type;
	(*outInfo).translator = 0;
	(*outInfo).group = inputFormats[0].group;
	(*outInfo).quality = inputFormats[0].quality;
	(*outInfo).capability = inputFormats[0].capability;
	sprintf((*outInfo).name, "%s", inputFormats[0].name);
	strcpy((*outInfo).MIME, inputFormats[0].MIME);
	
	return B_OK;
}


//	Return B_NO_TRANSLATOR if not handling the output format
//	If outputFormats exists, will only be called for those formats

static status_t
CopyLoop(
	BPositionIO &		input,
	BPositionIO &		output)
{
	size_t block = 65536L;
	void * buffer = malloc(block);
	char temp[1024];
	if (!buffer) {
		buffer = temp;
		block = 1024;
	}
	status_t err = B_OK;
	// Read until end of file or error
	while (1) {
		ssize_t to_read = block;
		err = input.Read(buffer, to_read);
		// Explicit check for EOF
		if (err == -1) {
			if (buffer != temp) free(buffer);
			return B_OK;
		}
		if (err <= B_OK) break;
		ssize_t to_write = err;
		if (debug) printf("Write %d\n", to_write);
		err = output.Write(buffer, to_write);
		if (err != to_write) if (err >= 0) err = B_DEVICE_FULL;
		if (err < B_OK) break;
	}
	if (buffer != temp)
		free(buffer);
	return (err >= 0) ? B_OK : err;
}

template<class T> class array_guard 
{
	T * & guarded;
public:
	array_guard(T * & init) : guarded(init) { }
	~array_guard() { if (guarded) delete[] guarded; }
};


static status_t 
WriteTIFF(BPositionIO &input, BPositionIO &output, BMessage *ioExtension)
{
	if (debug) printf("TIFFHandler - WriteTIFF - BEGIN\n");
	
	// WAA - just a shortcut for now since we don't know how
	// to write.
	return B_ILLEGAL_DATA;
	
/*
	TranslatorBitmap bits_hdr;
	Win3xHead w3head;
	Win3xInfoHead w3info;
	char magic[2] = { 'B', 'M' };

	status_t err = input.Read(&bits_hdr, sizeof(bits_hdr));
	if (err < sizeof(bits_hdr)) {
		return (err < B_OK) ? err : B_ILLEGAL_DATA;
	}
	bits_hdr.magic = B_BENDIAN_TO_HOST_INT32(bits_hdr.magic);
	bits_hdr.bounds.left = B_BENDIAN_TO_HOST_FLOAT(bits_hdr.bounds.left);
	bits_hdr.bounds.right = B_BENDIAN_TO_HOST_FLOAT(bits_hdr.bounds.right);
	bits_hdr.bounds.top = B_BENDIAN_TO_HOST_FLOAT(bits_hdr.bounds.top);
	bits_hdr.bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(bits_hdr.bounds.bottom);
	bits_hdr.rowBytes = B_BENDIAN_TO_HOST_INT32(bits_hdr.rowBytes);
	bits_hdr.colors = (color_space)B_BENDIAN_TO_HOST_INT32(bits_hdr.colors);
	bits_hdr.dataSize = B_BENDIAN_TO_HOST_INT32(bits_hdr.dataSize);

	if (bits_hdr.magic != B_TRANSLATOR_BITMAP) {
		if (debug) fprintf(stderr, "bad bitmap magic\n");
		return B_NO_TRANSLATOR;
	}
	size_t pix_size = 0;
	switch (bits_hdr.colors) {
	case B_RGB_32_BIT:
		pix_size = 32;
		break;
	case B_COLOR_8_BIT:
		pix_size = 8;
		break;
	default:
		if (debug) fprintf(stderr, "unknown bitmap colors %d\n", bits_hdr.colors);
		return B_NO_TRANSLATOR;
	}
	size_t palette_size = (pix_size < 24) ? 1024 : 0;
	size_t row_bytes = (pix_size*(bits_hdr.bounds.Width()+1)+31)/32*4;

	w3head.file_size = B_HOST_TO_LENDIAN_INT32(row_bytes*(bits_hdr.bounds.
		Height()+1)+sizeof(w3info)+sizeof(w3head)+2+palette_size);
	w3head.reserved[0] = w3head.reserved[1] = 0;
	w3head.offset_bits = B_HOST_TO_LENDIAN_INT32(sizeof(w3info)+sizeof(w3head)
		+2+palette_size);

	w3info.header_size = B_HOST_TO_LENDIAN_INT32(sizeof(w3info));
	w3info.width = B_HOST_TO_LENDIAN_INT32((int32)bits_hdr.bounds.Width()+1);
	w3info.height = B_HOST_TO_LENDIAN_INT32((int32)bits_hdr.bounds.Height()+1);
	w3info.planes = B_HOST_TO_LENDIAN_INT16(1);
	w3info.bit_count = B_HOST_TO_LENDIAN_INT16(pix_size);
	w3info.compression = 0;
	w3info.image_size = B_HOST_TO_LENDIAN_INT32(row_bytes*((int32)bits_hdr.
		bounds.Height()+1));
	w3info.x_pixels = B_HOST_TO_LENDIAN_INT32(2835);
	w3info.y_pixels = B_HOST_TO_LENDIAN_INT32(2835);
	if (pix_size < 24) {
		w3info.number_colors = B_HOST_TO_LENDIAN_INT32(256);
		w3info.colors_important = B_HOST_TO_LENDIAN_INT32(1 << pix_size);
	}
	else {
		w3info.number_colors = 0;
		w3info.colors_important = 0;
	}

	err = output.Write(magic, 2);
	if (err != 2) {
		if (debug) fprintf(stderr, "Write 2 returns %x\n", err);
		return (err < B_OK) ? err : B_ERROR;
	}
	err = output.Write(&w3head, sizeof(w3head));
	if (err != sizeof(w3head)) {
		if (debug) fprintf(stderr, "Write head returns %x\n", err);
		return (err < B_OK) ? err : B_ERROR;
	}
	err = output.Write(&w3info, sizeof(w3info));
	if (err != sizeof(w3info)) {
		if (debug) fprintf(stderr, "Write info returns %x\n", err);
		return (err < B_OK) ? err : B_ERROR;
	}
	if (pix_size < 24) {
		unsigned char palette[1024];
		BScreen scrn;
		rgb_color entry;
		for (int ix=0; ix<256; ix++) {
			entry = scrn.ColorForIndex(ix);
			palette[ix*4] = entry.blue;
			palette[ix*4+1] = entry.green;
			palette[ix*4+2] = entry.red;
			palette[ix*4+3] = entry.alpha;
		}
		err = output.Write(palette, 1024);
		if (err < 1024) {
			if (debug) fprintf(stderr, "Write palette returns %x\n", err);
			return (err < B_OK) ? err : B_ERROR;
		}
	}
	size_t rd_bytes = bits_hdr.rowBytes;
	void * data = malloc(rd_bytes > row_bytes ? rd_bytes : row_bytes);
	if (!data) {
		return B_NO_MEMORY;
	}
	for (int iy=bits_hdr.bounds.Height(); iy>=0; iy--) {
		err = input.Read(data, rd_bytes);
		if (err < rd_bytes) {
			if (debug) fprintf(stderr, "Read data %x returns %x\n", 
				rd_bytes, err);
			free(data);
			return (err < B_OK) ? err : B_ERROR;
		}
		err = output.Write(data, row_bytes);
		if (err < row_bytes) {
			if (debug) fprintf(stderr, "Write data %x returns %x\n", 
				row_bytes, err);
			free(data);
			return (err < B_OK) ? err : B_ERROR;
		}
	}
	free(data);
	// all done!
	return B_OK;
*/
}


//	assume that the bitmap is local (such as writing into a BitmapStream)
//	thus, we seek in the bitmap (output) and not in the TIFF (input)
static status_t
WriteBitmap(BPositionIO &input, BPositionIO &output, BMessage *ioExtension)
{
	//printf("TIFFHandler - WriteBitmap - BEGIN\n");

	TranslatorBitmap bmheader;
	status_t		err;
	ssize_t			rd;
	uint32 *		palette = NULL;
	unsigned char		magic[256];
	off_t 			pos;
	// delete[] the palette if and when we've allocated it
	array_guard<unsigned long> palette_guard(palette);

	TIFF *tif = TIFFStreamOpen(&input, "tiffstream", "r");
	if (!tif)
		return B_ILLEGAL_DATA;
		
	uint32 width, height;
	uint32 orient=0;
	uint32 samples=0;
	
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
	TIFFGetField(tif, TIFFTAG_ORIENTATION, &orient);
	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samples);
	
	if (debug)
	{
		printf("TIFF Image Info:\n");
		printf(" Width: %d\n", width);
		printf(" Height: %d\n", height);
		printf(" Samples: %d\n", samples);
	}

	// A quick sanity check of width and height.
	if ((width > 8192) || (height > 8192))
		return B_ILLEGAL_DATA;
		
	size_t npixels = width * height;
		
	//	set up header
	bmheader.magic = SWAPPED_B_TRANSLATOR_BITMAP;
	bmheader.bounds.left = B_HOST_TO_BENDIAN_FLOAT(0.0);
	bmheader.bounds.top = B_HOST_TO_BENDIAN_FLOAT(0.0);
	bmheader.bounds.right = B_HOST_TO_BENDIAN_FLOAT((float)(width)-1);
	bmheader.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT((float)(height)-1);
	bmheader.rowBytes = B_HOST_TO_BENDIAN_INT32((int)(INT_WIDTH(bmheader.bounds)+1)*4);
	bmheader.dataSize = B_HOST_TO_BENDIAN_INT32((int)S32(bmheader.rowBytes)*(int)(SFL(bmheader.bounds.bottom)+1));

	
	//	we always output in 24 bit format, since palettes cannot be represented well in 8 bit mode
#if B_HOST_IS_BENDIAN
	bmheader.colors = (color_space)S32(B_RGBA32_BIG);
#else
	bmheader.colors = (color_space)S32(B_RGBA32);
#endif
	err = output.SetSize(S32(bmheader.dataSize)+sizeof(bmheader));
	if (debug) 
		printf("SetSize bmheader returns %x\n", err);
	if (err < B_OK) 
		return err;                                               
	if (debug) 
		printf("Write %d\n", sizeof(bmheader));
	err = output.Write(&bmheader, sizeof(bmheader));
	if (debug) 
		printf("Write bmheader returns %x\n", err);
	if (err < B_OK) 
		return err;

	// Allocate a buffer into which the tiff image will be read
	// then read it as a RGBA image.
	uint32 *raster = (uint32 *)malloc(S32(bmheader.dataSize));
	if (!raster) {
		return B_NO_MEMORY;
	}
	if (!TIFFReadRGBAImage(tif, width, height, raster, 0))
	{
		free(raster);
		if (debug) printf("TIFFReadRGBAImage didn't return favorably.\n");
		return B_ILLEGAL_DATA;
	}
		
	// First do color conversion over the whole image 
	// to make sure it ends up in our native BGRA format.
#if 0
	for (int y=0; y < height; y++) 
	{
		for (int x = 0; x < width; x++) 
		{
			uint coord1 = y * width + x;
			convert((char *)&raster[coord1]);
		}
	}

	// Now swap the elements because the image is upside
	// down.
	for (int y=0; y < height/2; y++) 
	{
		for (int x = 0; x < width; x++) 
		{
			uint coord1 = y * width + x;
			uint coord2 = ((height - 1) - y) * width + x;
			uint32 pixel1 = raster[coord1];
			uint32 pixel2 = raster[coord2];
			raster[coord1] = pixel2;
			raster[coord2] = pixel1;
		}
	}
		
	// convert the middle row if there is one
	if (height & 1) 
	{
		for (int x = 0; x < width; x++) 
		{
			uint coord = (height / 2) * width + x;
			raster[coord] = raster[coord];
		}
	}

	// Now that we have the data in a buffer, just write it out 
	// to the output stream. Since it comes out flipped upside down, 
	// write it bottom to top. Mangle RGBA into BGRA.
	err = output.Write(raster, S32(bmheader.dataSize));
#else
	//	convert RGBA to BGRA, and also convert alpha channel to FF opaque
	for (int y=height-1; y>=0; y--) {
		uint32 * data = (uint32*)&raster[width*y];
		for (int x=0; x<width; x++) {
			uint32 d = *data;
			*data = ((d&0xff00ff00)|((d>>16)&0xff)|((d&0xff)<<16))^0xff000000UL;
			data++;
		}
		err = output.Write(&raster[width*y], width*4);
		if (err < width*4) break;
	}
#endif
	free(raster);
	if (err < width*4)
	{
		return err < B_OK ? err : B_ERROR;
	}
	
	
	return B_OK;
}


status_t
Translate(	//	required
	BPositionIO *		inSource,
	const translator_info *	inInfo,
	BMessage *			ioExtension,
	uint32				outType,
	BPositionIO *		outDestination)
{
	if (getenv("TIFF_HANDLER_DEBUG") != NULL) debug = 1;

	// 0 -> default bitmap format
	if (outType == 0) 
		outType = B_TRANSLATOR_BITMAP;

	if (outType == (*inInfo).type)
		return CopyLoop((*inSource), (*outDestination));
	else if ((outType == kTIF) && ((*inInfo).type == B_TRANSLATOR_BITMAP))
		return WriteTIFF((*inSource), (*outDestination), ioExtension);
	else if ((outType == B_TRANSLATOR_BITMAP) && ((*inInfo).type == kTIF))
		return WriteBitmap((*inSource), (*outDestination), ioExtension);

	//	if it was none of the permutations we know, this ain't for us!
	if (debug) 
		fprintf(stderr, "Unknown format %c%c%c%c\n", outType>>24, 
		outType>>16, outType>>8, outType);

	return B_NO_TRANSLATOR;
}


	//	right now, there is nothing to configure for this handler

	//	The view will get resized to what the parent thinks is 
	//	reasonable. However, it will still receive MouseDowns etc.
	//	Your view should change settings in the translator immediately, 
	//	taking care not to change parameters for a translation that is 
	//	currently running. Typically, you'll have a global struct for 
	//	settings that is atomically copied into the translator function 
	//	as a local when translation starts.
	//	Store your settings wherever you feel like it.

#include <StringView.h>

status_t
MakeConfig(	//	optional
	BMessage *			ioExtension,
	BView * *			outView,
	BRect *				outExtent)
{
	//	ignore config

	outExtent->Set(0,0,239,239);
	(*outView) = new BView(*outExtent, "TIFFTranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW);
	(*outView)->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	font_height fh;
	be_bold_font->GetHeight(&fh);
	BRect r(10, 15, 10 + be_bold_font->StringWidth(translatorName), 15 + fh.ascent + fh.descent);

	BStringView *str = new BStringView(r, "title", translatorName);
	str->SetFont(be_bold_font);
	(*outView)->AddChild(str);

	char versStr[100];
	sprintf(versStr, "v%d.%d.%d %s", translatorVersion>>8, (translatorVersion>>4)&0xf,
		translatorVersion&0xf, __DATE__);
	r.top = r.bottom + 20;
	be_plain_font->GetHeight(&fh);
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(versStr);
	
	str = new BStringView(r, "info", versStr);
	str->SetFont(be_plain_font);
	(*outView)->AddChild(str);
	
	const char *copyright_string = "© 1997-1999 Be Incorporated";
	r.top = r.bottom + 10;
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(copyright_string);

	str = new BStringView(r, "author", copyright_string);
	str->SetFont(be_plain_font);
	(*outView)->AddChild(str);

	return B_OK;
}


	//	Copy your current settings to a BMessage that may be passed 
	//	to DATATranslate at some later time when the user wants to 
	//	use whatever settings you're using right now.

status_t
GetConfigMessage(	//	optional
	BMessage *			ioExtension)
{
	//	ignore config
	return B_OK;
}

