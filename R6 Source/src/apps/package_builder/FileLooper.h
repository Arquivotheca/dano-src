#ifndef _FILELOOPER_H
#define _FILELOOPER_H


// #include "PackFile.h"

class PackWindow;
class StatusWindow;
class PackArc;

class FileLooper : public BLooper {
private:
	friend class PackArc;
	friend class PackWindow;
		
	PackArc			*packArc;
	StatusWindow	*statWindow;
	PackWindow 		*wind;
public:
	FileLooper();
	~FileLooper();
	virtual thread_id Run();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *msg);
	
	void DoAddItems(BMessage *msg);
	void AddPatchItem(BMessage *msg);
	void DoRemoveItems(BMessage *msg, bool doCatalog = TRUE);
	void DoWriteCatalog(BMessage *msg);
	void DoExtractItems(BMessage *msg);
	void DoSave(BMessage *msg);
	
	bool doCancel;
	
	// the document window

};

#define STATUS_FREQUENCY 50.0
#endif
