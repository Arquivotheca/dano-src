#include <ScreenSaver.h>

// Main module class
class Lens : public BScreenSaver
{
	unsigned char	*buffer;
	unsigned char	*video;
	int32			width, height;
	int32			rowbytes, pixelbits;
	color_space		pixel_format;

	float			r, x0, y0, z0;
	int32			maxd;
	float			dx0, dy0, dz0;
	int32			filter_side, *filter_x, *filter_y;

	void		*Fetch(int32 x, int32 y);

public:
				Lens(BMessage *message, image_id image);

	void		StartConfig(BView *view);

	status_t	StartSaver(BView *v, bool preview);
	void		StopSaver();
	void		DirectConnected(direct_buffer_info *info);
	void		DirectDraw(int32 frame);
};
