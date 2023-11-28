//
// BurnerProject.h
//
//   BurnerProject represents a Burner project file.  It understands how to
//   load and save a project.  The project is saved as an archived BMessage.
//
//  by Nathan Schrenk (nschrenk@be.com)

#ifndef _BURNER_PROJECT_H_
#define _BURNER_PROJECT_H_

#include <Entry.h>
#include <String.h>

class BMessage;
class CDTrack;

class BurnerProject
{
public:
				BurnerProject();
				BurnerProject(entry_ref *ref);
				~BurnerProject();
	
	status_t	InitCheck();
	
	status_t	Load();
	status_t	Save();
	
	status_t	SaveToMessage(BMessage *message);
	status_t	LoadFromMessage(BMessage *message);

	void		SetEntry(entry_ref *entry);
	status_t	GetEntry(entry_ref *entry) const;

	void		SetTrackList(CDTrack *head);
	CDTrack		*TrackList() const;
	
	bool		IsDirty() const;
	void		SetDirty(bool dirty);

	bool		OwnsTracks() const;
	void		SetOwnsTracks(bool ownership);
	
	const char *Name() const;
private:
	BString		fName;
	BEntry		fEntry;
	CDTrack		*fHeadTrack;
	status_t	fInit;
	bool		fDirty;
	bool		fOwnTracks;
};

#endif // _BURNER_PROJECT_H_
