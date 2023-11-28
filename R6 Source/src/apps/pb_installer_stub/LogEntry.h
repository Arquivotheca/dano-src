#ifndef _LOGENTRY_H_
#define _LOGENTRY_H_

#include <ListItem.h>
#include <View.h>
#include <Message.h>
#include <Rect.h>

enum {
	Log_Valet 				=0x00000001,
	Log_Jaegar 				=0x00000002,
	Log_Trolling 			=0x00000004,
	Log_Installations		=0x00000008,
	Log_Uninstallations		=0x00000010,
	Log_Registrations		=0x00000020,
	Log_Downloads			=0x00000040,
	Log_Updates				=0x00000080,
	Log_BeDepot				=0x00000100
};

class LogEntry : public BListItem{
	public:
		LogEntry();
		~LogEntry(void);
		virtual void DrawItem(BView *v, BRect bounds, bool complete = 0);
		void SetEntry(BMessage *data);
		char *actiontostr(int32 action, char *pkg);
		bool DisplayOrNot(int32 action);
	private:
friend class LogListView;
		BMessage *entry;
};

#endif
