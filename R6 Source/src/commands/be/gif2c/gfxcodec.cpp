//-------1---------2---------3---------4---------5---------6
#include "gfxcodec.h"

#include <unistd.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <Entry.h>
#include <Screen.h>
#include <image.h>
#include <math.h>

static long	sqr(long v)
{
	return(v * v);
}

/*
// This one is from BScreen, and it's no longer any good
// for R3 - WAA.  The color_mapper routine above is from
// the app_server, and is more accurate.
static uint8 IndexForColor( const color_map *amap, uint8 r, uint8 g, uint8 b, uint8 a )
{
	int			index;
	
	if( r == B_TRANSPARENT_32_BIT.red &&
		g == B_TRANSPARENT_32_BIT.green &&
		b == B_TRANSPARENT_32_BIT.blue &&
		a == B_TRANSPARENT_32_BIT.alpha
	) {
		return B_TRANSPARENT_8_BIT;
	}

	if( amap == NULL ) {
		return 0;
	}

	index = ((r & 0xf8) << 7) |
			((g & 0xf8) << 2) |
			((b & 0xf8) >> 3) ;

	return amap->index_map[index];
}
*/

static uint8
color_mapper(rgb_color a_color, const color_map *a_clut)
{
	rgb_color	color1;
	long		error;
	long		best_error;
	long		best_i;
	long		i;
	long		lum_1;
	long		lum_2;

	best_error = 10000000;

	lum_1 = a_color.red + a_color.green + (a_color.blue >> 1);

	for (i = 0; i <= 255; i++) {
		color1 = a_clut->color_list[i];
		lum_2 = a_color.red + a_color.green + (a_color.blue >> 1);


		error = 3 * sqr(lum_1 - lum_2);

		error += sqr(a_color.red - color1.red);
		if (error < best_error) {
			error += (sqr((a_color.blue - color1.blue) >> 1));
			
			if (error < best_error) {
				error += sqr(a_color.green - color1.green);
				if (error < best_error) {
					best_error = error;
					if (error == 0)
						return(i);
					best_i = i;
				}
			}
		}
	}	
	return(best_i);
}

/*
void
matchPalette(GfxImage *image)
{
	const color_map	*amap = system_colors();

	if (!image)
		return ;
		
	//printf("matchPalette - BEGIN\n");
	// Go through the image palette
	// For each entry, find its equivalent in the system palette
	// stick this new value into our local palette
	BScreen myScreen;
	
	//printf("MatchPalette - trans: %d (%d)  back: %d\n", 
	//	B_TRANSPARENT_8_BIT, image->Transparent, image->Background);
	uint8 localPalette[256];
	for (int i=0; i<256; i++)
	{
		// If the palette entry is the background color, then
		// just make that transparent.  Otherwise, lookup the correct value
		if (image->Transparent && (i == image->Background))
			localPalette[i] = B_TRANSPARENT_8_BIT;
		else
		{
			uint8 colorIndex = color_mapper(image->palette[i], amap);
			rgb_color aColor = myScreen.ColorForIndex(colorIndex);

			printf("%3d) image palette - (%d %d %d) -", i,
				image->palette[i].red,image->palette[i].green,image->palette[i].blue);
			printf("%d - ",colorIndex);
			printf("matched - (%d %d %d)\n", 
				aColor.red,aColor.green,aColor.blue);
		}
	}
	
	// Now go through all the pixels in the image
	// lookup the value in the local palette
	// and substitute this new value for the current one
	long aLength = image->height*image->bytes_per_row;
	for (int ctr = 0; ctr < aLength; ctr++)
	{
		image->data[ctr] = localPalette[image->data[ctr]];
	}
}
*/

void
matchPalette(GfxImage *image)
{
	//printf("matchPalette - BEGIN\n");
	fflush(stdout);
	// Go through the image palette
	// For each entry, find its equivalent in the system palette
	// stick this new value into our local palette

	BScreen screen( B_MAIN_SCREEN_ID );

	unsigned char localPalette[256];
	for (int i=0; i<256; i++)
	{
		// If the palette entry is the background color, then
		// just make that transparent.  Otherwise, lookup the correct value
		if (image->Transparent && (i == image->Background))
			localPalette[i] = B_TRANSPARENT_8_BIT;
		else
			localPalette[i] = screen.IndexForColor(image->palette[i]);
	}
	
	// Now go through all the pixels in the image
	// lookup the value in the local palette
	// and substitute this new value for the current one
	long aLength = image->height*image->bytes_per_row;
	for (int ctr = 0; ctr < aLength; ctr++)
	{
		image->data[ctr] = localPalette[image->data[ctr]];
	}
}

GfxCodec::GfxCodec(const char* modulename)
	: addon_IDName(0),
	addon_IDAuthor(0),
	addon_IDNotice(0),
	addon_IDEncoder(0),
	addon_IDDecoder(0),
	addon_CanCreate(0),
	next(0)
{
	strcpy(fFileName, modulename);
	
	fIdentifier = B_ERROR;
	fDecoder = B_ERROR;
	fEncoder = B_ERROR;
	next = 0;

	// Try to load in the identifier module based on 
	// the name
	fIdentifier = load_add_on(fFileName);
	
	if (B_ERROR != fIdentifier)
	{
		fDecoder = fIdentifier;
		fEncoder = fIdentifier;
		
		long error;
		
		// We have successfully loaded the module,
		// now get pointers to some functions we expect to exist.
		error = get_image_symbol(fIdentifier, "rrasaddon_IDName", 2, (void **)&addon_IDName);
		if (B_NO_ERROR != error)
			printf("IDName not loaded\n");
		error = get_image_symbol(fIdentifier, "rrasaddon_IDAuthor", 2, (void **)&addon_IDAuthor);
		error = get_image_symbol(fIdentifier, "rrasaddon_IDNotice", 2, (void **)&addon_IDNotice);
		if (B_NO_ERROR != error)
			printf("IDNotice not found\n");
		error = get_image_symbol(fIdentifier, "rrasaddon_IDEncoder", 2, (void **)&addon_IDEncoder);
		error = get_image_symbol(fIdentifier, "rrasaddon_IDDecoder", 2, (void **)&addon_IDDecoder);
		error = get_image_symbol(fIdentifier, "CanCreateImage", 2, (void **)&addon_CanCreate);
		error = get_image_symbol(fIdentifier, "CreateImage", 2, (void **)&addon_CreateImage);
		if (B_NO_ERROR != error)
			printf("CreateImage not loaded\n");		
		
		//if (error != B_ERROR && addon_IDName)
		//	printf("IDName loaded: %s\n", addon_IDName());
			
	} else
	{
		//printf("add_on not loaded: %s\n",fFileName); 
	}
}

//=============================================
// Destructor
//
// Unload images
//=============================================
GfxCodec::~GfxCodec()
{
	long error = 0;
		
	// Unload the identifier module
	if (fIdentifier != B_ERROR)			
		error = unload_add_on(fIdentifier);

	// Unload the Decoder Module
	if (fDecoder != B_ERROR)			
		error = unload_add_on(fDecoder);
		
	// Unload the Encoder Module
	if (fEncoder != B_ERROR)			
		error = unload_add_on(fEncoder);

}

bool
GfxCodec::IsValid()
{
	// This is a valid codec if the addon code has been 
	// loaded, and the Module name identifier function 
	// has been found
    return ((B_ERROR != fIdentifier) && (addon_IDName));
}

//=============================================
// Method: Print
//
// Print information related to this codec
//=============================================
void
GfxCodec::Print()
{
	long error = 0;
	image_info info;
	
	// Print Base Name of module
	printf("%s\n", fFileName);
	
	// Print the identifier module
	if (fIdentifier != B_ERROR)
	{		
		error = get_image_info (fIdentifier, &info);

		printf(" Identifier: /%s\n", info.name);
		printf("   Module Name: %s\n", addon_IDName());
		printf("   Module Author: %s\n", addon_IDAuthor());
		printf("   Module Notice: %s\n", addon_IDNotice());
		printf("   Module Encoder: %s\n", addon_IDEncoder());
		printf("   Module Decoder: %s\n", addon_IDDecoder());
	} else
		printf(" %s - No Identifier loaded\n", fFileName);
	
	// Print the Decoder Module
	if (fDecoder != B_ERROR)			
	{		
		error = get_image_info (fDecoder, &info);
		printf(" Decoder: %s\n", info.name);
	} else
		printf(" %s - No Decoder loaded\n", fFileName);
		
	// Print the Encoder Module
	if (fEncoder != B_ERROR)			
	{		
		error = get_image_info (fEncoder, &info);
		printf(" Encoder: %s\n", info.name);
	} else
		printf(" %s - No Encoder loaded\n", fFileName);
}


//=====================================================
// Method: CanCreateImage
//
// This method is responsible for reporting back whether
// or not this module is capable of creating an image
// given the amount of data fed in.
//=====================================================

float
GfxCodec::CanCreateImage(const char *file)
{
	// Early exits for lack of resources
	if (!file)
		return 0;

	if (!addon_CanCreate)
	{
		printf("GfxCodec::CanCreateImage - no addon_CanCreate\n");
		return 0.0;
	}	

	char data[128];
	long buffSize =128;
	long dataLen=0;
	FILE *aFile = fopen(file,"r");
	if (!aFile)
	{
		printf("GfxCodec::CanCreate - Could not open: %s\n", file);
		return 0.0;
	}

	dataLen = fread(data, buffSize, 1, aFile);
	fclose(aFile);

	float confidence = 	addon_CanCreate(data, buffSize);
	//printf(" %s: GfxCodec::CanCreateImage - confidence: %f with %d data read\n", addon_IDName(), confidence, dataLen);
	
	return confidence;
}

GfxImage *	
GfxCodec::CreateImage(const char *file)
{
	GfxImage *newImage = 0;
	
	// Early return due to lack of resources
	if (!file || !addon_CreateImage)
	{
		printf("GfxCodec::CreateImage() - leaving early\n");
		return 0;
	}	

	// Now that we have the decoder add-on, we should be
	// able to call the decode function and get an image
	// out.
	newImage = addon_CreateImage(file);
	
	return newImage;
}


void		
GfxCodec::WriteImage(GfxImage*, const char *)
{
}








GfxCodec *gCodecList = 0;
char gAddOnsDirectory[MAXPATHLEN+1];

//---------------------------------------------
// Functions of the external interface
//
//---------------------------------------------
void
SetAddOnsDirectory(const char *dir)
{
	strncpy(gAddOnsDirectory, dir, MAXPATHLEN);
	gAddOnsDirectory[MAXPATHLEN] = '\0';
	
	ReloadCodecs();
}

void
PrintCodecList()
{
	GfxCodec *tmpCodec;
	
	// Iterate through the current codec list
	tmpCodec = gCodecList;
	while (tmpCodec)
	{
		tmpCodec->Print();
		tmpCodec = tmpCodec->next;
	}
}

void	
ReloadCodecs()
{
	GfxCodec *tmpCodec;
	
	// Iterate through the current codec list
	while (gCodecList)
	{
		long error = 0;
		
		tmpCodec = gCodecList->next;
		
		delete gCodecList;
		gCodecList = tmpCodec;
	}
	
	// If we don't have an add-ons directory,
	// then just return immediately.
	// This is one way of wiping out the add-ons
	//if (status == B_ERROR)
	//{
	//	printf("ReloadCodecs - add-ons directory not valid.\n");
	//	return;
	//}
		
	// Now traverse the current directory looking
	// for new codecs to build.
	//printf ("ReloadCodecs: %s\n", gAddOnsDirectory);

	DIR *dirp = opendir(gAddOnsDirectory);
	if (!dirp)
	{
		return;
	}

	char fullname[MAXPATHLEN];
	struct dirent *afile;
	afile = readdir(dirp);
	while (NULL != afile)
	{
		GfxCodec *newCodec = 0;

		strcpy(fullname, gAddOnsDirectory);
		strcat(fullname,afile->d_name);
		//printf("add-on: %s\n", fullname);
		newCodec = new GfxCodec(fullname);
		
		// If we didn't load the codec, or if
		// it isn't valid, then don't add it to
		// the list.
		if (!newCodec || !newCodec->IsValid())
		{
			///printf("ReloadCodecs: '%s' not loaded.\n", afile->d_name);
			afile = readdir(dirp);
			continue;
		}

		// Make the new codec the beginning of the list if
		// the list is currently blank.  Otherwise
		// add it to the end of the list.
		if (0 == gCodecList)
			gCodecList = newCodec;
		else
			tmpCodec->next = newCodec;
		tmpCodec = newCodec;

		afile = readdir(dirp);
	}
}


//=========================================================
// Decoder functions
//=========================================================

//==========================================
// Function: FindImageDecoder
//
// Goes through the decoder list asking each
// of the decoders if they know how to decode
// the image.  The one that responds with the
// highest score wins.
//
// We don't set any threshold other than > 0.0
// because some applications might just want to
// read the information in the header and not
// actually decode and create the whole image.
//==========================================

static GfxCodec *
FindImageDecoder(const char *file)
{
	GfxCodec *candidate = 0;
	GfxCodec *tmpCodec = gCodecList;
	float confidence = 0.0;
	
	// Early exit if there is no file
	if (!file)
		return 0;
		
	
	// Traverse the list
	while (tmpCodec && confidence < 1.0)
	{
		float newConfidence = 0.0;
		newConfidence = tmpCodec->CanCreateImage(file);
		if (newConfidence > confidence)
		{
			confidence = newConfidence;
			candidate = tmpCodec;
		}
		
		tmpCodec = tmpCodec->next;
		
	}

	if ((confidence > 0.0) && candidate)
		return candidate;
			
	return 0;
}

GfxImage * 
CreateRasterImage(const char *file)
{
	GfxImage *newImage = NULL;
	GfxCodec *codec = 0;
	
	codec = FindImageDecoder(file);
	
	// Create an image using the codec
	if (codec)
	{
		newImage = codec->CreateImage(file);
		matchPalette(newImage);
	} else
	{
		printf(" CreateRasterImageBFile - no appropriate codec found\n");
	}
	
	
	return newImage;
}

/*
BBitmap *
CreateBitmap(const char *file)
{
	GfxImage *img = 0;
	img = CreateRasterImage(file);
	if (!img)
		return 0;
	int bytesperpixel=1;
	switch(img->type)
	{
		case B_COLOR_8_BIT:
			bytesperpixel = 1;
		break;
		
		case B_RGB_32_BIT:
		case B_BIG_RGB_32_BIT:
			bytesperpixel = 3;
	}
	
	BRect tempRect(0, 0, img->width-1, img->height-1);
	BBitmap *bitmap = new BBitmap(tempRect, img->type);
	int bytesperrow = bytesperpixel*img->width;
	unsigned char *dataPtr = img->data;

	for (int row=0; row < img->height; row++)
	{
		int bitsoffset = row*bytesperrow;
		int bitmapoffset = row*bitmap->BytesPerRow();
		bitmap->SetBits((char*)choiceimg_bits+bitsoffset, bytesperrow, bitmapoffset, img->type);
	}
}
*/

//=========================================================
// Encoder functions
//=========================================================

int WriteRasterImage(GfxImage *p, char *fname, int fmt, void *options)
{
	// Create a BFile based on the fname
	// Call the other function using the BFile
	return -1;
}

