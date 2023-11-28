#include <stdio.h>
#include <Rect.h>
#include <InterfaceDefs.h>
#include <Bitmap.h>

#include "RenderScope.h"



void render_scope( 	BBitmap *bitmap, 
				BRect destRect, 
				rgb_color color, 
				float *elements,
				int32 elementCount )
{
	// const color_map *cmap = system_colors();
	// uint8 colorIndex = inverse_cm_lookup( cmap, color );
	
	// Clip destRect
	BRect		bounds = bitmap->Bounds();
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
	float height = destRect.bottom - destRect.top+1;
	
	// Calculate base address of first pixel
	int32 rowBytes = bitmap->BytesPerRow();
	uint8 *base = ((uint8 *)bitmap->Bits()) + (rowBytes*int32(destRect.top))+(int32(destRect.left)*sizeof(rgb_color));
	
	float center = (destRect.bottom - destRect.top)/2;
	// Create table of base addresses for rows
	uint8	*rowBaseTable[256];
	rgb_color colorTable[256];
	
	float lineGain, red, green, blue;
	
	for( int32 i=0; i<height; i++ )
	{
		lineGain = 1.0/(1.1 - (fabs( center-float(i) )/center));
		red = color.red*lineGain;
		green = color.green*lineGain;
		blue = color.blue*lineGain;
		if( red > 255 )
			red = 255;
		if( green > 255 )
			green = 255;
		if( blue > 255 )
			blue = 255;
		colorTable[i].red = uint8(red);
		colorTable[i].green = uint8(green);
		colorTable[i].blue = uint8(blue);
		rowBaseTable[i] = base+(i*rowBytes);
	}
	
	float pixelsPerElement = width/elementCount;
	float fpixel = 0;
	
	int32 x, y;
	
	rgb_color	*pixelPtr;
	int32		redSum, greenSum, blueSum;
	
	// Render all elements
	
	for( int32 i=0; i<elementCount; i++, fpixel+=pixelsPerElement )
	{
		y = int32((elements[i]*center) + center);
		x = int32( fpixel );
		
		if( (x < 0 )||(y < 0)||(x >= width)||(y >= height) ) {  }
		//	printf( "Element out of range: %.3f, %ld, %ld\n", elements[i], x, y );
		else
		{	
			pixelPtr = ((rgb_color *)rowBaseTable[y])+x;
			
			redSum = int32(pixelPtr->red)+colorTable[y].red;
			greenSum = int32(pixelPtr->green)+colorTable[y].green;
			blueSum = int32(pixelPtr->blue)+colorTable[y].blue;
			
			if( redSum > 255 )
				redSum = 255;
			if( greenSum > 255 )
				greenSum = 255;
			if( blueSum > 255 )
				blueSum = 255;
			pixelPtr->red = redSum;
			pixelPtr->blue = blueSum;
			pixelPtr->green = greenSum;
		}
	}
}

