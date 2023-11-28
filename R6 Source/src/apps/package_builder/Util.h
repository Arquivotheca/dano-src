// Util.h

#ifndef _UTIL_H
#define _UTIL_H

extern const rgb_color light_gray_background;
extern const rgb_color disabled_color;

long doError(const char *msg, status_t errcode = 0);

long doError(const char *format, const char *subtext, status_t errcode = 0);

long doError(const char *format,
			 const char *subtext,
			 const char *button1,
			 const char *button2 = NULL,
			 const char *button3 = NULL,
			 status_t	errcode = 0);

void PositionWindow(BWindow *w,float horizFrac, float vertFrac);
void DrawHSeparator(float left, float right, float v, BView *view);
bool TryActivate(BMessenger &mess);


#define nel(x)	(sizeof(x)/sizeof(*(x)))

// this was never used
// it was supposed to be a resource based localization scheme
// for strings
// #include "IDStrings.h"
// extern	IDStrings	*gStrs;

#endif

