//========================================================================
//	ColorDropFilter.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _COLORDROPFILTER_H
#define _COLORDROPFILTER_H

#include <MessageFilter.h>

template <class T>
class ColorDropFilter : public BMessageFilter
{
public:
							ColorDropFilter(
								T&	inColorable)
							: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
							fColorable(inColorable){}

							ColorDropFilter(
								T*	inColorable)
							: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
							fColorable(*inColorable){}

virtual	filter_result		Filter(
								BMessage *message, 
								BHandler **/*target*/)
{
	filter_result		result = B_DISPATCH_MESSAGE;

	switch (message->what)
	{
		case B_MOUSE_MOVED:
			if (message->HasData("RGBColor", B_RGB_COLOR_TYPE))
				if (! fColorable.IsFocus())
					fColorable.MakeFocus();
			break;
		
		default:
			if (message->WasDropped())
			{
				rgb_color*		color;
				int32			len;
				
				if (fColorable.IsEnabled() && B_OK == message->FindData("RGBColor", B_RGB_COLOR_TYPE, (const void**)&color, &len))
				{
					fColorable.SetValue(*color);
					fColorable.Invoke();
					
					result = B_SKIP_MESSAGE;
				}
			}
	}
	
	return result;
}

private:
		T&		fColorable;
};

#endif
