/*	TGAHandler.cpp	*/
/*	Datatypes translator for the Targa format	*/

#include <InterfaceDefs.h>
#include <Debug.h>
#include <Rect.h>
#include <View.h>
#include <StringView.h>
#include <string.h>
#include <stdio.h>
#include <Mime.h>
#include <unistd.h>
#include <stdlib.h>

#include <TranslatorAddOn.h>
#include <TranslatorFormats.h>
#include <DataIO.h>
#include <byteorder.h>


#if !defined(kCommentExtension)
#define kCommentExtension "/comment"
#endif


char			translatorName[] = "TGA Images";		//	required, C string, ex "Jon's Sound"
char			translatorInfo[100];
int32			translatorVersion = B_BEOS_VERSION;		//	required, integer, ex 100

namespace BPrivate {
class infoFiller {
public:
	infoFiller()
		{
			sprintf(translatorInfo, "TGA image translator v%d.%d.%d, %s",
				(int)translatorVersion>>8, (int)((translatorVersion>>4)&0xf), (int)(translatorVersion&0xf),
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
	kTGA = 'TGA '
};

translation_format inputFormats[] = {
	{ kTGA, B_TRANSLATOR_BITMAP, 0.6, 0.5, "image/x-targa", "TGA image" },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.4, 0.6, "image/x-be-bitmap", "Be Bitmap image" },
	{ 0, 0, 0, 0, 0, 0 }
};		//	optional (else Identify is always called)

translation_format outputFormats[] = {
	{ 'TGA ', B_TRANSLATOR_BITMAP, 0.6, 0.5, "image/x-targa", "TGA image" },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.4, 0.6, "image/x-be-bitmap", "Be Bitmap image" },
	{ 0, 0, 0, 0, 0, 0 }
};	//	optional (else Translate is called anyway)


#define S32(x) B_BENDIAN_TO_HOST_INT32(x)
#define SFL(x) B_BENDIAN_TO_HOST_FLOAT(x)

#define INT_HEIGHT(x) (SFL(x.bottom)-SFL(x.top))
#define INT_WIDTH(x) (SFL(x.right)-SFL(x.left))

const uint32 SWAPPED_B_TRANSLATOR_BITMAP = S32(B_TRANSLATOR_BITMAP);

//	This is the header of the Targa file format
//	We only recognize version 1 and 2, and not 16 bit data or less than 8 bit data at that
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


	//	Return B_NO_TRANSLATOR if not handling this data.
	//	Even if inputFormats exists, may be called for data without hints
	//	If outType is not 0, must be able to export in wanted format

status_t
Identify(	//	required
	BPositionIO *		inSource,
	const translation_format *	inFormat,
	BMessage *			ioExtension,
	translator_info *			outInfo,
	uint32				outType)
{
	if (debug) fprintf(stderr, "TGAHandler::Identify()\n");
	//	Check that our assumptions hold water
	ASSERT(sizeof(short) == 2);
	if (sizeof(short) != 2) {
		fprintf(stderr, "TGATranslator sees sizeof(short) as %d, should be 2\n", (int)sizeof(short));
		return B_ERROR;
	}
	union {
		char c[2];
		short s;
	} t;
#if B_HOST_IS_LENDIAN
	t.s = 0x100;
#else
	t.s=1;
#endif
	ASSERT((t.c[0] == 0) && (t.c[1] == 1));
	if (t.c[1] != 1) {
		fprintf(stderr, "TGATranslator compiled with wrong endian-ness!\n");
		return B_ERROR;
	}
	if (getenv("TGA_HANDLER_DEBUG") != NULL) debug = 1;

	//	check that we can handle the output format requested
	if ((outType != 0) && (outType != B_TRANSLATOR_BITMAP) && (outType != 'TGA ')) {
		if (debug) fprintf(stderr, "outType %x is unknown\n", (int)outType);
		return B_NO_TRANSLATOR;
	}

	union {
		TargaHeader	data;	//	enough for a B_TRANSLATOR_BITMAP and Targa header
		char bitmap_data[sizeof(TranslatorBitmap)];
	}	buffer;

	//	read header and see what happens
	uint32 iThink = 0;	//	set to guess of format
	status_t err;
	ssize_t size = sizeof(buffer);
	err = (*inSource).Read(&buffer, size);
	if (err < B_OK)
		return err;

	//	make a guess based on magic numbers
	if ((*(TranslatorBitmap *)&buffer).magic == SWAPPED_B_TRANSLATOR_BITMAP) {
		iThink = B_TRANSLATOR_BITMAP;
		if (debug) printf("Translator bitmap\n");
	}
	else if (((buffer.data.version == 1) && (buffer.data.pixsize == 8)) || // paletted 8 bit uncompressed
			((buffer.data.version == 2) && ((buffer.data.pixsize == 16) || (buffer.data.pixsize == 24) || (buffer.data.pixsize == 32))) || // true color uncompressed
			((buffer.data.version == 3) && (buffer.data.pixsize == 8)) || // greyscale uncompressed
			((buffer.data.version == 9) && (buffer.data.pixsize == 8)) || // paletted 8 bit RLE
			((buffer.data.version == 10) && ((buffer.data.pixsize == 16) || (buffer.data.pixsize == 24) || (buffer.data.pixsize == 32)))) { // true color RLE
		iThink = 'TGA ';
		if (debug) printf("TGA, version %d, pixsize %d\n", buffer.data.version, buffer.data.pixsize);
	}
	else {
		if (debug) fprintf(stderr, "No magic number matches\n");
		return B_NO_TRANSLATOR;
	}

	//	verify our guess
	if (iThink == B_TRANSLATOR_BITMAP)
	{
		int multi = 0;
		switch (S32((*(TranslatorBitmap *)&buffer).colors))
		{
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
			multi = 4;
			break;
		case B_CMAP8:
			multi = 1;
			break;
		default:
			if (debug) fprintf(stderr, "color space unknown\n");
			return B_NO_TRANSLATOR;
		}
		if ((SFL((*(TranslatorBitmap *)&buffer).bounds.right) <= SFL((*(TranslatorBitmap *)&buffer).bounds.left)) ||
				(SFL((*(TranslatorBitmap *)&buffer).bounds.bottom) <= SFL((*(TranslatorBitmap *)&buffer).bounds.top))) {
			if (debug) puts("bounds test failed");
			return B_NO_TRANSLATOR;
		}
		if (S32((*(TranslatorBitmap *)&buffer).rowBytes) < INT_WIDTH((*(TranslatorBitmap *)&buffer).bounds)*multi) {
			if (debug) puts("rowBytes test failed");
			return B_NO_TRANSLATOR;
		}
		if (S32((*(TranslatorBitmap *)&buffer).dataSize) < S32((*(TranslatorBitmap *)&buffer).rowBytes)*INT_HEIGHT((*(TranslatorBitmap *)&buffer).bounds)) {
			if (debug) puts("dataSize test failed");
			return B_NO_TRANSLATOR;
		}

		//	set up outInfo
		(*outInfo).type = inputFormats[1].type;
		(*outInfo).translator = 0;
		(*outInfo).group = inputFormats[1].group;
		(*outInfo).quality = inputFormats[1].quality;
		(*outInfo).capability = inputFormats[1].capability;
		sprintf((*outInfo).name, "%s, type %d", inputFormats[1].name, S32((*(TranslatorBitmap *)&buffer).colors));
		strcpy((*outInfo).MIME, inputFormats[1].MIME);

	} else {	//	Targa

		//	maptype can be 0 (no map) or 1 (palette map)
		if ((buffer.data.maptype != 0) && (buffer.data.maptype != 1))
			return B_NO_TRANSLATOR;
		//	palette origin must be < 256 and size >=2 <= 256
		int porigin = buffer.data.cmap_origin + buffer.data.cmap_origin_hi*256;
		int psize = buffer.data.cmap_size + buffer.data.cmap_size_hi*256;
		if (buffer.data.pixsize < 16)
			if (buffer.data.version != 3 && buffer.data.version != 11) // don't consider greyscale images
				if ((porigin > 254) || (porigin+psize > 256) || (psize < 2) || (psize > 256))
					return B_NO_TRANSLATOR;
		//	descriptor must be for 0 or 8 alpha bits, plus maybe top-to-bottom
		int descriptor = buffer.data.descriptor;
		if (buffer.data.pixsize == 8) {
			//	for 8 bit, PhotoShop seems to think the palette index is an attribute
			if ((descriptor != 0) && (descriptor != 32) && (descriptor != 8) && (descriptor != 40))
				return B_NO_TRANSLATOR;
			if ((buffer.data.cmap_bits != 24) && (buffer.data.cmap_bits != 32) && (buffer.data.cmap_bits != 0)) // last one for greyscale
				return B_NO_TRANSLATOR;
		} else if (buffer.data.pixsize == 16) {
			// Either one bit of alpha or none
			if (((descriptor & 0x0f) != 0) && ((descriptor & 0x0f) != 1)) {
				if (debug) printf("Found %d bits of alpha\n", descriptor & 0x0f);
				return B_NO_TRANSLATOR;
			}
		} else if (buffer.data.pixsize == 24) {
			//	24 bit data should not have any alpha data
			if (descriptor & 0x0f != 0)
				return B_NO_TRANSLATOR;
		} else if (buffer.data.pixsize == 32) {
			//	32 bit data should have alpha data
			if (descriptor & 0x0f != 8)
				return B_NO_TRANSLATOR;
		} else
			return B_NO_TRANSLATOR;

		(*outInfo).type = inputFormats[0].type;
		(*outInfo).translator = 0;
		(*outInfo).group = inputFormats[0].group;
		(*outInfo).quality = inputFormats[0].quality;
		(*outInfo).capability = inputFormats[0].capability;
		sprintf((*outInfo).name, "%s, type %d (%d bits)", inputFormats[0].name, buffer.data.version, buffer.data.pixsize);
		strcpy((*outInfo).MIME, inputFormats[0].MIME);
	}
	if (debug) puts("TGAHandler::Identify() OK");
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

const char tga8trailer[] = "\0\0\0\0\0\0\0\0TRUEVISION-XFILE\x2E\0";
int trailerlen = 26;

static status_t
WriteTGAPalette(
	BPositionIO &		output)
{
	//	set up system palette so TGA can read it
	const color_map *ptr = system_colors();
	unsigned char data[256*4];
	for (int ix=0; ix<256; ix++)
	{
		data[ix*4] = ptr->color_list[ix].blue;
		data[ix*4+1] = ptr->color_list[ix].green;
		data[ix*4+2] = ptr->color_list[ix].red;
		data[ix*4+3] = ptr->color_list[ix].alpha;
	}
	data[B_TRANSPARENT_8_BIT*4+3] = 0;	//	transparent index should be transparent

	//	output to stream
	if (debug) printf("Write %d\n", 1024);
	status_t err = output.Write(data, 256*4);
	if (debug) printf("Writing TGA palette returns %x\n", err);
	if (err != 256*4) if (err >= 0) err = B_DEVICE_FULL;
	return err;
}


//	Write TGA in 24 bit format, top-to-bottom
static status_t
CopyTGA24(
	BPositionIO &		input,
	BPositionIO &		output,
	color_space			space,
	int					xsize,
	int					ysize,
	int					rowbytes)
{
	if (debug && ((rowbytes <= 0) || (rowbytes >= xsize*4+32))) {
		fprintf(stderr, "TGA: weird rowbytes %d (w %d  h %d)\n", rowbytes, xsize, ysize);
	}
	off_t pos = input.Position();
	status_t err = B_OK;
	char * buffer = (char *)malloc(xsize*4);
	if (!buffer) return B_NO_MEMORY;
	for (int ix=0; ix<=ysize-1; ix++)
	{
		err = input.Seek(pos+(off_t)ix*rowbytes, SEEK_SET);
		if (err < B_OK) break;
		ssize_t rd = xsize*4;
		err = input.Read(buffer, rd);
		if (debug) printf("Read returns %x\n", err);
		if (err < B_OK) break;
		if (rd != xsize*4)
		{
			err = B_ERROR;
			break;
		}
		char * src = buffer;
		char * dst = buffer;
		char * end = buffer+xsize*4;
/*** hplus: the byte order for these various constants needs to be checked */
		if (space == B_RGB32 || space == B_RGBA32)
		{
			while (src < end) {
				uchar blue = src[0];
				uchar green = src[1];
				uchar red = src[2];
				dst[0] = blue;
				dst[1] = green;
				dst[2] = red;
				dst += 3;
				src += 4;
			}
		} else {
			while (src < end) {
				uchar blue = src[3];
				uchar green = src[2];
				uchar red = src[1];
				dst[0] = blue;
				dst[1] = green;
				dst[2] = red;
				dst += 3;
				src += 4;
			}
		}
		if (debug) printf("Write %d\n", xsize*3);
		err = output.Write(buffer, xsize*3);
		if (err != xsize*3) if (err >= 0) err = B_DEVICE_FULL;
		if (debug) printf("Write returns %x\n", err);
		if (err < B_OK) break;
	}
	free(buffer);
	return (err < B_OK) ? err : B_OK;
}


//	Again, we assume the B_TRANSLATOR_BITMAP data is the more efficient for seeking. 
//	Again, we change write order to top-to-bottom, to eliminate out-of-order seeks.
static status_t
CopyTGA8(
	BPositionIO &		input,
	BPositionIO &		output,
	int					xsize,
	int					ysize,
	int					rowbytes)
{
	if (debug && ((rowbytes <= 0) || (rowbytes >= xsize*4+32))) {
		printf("RGA: weird rowbytes %d (w %d  h %d)\n", rowbytes, xsize, ysize);
	}
	off_t pos = input.Position();
	status_t err = B_OK;
	char * buffer = (char *)malloc(xsize);
	if (!buffer) return B_NO_MEMORY;
	for (int ix=0; ix<=ysize-1; ix++)
	{
		err = input.Seek(pos+(off_t)ix*rowbytes, SEEK_SET);
		if (err < B_OK) break;
		ssize_t rd = xsize;
		err = input.Read(buffer, rd);
		if (debug) printf("Read returns %x\n", err);
		if (err < B_OK) break;
		if (rd != xsize)
		{
			err = B_ERROR;
			break;
		}
		if (debug) printf("Write %d\n", xsize);
		err = output.Write(buffer, xsize);
		if (err != xsize) if (err >= 0) err = B_DEVICE_FULL;
		if (debug) printf("Write returns %x\n", err);
		if (err < B_OK) break;
	}
	free(buffer);
	return (err < B_OK) ? err : B_OK;
}


static status_t
WriteTarga(
	BPositionIO &		input,
	BPositionIO &		output,
	BMessage *			ioExtension)
{
	TargaHeader tgaheader;

	//	find the comment, if any
	const char * comment = "";
	if (ioExtension != NULL)
		if (ioExtension->FindString(kCommentExtension, &comment))
			comment = "";

	//	read bitmap header
	TranslatorBitmap bmheader;
	status_t err;
	ssize_t size = sizeof(bmheader);
	err = input.Read(&bmheader, size);
	if (err < B_OK) return err;
	if (size != sizeof(bmheader)) return B_ERROR;

	//	Create tga header and reserve output space.
	//	A network socket stream can be smart enough to always return OK for SetSize() and only do 
	//	buffering when it's really needed.
	switch (S32(bmheader.colors)) {
	case B_CMAP8:
		tgaheader.maptype = 1;
		tgaheader.version = 1;
		err = output.SetSize((off_t)sizeof(tgaheader)+((off_t)INT_HEIGHT(bmheader.bounds)+1)*
				((off_t)INT_WIDTH(bmheader.bounds)+1)+trailerlen);	//	8 bit files end in a magic string
		tgaheader.cmap_origin = 0;
		tgaheader.cmap_origin_hi = 0;
		tgaheader.cmap_size = 0;
		tgaheader.cmap_size_hi = 1;
		tgaheader.cmap_bits = 32;	//	we write alpha in palette
		tgaheader.pixsize = 8;
		tgaheader.descriptor = 0x20;
		break;
	case B_RGB32:
	case B_RGBA32:
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
		tgaheader.maptype = 0;
		tgaheader.version = 2;
		err = output.SetSize((off_t)sizeof(tgaheader)+((off_t)INT_HEIGHT(bmheader.bounds)+1)*
				((off_t)INT_WIDTH(bmheader.bounds)+1)*3+strlen(comment)+trailerlen);
		tgaheader.cmap_origin = 0;
		tgaheader.cmap_origin_hi = 0;
		tgaheader.cmap_size = 0;
		tgaheader.cmap_size_hi = 0;
		tgaheader.cmap_bits = 0;
		tgaheader.pixsize = 24;
		tgaheader.descriptor = 0x20;
		break;
	default:
		if (debug) printf("TGA: bmheader colors is bad [%x]\n", S32(bmheader.colors));
		err = B_NO_TRANSLATOR;
	}
	if (err < B_OK) return err;

	int cmtlen = strlen(comment);
	if (cmtlen > 255) cmtlen = 255;
	tgaheader.cmtlen = cmtlen;

	tgaheader.left = tgaheader.top = 0;
	tgaheader.width = B_HOST_TO_LENDIAN_INT16((int16)(SFL(bmheader.bounds.right)-SFL(bmheader.bounds.left)+1));
	tgaheader.height = B_HOST_TO_LENDIAN_INT16((int16)(SFL(bmheader.bounds.bottom)-SFL(bmheader.bounds.top)+1));

	//	output header
	if (debug) printf("Write %d\n", sizeof(tgaheader));
	err = output.Write(&tgaheader, sizeof(tgaheader));
	if (err != sizeof(tgaheader)) if (err >= 0) err = B_DEVICE_FULL;
	if (debug) printf("Write header returns %x\n", err);
	if (err < B_OK) return err;
	if (cmtlen > 0) {
		err = output.Write(comment, cmtlen);
		if (debug) printf("Write comment returns %x\n", err);
		if (err != cmtlen) if (err >= 0) err = B_DEVICE_FULL;
		if (err < B_OK) return err;
	}

	//	now, output data. TGA is top-to-bottom, left-to-right as we write it
	if (tgaheader.version == 2) {
		//	write the 24-bit data
		err = CopyTGA24(input, output, (color_space)S32(bmheader.colors), B_LENDIAN_TO_HOST_INT16(tgaheader.width),
			B_LENDIAN_TO_HOST_INT16(tgaheader.height), S32(bmheader.rowBytes));
	}
	else {
		//	write the (fixed) system palette
		err = WriteTGAPalette(output);
		if (err < B_OK) return err;
		//	write the 8-bit data
		err = CopyTGA8(input, output, B_LENDIAN_TO_HOST_INT16(tgaheader.width),
			B_LENDIAN_TO_HOST_INT16(tgaheader.height), S32(bmheader.rowBytes));
	}
	if (err < B_OK) return err;
	//	write the magic TRUEVISION trailer
	err = output.Seek(0, SEEK_END);
	if (debug) printf("Seek trailer returns %x\n", err);
	if (err < B_OK) return err;
	err = output.Write(tga8trailer, trailerlen);
	if (err != trailerlen) if (err >= 0) err = B_DEVICE_FULL;
	if (debug) printf("Write trailer returns %x\n", err);
	return err < B_OK ? err : B_OK;
}


//	assume that the bitmap is local (such as writing into a BitmapStream)
//	thus, we seek in the bitmap (output) and not in the Targa (input)
static status_t
WriteBitmap(
	BPositionIO &		input,
	BPositionIO &		output,
	BMessage *			ioExtension)
{
	TargaHeader		tgaheader;
	TranslatorBitmap		bmheader;
	status_t		err;
	ssize_t			rd;
	uint32			palette[256];

	//	read targa header
	rd = sizeof(tgaheader);
	err = input.Read(&tgaheader, rd);
	if (err < B_OK) return err;
	if (rd != sizeof(tgaheader)) return B_ERROR;

	//	set up header
	bmheader.magic = SWAPPED_B_TRANSLATOR_BITMAP;
	bmheader.bounds.left = 0;
	bmheader.bounds.top = 0;
	bmheader.bounds.right = B_HOST_TO_BENDIAN_FLOAT((float)B_LENDIAN_TO_HOST_INT16(tgaheader.width)-1);
	bmheader.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT((float)B_LENDIAN_TO_HOST_INT16(tgaheader.height)-1);
	bmheader.rowBytes = B_HOST_TO_BENDIAN_INT32((int32)(INT_WIDTH(bmheader.bounds)+1)*4);
	bmheader.dataSize = S32((int)S32(bmheader.rowBytes)*(int)(SFL(bmheader.bounds.bottom)+1));

	//	we always output in 24 bit format, since palettes cannot be represented well in 8 bit mode
	bmheader.colors = (color_space)S32(B_RGBA32);
	if (debug) printf("Write %d\n", sizeof(bmheader));
	err = output.Write(&bmheader, sizeof(bmheader));
	if (err != sizeof(bmheader)) if (err >= 0) err = B_DEVICE_FULL;
	if (debug) printf("Write bmheader returns %x\n", err);
	if (err < B_OK) return err;
	err = output.SetSize(S32(bmheader.dataSize)+sizeof(bmheader));
	if (debug) printf("SetSize bmheader returns %x\n", err);
	if (err < B_OK) return err;

	//	deal with comment, if any
	if (tgaheader.cmtlen) {
		char comment[256];
		ssize_t rd = tgaheader.cmtlen;
		err = input.Read(comment, rd);
		if (err < B_OK) return err;
		if (rd != tgaheader.cmtlen) return B_ERROR;
		comment[tgaheader.cmtlen] = 0;
		if (ioExtension != NULL)
			ioExtension->AddString(kCommentExtension, comment);
		if (debug) printf("comment len %d: %s\n", tgaheader.cmtlen, comment);
	}

	if (debug) printf("maptype %d\n", tgaheader.maptype);
	//	deal with color map, if any
	if (tgaheader.maptype == 1) {
		for (int ix=0; ix<256; ix++)
			palette[ix] = S32((ix*0x01010100)|0xff);	//	bgra, black to white
		ssize_t rd = tgaheader.cmap_size+256*tgaheader.cmap_size_hi;
		int nitems = rd;
		if (rd+tgaheader.cmap_origin > 256)
			return B_NO_TRANSLATOR;
		rd = rd*tgaheader.cmap_bits/8;	//	read bytes
		err = input.Read(&palette[tgaheader.cmap_origin], rd);
		if (debug) printf("Read palette returns %d\n", err);
		if (err < B_OK) return err;
		if (rd != nitems*tgaheader.cmap_bits/8) return B_ERROR;
		//	adjust palette if necessary
		if (tgaheader.cmap_bits != 32) {
			uchar * ptr = (uchar *)&palette[tgaheader.cmap_origin];
			for (int ix=nitems-1; ix>=0; ix--) {
				uint32 blue = ptr[ix*3];
				uint32 green = ptr[ix*3+1];
				uint32 red = ptr[ix*3+2];
				ptr[ix*4] = blue;
				ptr[ix*4+1] = green;
				ptr[ix*4+2] = red;
				ptr[ix*4+3] = 255;	//	no transparency
			}
		}
	}

	//	set up buffer
	uint32 * scanline = (uint32 *)malloc(S32(bmheader.rowBytes));
	if (scanline == NULL) return B_NO_MEMORY;

	off_t pos = output.Position();
	
	// Common to all
	int start, end, add;
	int width = B_LENDIAN_TO_HOST_INT16(tgaheader.width);
	if (tgaheader.descriptor & 0x20) {
		start = 0;
		end = B_LENDIAN_TO_HOST_INT16(tgaheader.height);
		add = 1;
	} else {
		start = B_LENDIAN_TO_HOST_INT16(tgaheader.height)-1;
		end = -1;
		add = -1;
	}

	// True color uncompressed images
	if ((tgaheader.version == 2) && ((tgaheader.pixsize == 32) || (tgaheader.pixsize == 24) || (tgaheader.pixsize == 16))) {
		if (debug) printf("type 2 pixsize %d\n", tgaheader.pixsize);
		while (start != end) {
			err = output.Seek(pos+S32(bmheader.rowBytes)*start, SEEK_SET);
			if (err < B_OK) { free(scanline); return err; }
			ssize_t wdata = width*tgaheader.pixsize / 8;
			
			if (tgaheader.pixsize == 32) {
				err = input.Read(scanline, wdata);
				if (err < B_OK) { free(scanline); return err; }
				if (wdata != err) { free(scanline); return B_ERROR; }
			} else if (tgaheader.pixsize == 24) {
				uchar *src = ((uchar *)scanline) + width;
				err = input.Read(src, wdata);
				if (err < B_OK) { free(scanline); return err; }
				if (wdata != err) { free(scanline); return B_ERROR; }
				
				uchar *dst = (uchar *)scanline;
				while (dst != src) {
					dst[0] = src[0]; // B
					dst[1] = src[1]; // G
					dst[2] = src[2]; // R
					dst[3] = 255;    // A
					dst += 4;
					src += 3;
				}
			} else  {
				uchar *src = ((uchar *)scanline) + (width * 2);
				err = input.Read(src, wdata);
				if (err < B_OK) { free(scanline); return err; }
				if (wdata != err) { free(scanline); return B_ERROR; }
				
				uchar *dst = (uchar *)scanline;
				while (dst != src) {
					dst[0] = (src[0] & 0x1f) << 3;								// B
					dst[1] = ((src[1] & 0x03) << 6) | ((src[0] & 0xe0) >> 2);	// G
					dst[2] = (src[1] & 0x7c) << 1;								// R
					dst[3] = 255;												// A
					dst += 4;
					src += 2;
				}
			}
			if (debug) printf("Write %d\n", S32(bmheader.rowBytes));
			err = output.Write(scanline, S32(bmheader.rowBytes));
			if (err != S32(bmheader.rowBytes)) if (err >= 0) err = B_DEVICE_FULL;
			if (err < B_OK) { free(scanline); return err; }
			start += add;
		}
	}
	// True color RLE compressed images
	else if ((tgaheader.version == 10) && ((tgaheader.pixsize == 32) || (tgaheader.pixsize == 24) || (tgaheader.pixsize == 16))) {
		if (debug) printf("type 10 pixsize %d\n", tgaheader.pixsize);
		
		// The worst RLE case, depending on the encoder, is five bytes per pixel
		int packet_buffer_size = width * (1 + tgaheader.pixsize / 8);
		uchar *packet_buffer = (uchar *)malloc(packet_buffer_size);
		if (packet_buffer == NULL) {
			free(scanline);
			return B_NO_MEMORY;
		}
		
		// These must persist across scanlines
		uchar *packet_pos = (uchar *)packet_buffer;
		int packet_count = 0, normal = 0, compressed = 0;
		bool get_more_data = true;
		int bpp = tgaheader.pixsize >> 3;
		
		while (start != end) {
			int w = width;
			err = output.Seek(pos+S32(bmheader.rowBytes)*start, SEEK_SET);
			if (err < B_OK) { free(scanline); free(packet_buffer); return err; }
			
			// Grab batches of data in advance so we don't Read() for every packet
			if (get_more_data) {
				if (packet_count != 0) memmove(packet_buffer, packet_pos, packet_count); // slide everything to the front
				err = input.Read(packet_buffer + packet_count, packet_buffer_size - packet_count);
				if (err < B_OK) { free(scanline); free(packet_buffer); return err; }
				if (err < packet_buffer_size - packet_count) get_more_data = false; // done fetching
				packet_count = packet_count + err;
				packet_pos = (uchar *)packet_buffer;
			}
			
			uchar *dst = (uchar *)scanline;
			while (w > 0) {
				// Add leftover compressed pixels if present
				if (compressed > 0 && w > 0) {
					if (packet_count < bpp) { free(scanline); free(packet_buffer); return B_ERROR; }
					int count = (compressed > w) ? w : compressed;
					if (tgaheader.pixsize == 32) {
						for (int x = 0; x < count; x++) {
							dst[0] = packet_pos[0]; // B
							dst[1] = packet_pos[1]; // G
							dst[2] = packet_pos[2]; // R
							dst[3] = packet_pos[3]; // A
							dst += 4;
						}
						packet_pos += 4;
						packet_count -= 4;
					} else if (tgaheader.pixsize == 24) {
						for (int x = 0; x < count; x++) {
							dst[0] = packet_pos[0]; // B
							dst[1] = packet_pos[1]; // G
							dst[2] = packet_pos[2]; // R
							dst[3] = 255;           // A
							dst += 4;
						}
						packet_pos += 3;
						packet_count -= 3;
					} else {
						for (int x = 0; x < count; x++) {
							dst[0] = (packet_pos[0] & 0x1f) << 3;									// B
							dst[1] = ((packet_pos[1] & 0x03) << 6) | ((packet_pos[0] & 0xe0) >> 2);	// G
							dst[2] = (packet_pos[1] & 0x7c) << 1;									// R
							dst[3] = 255;															// A
							dst += 4;
						}
						packet_pos += 2;
						packet_count -= 2;
					}
					w -= count;
					compressed -= count;
				}
				// Add leftover normal pixels if present
				if (normal > 0 && w > 0) {
					int count = (normal > w) ? w : normal;
					if (packet_count < bpp * count) { free(scanline); free(packet_buffer); return B_ERROR; }
					if (tgaheader.pixsize == 32) {
						memcpy(dst, packet_pos, count * 4);
						dst += (count * 4);
						packet_pos += (count * 4);
						packet_count -= (count * 4);
					} else if (tgaheader.pixsize == 24) {
						for (int x = 0; x < count; x++) {
							dst[0] = packet_pos[0]; // B
							dst[1] = packet_pos[1]; // G
							dst[2] = packet_pos[2]; // R
							dst[3] = 255;           // A
							dst += 4;
							packet_pos += 3;
						}
						packet_count -= (count * 3);
					} else {
						for (int x = 0; x < count; x++) {
							dst[0] = (packet_pos[0] & 0x1f) << 3;									// B
							dst[1] = ((packet_pos[1] & 0x03) << 6) | ((packet_pos[0] & 0xe0) >> 2);	// G
							dst[2] = (packet_pos[1] & 0x7c) << 1;									// R
							dst[3] = 255;															// A
							dst += 4;
							packet_pos += 2;
						}
						packet_count -= (count * 2);
					}
					normal -= count;
					w -= count;
				}
				// Read new packet header
				if (compressed == 0 && normal == 0 && w > 0) {
					if (packet_count < 1) { free(scanline); free(packet_buffer); return B_ERROR; }
					if (*packet_pos & 0x80) compressed = (*packet_pos & 0x7f) + 1;
					else normal = (*packet_pos & 0x7f) + 1;
					packet_pos++;
					packet_count--;
	            }
			}
			
			// output the 32 bit data
			if (debug) printf("Write %d\n", S32(bmheader.rowBytes));
			err = output.Write(scanline, S32(bmheader.rowBytes));
			if (err != S32(bmheader.rowBytes)) if (err >= 0) err = B_DEVICE_FULL;
			if (err < B_OK) { free(scanline); free(packet_buffer); return err; }
			start += add;
		}
		free(packet_buffer);
	}
	// 8 bit paletted RLE images
	else if ((tgaheader.version == 9) && (tgaheader.pixsize == 8)) {
		// The worst RLE case, depending on the encoder, is two bytes per pixel
		int packet_buffer_size = width * 2;
		uchar *packet_buffer = (uchar *)malloc(packet_buffer_size);
		if (packet_buffer == NULL) {
			free(scanline);
			return B_NO_MEMORY;
		}
		
		// These must persist across scanlines
		uchar *packet_pos = (uchar *)packet_buffer;
		int packet_count = 0, normal = 0, compressed = 0;
		bool get_more_data = true;
		
		while (start != end) {
			int w = width;
			err = output.Seek(pos+S32(bmheader.rowBytes)*start, SEEK_SET);
			if (err < B_OK) { free(scanline); free(packet_buffer); return err; }
			
			// Grab batches of data in advance so we don't Read() for every packet
			if (get_more_data) {
				if (packet_count != 0) memmove(packet_buffer, packet_pos, packet_count); // slide everything to the front
				err = input.Read(packet_buffer + packet_count, packet_buffer_size - packet_count);
				if (err < B_OK) { free(scanline); free(packet_buffer); return err; }
				if (err < packet_buffer_size - packet_count) get_more_data = false; // done fetching
				packet_count = packet_count + err;
				packet_pos = (uchar *)packet_buffer;
			}
			
			uint32 *dst = (uint32 *)scanline;
			while (w > 0) {
				// Add leftover compressed pixels if present
				if (compressed > 0 && w > 0) {
					if (packet_count < 1) { free(scanline); free(packet_buffer); return B_ERROR; }
					int count = (compressed > w) ? w : compressed;
					for (int x = 0; x < count; x++) {
						dst[0] = palette[*packet_pos];
						dst++;
					}
					packet_pos++;
					packet_count--;
					w -= count;
					compressed -= count;
				}
				// Add leftover normal pixels if present
				if (normal > 0 && w > 0) {
					int count = (normal > w) ? w : normal;
					if (packet_count < count) { free(scanline); free(packet_buffer); return B_ERROR; }
					for (int x = 0; x < count; x++) {
						dst[0] = palette[*packet_pos];
						packet_pos++;
						dst++;
					}
					packet_count -= count;
					normal -= count;
					w -= count;
				}
				// Read new packet header
				if (compressed == 0 && normal == 0 && w > 0) {
					if (packet_count < 1) { free(scanline); free(packet_buffer); return B_ERROR; }
					if (*packet_pos & 0x80) compressed = (*packet_pos & 0x7f) + 1;
					else normal = (*packet_pos & 0x7f) + 1;
					packet_pos++;
					packet_count--;
	            }
			}
			
			// output the 32 bit data
			if (debug) printf("Write %d\n", S32(bmheader.rowBytes));
			err = output.Write(scanline, S32(bmheader.rowBytes));
			if (err != S32(bmheader.rowBytes)) if (err >= 0) err = B_DEVICE_FULL;
			if (err < B_OK) { free(scanline); free(packet_buffer); return err; }
			start += add;
		}
		free(packet_buffer);
	}
	// 8 bit color and greyscale uncompressed images
	else if ((tgaheader.pixsize == 8) && ((tgaheader.version == 1) || (tgaheader.version == 3))) {
		while (start != end) {
			err = output.Seek(pos+S32(bmheader.rowBytes)*start, SEEK_SET);
			if (err < B_OK) { free(scanline); return err; }
			ssize_t wdata = width;
			// Based on advice from JBQ, I'm reading into the last 1/4 of the buffer
			// and expanding my four byte values from lower to higher memory for
			// performance reasons.
			err = input.Read(((uchar *)scanline) + (width * 3), wdata);
			if (err < B_OK) { free(scanline); return err; }
			if (wdata != err) { free(scanline); return B_ERROR; }
			//	expand pixels
			uchar *src = ((uchar *)scanline) + (width * 3);
			if (tgaheader.version == 1) { // color
				uint32 *dst = scanline;
				while (src != (uchar *)dst) {
					dst[0] = palette[src[0]]; // no swapping needed, because it's a verbatim copy
					src += 1;
					dst += 1;
				}
			} else {
				uchar *dst = (uchar *)scanline;
				// We are writing to B_RGB32 on both platforms, always little endian
				// By using uchar we avoid writing different code for each
				while (src != dst) { // greyscale
					dst[0] = src[0]; // B
					dst[1] = src[0]; // G
					dst[2] = src[0]; // R
					dst[3] = 255;    // A
					src += 1;
					dst += 4;
				}
			}
			//	output the 24 bit data
			if (debug) printf("Write %d\n", S32(bmheader.rowBytes));
			err = output.Write(scanline, S32(bmheader.rowBytes));
			if (err != S32(bmheader.rowBytes)) if (err >= 0) err = B_DEVICE_FULL;
			if (err < B_OK) { free(scanline); return err; }
			start += add;
		}
	}
	else {
		free(scanline);
		return B_NO_TRANSLATOR;
	}

	free(scanline);
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
	if (getenv("TGA_HANDLER_DEBUG") != NULL) debug = 1;

	// 0 -> default bitmap format
	if (outType == 0) outType = B_TRANSLATOR_BITMAP;

	if (outType == (*inInfo).type)
		return CopyLoop((*inSource), (*outDestination));
	else if ((outType == 'TGA ') && ((*inInfo).type == B_TRANSLATOR_BITMAP))
		return WriteTarga((*inSource), (*outDestination), ioExtension);
	else if ((outType == B_TRANSLATOR_BITMAP) && ((*inInfo).type == 'TGA '))
		return WriteBitmap((*inSource), (*outDestination), ioExtension);

	//	if it was none of the permutations we know, this ain't for us!
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
	(*outView) = new BView(*outExtent, "TGATranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW);
	(*outView)->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	font_height fh;
	be_bold_font->GetHeight(&fh);
	BRect r(10, 15, 10 + be_bold_font->StringWidth(translatorName), 15 + fh.ascent + fh.descent);

	BStringView *str = new BStringView(r, "title", translatorName);
	str->SetFont(be_bold_font);
	(*outView)->AddChild(str);

	char versStr[100];
	sprintf(versStr, "v%d.%d.%d %s", (int)(translatorVersion>>8), (int)(translatorVersion>>4)&0xf,
		(int)(translatorVersion&0xf), __DATE__);
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

