#if ! defined MODULELISTITEM_INCLUDED
#define MODULELISTITEM_INCLUDED

#include <ListItem.h>
#include "OldScreenSaver.h"
#include <image.h>
#include <SupportDefs.h>
#include <View.h>
#include <Path.h>

class ModuleListItem : public BStringItem
{
protected:
	image_id		id;
	BPath			addon;

public:
				ModuleListItem(BPath path);
	virtual		~ModuleListItem();

	virtual const char	*InternalName();
	void				GetSettingsName(char *name);
	void				SaveSettings();
	virtual status_t	Load();
	virtual void		Unload();
	void				ModulesChanged(const BMessage *msg);
	virtual void		Test();
	const BPath			*Path() const;

	BScreenSaver		*mod;

	BView				*view;
};

class BlacknessListItem : public ModuleListItem
{
public:
				BlacknessListItem();

	const char	*InternalName();
	status_t	Load();
	void		Unload();
	void		Test();
};

#endif
