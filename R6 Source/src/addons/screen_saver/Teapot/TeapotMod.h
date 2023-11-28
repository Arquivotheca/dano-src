#include <ScreenSaver.h>

class ObjectView;

// Main module class
class Teapot : public BScreenSaver
{
	ObjectView	*bgl;

public:
				Teapot(BMessage *message, image_id image);

	void		StartConfig(BView *view);

	status_t	StartSaver(BView *v, bool preview);
	void		StopSaver();
	void		DirectConnected(direct_buffer_info *info);
	void		DirectDraw(int32 frame);
};
