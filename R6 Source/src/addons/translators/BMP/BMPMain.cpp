/*	BMPMain.cpp	*/


#include <Application.h>
#include <Window.h>
#include <Alert.h>
#include <stdlib.h>


extern "C" status_t MakeConfig(BMessage *, BView **, BRect *);


class BMPWindow :
	public BWindow
{
public:
								BMPWindow(
									BRect			area,
									const char *	title,
									window_type		kind,
									uint32			flags);
		bool					QuitRequested();
};

BMPWindow::BMPWindow(
	BRect			area,
	const char *	title,
	window_type		kind,
	uint32			flags) :
	BWindow(area, title, kind, flags)
{
}


bool
BMPWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;	/*	we will die!		*/
}


int
main()
{
	BApplication app("application/x-vnd.Be-BMPTranslator");
	BRect extent(0,0,239,239);
	BRect window_rect(extent);
	window_rect.OffsetTo(100, 100);
	BMPWindow *window = new BMPWindow(window_rect, 
		"BMP Settings", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE);
	BView *config = NULL;
	status_t err = MakeConfig(NULL, &config, &extent);
	if ((err < B_OK) || (config == NULL))
	{
		BAlert *alrt = new BAlert("No Config", "BMPTranslator does not currently allow user configuration.", "Quit");
		alrt->Go();
		exit(1);
	}
	window->ResizeTo(extent.Width(), extent.Height());
	window->AddChild(config);
	window->Show();
	app.Run();
	return 0;
}
