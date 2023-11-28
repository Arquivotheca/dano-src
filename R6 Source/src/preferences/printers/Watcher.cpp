#include <string.h>
#include "Watcher.h"
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Entry.h>
#include <Node.h>
#include <Directory.h>
#include <Path.h>

#include <stdlib.h>
#include <stdio.h>
#include <fs_attr.h>

// ********************************************************************

TWatcher::TWatcher(const char *path, BLooper *looper)
	: 	NodeWatcher(looper),
		fPath(strdup(path)),
		fFlags(0),
		fFirstJob(NULL)
{	
	BEntry entry(Path());
	entry.GetNodeRef(&fRef);
	StartWatching(fRef, B_WATCH_DIRECTORY | B_WATCH_ATTR);
}

TWatcher::~TWatcher()
{
	// Empty the job list. Set flags to 0 first to avoid useless messages.
	SetFlags(0);
	while (FirstJob())
		RemoveJob(FirstJob());
	StopWatching(fRef);
	free((char *)fPath);
}

void TWatcher::SetFlags(uint32 flags)
{	
	// decide what need to be resent.
	bool send_list = (flags & WATCH_ITEMS) && !(fFlags & WATCH_ITEMS);	// Watch items was just added
	bool send_count = (flags & ~fFlags);
	fFlags = flags;

	// send what is needed
	if (send_count)
		PostMessage(NULL, 0, JOB_COUNT_MODIFIED);

	if (send_list)
	{
		TJobInfo *job = FirstJob();
		while (job)
		{
			if (job->IsEnabled())
				PostMessage(job, 0, JOB_CREATED);
			job = GetNextJob(job);
		}
	}
}

void TWatcher::EntryMoved(entry_ref&, entry_ref&, node_ref& nref)
{
	if (Ref() == nref)
		PostMessage(NULL, 0, PRINTER_MODIFIED);
}

void TWatcher::AttributeChanged(node_ref& nref)
{
	if (Ref() == nref)
		PostMessage(NULL, 0, PRINTER_MODIFIED);
}

	
void TWatcher::PostMessage(TJobInfo *job, uint32 send_mask, uint32 what, const char *path)
{
	BMessage msg(what);
	msg.AddInt32("device", Ref().device);
	msg.AddInt64("node", Ref().node);

	switch (what)
	{
		case JOB_CREATED:
			send_mask = SEND_NAME | SEND_PATH | SEND_STATUS | SEND_MIME | SEND_PAGE | SEND_SIZE;
			break;
		case JOB_DESTROYED:
			send_mask = SEND_PATH;
			break;
		case JOB_MODIFIED:
			msg.AddString("old_path", path);
			break;
		case JOB_COUNT_MODIFIED:
			msg.AddInt32("job_count", JobCount());
			send_mask = 0;
			break;
		case PRINTER_MODIFIED:
			send_mask = 0;
			break;
		case FAILED_CHANGED:
			msg.AddBool("has_failed", HasFailed());
			send_mask = 0;
			break;
	}

	if (send_mask & SEND_STATUS)
	{
		msg.AddString("status", job->status);
		msg.AddInt32("errorcode", job->errorcode);
	}

	if (send_mask & SEND_NAME)		msg.AddString("name", job->name);
	if (send_mask & SEND_PATH)		msg.AddString("path", job->path);
	if (send_mask & SEND_MIME)		msg.AddString("mime", job->mimeType);
	if (send_mask & SEND_PAGE)		msg.AddInt32("page_count", job->pageCount);
	if (send_mask & SEND_SIZE)		msg.AddFloat("size", job->size);

	Looper()->PostMessage(&msg);
}
	
void TWatcher::AddJob(TJobInfo *job)
{
	if (fFirstJob == NULL)
	{
		fFirstJob = job;
		job->next = NULL;
		job->prev = NULL;
	}
	else
	{
		TJobInfo *cur = fFirstJob;
		while (cur->next != NULL)
			cur = cur->next;
		cur->next = job;
		job->prev = cur;
		job->next = NULL;
	}
	
	if (job->IsEnabled())
	{
		if (Flags() & WATCH_COUNT)	PostMessage(NULL, 0, JOB_COUNT_MODIFIED);
		if (Flags() & WATCH_ITEMS)	PostMessage(job, 0, JOB_CREATED);
	}
}

void TWatcher::RemoveJob(TJobInfo *job)
{
	if (job == fFirstJob)
	{
		fFirstJob = job->next;
		if (fFirstJob != NULL)
			fFirstJob->prev = NULL;
	}
	else
	{
		job->prev->next = job->next;
		if (job->next != NULL)
			job->next->prev = job->prev;
	}
	
	if (job->IsEnabled())
	{
		if (Flags() & WATCH_COUNT)	PostMessage(NULL, 0, JOB_COUNT_MODIFIED);
		if (Flags() & WATCH_ITEMS)	PostMessage(job, 0, JOB_DESTROYED);
	}

	delete job;
}

int32 TWatcher::JobCount() const
{
	int32 count = 0;
	TJobInfo *job = fFirstJob;
	while (job)
	{
		if (job->IsEnabled())
			count++;
		job = job->next;
	}
	return count;
}

TJobInfo *TWatcher::FirstJob() const
{
	return fFirstJob;
}

TJobInfo *TWatcher::GetNextJob(TJobInfo *job) const
{
	return job->next;
}

TJobInfo *TWatcher::FindJob(node_ref ref) const
{
	TJobInfo *job = fFirstJob;
	while (job != NULL)
	{
		if ((ref.node == job->ref.node) && (ref.device == job->ref.device))
			return job;
		job = job->next;
	}
	return NULL;
}

bool TWatcher::HasFailed() const
{
	const TJobInfo *job = FirstJob();
	while (job != NULL)
	{
		if (job->IsFailed())
			return true;
		job = job->next;
	}
	return false;
}


//********************************************************************
#pragma mark -

TStdWatcher::TStdWatcher(const char *path, BLooper *looper)
	: TWatcher(path, looper)
{
	BEntry entry(path);
	BDirectory dir(&entry);

	// Go through all the file of the spool folder one first time...
	dir.Rewind();	
	while(dir.GetNextEntry(&entry) == B_OK)
	{
		TJobInfo *new_job = new TJobInfo;
		if (ExtractJobInfo(&entry, new_job) == B_OK)
			AddJob(new_job);
		else
			delete new_job;
	}
	PostMessage(NULL, 0, FAILED_CHANGED, NULL);
}

TStdWatcher::~TStdWatcher()
{
	while (FirstJob())
		RemoveJob(FirstJob());
}

void TStdWatcher::EntryCreated(entry_ref& eref, node_ref&)
{
	BEntry entry(&eref);
	TJobInfo *job = new TJobInfo;
	if (ExtractJobInfo(&entry, job) != B_OK)
	{
		delete job;
		return;
	}
	AddJob(job);
	PostMessage(NULL, 0, FAILED_CHANGED, NULL);
}

void TStdWatcher::EntryRemoved(node_ref&, node_ref& nref)
{
	TJobInfo *job = FindJob(nref);
	if (!job)
		return;
	RemoveJob(job);
	PostMessage(NULL, 0, FAILED_CHANGED, NULL);
}

void TStdWatcher::EntryMoved(entry_ref& src_eref, entry_ref& dst_eref, node_ref& nref)
{
	TWatcher::EntryMoved(src_eref, dst_eref, nref);
	TJobInfo *job;
	if ((job = FindJob(nref)) != NULL)
		CheckJob(job);
}

void TStdWatcher::AttributeChanged(node_ref& nref)
{
	TWatcher::AttributeChanged(nref);
	TJobInfo *job;
	if ((job = FindJob(nref)) != NULL)
		CheckJob(job);
}

void TStdWatcher::CheckJob(TJobInfo *job)
{
	BEntry entry(job->path);
	TJobInfo job2;
	if (ExtractJobInfo(&entry, &job2) == B_OK)
	{
		// Did we just enabled the job ?
		bool was_enabled = job->IsEnabled();
		bool is_enabled = job2.IsEnabled();
		bool failed_changed = (was_enabled && is_enabled) && (job->IsFailed() != job2.IsFailed());
		int32 flags = 0;

		if (strcmp(job->name, job2.name) != 0)
		{
			strcpy(job->name, job2.name);
			flags |= SEND_NAME;
		}

		if (strcmp(job->path, job2.path) != 0)
		{
			strcpy(job->path, job2.path); // added (?)
			flags |= SEND_PATH;
		}
		
		if (strcmp(job->status, job2.status) != 0)
		{
			strcpy(job->status, job2.status);
			flags |= SEND_STATUS;
		}

		if (job->errorcode != job2.errorcode)
		{
			job->errorcode = job2.errorcode;
			flags |= SEND_STATUS;
		}

		if (strcmp(job->mimeType, job2.mimeType) != 0)
		{
			strcpy(job->mimeType, job2.mimeType);
			flags |= SEND_MIME;
		}

		if (job->pageCount != job2.pageCount)
		{
			job->pageCount = job2.pageCount;
			flags |= SEND_PAGE;
		}

		if (job->size != job2.size)
		{
			job->size = job2.size;
			flags |= SEND_SIZE;
		}

		job->flags = job2.flags;

		if (is_enabled) 
		{
			if (!was_enabled)
			{
				if (Flags() & WATCH_COUNT)	PostMessage(NULL, 0, JOB_COUNT_MODIFIED);
				if (Flags() & WATCH_ITEMS)	PostMessage(&job2, 0, JOB_CREATED);
			}
			else if ((Flags() & WATCH_ITEMS) && (flags != 0))
				PostMessage(&job2, flags, JOB_MODIFIED, job->path);
			
			if (failed_changed)
				PostMessage(NULL, 0, FAILED_CHANGED, NULL);
		}
		if (flags & SEND_PATH)
			strcpy(job->path, job2.path);
	}
}

void TStdWatcher::AddJob(TJobInfo *job)
{
	TWatcher::AddJob(job);
	SetMonitor(job, true);
}

void TStdWatcher::RemoveJob(TJobInfo *job)
{
	SetMonitor(job, false);
	TWatcher::RemoveJob(job);
}

void TStdWatcher::SetMonitor(TJobInfo *job, bool enable)
{
	if (enable)
	{
		StartWatching(job->ref,  B_WATCH_ATTR);
		BMessage msg(B_NODE_MONITOR);
		msg.AddInt32("opcode", B_ATTR_CHANGED);
		msg.AddInt64("node", job->ref.node);
		msg.AddInt32("device", job->ref.device);
		Looper()->PostMessage(&msg);
	}
	else
	{
		StopWatching(job->ref);
	}
}

status_t TStdWatcher::ExtractJobInfo(BEntry *entry, TJobInfo *job)
{
	char name[256], status[256], mime[B_MIME_TYPE_LENGTH+1];
	BPath path;
	off_t size;
	int32 page_count;
	int32 errorcode;

	// Get the path	
	entry->GetPath(&path);
	strcpy(job->path, path.Path());		
	
	// Get the file size
	BFile file(entry, 0);
	if (file.InitCheck() != B_OK)
		return B_ERROR;
		
	// Get the node ref
	entry->GetNodeRef(&job->ref);
	BNode node(entry);
	
	// Get the file size
	file.GetSize(&size);
	job->size = size/1024.0f;

	// Get the page count
	if (node.ReadAttr(PSRV_SPOOL_ATTR_PAGECOUNT, B_INT32_TYPE, 0, &page_count, 4) == 4) {
		job->pageCount = page_count;
		job->flags |= HAS_PAGECOUNT;
	}
		
	// Get the name
	if (node.ReadAttr(PSRV_SPOOL_ATTR_DESCRIPTION, B_STRING_TYPE, 0, name, 255) >= 1) {
		strcpy(job->name, name);	
		job->flags |= HAS_NAME;
	}

	// Get the status
	if (node.ReadAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, status, 255) >= 1) {
		strcpy(job->status, status);
		job->flags |= HAS_STATUS;
	}

	// Get the errorcode
	if (node.ReadAttr(PSRV_SPOOL_ATTR_ERRCODE, B_SSIZE_T_TYPE, 0, &errorcode, sizeof(int32)) == sizeof(int32)) {
		job->errorcode = errorcode;
	}
		
	// Get the mime_type
	if (node.ReadAttr(PSRV_SPOOL_ATTR_MIMETYPE, B_STRING_TYPE, 0, mime, B_MIME_TYPE_LENGTH+1) >= 1) {
		strcpy(job->mimeType, mime);
		job->flags |= HAS_MIMETYPE;
	}
	
	return B_OK;
}
