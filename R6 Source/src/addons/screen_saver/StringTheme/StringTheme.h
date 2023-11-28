#include <ScreenSaver.h>

class StringTheme : public BScreenSaver
{
public:
				StringTheme(BMessage *message, image_id id);
	void		StartConfig(BView *view);
	status_t	StartSaver(BView *v, bool preview);
	void		Draw(BView *v, int32 frame);
};
