//
// TrackListView.h
//
//  by Nathan Schrenk (nschrenk@be.com)

#ifndef _TRACK_LIST_VIEW_H_
#define _TRACK_LIST_VIEW_H_

#include <controls/ColumnListView.h>
#include <controls/ColumnTypes.h>
#include <View.h>
#include <map>
#include "CDTrack.h"


class TrackRow : public BRow
{
public:
				TrackRow(CDTrack *track);
	virtual		~TrackRow();
	
	CDTrack		*Track() const;
	void		SyncWithTrack();

	void		SetDeleteTrack(bool);
	bool		WillDeleteTrack() const;
	
private:
	CDTrack		*fTrack;
	bool		fDeleteTrack;
};

class TrackLengthField : public BSizeField
{
public:
			TrackLengthField(uint32 size, bool isData)
				: BSizeField(size)
			{
				fIsData = isData; 
			}
	bool	fIsData;	
};

class TrackLengthColumn : public BSizeColumn
{
public:
				TrackLengthColumn(	const char *title, float width,
								float minWidth, float maxWidth);
	virtual void	DrawField(BField *field, BRect rect, BView *parent);
};

class TrackListStatusView;

class TrackListView : public BColumnListView
{
public:
				TrackListView(BRect frame, uint32 resizeMode = B_FOLLOW_ALL);
	virtual		~TrackListView();

	void		AddTrack(CDTrack *track, int16 index = -1); // adds a CDTrack to the list
	status_t	RemoveTrack(CDTrack *track);	// removes a CDTrack from the list
	void		RemoveAllTracks();				// empties the track list
	CDTrack		*GetTrackList();				// returns a linked list of CDTrack objects
	TrackRow	*RowForTrack(CDTrack *track);
	void		TrackUpdated(CDTrack *track);	// forces an update of the track's row
	
	void		RemoveSelectedTracks();

	void		SetEditEnabled(bool enabled);
	bool		EditEnabled();
	
protected:
	virtual void MessageReceived(BMessage *msg);
	virtual void SelectionChanged();
	virtual void KeyDown(const char *bytes, int32 numBytes);
	virtual bool InitiateDrag(BPoint, bool wasSelected);
	virtual void MessageDropped(BMessage*, BPoint point);
	virtual void ItemInvoked();
	
private:
	void NumberTracks();			// assigns the appropriate track number to each
									// track in the map
	map<CDTrack *, TrackRow *, ltTrack> tracks;
	TrackListStatusView		*fStatusView;
	bool					fEditable;
};

#endif // _TRACK_LIST_VIEW_H_
