#ifndef __KalView
#define __KalView

#ifndef _VIEW_H
#include <View.h>
#endif

class KalView : public BView {

public:
				KalView (BRect frame, char *name);
	void		Draw (BRect updateRect);
	void		AttachedToWindow();
};

#endif
