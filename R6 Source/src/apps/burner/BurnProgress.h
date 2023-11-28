//
// BurnProgress.h
//

#ifndef _BURN_PROGRESS_H_
#define _BURN_PROGRESS_H_

#include <View.h>

class CDTrack;
class BBitmap;

class BurnProgress : public BView
{
public:
					BurnProgress(BRect frame, CDTrack *head,
								 uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP);
	virtual			~BurnProgress();

	void			SetPercentComplete(float percent);
	void			InvalidateBuffer();
		
protected:
	virtual void	FrameResized(float width, float height);
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();	
	virtual void	Draw(BRect update);

private:
	void			SetBackgroundColor();
	void			DrawBuffer();
	void			CreateBuffer(float width, float height);

	BRect			fBarRect;
	float			fPercentComplete;
	CDTrack			*fHeadTrack;
	BView			*fOffscreen;
	BBitmap			*fBuffer;
	bool			fBufferValid;
};

#endif // _EDIT_WIDGET_H_
