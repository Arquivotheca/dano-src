#ifndef _ICONITEM_H
#define _ICONITEM_H

#include <Bitmap.h>
#include <ListItem.h>
#include <String.h>
#include <View.h>

const int32 kSmallIconWidth = 16;
const int32 kSmallIconHeight = 16;
const color_space kSmallIconColorSpace = B_COLOR_8_BIT;

class IconItem : public BListItem
{
public:
			IconItem();
			~IconItem();
			
	virtual void DrawItem(BView *owner, BRect bounds, bool complete = false);
			void SetIndent(float indent);
			void SetBaseline(float baseline);
			void SetName(const char *name);
			void SetInputIcon(void *bits = NULL);
			void SetOutputIcon(void *bits = NULL);
			
	const char *Name();		
			
private:
	bool 		mIn;
	bool 		mOut;
	BBitmap		mInputIcon;
	BBitmap		mOutputIcon;
	BString		mName;
	float 		mIndent;
	float 		mBaseline;
};

#endif
