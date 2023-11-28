#ifndef _SPECIAL_KEYS_H
#define _SPECIAL_KEYS_H

#include <OS.h>
#include <InputServerFilter.h>
#include <Message.h>
#include <PlaySound.h>
#include "array.h"

struct command {
	int32 cmd;
	char *exec;
	command *next;
};

struct key_mapping {
	int32 keycode;
	command cmd;
};

class SpecialKeys : public BInputServerFilter {
//	entry_ref				m_beepRef;
	sound_handle			m_beepHandle;
	char *					m_dir;
	BArray<key_mapping>		m_map;
	bigtime_t				m_lastExec;

public:
							SpecialKeys();

	virtual	filter_result	Filter(BMessage *message, BList *outList);
};


#endif
