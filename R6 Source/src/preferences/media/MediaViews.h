#if !defined(MEDIA_VIEWS_H)
#define MEDIA_VIEWS_H

#include <MediaAddOn.h>
#include <MediaRoster.h>
#include <View.h>

class BMenuField;
class BBox;
class BStringView;
class BCheckBox;
class BMenu;

class BEmptyView : public BView {
public:
		BEmptyView(
				const BRect area,
				const char * name,
				uint32 resize,
				uint32 flags) :
			BView(area, name, resize, flags | B_FULL_UPDATE_ON_RESIZE)
			{
				SetViewColor(216, 216, 216);
			}
virtual	void Draw(
				BRect area)
			{
				const char * str = "This hardware has no controls.";
				font_height fh;
				GetFontHeight(&fh);
				float h = fh.ascent-fh.descent-fh.leading;
				float w = StringWidth(str);
				float y = (int)(Bounds().Height()+h)/2;
				float x = (int)(Bounds().Width()-w)/2;
				BRect r(x, y-h, x+w, y+h);
				if (area.Intersects(r)) {
					SetLowColor(ViewColor());
					SetHighColor(0,0,0);
					DrawString(str, BPoint(x, y));
				}
			}
};

const int32 M_SETINPUT = 'seti';
const int32 M_SETINPUT_CHANNEL = 'sich';
const int32 M_SETOUTPUT = 'seto';
const int32 M_SETOUTPUT_CHANNEL = 'soch';
const int32 M_REALTIME = 'satn';
const int32 M_SETBUFFERSIZE = 'sbuf';

#define MAX_NODES 50
#define MAX_BUFFER_SIZE 8192

class HardwareView : public BView {
public:

					HardwareView(BRect);

	virtual void	MessageReceived(BMessage*); 	
	virtual void	AttachedToWindow();
	virtual void	AllAttached();
	virtual void	Draw(BRect area);
	virtual void	FrameResized(float width, float height);

protected:

	BBox *mIOBox;
	BMenuField *mOutField;
	BMenuField *mInField;
	BMenuField *mOutChannelField;
	BMenuField *mInChannelField;
	BStringView *mWarningString;
	BCheckBox *mRealTimeCheck;
	bool mDrawWarning;

	media_node mInputNode;
	media_node mOutputNode;

	dormant_node_info mInNodeList[MAX_NODES];
	dormant_node_info mOutNodeList[MAX_NODES];

	BMenu *mInMenu;
	BMenu *mOutMenu;
		
	uint32 mInIndex;
	uint32 mOutIndex;

	BMediaRoster *roster;
};

class VideoView : public HardwareView {
public:
					VideoView(BRect);
	virtual void	MessageReceived(BMessage*);
	virtual void	AttachedToWindow();
};

class AudioView : public HardwareView {
public:
					AudioView(BRect);
	virtual void	MessageReceived(BMessage*);
	virtual void	AttachedToWindow();
	virtual void	AllAttached();
private:
	bool		IsNodeLive(const dormant_node_info &info, media_node *out_node = NULL);
	
	media_input 	mChannelInputs[MAX_NODES];
	int32			mCurrentChannel;
	BMenu			*mOutputChannelMenu;
	BCheckBox 		*mVolumeCheck;
	BMenuField		*mSizeField;
	BMenu 			*mSizeMenu;
	bool			mShowVolume;
	bool			mNeedChannelSelection;
};

#endif //MEDIA_VIEWS_H
