
#include "special_keys.h"
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "miniplay.h"
#include <Beep.h>


/* Throttle cmdExec commands so that you can't do back-to-back commands */
/* faster than this. It's a defense against the Baron-Mashing-Special-Keys */
/* DOS attack. A better solution might be to use some blocking primitive */
/* instead of roster->Launch() for monitoring when things complete. */
#define EXEC_TIME 1000000LL

extern "C" _EXPORT BInputServerFilter *instantiate_input_filter(void);
extern void show_volume_window();

enum commands {
	cmdNull=0,
	cmdVolumeUp,
	cmdVolumeDown,
	cmdMuteToggle,
	cmdExec,
	cmdJavaScript,
	cmdDone
};

BInputServerFilter *
instantiate_input_filter(void)
{
	return (new SpecialKeys());
}

SpecialKeys::SpecialKeys() :
	m_map(1),
	m_beepHandle(-1)
{
	int32 keycode;
	char str[256];


//	{
//		BEntry entry("/boot/custom/sounds/BeBeep",TRUE);
//		if (entry.Exists()) entry.GetRef(&m_beepRef);
//		m_beepHandle = B_BAD_SEM_ID;
//	};

	m_lastExec = 0;
	m_dir = strdup("/boot/custom/special_keys/");
	strcpy(str, m_dir);
	strcat(str, "map");
	FILE *f = fopen(str,"r");
	if (f) {
	
		while (fscanf(f,"%li",&keycode) == 1) {
			key_mapping map;
			map.keycode = keycode;
			command *cmd = &map.cmd;
			command **cmdptr = &cmd;

			if (!((fscanf(f,"%s",str) != 1) || (strcasecmp(str,"done") == 0))) {
				do {
					if (strcasecmp(str,"done") == 0) break;
					if (!(*cmdptr)) *cmdptr = (command*)malloc(sizeof(command));
					(*cmdptr)->cmd = cmdNull;
					(*cmdptr)->exec = NULL;
					(*cmdptr)->next = NULL;
					if (strcasecmp(str,"volume_up") == 0) {
						(*cmdptr)->cmd = cmdVolumeUp;
					} else if (strcasecmp(str,"volume_down") == 0) {
						(*cmdptr)->cmd = cmdVolumeDown;
					} else if (strcasecmp(str,"toggle_mute") == 0) {
						(*cmdptr)->cmd = cmdMuteToggle;
					} else if (strcasecmp(str,"exec") == 0) {
						fscanf(f,"%s",str);
						(*cmdptr)->cmd = cmdExec;
						(*cmdptr)->exec = strdup(str);
					} else if (strcasecmp(str,"javascript") == 0) {
						fscanf(f,"%s",str);
						(*cmdptr)->cmd = cmdJavaScript;
						(*cmdptr)->exec = strdup(str);
					}
					cmdptr = &(*cmdptr)->next;
				} while (fscanf(f,"%s",str) == 1);
				m_map.AddItem(map);
			};
		};
		fclose(f);
	};
}


filter_result
SpecialKeys::Filter(
	BMessage	*message, 
	BList		*)
{
	bool handled = false;

	if (message->what != B_UNMAPPED_KEY_DOWN)
		return (B_DISPATCH_MESSAGE);

	int32 key = 0;
	if (message->FindInt32("key", &key) != B_OK)
		return (B_DISPATCH_MESSAGE);

	if (key & 0x80000000) {
		if (m_lastExec+EXEC_TIME > system_time()) {
			fprintf(stderr, "unknown key (%08x): not ready yet\n", key);
		}
		else {
			char str[256];
			sprintf(str,"%sunknown_key",m_dir);
			BEntry entry(str, TRUE);
			if (entry.Exists()) {
				char arg[16];
				sprintf(arg,"%08x",key&0x7fffffff);
				entry_ref entryRef;
				entry.GetRef(&entryRef);
				char *argptr=arg;
				be_roster->Launch(&entryRef,1,&argptr);
				m_lastExec = system_time();
			}
		}
		return B_SKIP_MESSAGE;
	} 

	int32 repeat = 0;
	if (message->FindInt32("be:key_repeat", &repeat) == B_OK)
		return (B_DISPATCH_MESSAGE);	

	int32 count = m_map.CountItems();
	for (int32 i=0;i<count;i++) {
		if (key == m_map[i].keycode) {
			handled = true;
			command *cmd = &m_map[i].cmd;
			while (cmd) {
				switch (cmd->cmd) {
					case cmdVolumeUp: {
						show_volume_window();
						mini_adjust_volume(0,0,0,VOL_LOUDER|VOL_CLEAR_MUTE);
						stop_sound(m_beepHandle);
//						m_beepHandle = play_sound(&m_beepRef,true,true,true);
						m_beepHandle = system_beep("Volume Up");
					} break;
					case cmdVolumeDown: {
						show_volume_window();
						mini_adjust_volume(0,0,0,VOL_SOFTER|VOL_CLEAR_MUTE|VOL_MUTE_IF_ZERO);
						stop_sound(m_beepHandle);
//						m_beepHandle = play_sound(&m_beepRef,true,true,true);
						m_beepHandle = system_beep("Volume Down");
					} break;
					case cmdMuteToggle: {
						show_volume_window();
						mini_adjust_volume(0,0,0,0x20);
						stop_sound(m_beepHandle);
//						m_beepHandle = play_sound(&m_beepRef,true,true,true);
						m_beepHandle = system_beep("Mute Toggle");
					} break;
					case cmdJavaScript: {
						BMessenger msngr("application/x-vnd.Web");
						BMessage msg('$KEY');
						msg.AddString("keyname",cmd->exec);
						msngr.SendMessage(&msg);
					} break;
					case cmdExec: {
						if (m_lastExec+EXEC_TIME > system_time()) {
							fprintf(stderr, "cmdExec(%s): not ready yet\n", cmd->exec);
						}
						else {
							char str[256];
							sprintf(str,"%s%s",m_dir,cmd->exec);
							BEntry entry(str, TRUE);
							if (entry.Exists()) {
								entry_ref entryRef;
								entry.GetRef(&entryRef);
								be_roster->Launch(&entryRef);
								m_lastExec = system_time();
							}
						}
					} break;
				};
				cmd = cmd->next;
			};
			break;
		};
	};

	if (handled) return (B_SKIP_MESSAGE);
	return (B_DISPATCH_MESSAGE);
}
