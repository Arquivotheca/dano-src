#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <InterfaceDefs.h>
#include <Bitmap.h>

#include "LevelMeter.h"

LevelMeter::LevelMeter( BRect frame, const char *name, uint32 resizeMask, uint32 flags  )
	: RenderView( frame, B_RGB32, name, resizeMask, flags )
{
	level[0] = 0; level[1] = 0;
	lastLevel[0] = 0; lastLevel[1] = 0;
	elementPtr = NULL;
}

LevelMeter::~LevelMeter( void )
{
	if( elementPtr )
		free( elementPtr );
}

void LevelMeter::ThreadInit( void )
{
	InitElements();
}

status_t LevelMeter::ThreadExit( void )
{
	
	return B_OK;
}

void LevelMeter::Render( BBitmap *bmap )
{
	NextLevels();
	// Set all pixels to black
	for( int32 *next = (int32 *)bmap->Bits(), *last = (next + bmap->BitsLength()/sizeof(int32)); next < last; )
		*next++ = 0;
	rgb_color	meterColor;
	meterColor.red = 80; meterColor.green = 240; meterColor.blue = 80;
	BRect		destRect = bounds;
	float		offset = destRect.right = destRect.right/2;
	
	for( int32 i=0; i<2; i++ )
	{
		RenderMeter( bmap, meterColor, destRect, elements[i], elementCount );
		destRect.OffsetBy( offset, 0 );
	}
}

static const float kMeterMargin = 3;

void LevelMeter::RenderMeter( 
	BBitmap *bmap, 
	rgb_color color, 
	BRect destRect, 
	float *elements,
	int32 elementCount )
{
	// Clip destRect
	BRect		bounds = bmap->Bounds();
	if( destRect.left < 0 )
		destRect.left = 0;
	if( destRect.top < 0 )
		destRect.top = 0;
	if( destRect.right > bounds.right )
		destRect.right = bounds.right;
	if( destRect.bottom > bounds.bottom )
		destRect.bottom = bounds.bottom;
	
	// Calculate height and width
	float width = destRect.right - destRect.left+1;
	// float height = destRect.bottom - destRect.top+1;
	int32 meterWidth = int32(width-(2*kMeterMargin));
	
	// Calculate base address of first pixel
	int32 rowBytes = bmap->BytesPerRow();
	uint8 *base = ((uint8 *)bmap->Bits()) + (rowBytes*int32(destRect.bottom))+(int32(destRect.left+kMeterMargin)*sizeof(rgb_color));
	uint8 *rowBase = base;
	rgb_color *pixelPtr, *lastPtr;
	rgb_color destColor;
	float red, green, blue;
	
	// Do for each element
	for( int32 i=0; i<elementCount; i++, rowBase -= rowBytes*2 )
	{
		red = float(color.red)*elements[i];
		green = float(color.green)*elements[i];
		blue = float(color.blue)*elements[i];
		
		
		if( red > 255 )
			red = 255;
		if( green > 255 )
			green = 255;
		if( blue > 255 )
			blue = 255;
		destColor.red = uint8(red);
		destColor.green = uint8(green);
		destColor.blue = uint8(blue);
		pixelPtr = (rgb_color *)rowBase;
		for( lastPtr = pixelPtr+meterWidth; pixelPtr<lastPtr; pixelPtr++ )
			*pixelPtr = destColor;
	}
}

void LevelMeter::Resize( void )
{
	RenderView::Resize();
	InitElements();
}

void LevelMeter::InitElements( void )
{
	if( elementPtr )
		free( elementPtr );
	elementCount = int32(bounds.bottom/2);
	elementPtr = (float *)malloc( sizeof(float)*elementCount*2 );
	memset( elementPtr, 0, sizeof(float)*elementCount*2 );
	elements[0] = elementPtr;
	elements[1] = elementPtr+elementCount;
}

static const float kMinLevel = 0.15, kMaxLevel = 3.0, kDecayRate = 0.94, 
	kEnergizeRate = 0.7, kMinActiveLevel = 0.5;
void LevelMeter::NextLevels( void )
{
	int32 nElement, lElement, delta;
	float *element;
	
	// Do for each meter
	for( int32 i=0; i<2; i++ )
	{
		nElement = int32(float(elementCount-1)*level[i]);
		lElement = int32(float(elementCount-1)*lastLevel[i]);
		
		// Decay energy level of elements
		// Do for each element
		for( int32 j=0; j<elementCount; j++ )
		{
			element = elements[i]+j;
			*element *= kDecayRate;
			// Check for minimum energy of active elements
			if( (j <= nElement)&&(*element<kMinActiveLevel) )
				*element = kMinActiveLevel;
			else if( *element < kMinLevel )
				*element = kMinLevel;
		}
		delta = nElement - lElement;
		
		// Was it an increase?
		if( (nElement - lElement) > 0 ) 
		{
			float energyInc = kEnergizeRate/float(delta);
			float energySum = energyInc; 
			// Energize elements
			for( int32 j=lElement; j<=nElement; j++, energySum+=energyInc )
			{
				element = elements[i]+j;
				*element += energySum;
				// Check for level above maximum
				if( *element > kMaxLevel )
					*element = kMaxLevel;
			}
		}
		// Set next element
		element = elements[i]+nElement;
		*element += kEnergizeRate;
		// Check for level above maximum
		if( *element > kMaxLevel )
			*element = kMaxLevel;
		lastLevel[i] = level[i];
	}
}
