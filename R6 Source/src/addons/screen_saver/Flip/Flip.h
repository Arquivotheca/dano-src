#include <ScreenSaver.h>

// Main module class
class Flip : public BScreenSaver
{
	unsigned char	*buffer;
	unsigned char	*video;
	double			v;
	double			step;
	int32			height;
	int32			width;
	int32			gap;

public:
				Flip(BMessage *message, image_id image);

	void		StartConfig(BView *view);

	status_t	StartSaver(BView *v, bool preview);
	void		StopSaver();
	void		DirectConnected(direct_buffer_info *info);
	void		DirectDraw(int32 frame);
};
