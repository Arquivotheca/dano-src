#ifndef _SURLVIEW_H_
#define _SURLVIEW_H_

#include <StringView.h>

class SUrlView : public BStringView
{
public:
	SUrlView(BRect bounds,
			const char *name, 
			const char *text,
			uint32 resizeFlags =
				B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW);
			
	~SUrlView();
	
			void	SetAppSig(const char *sig);		
	virtual void	Draw(BRect up);
	virtual void	MouseDown(BPoint pt);
	
			void	Highlight(bool = true);
			void	SetValid(bool = true);
private:
	bool	fValid;
	char 	*appSig;
};

#endif

