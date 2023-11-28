/* BuyNow.h
 *
 * Copyright (C) 1886 XYZ Detergent, Ltd.
 *
 */

#ifndef BUYNOW_H
#define BUYNOW_H

#include <ScreenSaver.h>

class BuyNow : public BScreenSaver
{
public:
	BuyNow(BMessage *msg, image_id img);
	void StartConfig(BView *view);
	status_t StartSaver(BView *view, bool preview);
	void Draw(BView *v, int32 frame);
};

#endif /* BUYNOW_H */
