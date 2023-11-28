//
// BurnerProject.cpp
// 
//  by Nathan Schrenk (nschrenk@be.com)
#include <Message.h>
#include <Entry.h>
#include <File.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Alert.h>
#include <stdio.h>
#include "Burner.h"
#include "BurnerProject.h"
#include "CDTrack.h"
#include "CDDataSource.h"

const uint32 kBurnerProjectArchiveMessage	= 'bPRJ';
const uint32 kBurnerTrackArchiveMessage		= 'bTRK';

const uchar kBurnerProjectMagic[]			= "BURN\000\000\000\001";
const size_t kMagicLength 					= 8;


BurnerProject::BurnerProject()
{
	fHeadTrack = NULL;
	fOwnTracks = true;
	fDirty = false;
	fName.SetTo("Untitled");
	fInit = B_OK;
}

BurnerProject::BurnerProject(entry_ref *ref)
{
	fHeadTrack = NULL;
	fOwnTracks = true;
	fDirty = false;
	SetEntry(ref);
	Load();
}

BurnerProject::~BurnerProject()
{
	if (fOwnTracks) {
		CDTrack *track;
		while (fHeadTrack != NULL) {
			track = fHeadTrack;	
			fHeadTrack = fHeadTrack->Next();
			delete track;
		}
	}
}

status_t BurnerProject::InitCheck()
{
	return fInit;
}


status_t BurnerProject::Load()
{
	status_t ret;
	entry_ref ref;
	ret = fEntry.GetRef(&ref);
	uchar buf[kMagicLength];
	if (ret == B_OK) {
		BFile file(&ref, B_READ_ONLY);
		ret = file.InitCheck();
		if (ret == B_OK) {
			file.Read(buf, kMagicLength);
			if (!memcmp(buf, kBurnerProjectMagic, kMagicLength)) {
				BMessage message;
				if ((message.Unflatten(&file) == B_OK) &&
					(message.what == kBurnerProjectArchiveMessage))
				{
					ret = LoadFromMessage(&message);
				}
			} else {
				ret = B_ERROR;
			}
		}
	}
	
	fInit = ret;
	return ret;
}

status_t BurnerProject::Save()
{
	status_t ret;
	BMessage message(kBurnerProjectArchiveMessage);
	ret = SaveToMessage(&message);
	if (ret == B_OK) {
		entry_ref ref;
		ret = fEntry.GetRef(&ref);
		if (ret == B_OK) {
			BFile file(&ref, B_READ_WRITE | B_CREATE_FILE);
			ret = file.InitCheck();
			// write magic identifier
			file.Write((const void *)kBurnerProjectMagic, kMagicLength);
			if (ret == B_OK) {
				message.Flatten(&file);
			}
			// set file to proper MIME type
			file.Unset();
			BNode node(&ref);
			BNodeInfo nodeInfo(&node);
			nodeInfo.SetType(BURNER_PROJECT_MIME_TYPE);
		}
	}

	fDirty = false;
	return ret;
}

status_t BurnerProject::SaveToMessage(BMessage *message)
{
	if (fHeadTrack == NULL) {
		return B_ERROR;
	}
	
	message->what = kBurnerProjectArchiveMessage;
	
	CDTrack *track;
	BMessage trackMsg;
	BMessage datasourceMsg;
	uint32 dataPregap, emptyPregap;
	
	for (track = fHeadTrack; track != NULL; track = track->Next()) {
		track->PreGap(&dataPregap, &emptyPregap);
		// Here's the scoop on how the path is stored...
		// If the track in question is in or under the projects directory,
		// then the path is stored in a relative format. Otherwise the path
		// is absolute. The grunt work is done in MediaFileDataSource::Archive()
		BPath path;
		BEntry parent;
		fEntry.GetParent(&parent);
		parent.GetPath(&path);
		
		datasourceMsg.AddString("project_path", path.Path());
		track->DataSource()->Archive(&datasourceMsg);

		trackMsg.AddInt32("pregap_data", dataPregap);
		trackMsg.AddInt32("pregap_empty", emptyPregap);
		trackMsg.AddMessage("datasource", &datasourceMsg);

		message->AddMessage("track", &trackMsg);
		trackMsg.MakeEmpty();
		datasourceMsg.MakeEmpty();
	}
	
	return B_OK;
}

status_t BurnerProject::LoadFromMessage(BMessage *message)
{
	if (message->what != kBurnerProjectArchiveMessage) {
		return B_ERROR;
	}
	
	status_t ret;
	int32 count;
	type_code code;
	ret = message->GetInfo("track", &code, &count);
	if (ret == B_OK) {
		CDTrack *track, *prev(NULL);
		CDDataSource *src;
		BMessage trackMsg;
		BMessage datasourceMsg;
		uint32 data, empty;
		
		BPath parentPath;
		BEntry parent;
		fEntry.GetParent(&parent);
		parent.GetPath(&parentPath);

		for (int32 i = 0; i < count; i++) {
			trackMsg.MakeEmpty();
			datasourceMsg.MakeEmpty();

			if (message->FindMessage("track", i, &trackMsg) != B_OK) {
				ret = B_ERROR;
				break;
			}
			if (trackMsg.FindMessage("datasource", &datasourceMsg) != B_OK) {
				ret = B_ERROR;
				break;
			}
			// Make sure the project_path is up to date...
			datasourceMsg.ReplaceString("project_path", parentPath.Path());

			src = dynamic_cast<CDDataSource *>(instantiate_object(&datasourceMsg));
			if (!src || (src->InitCheck() != B_OK)) {
				ret = B_ERROR;
				delete src;
				// we couldn't load this track, but continue to try and load the next
				BMessage sub;
				datasourceMsg.FindMessage("subdatasource",&sub);
				const char *file;
				sub.FindString("path", &file);
				BString error;
				error << "Error loading track: " << file;
				BAlert *alert = new BAlert("alert", error.String(), "Ok");
				alert->Go();
				continue;
			}
			track = new CDTrack(src);
			trackMsg.FindInt32("pregap_data", (int32 *)&data);
			trackMsg.FindInt32("pregap_empty", (int32 *)&empty);
			track->SetPreGap(data + empty, empty);
			track->SetIndex(i + 1);

			if (prev != NULL) {
				prev->SetNext(track);
				prev = track;
			} else {
				fHeadTrack = prev = track;
			}
		}
	}
	
	fDirty = false;
	return ret;
}

void BurnerProject::SetEntry(entry_ref *entry)
{
	fInit = fEntry.SetTo(entry, true);
	BPath path;
	fEntry.GetPath(&path);
	fName.SetTo(path.Leaf());
	fDirty = true;
}

status_t BurnerProject::GetEntry(entry_ref *entry) const
{
	return fEntry.GetRef(entry);
}

void BurnerProject::SetTrackList(CDTrack *head)
{
	fHeadTrack = head;
}

CDTrack *BurnerProject::TrackList() const
{
	return fHeadTrack;
}

bool BurnerProject::IsDirty() const
{
	return fDirty;
}

void BurnerProject::SetDirty(bool dirty)
{
	fDirty = dirty;
}
bool BurnerProject::OwnsTracks() const
{
	return fOwnTracks;
}

void BurnerProject::SetOwnsTracks(bool ownership)
{
	fOwnTracks = ownership;
}


const char *BurnerProject::Name() const
{
	return fName.String();
}


