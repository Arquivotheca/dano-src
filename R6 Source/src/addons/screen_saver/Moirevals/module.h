#include <ScreenSaver.h>

class module : public BScreenSaver
{
public:
	module(BMessage *message, image_id id);
	void StartConfig(BView *view);
	status_t StartSaver(BView *v, bool preview);
	void Draw(BView *v, int32 frame);
};
