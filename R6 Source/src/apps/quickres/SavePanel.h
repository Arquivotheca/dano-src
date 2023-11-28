#ifndef QUICKRES_SAVE_PANEL_H
#define QUICKRES_SAVE_PANEL_H

#include <FilePanel.h>

class BMenu;

class SavePanel : public BFilePanel
{
public:
	struct format_def {
		const char* name;
		const char* signature;
		int32 id;
	};
	
	SavePanel(const format_def* formats = 0,
			  BMessenger *target = 0,
			  const entry_ref *start_directory = 0,
			  uint32 node_flavors = 0,
			  bool allow_multiple_selection = true,
			  BMessage *message = 0, 
			  BRefFilter * = 0,
			  bool modal = false,
			  bool hide_when_done = true);

	virtual ~SavePanel();

	virtual	void SendMessage(const BMessenger*, BMessage*);

	void SetFormatName(const char* name);
	void SetFormatSignature(const char* signature);
	void SetFormatID(int32 id);
	void SetFormatIndex(int32 idx);
	
	BMessage* CurrentFormat() const;
	
private:
	typedef BFilePanel inherited;
	
	void SetFormat(const char* name, const char* signature, int32 id, int32 idx);
	
	BMenu* fFormatMenu;
};

#endif
