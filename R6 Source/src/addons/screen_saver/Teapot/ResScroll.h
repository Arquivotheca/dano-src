
#ifndef ResScroll_h
#define ResScroll_h

#include <interface/ScrollBar.h>

class ObjectView;

class ResScroll : public BScrollBar {
public:
		ObjectView *	objectView;
		
						ResScroll(BRect r, const char *name,
							ObjectView *target, orientation posture);
virtual	void			ValueChanged(float value);

};

#endif
