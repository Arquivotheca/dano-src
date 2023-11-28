#ifndef _TWATCHER_H
#define _TWATCHER_H

#include <SupportDefs.h>
#include <Message.h>
#include <Messenger.h>
#include <Handler.h>
#include <Looper.h>
#include <Node.h>
#include <OS.h>
#include <pr_server.h>
#include <NodeWatcher.h>

enum {
	MAX_STATUS_LENGTH =		128,		/* has to be smaller than 256 */
	MAX_NAME_LENGTH =		128			/* has to be smaller than 256 */
};

enum {	
	WATCH_COUNT = 			0x00000001,
	WATCH_ITEMS = 			0x00000002
};

enum {	
	SEND_NAME = 			0x00000001,
	SEND_PATH = 			0x00000002,
	SEND_STATUS = 			0x00000004,
	SEND_MIME = 			0x00000008,
	SEND_PAGE = 			0x00000010,
	SEND_SIZE = 			0x00000020
};

enum {	
	JOB_CREATED = 			20000,
	JOB_DESTROYED = 		20001,
	JOB_MODIFIED = 			20002,
	JOB_COUNT_MODIFIED = 	20100,
	PRINTER_MODIFIED = 		20101,
	FAILED_CHANGED = 		20102
};

enum {
	HAS_STATUS = 			0x0001,
	HAS_MIMETYPE =			0x0002,
	HAS_PAGECOUNT =			0x0004,
	HAS_NAME =				0x0008,
	HAS_EVERYTHING = 		0x000f
};

class TJobInfo
{
public:
					TJobInfo() : errorcode(B_NO_INIT), flags(0) {}					
	bool			IsEnabled() { return ((flags & HAS_EVERYTHING) == HAS_EVERYTHING); }
	bool			IsFailed() const {return (!(strcmp(status, PSRV_JOB_STATUS_FAILED)));};
	class TJobInfo	*next;
	class TJobInfo	*prev;
	char			name[B_FILE_NAME_LENGTH];
	char			path[B_PATH_NAME_LENGTH];
	char			status[MAX_STATUS_LENGTH];
	char			mimeType[B_MIME_TYPE_LENGTH];
	int32			pageCount;
	int32			errorcode;
	float			size;
	node_ref		ref;
	uint32			flags;
};


// Support class
class TWatcher : public NodeWatcher
{
public:
						TWatcher(const char *path, BLooper *looper);
	virtual				~TWatcher();

	virtual	void		SetFlags(uint32 flags);
	virtual void		EntryMoved(entry_ref& src_eref, entry_ref& dst_eref, node_ref& nref);
	virtual void		AttributeChanged(node_ref& nref);

	const char			*Path() const { return fPath; }
	int32				Flags() const { return fFlags; }
	node_ref			Ref() const { return fRef; }

protected:	
	void				PostMessage(TJobInfo *job, uint32 send_mask, uint32 what, const char *path = NULL);
	virtual void		AddJob(TJobInfo *job);
	virtual void		RemoveJob(TJobInfo *job);
	int32				JobCount() const;
	TJobInfo			*FirstJob() const;
	TJobInfo			*GetNextJob(TJobInfo *job) const;
	TJobInfo			*FindJob(node_ref ref) const;	
	bool				HasFailed() const;

private:
	const char			*fPath;
	int32				fFlags;
	TJobInfo			*fFirstJob;
	node_ref			fRef;
};

// Standard implementation
class TStdWatcher : public TWatcher
{
public:
						TStdWatcher(const char *path, BLooper *looper);
	virtual				~TStdWatcher();

	virtual void		EntryCreated(entry_ref& eref, node_ref& nref);
	virtual void		EntryRemoved(node_ref& parent_eref, node_ref& nref);
	virtual void		EntryMoved(entry_ref& src_eref, entry_ref& dst_eref, node_ref& nref);
	virtual void		AttributeChanged(node_ref& nref);
	
protected:	
	virtual void		AddJob(TJobInfo *job);
	virtual void		RemoveJob(TJobInfo *job);

private:
	void				SetMonitor(TJobInfo *job, bool enable);	
	void				CheckJob(TJobInfo *job);
	status_t			ExtractJobInfo(BEntry *entry, TJobInfo *job);
};

#endif
