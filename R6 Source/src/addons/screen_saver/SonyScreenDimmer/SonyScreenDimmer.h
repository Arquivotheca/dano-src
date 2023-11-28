#include <ScreenSaver.h>

class NetronDisplay;

class SonyScreenDimmer : public BScreenSaver {
public:
						SonyScreenDimmer(BMessage *message, image_id id);
	virtual				~SonyScreenDimmer();
	virtual	void		StartConfig(BView *view);
	virtual status_t	StartSaver(BView *v, bool preview);
	virtual void		StopSaver();
	virtual void		Draw(BView *v, int32 frame);

private:

	NetronDisplay	*fNetronDisplay;
	bool			fPowerOnScreen;
};
