/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993, David Koblas.  (koblas@netcom.com)    | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */


#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <InterfaceDefs.h>

#include "IDgif.h"

#define        MAXCOLORMAPSIZE         256

#define CM_RED         0
#define CM_GREEN       1
#define CM_BLUE        2

#define        MAX_LWZ_BITS            12

#define GIFIMG_IDENTIFIER	0x2C
#define GIFIMG_TERMINATOR	0x3B
#define INTERLACE           0x40
#define LOCALCOLORMAP  0x80
#define BitSet(byte, bit)      (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len) (fread(buffer, len, 1, file))

#define LM_to_uint(a,b)                        (((b)<<8)|(a))

#ifndef pm_message
#define pm_message printf
#define pm_error printf
#endif

//#define DEBUG 1

typedef struct {
	char 	Signature[3];
	char 	Version[3];
	unsigned short	ScreenWidth;
	unsigned short	ScreenHeight;
	unsigned char	SizeOfGlobalTable;
	unsigned short	TableEntries;
	unsigned char	TableSorted;
	unsigned char	ColorResolution;
	unsigned char	TableExists;
	unsigned char	Background;
	unsigned char	AspectRatio;
	rgb_color   	ColorMap[MAXCOLORMAPSIZE];
	char			GrayScale;
} GifScreenInfo;

GifScreenInfo GifScreen;

static struct {
       int     transparent;
	int background;
       int     delayTime;
       int     inputFlag;
       int     disposal;
} Gif89 = {-1,-1,-1,0};

int    verbose = 0;
int    showComment;

static GfxImage* ReadGIF (FILE *fd, int imageNumber);
static int ReadColorMap ( FILE *fd, int number, rgb_color* );
static int DoExtension ( FILE *fd, int label );
static int GetDataBlock ( FILE *fd, unsigned char  *buf );
static int GetCode ( FILE *fd, int code_size, int flag );
static int LWZReadByte ( FILE *fd, int flag, int input_code_size );
static GfxImage* ReadImage ( FILE *fd, GfxImage *, int interlace, int ignore );



static GfxImage*
ReadGIF(FILE *fd, int imageNumber)
{
	unsigned char   buf[16];
	unsigned char   c;
	int             grayScale;
	int             useGlobalColormap;
	int             bitPixel;
	int				interlaced;
	int             imageCount = 0;
	char            version[4];

	if (! ReadOK(fd,GifScreen.Signature,3))
		pm_error("error reading signature" );
	if (! ReadOK(fd,GifScreen.Version,3))
		pm_error("error reading version" );

	if (strncmp(GifScreen.Signature,"GIF",3) != 0)
		pm_error("not a GIF file" );

	if ((strncmp(GifScreen.Version, "87a",3) != 0) && 
		(strncmp(GifScreen.Version, "89a",3) != 0))
		pm_error("bad version number, not '87a' or '89a'" );

	if (! ReadOK(fd,buf,7))
		pm_error("failed to read screen descriptor" );

	GifScreen.ScreenWidth           = LM_to_uint(buf[0],buf[1]);
	GifScreen.ScreenHeight          = LM_to_uint(buf[2],buf[3]);
	GifScreen.SizeOfGlobalTable     = (buf[4] & 0x07);
	GifScreen.TableEntries			= (1L << (GifScreen.SizeOfGlobalTable +1));
	GifScreen.TableSorted		= (buf[4]&0x08);
	GifScreen.ColorResolution = (((buf[4]&0x70)>>3)+1);
	GifScreen.TableExists			= ((buf[4] & 0x80) == 0x80);
	GifScreen.Background      = buf[5];
	GifScreen.AspectRatio     = buf[6];
#ifdef DEBUG
	printf("CODECgif - Screen: %d %d\n", GifScreen.ScreenWidth, GifScreen.ScreenHeight);
	printf("CODECgif - Size Of Global: 0x%x\n", GifScreen.SizeOfGlobalTable);
	printf("CODECgif - Sorted: %d\n", GifScreen.TableSorted);
	printf("CODECgif - Entries: %d\n", GifScreen.TableEntries);
	printf("CODECgif - Resolution: %d\n", GifScreen.ColorResolution);
	printf("CODECgif - Background: %d\n", GifScreen.Background);
	printf("CODECgif - Table: %d\n", GifScreen.TableExists);
#endif	
	// Allocate space for the image
	GfxImage *tmpImage = 0;
	GfxImage *newImage = (GfxImage *)malloc(sizeof(GfxImage));
	newImage->XOffset = 0;
	newImage->YOffset = 0;
	newImage->width = 0;
	newImage->height = 0;
	newImage->Background = GifScreen.Background;
	newImage->bytes_per_row = 0;
	newImage->type = B_COLOR_8_BIT;
	newImage->data = 0;

	if (GifScreen.TableExists) 
	{    /* Global Colormap */
		if (ReadColorMap(fd,GifScreen.TableEntries,GifScreen.ColorMap))
			pm_error("error reading global colormap" );
	} else
		printf("Not color table exists\n");

	if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49) 
	{
		float   r;
		r = ( (float) GifScreen.AspectRatio + 15.0 ) / 64.0;
		pm_message("warning - non-square pixels; to fix do a 'pnmscale -%cscale %g'",
			r < 1.0 ? 'x' : 'y',
			r < 1.0 ? 1.0 / r : r );
	}

//	printf("ReadGIF() - Width: %d Height: %d\n", GifScreen.Width, GifScreen.Height);
	
	for (;;) 
	{
		if (! ReadOK(fd,&c,1))
			pm_error("EOF / read error on image data" );

		if (c == GIFIMG_TERMINATOR) 
		{         /* GIF terminator */
			if (imageCount < imageNumber)
				pm_error("only %d image%s found in file",
					imageCount, imageCount>1?"s":"" );
			return tmpImage;
		}

		if (c == '!') 
		{         /* Extension */
			if (! ReadOK(fd,&c,1))
				pm_error("OF / read error on extention function code");
				DoExtension(fd, c);
			continue;
		}

		if (c != GIFIMG_IDENTIFIER) 
		{         /* Not a valid start character */
			pm_message("bogus character 0x%02x, ignoring", (int) c );
			continue;
		}

		++imageCount;

		if (! ReadOK(fd,buf,9))
			pm_error("couldn't read left/top/width/height");

		newImage->XOffset = LM_to_uint(buf[0],buf[1]);
		newImage->YOffset = LM_to_uint(buf[2],buf[3]);
		newImage->width = LM_to_uint(buf[4],buf[5]);
		newImage->height = LM_to_uint(buf[6],buf[7]);
		newImage->data = (unsigned char *)malloc(newImage->width*newImage->height);
		newImage->bytes_per_row = newImage->width;
		
		useGlobalColormap = ! (buf[8] &0x1);

		bitPixel = 1L<<((buf[8]&0x07)+1);
		interlaced = (buf[8] & 0x40);
		
		if (useGlobalColormap)
		{
#ifdef DEBUG
			printf("USING GLOBAL COLOR MAP!!\n");
#endif
			// Make sure the image copies the global palette
			for (int i=0; i<256; i++)
				newImage->palette[i] = GifScreen.ColorMap[i];					
		} else
		{
			grayScale = ReadColorMap(fd, bitPixel, newImage->palette);
			printf("end of localcolormap: %d\n", grayScale);
		}
					
		
		//printf("CODECgif - Image: %d %d\n", newImage->width, newImage->height);

		tmpImage = ReadImage(fd, newImage, interlaced, 
				imageCount != imageNumber);
	}
	
	return tmpImage;
}

static int
ReadColorMap(FILE *fd, int number, rgb_color *buffer)
{
	int             i;
	unsigned char   rgb[3];
	int             flag;

	flag = true;

	
	//printf("ReadColorMap - %d\n", number);
	
	for (i = 0; i < number; ++i) 
	{
		if (! ReadOK(fd, rgb, 3))
			pm_error("bad colormap" );

		buffer[i].red = rgb[0];
		buffer[i].green = rgb[1];
		buffer[i].blue = rgb[2];
		//buffer[i].alpha = 255;
		
		//printf("%3d, %3d, %3d\n", rgb[0], rgb[1], rgb[2]);
		flag &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
	}

	return false;
}

static int
DoExtension(FILE *fd, int label)
{
	static char     buf[256];
	char            *str;
	//Gif89Info Gif89 = { -1, -1, -1, 0 };

	switch (label) 
	{
		case 0x01:              /* Plain Text Extension */
			str = "Plain Text Extension";
#ifdef notdef
			if (GetDataBlock(fd, (unsigned char*) buf) == 0)
                       ;

			lpos   = LM_to_uint(buf[0], buf[1]);
			tpos   = LM_to_uint(buf[2], buf[3]);
			width  = LM_to_uint(buf[4], buf[5]);
			height = LM_to_uint(buf[6], buf[7]);
			cellw  = buf[8];
			cellh  = buf[9];
			foreground = buf[10];
			background = buf[11];

			while (GetDataBlock(fd, (unsigned char*) buf) != 0) 
			{
                       PPM_ASSIGN(image[ypos][xpos],
                                       cmap[CM_RED][v],
                                       cmap[CM_GREEN][v],
                                       cmap[CM_BLUE][v]);
                       ++index;
			}

			return false;
#else
		break;
#endif

		case 0xff:              /* Application Extension */
			str = "Application Extension";
		break;
       
		case 0xfe:              /* Comment Extension */
			str = "Comment Extension";
			while (GetDataBlock(fd, (unsigned char*) buf) != 0) 
			{
				if (showComment)
					pm_message("gif comment: %s", buf );
			}
			return false;
		break;
		
		case 0xf9:              /* Graphic Control Extension */
			str = "Graphic Control Extension";
			(void) GetDataBlock(fd, (unsigned char*) buf);
			Gif89.transparent = (buf[0] & 0x1);
			Gif89.disposal    = (buf[0] >> 2) & 0x7;
			Gif89.inputFlag   = (buf[0] >> 1) & 0x1;
			Gif89.delayTime   = LM_to_uint(buf[1],buf[2]);
			if (Gif89.transparent)
			{
				Gif89.background = (unsigned char )buf[3];
			}
			
			while (GetDataBlock(fd, (unsigned char*) buf) != 0)
                       ;
			return false;
		break;
		
		default:
			str = buf;
			sprintf(buf, "UNKNOWN (0x%02x)", label);
		break;
	}

	pm_message("got a '%s' extension", str );

	while (GetDataBlock(fd, (unsigned char*) buf) != 0)
               ;

	return false;
}

int    ZeroDataBlock = false;

static int
GetDataBlock(FILE *fd, unsigned char *buf)
{
	unsigned char   count;

	if (! ReadOK(fd,&count,1)) 
	{
		pm_message("error in getting DataBlock size" );
		return -1;
	}

	ZeroDataBlock = count == 0;

	if ((count != 0) && (! ReadOK(fd, buf, count))) 
	{
		pm_message("error in reading DataBlock" );
		return -1;
	}

	return count;
}

static int
GetCode(FILE *fd, int code_size, int flag)
{
	static unsigned char    buf[280];
	static int              curbit, lastbit, done, last_byte;
	int                     i, j, ret;
	unsigned char           count;

	if (flag) 
	{
		curbit = 0;
		lastbit = 0;
		done = false;
		return 0;
	}

	if ( (curbit+code_size) >= lastbit) 
	{
		if (done) 
		{
			if (curbit >= lastbit)
				pm_error("ran off the end of my bits" );
			return -1;
		}
		buf[0] = buf[last_byte-2];
		buf[1] = buf[last_byte-1];

		if ((count = GetDataBlock(fd, &buf[2])) == 0)
			done = true;

		last_byte = 2 + count;
		curbit = (curbit - lastbit) + 16;
		lastbit = (2+count)*8 ;
	}

	ret = 0;
	for (i = curbit, j = 0; j < code_size; ++i, ++j)
		ret |= ((buf[ i / 8 ] & (1 << (i % 8))) != 0) << j;

	curbit += code_size;

	return ret;
}

static int
LWZReadByte(FILE *fd, int flag, int input_code_size)
{
	static int      fresh = false;
	int             code, incode;
	static int      code_size, set_code_size;
	static int      max_code, max_code_size;
	static int      firstcode, oldcode;
	static int      clear_code, end_code;
	static int      table[2][(1<< MAX_LWZ_BITS)];
	static int      stack[(1<<(MAX_LWZ_BITS))*2], *sp;
	register int    i;

	if (flag) 
	{
		set_code_size = input_code_size;
		code_size = set_code_size+1;
		clear_code = 1 << set_code_size ;
		end_code = clear_code + 1;
		max_code_size = 2*clear_code;
		max_code = clear_code+2;

		GetCode(fd, 0, true);
               
		fresh = true;

		for (i = 0; i < clear_code; ++i) 
		{
                       table[0][i] = 0;
                       table[1][i] = i;
		}
		
		for (; i < (1<<MAX_LWZ_BITS); ++i)
			table[0][i] = table[1][0] = 0;

		sp = stack;

		return 0;
	} else if (fresh) 
	{
		fresh = false;
		do {
			firstcode = oldcode =GetCode(fd, code_size, false);
		} while (firstcode == clear_code);
		return firstcode;
	}

	if (sp > stack)
		return *--sp;

	while ((code = GetCode(fd, code_size, false)) >= 0) 
	{
		if (code == clear_code) 
		{
			for (i = 0; i < clear_code; ++i) 
			{
				table[0][i] = 0;
				table[1][i] = i;
			}
			
			for (; i < (1<<MAX_LWZ_BITS); ++i)
				table[0][i] = table[1][i] = 0;
			
			code_size = set_code_size+1;
			max_code_size = 2*clear_code;
			max_code = clear_code+2;
			sp = stack;
			firstcode = oldcode =GetCode(fd, code_size, false);
			return firstcode;
		} else if (code == end_code) 
		{
                       int             count;
                       unsigned char   buf[260];

                       if (ZeroDataBlock)
                               return -2;

                       while ((count = GetDataBlock(fd, buf)) > 0)
                               ;

                       if (count != 0)
                               pm_message("missing EOD in data stream (common occurence)");
                       return -2;
               }

               incode = code;

               if (code >= max_code) {
                       *sp++ = firstcode;
                       code = oldcode;
               }

               while (code >= clear_code) {
                       *sp++ = table[1][code];
                       if (code == table[0][code])
                               pm_error("circular table entry BIG ERROR");
                       code = table[0][code];
               }

               *sp++ = firstcode = table[1][code];

               if ((code = max_code) <(1<<MAX_LWZ_BITS)) {
                       table[0][code] = oldcode;
                       table[1][code] = firstcode;
                       ++max_code;
                       if ((max_code >= max_code_size) &&
                               (max_code_size < (1<<MAX_LWZ_BITS))) {
                               max_code_size *= 2;
                               ++code_size;
                       }
               }

               oldcode = incode;

               if (sp > stack)
                       return *--sp;
       }
       return code;
}



GfxImage*
ReadImage(FILE *fd, GfxImage *theImage, 
	int interlace, int ignore)
{
	unsigned char   c;      
	int             v;
	int             xpos = 0, ypos = 0, pass = 0;
	int width = theImage->width;
	int height = theImage->height;
	
	/*
	**  Initialize the Compression routines
	*/
	if (! ReadOK(fd,&c,1))
		pm_error("EOF / read error on image data" );

	if (LWZReadByte(fd, true, c) < 0)
		pm_error("error reading image" );

	/*
	**  If this is an "uninteresting picture" ignore it.
	*/
	if (ignore) 
	{
		if (verbose)
			pm_message("skipping image..." );

		while (LWZReadByte(fd, false, c) >= 0)
                       ;
		return NULL;
	}

	unsigned char *pixels = (unsigned char *)theImage->data;

	if (verbose)
		pm_message("reading %d by %d%s GIF image",
                       width, height, interlace ? " interlaced" : "" );


	while ((v = LWZReadByte(fd,false,c)) >= 0 ) 
	{
		int offset = ypos*theImage->bytes_per_row+xpos;
		
		if (Gif89.transparent && (Gif89.background == v))
		{
			theImage->Transparent = 1;
			theImage->Background = Gif89.background;
#ifdef DEBUG
			printf("IMAGE HAS TRANSPARENCY: %d\n", theImage->Background);
#endif
		}
		
		pixels[offset] = v;
				
		++xpos;
		if (xpos == width) 
		{
			xpos = 0;
			if (interlace) 
			{
				switch (pass) 
				{
					case 0:
					case 1:
						ypos += 8; 
					break;
					
					case 2:
						ypos += 4; 
					break;
					
					case 3:
						ypos += 2; 
					break;
				}

				if (ypos >= height) 
				{
					++pass;
					switch (pass) 
					{
						case 1:
							ypos = 4; 
						break;
						
						case 2:
							ypos = 2; 
						break;
						
						case 3:
							ypos = 1; 
						break;
						
						default:
							goto fini;
					}
				}
				
			} else 
			{
				++ypos;
			}
		} else
		{
			// Still on the same line
		}
		
		if (ypos >= height)
			break;
	}

fini:
	if (LWZReadByte(fd,false,c)>=0)
		pm_message("too much input data, ignoring extra...");


	//printf("CODECgif - ReadImage: END\n");
	
	return theImage;
}








char *IDName = "GIF Codec";
char *IDAuthor = "William Adams";
char *IDNotice = "Copyright Be Inc. 1996, All Rights reserved.";
char *IDEncoder = "IDgif";
char *IDDecoder = "IDgif";

char *rrasaddon_IDName()
{
	return IDName;
}

char *rrasaddon_IDAuthor()
{
	return IDAuthor;
}

char *rrasaddon_IDNotice()
{
	return IDNotice;
}

char *rrasaddon_IDEncoder()
{
	return IDEncoder;
}

char *rrasaddon_IDDecoder()
{
	return IDDecoder;
}


//====================================================
// Function: CanReadImage
// 
// Returns a float value representing the degree to
// which this module is capable of reading the image.
// The closer it returns to 1.0, the more confident
// it is that the image can be read successfully.
//
// If the image can be read at all, then a value of 0.8
// should be returned.  If it can't be read at all, then
// a value of 0.0 should be returned.
//====================================================

float
CanCreateImage(void *data, long dataLen)
{
	if (dataLen < 6)
		return 0.0;
		
	if (strncmp((char *) data,"GIF87a", (size_t) 6)==0 ||
      strncmp((char *) data,"GIF89a", (size_t) 6)==0)
		return 1.0;

	return 0.0;
}



//====================================================
// Function: CreateImage
//
// Create a GfxImage based on a file of GIF data
//====================================================

GfxImage *
CreateImage(char *file)
{
	FILE *fd;

	//printf("CODECgif - attempting to create image\n");

	if (!file)
		return 0;
		
	GfxImage *newImage = 0;	
	
	int	imageNumber = 1;

	showComment = false;

	fd = fopen(file, "r");
		
	newImage = ReadGIF(fd, imageNumber);

	return newImage;
	
}

