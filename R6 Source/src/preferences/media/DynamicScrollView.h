#if !defined(DYNAMIC_SCROLL_BAR_H)
#define DYNAMIC_SCROLL_BAR_H

#include <View.h>
#include <ScrollBar.h>

class BDynamicScrollView : public BView {
public:
		BDynamicScrollView(
				const BRect area,
				const char * name,
				uint32 resize,
				uint32 flags,
				border_style border = B_FANCY_BORDER);
		~BDynamicScrollView();

		BView *Target();
		void SetTarget(BView *newTarget);
		void SetTargetBounds(BRect bounds);
		void DiscardTarget();
		void FrameResized(float width, float height);
		void Draw(BRect area);

private:
	BScrollBar *mVBar;
	BScrollBar *mHBar;
	BView *mTarget;
	BRect mTargetBounds;
	float mScrollHeight;
	float mScrollWidth;
	bool mBorderStyle;
};

#endif //DYNAMIC_SCROLL_BAR_H
