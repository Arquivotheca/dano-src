/*
	ColorUtils.h
	
	Copyright Hekkelman Programmatuur
	
	Created: 10/08/97 21:51:06
*/

#ifndef COLORUTILS_H
#define COLORUTILS_H

#include <GraphicsDefs.h>

struct roSColor
{ 
    float  m_Red; 
    float  m_Green; 
    float  m_Blue; 
    float  m_Alpha; 
    float  m_Hue;
};

extern unsigned char gSelectedMap[256];
extern unsigned char gDisabledMap[256];
void InitSelectedMap();

void rgb2ro(rgb_color rgb, roSColor& ro);
rgb_color ro2rgb(roSColor& ro);
void rgb2hsv(float r, float g, float b, float& h, float& s, float& v);
void hsv2rgb(float h, float s, float v, float& r, float& g, float& b);
void rgb2f(rgb_color rgb, float& r, float& g, float& b, float& a);
rgb_color f2rgb(float r, float g, float b, float a = 0.0);

#endif
