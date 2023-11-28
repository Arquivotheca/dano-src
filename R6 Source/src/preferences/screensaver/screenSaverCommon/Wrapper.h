#include <ScreenSaver.h>

typedef void	(*module_initialize_fn)(void *inSettings, long inSettingsSize);
typedef void	(*module_cleanup_fn)(void **outSettings, long *outSettingsSize);
typedef void	(*module_start_saving_fn)(BView *inView);
typedef void	(*module_stop_saving_fn)();
typedef void	(*module_start_config_fn)(BView *inView);
typedef void	(*module_stop_config_fn)();

class Wrapper : public BScreenSaver
{
	module_initialize_fn	module_initialize;
	module_cleanup_fn		module_cleanup;
	module_start_saving_fn	module_start_saving;
	module_stop_saving_fn	module_stop_saving;
	module_start_config_fn	module_start_config;
	module_stop_config_fn	module_stop_config;

	BView		*settings;

	void		init(BMessage *message);
	void		term(BMessage *into);

	bool		settingsopen;

public:
				Wrapper(BMessage *message, image_id id);
				~Wrapper();
	status_t	SaveState(BMessage *msg) const;
	void		StartConfig(BView *view);
	void		StopConfig();
	status_t	StartSaver(BView *v, bool preview);
	void		StopSaver();
};
