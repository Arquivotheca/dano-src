#include "ProfilerApp.h"
#include "MWAboutView.h"

class MWAbout : public BWindow
{

	public:
		MWAbout();
		~MWAbout();
		
		virtual bool QuitRequested();
	private:
	MWAboutView *ImageView;
};