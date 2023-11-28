#ifndef BITMAP_CONTROLS
#define BITMAP_CONTROLS

#include <Messenger.h>
#include <View.h>

class BScrollView;

enum {
	kDimensChangedMsg		= 'tdcg',
	kColorSpaceChangedMsg	= 'tccg',
	kFormatChangedMsg		= 'tfcg'
};

class BTextControl;
class BMenuField;

class TBitmapControls : public BView
{
public:
	TBitmapControls(BRect frame, const char* name,
					uint32 resize,
					uint32 flags = B_NAVIGABLE_JUMP | B_FRAME_EVENTS);
	~TBitmapControls();
	
	void SetAllTargets(BMessenger who);
	void SetAttributes(float width, float height,
					   color_space colors, type_code type);
	
	void AttachedToWindow();
	void AllAttached();
	void DetachedFromWindow();
	void GetPreferredSize(float *width, float *height);
	void FrameResized(float width, float height);
	void MessageReceived(BMessage* msg);
	
private:
	BMessenger		fTarget;
	BTextControl*	fWidth;
	BTextControl*	fHeight;
	BMenuField*		fColorSpace;
	BMenuField*		fFormat;
};

#endif
