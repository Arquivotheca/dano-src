#include <ScreenSaver.h>

class SetupView;

class Kaleid : public BScreenSaver
{
	bool		previewmode;
    bool		bForce;
	rgb_color	colorref;
    int			nPosX;
    int			nPosY;
	bigtime_t	LastCleared;	// Time screen was last cleared
	int32		nLimitRange,	// Display and control variables
				nPixels,
				nWindowMidX,
				nWindowMidY;
	SetupView	*setup;

public:
				Kaleid(BMessage *message, image_id id);
	status_t	SaveState(BMessage *into) const;
	void		StartConfig(BView *view);
	status_t	StartSaver(BView *v, bool preview);
	void		Draw(BView *v, int32 frame);
};
