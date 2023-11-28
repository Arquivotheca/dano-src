/*	BMPHandler.cpp	*/
/*	Datatypes translator for the BMP format	*/

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


#define PRELOAD_BMP 1



char			translatorName[] = "BMP Images";		//	required, C string, ex "Jon's Sound"
char			translatorInfo[100];
int32			translatorVersion = B_BEOS_VERSION;		//	required, integer, ex 100

namespace BPrivate {
class infoFiller {
public:
	infoFiller()
		{
			sprintf(translatorInfo, "BMP image translator v%d.%d.%d, %s",
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
	kBMP = 'BMP '
};

translation_format inputFormats[] = {
	{ kBMP, B_TRANSLATOR_BITMAP, 0.5, 0.6, "image/x-bmp", "BMP image" },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.4, 0.6, "image/x-be-bitmap", "Be Bitmap image" },
	{ 0, 0, 0, 0, 0, 0 },
};		//	optional (else Identify is always called)

translation_format outputFormats[] = {
	{ 'BMP ', B_TRANSLATOR_BITMAP, 0.5, 0.6, "image/x-bmp", "BMP image" },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.4, 0.6, "image/x-be-bitmap", "Be Bitmap image" },
	{ 0, 0, 0, 0, 0, 0 },
};	//	optional (else Translate is called anyway)


#define S32(x) B_BENDIAN_TO_HOST_INT32(x)
#define SFL(x) B_BENDIAN_TO_HOST_FLOAT(x)

#define INT_HEIGHT(x) (SFL(x.bottom)-SFL(x.top))
#define INT_WIDTH(x) (SFL(x.right)-SFL(x.left))

const uint32 SWAPPED_B_TRANSLATOR_BITMAP = S32(B_TRANSLATOR_BITMAP);

/* structs cribbed from IDbmp.cpp */

/* 
	The images consist of two sections.  First is a bitmap header.
	This simply tells you the size of the file and where in the file
	the image data starts.

	The second structure tells you on a per image basis the critical
	things such as bit-depth, image size, and the like.
*/

typedef struct _Win3xHead {
	unsigned long	file_size;
	unsigned short	reserved[2];
	unsigned long	offset_bits;
} Win3xHead;

typedef struct _Win3xInfoHead {
	unsigned long	header_size;
	unsigned long	width;
	unsigned long	height;

	unsigned short	planes;
	unsigned short	bit_count;

	unsigned long	compression;
	unsigned long	image_size;
	unsigned long	x_pixels;
	unsigned long	y_pixels;
	unsigned long	number_colors;
	unsigned long	colors_important;
} Win3xInfoHead;

typedef struct _OS2InfoHead {
	unsigned long header_size;
	unsigned short width;
	unsigned short height;
	unsigned short planes;
	unsigned short bit_count;
} OS2InfoHead;


static status_t
check_header(
	unsigned char * data,
	Win3xHead & w3head,
	Win3xInfoHead & w3info,
	off_t & pos)
{
	pos = 0;
	memcpy(&w3head, &data[2], sizeof(w3head));
	w3head.file_size = B_LENDIAN_TO_HOST_INT32(w3head.file_size);
	w3head.offset_bits = B_LENDIAN_TO_HOST_INT32(w3head.offset_bits);
	if (w3head.offset_bits >= w3head.file_size) {
		if (debug) fprintf(stderr, "offset_bits %d, file_size %d\n", 
			w3head.offset_bits, w3head.file_size);
		return B_NO_TRANSLATOR; /* inconsistent header */
	}
	memcpy(&w3info, &data[2+sizeof(w3head)], sizeof(w3info));
#define sw32(x) x = B_LENDIAN_TO_HOST_INT32(x)
#define sw16(x) x = B_LENDIAN_TO_HOST_INT16(x)
	sw32(w3info.header_size);
	if (w3info.header_size < 12 || w3info.header_size >= w3head.file_size) {
		if (debug) fprintf(stderr, "header_size %d, file_size %d\n", 
			w3info.header_size, w3head.file_size);
		return B_NO_TRANSLATOR;
	}
	if (w3info.header_size == 12) {
		/* OS/2 */
		OS2InfoHead os2info;
		memcpy(&os2info, &data[2+sizeof(w3head)], sizeof(os2info));
		w3info.width = B_LENDIAN_TO_HOST_INT16(os2info.width);
		w3info.height = B_LENDIAN_TO_HOST_INT16(os2info.height);
		w3info.planes = B_LENDIAN_TO_HOST_INT16(os2info.planes);
		w3info.bit_count = B_LENDIAN_TO_HOST_INT16(os2info.bit_count);
		w3info.compression = 0;
		w3info.image_size = 0;
		w3info.x_pixels = 72;
		w3info.y_pixels = 72;
		w3info.number_colors = 0;
		w3info.colors_important = 0;
	}
	else {
		sw32(w3info.width);
		sw32(w3info.height);
		sw16(w3info.planes);
		sw16(w3info.bit_count);
		sw32(w3info.compression);
		sw32(w3info.image_size);
		sw32(w3info.x_pixels);
		sw32(w3info.y_pixels);
		sw32(w3info.number_colors);
		sw32(w3info.colors_important);
	}
	switch (w3info.bit_count) {
	case 1:
	case 4:
	case 8:
	case 24:
	case 32:
		/* these are accepted sizes */
		break;
	default:
		if (debug) fprintf(stderr, "bit_count %d\n", w3info.bit_count);
		return B_NO_TRANSLATOR;
	}
	if ((w3info.compression==0) && (w3info.bit_count*w3info.width*w3info.height/8 > w3head.file_size)) {
		if (debug) fprintf(stderr, "calc_size %d > file_size %d\n", 
			w3info.bit_count*w3info.width*w3info.height/8, w3head.file_size);
		return B_NO_TRANSLATOR;
	}
	if ((w3info.compression < 0) || (w3info.compression > 1)) {
		if (debug) fprintf(stderr, "compression %d\n", w3info.compression);
		return B_NO_TRANSLATOR;	/* uncompressed and rle only */
	}
	if((w3info.compression == 1)&&(w3info.bit_count!=8)) {
		if (debug) fprintf(stderr, "compression bits %d\n", w3info.bit_count);
		return B_NO_TRANSLATOR;	/* can only handle compressed 8 bit */
	}
	pos = 2 + sizeof(w3head) + w3info.header_size;
	return B_OK;
}


static void
expand_bits(
	unsigned char * ptr,
	unsigned long * palette,
	int count)
{
	unsigned int shift = 7-((count-1)&7);
	while (count-- > 0) {
		((unsigned long *)ptr)[count] = palette[(ptr[count/8]>>shift)&1];
		shift = (shift+1) & 7;
	}
}


static void
expand_nybbles(
	unsigned char * ptr,
	unsigned long * palette,
	int count)
{
	if (count & 1) {
		--count;
		((unsigned long *)ptr)[count] = palette[(ptr[count/2]>>4)&0xf];
	}
	unsigned char shift = 0;
	while (count-- > 0) {
		((unsigned long *)ptr)[count] = palette[(ptr[count/2]>>shift)&0xf];
		shift = shift ^ 4;
	}
}


static void
expand_bytes(
	unsigned char * ptr,
	unsigned long * palette,
	int count)
{
	while (count-- > 0) {
		((unsigned long *)ptr)[count] = palette[ptr[count]];
	}
}


static void
expand_bgr(
	unsigned char * ptr,
	int count)
{
	while (count-- > 0) {
		ptr[count*4+3] = 0xff;
		ptr[count*4+2] = ptr[count*3+2];
		ptr[count*4+1] = ptr[count*3+1];
		ptr[count*4] = ptr[count*3];
	}
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
	if (getenv("BMP_HANDLER_DEBUG") != NULL) debug = 1;
	if (debug) puts("BMPHandler::Identify()");
	//	Check that our assumptions hold water
	ASSERT(sizeof(short) == 2);
	if (sizeof(short) != 2) {
		fprintf(stderr, "BMPTranslator sees sizeof(short) as %d, should be 2\n", sizeof(short));
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
		fprintf(stderr, "BMPTranslator compiled with wrong endian-ness!\n");
		return B_ERROR;
	}

	//	check that we can handle the output format requested
	if ((outType != 0) && (outType != B_TRANSLATOR_BITMAP) && 
			(outType != kBMP)) {
		if (debug) fprintf(stderr, "outType %c%c%c%c\n", outType>>24, 
			outType>>16, outType>>8, outType);
		return B_NO_TRANSLATOR;
	}

#define BYTES 100

	/* re-define the header struct because we can't make a union */
	/* with BRect (it has a constructor) */

	struct CRect {
		float	left;
		float	top;
		float	right;
		float	bottom;
	};
	
	struct CTranslatorBitmap {
		uint32		magic;
		CRect		bounds;
		uint32		rowBytes;
		color_space	colors;
		uint32		dataSize;
	};

	union {
		unsigned char			data[BYTES];
		CTranslatorBitmap		bitmap;
	}	buffer;

	//	read header and see what happens
	uint32 iThink = 0;	//	set to guess of format
	status_t err;
	ssize_t size = sizeof(buffer);
	err = (*inSource).Read(&buffer, size);
	if ((err < B_OK) || (size != sizeof(buffer))) 
		return err;

	//	make a guess based on magic numbers
	if (buffer.bitmap.magic == SWAPPED_B_TRANSLATOR_BITMAP) {
		iThink = B_TRANSLATOR_BITMAP;
	}
	else if (buffer.data[0] == 'B' && buffer.data[1] == 'M') {
		iThink = kBMP;
		if (debug) printf("BMP magic\n");
	}
	else {
		return B_NO_TRANSLATOR;
	}

	//	verify our guess
	if (iThink == B_TRANSLATOR_BITMAP)
	{
		int multi = 0;
		switch (S32(buffer.bitmap.colors))
		{
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGBA32:
		case B_RGB32:
			multi = 4;
			break;
		case B_COLOR_8_BIT:
			multi = 1;
			break;
		default:
			if (debug) fprintf(stderr, "buffer.bitmap.colors %d\n", 
				S32(buffer.bitmap.colors));
			return B_NO_TRANSLATOR;
		}
		if ((SFL(buffer.bitmap.bounds.right) <= SFL(buffer.bitmap.bounds.left)) ||
				(SFL(buffer.bitmap.bounds.bottom) <= SFL(buffer.bitmap.bounds.top))) {
			if (debug) fprintf(stderr, "bad bitmap bounds\n");
			return B_NO_TRANSLATOR;
		}
		if (S32(buffer.bitmap.rowBytes) < INT_WIDTH(buffer.bitmap.bounds)*multi) {
			if (debug) fprintf(stderr, "too small rowBytes %d\n", buffer.bitmap.rowBytes);
			return B_NO_TRANSLATOR;
		}
		if (S32(buffer.bitmap.dataSize) < S32(buffer.bitmap.rowBytes)*INT_HEIGHT(buffer.bitmap.bounds)) {
			if (debug) fprintf(stderr, "too small dataSize %d\n", buffer.bitmap.dataSize);
			return B_NO_TRANSLATOR;
		}

		//	set up outInfo
		(*outInfo).type = inputFormats[1].type;
		(*outInfo).translator = 0;
		(*outInfo).group = inputFormats[1].group;
		(*outInfo).quality = inputFormats[1].quality;
		(*outInfo).capability = inputFormats[1].capability;
		sprintf((*outInfo).name, "%s, type %d", inputFormats[1].name, S32(buffer.bitmap.colors));
		strcpy((*outInfo).MIME, inputFormats[1].MIME);

	} else {	//	BMP

		//	maptype can be 0 (no map) or 1 (palette map)
		Win3xHead w3head;
		Win3xInfoHead w3info;
		off_t pos;
		status_t err = check_header(buffer.data, w3head, w3info, pos);
		if (err < 0) {
			return err;
		}
		/* so it passed our "stringent" consistency checks... */

		(*outInfo).type = inputFormats[0].type;
		(*outInfo).translator = 0;
		(*outInfo).group = inputFormats[0].group;
		(*outInfo).quality = inputFormats[0].quality;
		(*outInfo).capability = inputFormats[0].capability;
		sprintf((*outInfo).name, "%s, %s (%d bits)", inputFormats[0].name, 
			w3info.header_size == 12 ? "OS/2 format" : "MS format", w3info.bit_count);
		strcpy((*outInfo).MIME, inputFormats[0].MIME);
	}
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


template<class T> class array_guard {
	T * & guarded;
public:
	array_guard(T * & init) : guarded(init) { }
	~array_guard() { if (guarded) delete[] guarded; }
};


static status_t 
WriteBMP(
	BPositionIO &		input,
	BPositionIO &		output,
	BMessage *			ioExtension)
{
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
	bool set_alpha = false;
	switch (bits_hdr.colors) {
	case B_RGB32:
		set_alpha = true;
	case B_RGBA32:
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
	size_t row_bytes = ((int)(pix_size*(bits_hdr.bounds.Width()+1)+31)/32)*4;

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
		if (debug) fprintf(stderr, "Write 2 returns %d\n", err);
		return (err < B_OK) ? err : B_DEVICE_FULL;
	}
	err = output.Write(&w3head, sizeof(w3head));
	if (err != sizeof(w3head)) {
		if (debug) fprintf(stderr, "Write head returns %d\n", err);
		return (err < B_OK) ? err : B_DEVICE_FULL;
	}
	err = output.Write(&w3info, sizeof(w3info));
	if (err != sizeof(w3info)) {
		if (debug) fprintf(stderr, "Write info returns %d\n", err);
		return (err < B_OK) ? err : B_DEVICE_FULL;
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
			if (debug) fprintf(stderr, "Write palette returns %d\n", err);
			return (err < B_OK) ? err : B_DEVICE_FULL;
		}
	}
	size_t rd_bytes = bits_hdr.rowBytes;
	void * data = malloc(rd_bytes > row_bytes ? rd_bytes : row_bytes);
	if (!data) {
		return B_NO_MEMORY;
	}
	for (int iy=(int)bits_hdr.bounds.Height(); iy>=0; iy--) {
		/* assume that the bitmap is local, so seek in that */
		input.Seek(rd_bytes*iy+sizeof(bits_hdr), 0);
		err = input.Read(data, rd_bytes);
		if (err < rd_bytes) {
			if (debug) fprintf(stderr, "Read data %d returns %d\n", 
				rd_bytes, err);
			free(data);
			return (err < B_OK) ? err : B_ERROR;
		}
		err = output.Write(data, row_bytes);
		if (err < row_bytes) {
			if (debug) fprintf(stderr, "Write data %d returns %d\n", 
				row_bytes, err);
			free(data);
			return (err < B_OK) ? err : B_DEVICE_FULL;
		}
	}
	free(data);
	/* all done! */
	return B_OK;
}


//	assume that the bitmap is local (such as writing into a BitmapStream)
//	thus, we seek in the bitmap (output) and not in the BMP (input)
static status_t
WriteBitmap(
	BPositionIO &		input,
	BPositionIO &		output,
	BMessage *			ioExtension)
{
	if (debug) fprintf(stderr, "WriteBitmap\n");
	Win3xHead		w3head;
	Win3xInfoHead	w3info;
	TranslatorBitmap bmheader;
	status_t		err;
	ssize_t			rd;
	ulong *	palette = NULL;
	unsigned char	magic[256];
	off_t 			pos;
	/* delete[] the palette if and when we've allocated it */
	array_guard<ulong> palette_guard(palette);

	rd = sizeof(magic);
	err = input.Read(magic, rd);
	if (err < B_OK) return err;
	err = check_header(magic, w3head, w3info, pos);
	if (err < B_OK) return err;

	//	set up header
	bmheader.magic = SWAPPED_B_TRANSLATOR_BITMAP;
	bmheader.bounds.left = B_HOST_TO_BENDIAN_FLOAT(0.0);
	bmheader.bounds.top = B_HOST_TO_BENDIAN_FLOAT(0.0);
	bmheader.bounds.right = B_HOST_TO_BENDIAN_FLOAT((float)(w3info.width)-1);
	bmheader.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT((float)(w3info.height)-1);
	bmheader.rowBytes = B_HOST_TO_BENDIAN_INT32((int)(INT_WIDTH(bmheader.bounds)+1)*4);
	bmheader.dataSize = B_HOST_TO_BENDIAN_INT32((int)S32(bmheader.rowBytes)*(int)(SFL(bmheader.bounds.bottom)+1));

	//	we always output in 24 bit format, since palettes cannot be represented well in 8 bit mode
	bmheader.colors = (color_space)S32(B_RGB_32_BIT);
	err = output.SetSize(S32(bmheader.dataSize)+sizeof(bmheader));
	if (debug) printf("SetSize bmheader returns %d\n", err);
	if (err < B_OK) return err;                                               
	if (debug) printf("Write %d\n", sizeof(bmheader));
	err = output.Write(&bmheader, sizeof(bmheader));
	if (debug) printf("Write bmheader returns %d\n", err);
	if (err != sizeof(bmheader)) if (err >= 0) err = B_DEVICE_FULL;
	if (err < B_OK) return err;

	//	deal with color map, if any
	if (w3info.bit_count < 24) {

		err = input.Seek(pos, SEEK_SET);
		if (err < B_OK) return err;

		if (w3info.number_colors == 0) {
			w3info.number_colors = (1 << w3info.bit_count);
		}
		if (debug) printf("number_colors = %d\n", w3info.number_colors);
		palette = new unsigned long[w3info.number_colors];
		for (int ix=0; ix<w3info.number_colors; ix++)
			palette[ix] = S32((ix*0x01010100)|0xff);	//	bgra, black to white
		if (w3info.header_size == 12) {
			/* OS/2: packed RGB */
			err = input.Read(palette, 3*w3info.number_colors);
			expand_bgr((uchar *)palette, w3info.number_colors);
		}
		else {
			err = input.Read(palette, 4*w3info.number_colors);
		}
		if (err < B_OK) return err;
	}

	err = input.Seek(w3head.offset_bits, SEEK_SET);
	if (err < B_OK) return err;

	//	set up buffer
	size_t to_read = ((w3info.bit_count*w3info.width+31)/32)*4;
	size_t to_write = S32(bmheader.rowBytes);
	uint32 * scanline = (uint32 *)malloc(to_write+12);	/* misalligned by 3 32-bit pixels as the worst case */
	if (scanline == NULL) return B_NO_MEMORY;

// define this symbol to preload compressed data, which greatly speeds up decompression

	int deltabytes = 0;
	if (debug) printf("read_rowbytes = %d (of %d)\n", to_read, w3info.bit_count*w3info.width);
	if (debug) printf("write_rowbytes = %d\n", to_write);

#if PRELOAD_BMP
	uint8 *compdata= NULL;
	uint8 *compptr;
	if (debug) printf("compression = %d\n", w3info.compression);
	if(w3info.compression==1)
	{
		size_t datasize=w3head.file_size-w3head.offset_bits;
		if (debug) printf("compressed datasize = %d\n", datasize);
		compdata=(uint8*)malloc(datasize);
		if(compdata==NULL)
		{
			free(scanline);
			if(debug) fprintf(stderr,"not enough memory (%d bytes)\n",datasize);
			return B_NO_MEMORY;
		}
		err=input.Read(compdata,datasize);
		if(err<datasize)
		{
			free(scanline);
			free(compdata);
			if(debug) fprintf(stderr,"short file: %d<%d \n",err,datasize);
			return err < B_OK ? err : B_ILLEGAL_DATA;
		}
		compptr=compdata;
	}
#endif

	if (debug) printf("expanding %d lines\n", w3info.height);
	for (int iy=0; iy<w3info.height; iy++)
	{
		if(w3info.compression==0)
		{
			err = input.Read(scanline, to_read);
			if (err < to_read) {
				free(scanline);
				return err < B_OK ? err : B_ILLEGAL_DATA;
			}
			if (w3info.bit_count == 1) {
				expand_bits((uchar *)scanline, palette, w3info.width);
			}
			else if (w3info.bit_count == 4) {
				expand_nybbles((uchar *)scanline, palette, w3info.width);
			}
			else if (w3info.bit_count == 8) {
				expand_bytes((uchar *)scanline, palette, w3info.width);
			}
			else if (w3info.bit_count == 24) {
				expand_bgr((uchar *)scanline, w3info.width);
			}
			/* else assume data is correct 32 bits */
		}
		else if(w3info.compression==1)
		{
			if (w3info.bit_count == 4)
			{
				// 4 bit run length compression
				// not implemented yet (and code cannot be reached yet)
				if (debug)
					fprintf(stderr,"BMP translator: unsupported number of bits for decompression\n");
				return B_ILLEGAL_DATA;
			}
			else if (w3info.bit_count == 8)
			{
				// loop until the scanline is filled
				int x=0;
				uint8 bytepair[2];
				uint32 * scl=scanline;
				while(true)
				{
					if (debug && deltabytes > 0) printf("deltabytes = %d, x = %d\n", deltabytes, x);
					while ((deltabytes>0) && (x<to_read)) {
						*scl++ = 0x00ffffff;
						x++;
						deltabytes--;
					}
					if (x >= to_read) {
						break;
					}
#if PRELOAD_BMP
					bytepair[0] = compptr[0];
					bytepair[1] = compptr[1];
					compptr+=2;
#else
					err = input.Read(bytepair, 2);
					if (err < 2)
					{
						free(scanline);
						if(debug) fprintf(stderr,"BMP translator bailing out: not enough data\n");
						return err < B_OK ? err : B_ILLEGAL_DATA;
					}
#endif

					if(bytepair[0]==0)
					{
						if(bytepair[1]==0) {
							if (debug > 1) fprintf(stderr, "eol with %d to go\n", x);
							if (x > 0)
							{
								while (x < to_read) {
									*scl++ = 0x00ffffff;
									x++;
								}
								break; // end of line reached
							}
							else
							{
								if (debug > 1) fprintf(stderr, "eol is bogus\n");
							}
						}
						else if(bytepair[1]==1)
						{
							// end of bitmap
//							iy=w3info.height-1; 	// force break from for loop
							//	force break by clearing to white transparent
							deltabytes = (to_read-x)+(w3info.height-iy)*w3info.width;
							if (debug > 1) fprintf(stderr, "EOB (deltabytes = %d)\n", deltabytes);
							break;
						}
						else if(bytepair[1]==2)
						{
#if PRELOAD_BMP
							deltabytes = w3info.width*(unsigned char)compptr[1] +
								(unsigned char)compptr[0];
							compptr += 2;
#else
							err = input.Read(bytepair, 2);
							deltabytes = w3info.width*(unsigned char)bytepair[1] +
								(unsigned char)bytepair[0];
#endif
							if (debug > 1) fprintf(stderr, "delta %d pixels\n", deltabytes);
							continue;
						}
						else if(bytepair[1]>=3)
						{
							if (debug > 1) fprintf(stderr, "%d pixels\n", bytepair[1]);
							if(x>=to_read)
							{
								if(debug) fprintf(stderr,"BMP: scanline overflow\n");
								free(scanline);
#if PRELOAD_BMP
								free(compdata);
#endif
								return B_ILLEGAL_DATA;
							}
							for(int i=0;i<bytepair[1]&&x<to_read;i++,x++)
							{
								uint8 databyte;
#if PRELOAD_BMP
								databyte=*compptr++;
#else
								err = input.Read(&databyte, 1);
								if (err < 1) {
									free(scanline);
									if(debug) fprintf(stderr,"BMP bailing out: not enough data\n");
									return err < B_OK ? err : B_ILLEGAL_DATA;
								}
#endif
								*scl++=palette[databyte];
							}
							// runs are always on word boundaries
							if(bytepair[1]&1)
							{
								if (debug > 1) printf("adjust to word boundary\n");
#if PRELOAD_BMP
								compptr++;
#else
								input.Seek(1,SEEK_CUR);
#endif
							}
						}
					}
					else
					{
						if (debug > 1) fprintf(stderr, "%d RLE-d\n", bytepair[0]);
						if(x>=to_read)
						{
							if(debug) fprintf(stderr,"BMP: RLE scanline overflow\n");
							free(scanline);
#if PRELOAD_BMP
							free(compdata);
#endif
							return B_ILLEGAL_DATA;
						}
						// run length-decode the data
						for(int i=0;i<bytepair[0]&&x<to_read;i++,x++)
							*scl++=palette[bytepair[1]];
					}
				}
			}
		}
		/* write data down to up as per windows conventions */
		err = output.Seek(sizeof(bmheader) + S32(bmheader.rowBytes)*(w3info.height-iy-1), SEEK_SET);
		if (err < B_OK)
		{
			free(scanline);
#if PRELOAD_BMP
			free(compdata);
#endif
			return err;
		}
		err = output.Write(scanline, to_write);
		if (err < to_write) {
			free(scanline);
#if PRELOAD_BMP
			free(compdata);
#endif
			return err < B_OK ? err : B_ERROR;
		}
	}
	free(scanline);
#if PRELOAD_BMP
	free(compdata);
#endif

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
	if (getenv("BMP_HANDLER_DEBUG") != NULL) 
	{
		debug = atoi(getenv("BMP_HANDLER_DEBUG"));
		if (debug < 1) debug = 1;
	}

	// 0 -> default bitmap format
	if (outType == 0) outType = B_TRANSLATOR_BITMAP;

	if (outType == (*inInfo).type)
		return CopyLoop((*inSource), (*outDestination));
	else if ((outType == kBMP) && ((*inInfo).type == B_TRANSLATOR_BITMAP))
		return WriteBMP((*inSource), (*outDestination), ioExtension);
	else if ((outType == B_TRANSLATOR_BITMAP) && ((*inInfo).type == kBMP))
		return WriteBitmap((*inSource), (*outDestination), ioExtension);

	//	if it was none of the permutations we know, this ain't for us!
	if (debug) fprintf(stderr, "Unknown format %c%c%c%c\n", outType>>24, 
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

	(*outExtent).Set(0,0,239,239);
	(*outView) = new BView(*outExtent, "BMPTranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW);
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

