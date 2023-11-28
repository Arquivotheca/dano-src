#include <Window.h>
#include <View.h>
#include <Application.h>
#include <MessageRunner.h>
#include <stdio.h>

const rgb_color kBarColor = { 0, 0xcf, 0, 0 };
const rgb_color kBackgroundColor = { 0, 0, 0, 0 };
const rgb_color kTextColor = { 0xcd, 0, 0, 0};

class BarView : public BView {
public:
	BarView(BRect rect);
	void SetValue(float percent, float total);
	virtual void Draw(BRect rect);
	virtual	void FrameResized(float new_width, float new_height);

private:
	float fCurrentRatio;
	float fCurrentUsage;
	float fWidth;
	float fHeight;
};

class UsageWindow : public BWindow {
public:
	UsageWindow(BRect rect);
	virtual ~UsageWindow();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *message);
private:
	void UpdateUsage();
	
	BMessageRunner *fRunner;
	BarView *fBar;
};

BarView::BarView(BRect rect)
	:	BView(rect, "bar view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
		fCurrentRatio(0.0),
		fCurrentUsage(0.0),
		fWidth(rect.Width()),
		fHeight(rect.Height())
{
}

void BarView::SetValue(float ratio, float usage)
{
	Invalidate(BRect(0, 0, fWidth, fHeight));
	fCurrentRatio = ratio;
	fCurrentUsage = usage;
}

void BarView::Draw(BRect rect)
{
	SetHighColor(kBarColor);
	FillRect(BRect(0, 0, fCurrentRatio * fWidth, fHeight));
	SetHighColor(kBackgroundColor);
	FillRect(BRect(fCurrentRatio * fWidth + 1, 0, fWidth, fHeight));

	SetDrawingMode(B_OP_OVER);
	char buf[50];
	sprintf(buf, "%.2fM", fCurrentUsage);
	SetHighColor(kTextColor);
	MovePenTo(7, fHeight - 7);
	DrawString(buf);
}

void BarView::FrameResized(float newWidth, float newHeight)
{
	fWidth = newWidth;
	fHeight = newHeight;
}

UsageWindow::UsageWindow(BRect rect)
	:	BWindow(rect, "Memory Usage", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_NOT_RESIZABLE)
{
	fBar = new BarView(Bounds());
	AddChild(fBar);
	fRunner = new BMessageRunner(BMessenger(0, this), new BMessage('updt'), 1000000);
	UpdateUsage();
}

UsageWindow::~UsageWindow()
{
	delete fRunner;
}

bool UsageWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void UsageWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case 'updt':
			UpdateUsage();
			break;
		default:
			BWindow::MessageReceived(message);
	}
}

void UsageWindow::UpdateUsage()
{
	system_info info;
	get_system_info(&info);
	fBar->SetValue((float) info.used_pages / (float) info.max_pages,
		(float) info.used_pages * B_PAGE_SIZE / (1024 * 1024));
}

int main()
{
	BApplication app("application/x-vnd.Be-MemUsage");
	(new UsageWindow(BRect(100,100,400,130)))->Show();
	app.Run();
}
