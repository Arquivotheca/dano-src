#include "OldScreenSaver.h"

class Blackness : public BScreenSaver
{
public:
				Blackness(BMessage *message, image_id id);
	void		StartConfig(BView *view);
	status_t	StartSaver(BView *v, bool preview);
	void		Draw(BView *v, int32 frame);
};
