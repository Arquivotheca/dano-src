#ifndef VIDEOVIEW_H
#define VIDEOVIEW_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <OS.h>
#include <View.h>

const int32 M_UPDATE_POS = 'updp';


class BHandler;
class BTimedEventQueue;

class NodeWrapper;

class VideoView : public BView {
public:

	VideoView(BRect, BPoint videoSize, const char *, uint32);
	~VideoView();
	media_node Node();
	media_node TimeSource();
	void SetBitmap(BBitmap *);

	static BRect ViewRect(BRect videoRect);
	BRect ContentRect();

	void SetProprotionalResize(bool);
	bool ProportionalResize() const;
	
protected:

	virtual void Draw(BRect);
	virtual void AttachedToWindow();

	static BRect VideoRect(BRect frame, BPoint videoSize);

private:

	BBitmap	*fCurrentBitmap;
	BWindow	*fWindow;
	BPoint fVideoSize;
	bool fProportionalResize;

	typedef	BView _inherited;
};

const float kMinViewWidth = 180;
const float kMinViewHeight = 125;

#endif
