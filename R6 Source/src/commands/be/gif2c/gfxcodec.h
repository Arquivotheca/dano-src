#pragma once

#ifndef GFXCODEC_H
#define GFXCODEC_H

#include <Directory.h>
#include <File.h>
#include <image.h>
#include <InterfaceDefs.h>

typedef unsigned char byte;

enum EDataSource {
	RECORDREF = 1,
	FILENAME,
	STREAM
};

/* values for GfxImage->type */
enum EColorType {
	IMAGE_8_BIT = 1,   
	IMAGE_24_BIT,
	IMAGE_32_BIT
};

/* values for GfxImage->color_format */

enum EColorFormat {
	F_FULLCOLOR =1,   
	F_GREYSCALE,
	F_BWDITHER,
	F_REDUCED
};

/* values for GfxImage->file_format */
enum EFileFormat {
	F_GIF = 1,
	F_JPEG,
	F_TIFF,
	F_PS,
	F_PBMRAW,
	F_PBMASCII,
	F_XBM,
	F_XPM,
	F_BMP,
	F_SUNRAS,
	F_IRIS,
	F_TARGA,
	F_FITS,
	F_PM
};

/* MONO returns total intensity of r,g,b triple (i = .33R + .5G + .17B) */
#define MONO(rd,gn,bl) ( ((int)(rd)*11 + (int)(gn)*16 + (int)(bl)*5) >> 5)

typedef struct GfxImage {
	unsigned char *data;			// image data
	long XOffset;
	long YOffset;
	long width;
	long height;			// image dimensions
	long bytes_per_row;
	int	Background;			// In the case of indexed, this is background
	int Transparent;
	color_space type;			// EColorType
	int file_format;	// EFileFormat
	int color_format;	// EColorFormat

    int   num_colors;
	rgb_color palette[256];		/* colormap, if type == IMAGE_8_BIT */

	char  full_info[128];       /* long format identifier, set on load */
	char  short_info[128];      /* short format identifier, set on load */
	char *comment;              /* image comment; saved if format supports it*/
	
	int   numpages;             /* # of page files, if >1 */
	char  pagebname[64];        /* basename of page files */
} GfxImage;



class GfxCodec 
{
public:
				GfxCodec(const char *modulename );
				~GfxCodec();

	virtual	float		CanCreateImage(const char *);
	virtual GfxImage *	CreateImage(const char *);
	virtual void		WriteImage(GfxImage*, const char *);
	
	virtual bool		IsValid();
	virtual	void		Print();
	
		GfxCodec	*next;	// Pointer to next codec so a chain can
					// be created easily.

protected:

	// Informational stuff so that we can identify
	// the add-on and find other parts.
	BDirectory	fBaseDirectory;
	char 		fFileName[B_FILE_NAME_LENGTH+1];  
	
	// Function pointers in the identifier module with useful
	// information.
	char	*(*addon_IDName)();
	char	*(*addon_IDAuthor)();
	char	*(*addon_IDNotice)();
	char	*(*addon_IDEncoder)();
	char	*(*addon_IDDecoder)();

	float	(*addon_CanCreate)(void *, long);
	GfxImage	*(*addon_CreateImage)(const char *);
	long	(*addon_WriteImage)();
	
	image_id	fIdentifier;
	image_id	fDecoder;
	image_id	fEncoder;

private:

};

#ifdef __cplusplus
extern "C" {
#endif

void	SetAddOnsDirectory(const char *);
void	ReloadCodecs();
void	PrintCodecList();

GfxImage * CreateRasterImage(const char *fname);
int WriteRasterImage(GfxImage *p, char *fname, int fmt, void *options);
void matchPalette(GfxImage *image);

#ifdef __cplusplus
}
#endif

#endif

