// StatusWindow.h
//
// Like a BStatusBar, but with its own window.
//

#ifndef STATUSWINDOW_H
#define STATUSWINDOW_H

#include <Window.h>
#include <StatusBar.h>

class StatusWindow : public BWindow
{
public:
	StatusWindow(BRect r, const char *label = NULL,
						  const char *trailing_label = NULL)
		: BWindow(r, "Status", B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
	{
		r.OffsetTo(B_ORIGIN);
		mBar = new BStatusBar(r, "status_bar", label, trailing_label);
		mBar->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(mBar);
	}
	
	virtual ~StatusWindow() {};

	inline void Update(float delta, const char *label = NULL,
		const char *trail = NULL) {	mBar->Update(delta, label, trail); }
	inline void Reset(const char *label = NULL,
		const char *trail = NULL) { mBar->Reset(label, trail); }
	inline void SetBarColor(rgb_color color) { mBar->SetBarColor(color); }
	inline void	SetBarHeight(float height) { mBar->SetBarHeight(height); }
	inline void	SetText(const char *str) { mBar->SetText(str); }
	inline void	SetTrailingText(const char *str) { mBar->SetTrailingText(str); }
	inline void SetMaxValue(float max) { mBar->SetMaxValue(max); }

	inline float CurrentValue() const { return mBar->CurrentValue(); }
	inline float MaxValue() const { return mBar->MaxValue(); }
	inline rgb_color BarColor() const { return mBar->BarColor(); }
	inline float BarHeight() const { return mBar->BarHeight(); }
	inline const char *Text() const { return mBar->Text(); }
	inline const char *TrailingText() const { return mBar->TrailingText(); }
	inline const char *Label() const { return mBar->Label(); }
	inline const char *TrailingLabel() const { return mBar->TrailingLabel(); }

private:
	BStatusBar		*mBar;
};

#endif // STATUSWINDOW_H
