// LicenseView.h
#ifndef _LICENSEVIEW_H_
#define _LICENSEVIEW_H_

#include <TextView.h>
#include <View.h>
#include <Rect.h>


class LicenseView : public BView
{
public:
				LicenseView(BRect b,char *text, size_t, char *styles);
virtual void	AttachedToWindow();
virtual void	FrameResized(float w, float h);


BTextView 	*licView;
char		*ltext;
size_t		lsize;
char		*lstyle;
};


#endif
