#ifndef BG_BOX_H
#define BG_BOX_H

#include <View.h>

class TBackgroundBox : public BView
{
public:
				TBackgroundBox(BRect);
	virtual		~TBackgroundBox();
	virtual	void AttachedToWindow();
		void	Draw(BRect);
		void	AddLine(BRect, bool);

private:
	struct line_info
	{
		BRect frame;
		bool orientation;	// true is horizontal
	};

	line_info	fLineList[8];
	int			fLineCount;
};

#endif
