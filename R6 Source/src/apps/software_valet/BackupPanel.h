#ifndef _BACKUPPANEL_H_
#define _BACKUPPANEL_H_


#include "MFilePanel.h"

class BackupPanel : public MFilePanel
{
public:
	BackupPanel(bool save, BMessenger *dest);
};

status_t	BackupRegistry(entry_ref &ref, const char *name);

#endif
