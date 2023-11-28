#include <ScreenSaver.h>

class SuperString : public BScreenSaver
{
	bool		first;
public:
				SuperString(BMessage *message, image_id id);
	void		StartConfig(BView *view);
	status_t	StartSaver(BView *v, bool preview);
	void		Draw(BView *v, int32 frame);
	status_t	SaveState(BMessage *into) const;
};
