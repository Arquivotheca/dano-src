// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef BMPVIEW_H
#define BMPVIEW_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Errors.h>
#include <View.h>
#include <Bitmap.h>
#include <Point.h>
#include <Rect.h>

// For the ressources
#include "AboutBox.h"

namespace BPrivate
{

class BMPView : public BView
{
public:
			BMPView(	BRect frame,
						const char *name 	= NULL, 
						uint32 resizingMode	= B_FOLLOW_LEFT | B_FOLLOW_TOP,
						uint32 flags		= B_WILL_DRAW);
	virtual	~BMPView(void);
						
	virtual void AttachedToWindow(void);
	virtual	void Draw(BRect frame);			
	virtual	void GetPreferredSize(float *x, float *y);			

	status_t SetBitmap(int32 id);

private:
	BBitmap *fBitmap;
	BImageResources *resourcehandler;
};


} using namespace BPrivate;

#endif
