#ifndef DT300SCREENDIMMER_H
#define DT300SCREENDIMMER_H

#include <add-ons/screen_saver/ScreenSaver.h>

class DT300ScreenDimmer : public BScreenSaver {
public:
	DT300ScreenDimmer(BMessage *msg, image_id img);
	void StartConfig(BView *view);
	status_t StartSaver(BView *view, bool preview);
	void StopSaver(void);
	void Draw(BView *view, int32 frame);
};

#endif /* DT300SCREENDIMMER_H */
