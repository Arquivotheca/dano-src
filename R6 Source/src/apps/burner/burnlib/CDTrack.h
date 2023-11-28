
#ifndef CDTRACK_H
#define CDTRACK_H

#include <OS.h>

#include <algorithm>

class CDDataSource;
class CDCueSheet;

class CDTrack
{
public:
	CDTrack(CDDataSource *data);
	~CDTrack();

	// CDTrack takes ownership of the datasource and deletes it when the track
	// is deleted, or when a new datasource is set for the track
	void SetDataSource(CDDataSource *data);
	CDDataSource *DataSource();

	// called when the datasource changes length so that internal variables
	// can be updated appropriately
	void SyncWithDataSource();
	
	void SetPreGap(uint32 frames, uint32 deadblocks);
	void SetPreGap(uint32 frames);
	uint32 PreGap(void);
	void PreGap(uint32 *dataFrames, uint32 *emptyFrames);
	
	uint32 Length(void);    // total length in frames
	uint32 FrameSize(void); // size of a single frame
	void FrameData(off_t frame, size_t count, void *data);

	int32 LBA(void); // Logical Block Address of the start of Frame
	void MSF(uint32 &min, uint32 &sec, uint32 &frm); // Minute:Second:Frame Address

	CDTrack *Next(void) { return next; }
	
	// static function to find the track previous to 'track' in the list with
	// 'head' at its head.  If audio_only is true, the data tracks in the list
	// will be passed over.
	static CDTrack *FindPrevious(CDTrack *head, CDTrack *track, bool audio_only);
	
	bool IsData(void);
	
	void SetNext(CDTrack *t) { next = t; }
	void SetLBA(int32 LBA) { lba = LBA; }

	void SetIndex(uint16 i);
	uint16 Index() const;	

//	bool operator<(const CDTrack &t) const { return index < t.Index(); }
private:
	CDTrack *next;
	CDDataSource *data;
	
	enum { cd_data, cd_audio } type;

//  |- deadblocks -|-- blockcount                                   --|
//  |- Length()                                                     --|
//  |- PreGap() ---| 

	uint32 pre_gap;       /* pre-gap in frames. */
	uint32 deadblocks;    /* number of 0 frames at the head of the track */
	uint32 blockcount;    /* number of data frames in the track */
	uint32 blocksize;
	size_t datalength;
	
	int32 lba;
	uint16 index;
};

inline void CDTrack::SetIndex(uint16 i) { index = i; }
inline uint16 CDTrack::Index() const { return index; }	

// CDTrack comparator object for use with STL
struct ltTrack 
{
	bool operator()(const CDTrack *t1, const CDTrack *t2) const
	{
		return t1->Index() < t2->Index();
	}
};


#endif // CDTRACK_H

