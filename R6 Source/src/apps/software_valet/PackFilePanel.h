#ifndef _PACKFILEPANEL_H_
#define _PACKFILEPANEL_H_


#include "MFilePanel.h"

class 		PackData;

class PackFilePanel : public MFilePanel
{
public:
	PackFilePanel(BMessenger *target,
						entry_ref	*start_dir,
						uint32		message);
	~PackFilePanel();
	virtual void SelectionChanged();
private:

	BRefFilter	*filter;
	PackData	*data;
};

#endif

