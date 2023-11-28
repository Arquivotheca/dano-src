#include <ScreenSaver.h>
#include <image.h>

class ConfigView;

class Random : public BScreenSaver
{
	BMessage		modulesettings;
	ConfigView		*config;
	BScreenSaver	*current;
	image_id		currentid;

public:
				Random(BMessage *message, image_id id);
	status_t	SaveState(BMessage *msg) const;
	void		StartConfig(BView *view);
	status_t	StartSaver(BView *v, bool preview);
	void		StopSaver();
	void		DirectConnected(direct_buffer_info *info);
	void		Draw(BView *v, int32 frame);
	void		DirectDraw(int32 frame);
	void		ModulesChanged(const BMessage *msg);
	void		SupplyInfo(BMessage *info) const;
};
