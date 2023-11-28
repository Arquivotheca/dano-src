#ifndef GIMAGES_H
#define GIMAGES_H

#include <InterfaceDefs.h>
#include "simplelist.h"
#include "graphic.h"

// Classes we use in here
class BMGBitmap;
class BBitmap;

//===================================================================
// Function Declaration: CreateImageGraphic
//
// The idea behind this function is that you want to be able to create
// bitmap graphics quickly and easily.  You have all the data you need,
// and you don't want to create the BBitmap graphic and then pass it
// to the constructor of BMGBitmap.  You just want to get a BMGBitmap
// quickly and easily.  The data typically comes from a resource, or
// from some #defines in a header file for image data.
//===================================================================

BMGBitmap * CreateImageGraphic(const char *aName, 
				const int32 width, const int32 height, 
				color_space cspace, const int8 bytesperpixel, 
				const unsigned char bits[], const int32 bytesperrow = -1);

BBitmap * CreateBitmap(const char *aName, 
				const int32 width, const int32 height, 
				color_space cspace, const int8 bytesperpixel, 
				const unsigned char bits[], const int32 bytesperrow = -1);
				
//==========================================================
//
//==========================================================
struct bitmapAssociation
{
	bitmapAssociation(const char *name, BBitmap *bitmap);
	
	BString	fName;
	BBitmap	*fBitmap;
};

class BMBitmapRoster
{
public:
	// Graphic cache management
	// Having a cache allows you to lookup a bitmap
	// by name once it is created.
	static bool		Add(const char *, BBitmap *);
	static BBitmap	*GetBitmap(const char *name);
	static void		DeleteAll();
	
private:
	// don't call this
	static simplelist<bitmapAssociation *>	*BitmapCache();
};

#endif
