#include <TranslatorAddOn.h>
#include <Window.h>
#include <Application.h>
#include <Alert.h>
#include <Screen.h>

#include <stdio.h>
#include <stdlib.h>

BPoint get_window_origin();
void set_window_origin(BPoint pt);

class PPMWindow :
	public BWindow
{
public:
	PPMWindow(
			BRect area) :
		BWindow(area, "PPMTranslator", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
		{
		}
	~PPMWindow()
		{
			BPoint pt(0,0);
			ConvertToScreen(&pt);
			set_window_origin(pt);
			be_app->PostMessage(B_QUIT_REQUESTED);
		}
};

int
main()
{
	BApplication app("application/x-vnd.Be-PPMTranslator");
	BRect extent(0,0,239,239);
	BRect window_rect(extent);
	window_rect.OffsetTo(100, 100);
	PPMWindow *window = new PPMWindow(window_rect);
	BView *config = NULL;
	status_t err = MakeConfig(NULL, &config, &extent);
	if ((err < B_OK) || (config == NULL))
	{
		BAlert *alrt = new BAlert("No Config", "PPMTranslator does not currently allow user configuration.", "Quit");
		alrt->Go();
		exit(1);
	}
	window->ResizeTo(extent.Width(), extent.Height());
	window->AddChild(config);
	window->Show();
	app.Run();
	return 0;
}

