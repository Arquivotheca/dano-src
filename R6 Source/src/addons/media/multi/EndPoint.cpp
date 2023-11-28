#include "EndPoint.h"

#include <BufferGroup.h>
#include <Debug.h>

EndPoint::EndPoint(int32 id, endpoint_type type, const char *name) :
	fId(id),
	fType(B_NO_TYPE),
	fName(name),
	fNode(media_node::null),
	fSource(media_source::null),
	fDestination(media_destination::null),
	fDataStatus(B_DATA_NOT_AVAILABLE),
	fOutputEnabled(false),
	fBuffersOwned(false),
	fBuffers(NULL),
	fProcessLatency(0LL),
	fDownstreamLatency(0LL),
	fSpouses(type)
{
	if (fId < 0)
		fId = -1;
	else fType = type;	
	fFormat.type = B_MEDIA_UNKNOWN_TYPE;
	fName.Truncate(B_MEDIA_NAME_LENGTH);
	fSpouses.ForbidIDCheckOut();
}


EndPoint::~EndPoint()
{
	fSpouses.ClearMap();
}


status_t 
EndPoint::SetOutput(const media_output *output)
{
	if (!output)
		return B_BAD_VALUE;
	if (fType != B_OUTPUT)
		return B_ERROR;

	if (output->node == media_node::null ||
		output->source == media_source::null)
		return B_BAD_VALUE;

	/* check the source for internal consistency */
	if (output->source.port != output->node.port)
		return B_BAD_VALUE;
	
	/* check to see if the ids match */	
	if (output->source.id != fId)
		return B_BAD_VALUE;
		
	fNode = output->node;
	fSource = output->source;
	fDestination = output->destination;
	fFormat = output->format;
	fName = output->name;
	
	return B_OK;
}

status_t 
EndPoint::SetInput(const media_input *input)
{
	if (!input)
		return B_BAD_VALUE;
	if (fType != B_INPUT)
		return B_ERROR;

	if (input->node == media_node::null ||
		input->destination == media_destination::null)
		return B_BAD_VALUE;

	/* check to see if the ids match */	
	if (input->destination.id != fId)
		return B_BAD_VALUE;
		
	fNode = input->node;
	fSource = input->source;
	fDestination = input->destination;
	fFormat = input->format;
	fName = input->name;
	
	return B_OK;
}

status_t 
EndPoint::SetSource(const media_source &source)
{
	if (fType == B_OUTPUT)
	{
		if (source.id != fId && source != media_source::null)
			return B_MEDIA_BAD_SOURCE;
	}
	fSource = source;
	return B_OK;
}

status_t 
EndPoint::SetDestination(const media_destination &dest)
{
	if (fType == B_INPUT)
	{
		if (dest.id != fId && dest != media_destination::null)
			return B_MEDIA_BAD_DESTINATION;
	}
	fDestination = dest;
	return B_OK;
}

status_t 
EndPoint::SetFormat(const media_format *format)
{
	if (!format)
		return B_BAD_VALUE;
		
	fFormat = *format;
	return B_OK;
}

void 
EndPoint::SetDataStatus(const int32 dataStatus)
{
	fDataStatus = dataStatus;
}

void 
EndPoint::SetOutputEnabled(bool enabled)
{
	fOutputEnabled = enabled;
}

void 
EndPoint::SetBufferGroup(BBufferGroup *buffers, bool endPtWillOwn)
{
	if (fBuffers && fBuffersOwned)
	{
		fBuffers->ReclaimAllBuffers();
	}

	delete fBuffers;
	fBuffers = NULL;
	fBuffersOwned = false;

	fBuffers = buffers;
	fBuffersOwned = endPtWillOwn;
}

void 
EndPoint::SetNextBuffer(BBuffer *buffer)
{
	fNextBuffer = buffer;
}

void 
EndPoint::SetLatencies(const bigtime_t *process, const bigtime_t *downstream)
{
	if (!process && !downstream)
		return;
		
	if (process)
		fProcessLatency = *process;
	if (downstream)
		fDownstreamLatency = *downstream;
}

media_input 
EndPoint::Input()
{
	media_input in;
	in.node = fNode;
	in.source = fSource;
	in.destination = fDestination;
	in.format = fFormat;
	strcpy(in.name, fName.String());
	return in;
}

media_output 
EndPoint::Output()
{
	media_output out;
	out.node = fNode;
	out.source = fSource;
	out.destination = fDestination;
	out.format = fFormat;
	strcpy(out.name, fName.String());
	return out;
}

const EndPoint::endpoint_type 
EndPoint::Type() const
{
	return fType;
}

const char *
EndPoint::Name() const
{
	return fName.String();
}

const int32
EndPoint::ID() const
{
	return fId;
}

const media_node &
EndPoint::Node() const
{
	return fNode;
}

const media_source &
EndPoint::Source() const
{
	return fSource;
}

const media_destination &
EndPoint::Destination() const
{
	return fDestination;
}

const media_format &
EndPoint::Format() const
{
	return fFormat;
}

int32 
EndPoint::DataStatus() const
{
	return fDataStatus;
}

bool 
EndPoint::OutputEnabled() const
{
	return fOutputEnabled;
}

bool 
EndPoint::BuffersOwned() const
{
	return fBuffersOwned;
}

BBufferGroup *
EndPoint::BufferGroup() const
{
	return fBuffers;
}

BBuffer *
EndPoint::NextBuffer() const
{
	return fNextBuffer;
}

bigtime_t 
EndPoint::ProcessLatency() const
{
	return fProcessLatency;
}

bigtime_t 
EndPoint::DownstreamLatency() const
{
	return fDownstreamLatency;
}

void 
EndPoint::GetLatencies(bigtime_t *process, bigtime_t *downstream)
{
	if (process)
		*process = fProcessLatency;
	if (downstream)
		*downstream = fDownstreamLatency;
}

const EndPointMap &
EndPoint::Spouses() const
{
	return fSpouses;
}


#pragma mark -

#include <map>
#include <rt_allocator.h>

#define LOCKINFO PRINT

using namespace std;

typedef map<int32, EndPoint *, less<int32>, rt_allocator<pair<const int32, EndPoint*> > >
	endpt_map_type;

class _end_pt_map_imp {
	public:
		endpt_map_type	fEndPoints;
		mutable int32 	fLock;
		sem_id			fSem;
		
		_end_pt_map_imp() {
			fLock = 1;
			fSem = create_sem(0, "_end_pt_map_imp");
		}

		~_end_pt_map_imp() {
			delete_sem(fSem);
		}
		
		bool lock() const {
			//LOCKINFO(("_event_queue_imp.lock()\n"));
			if (atomic_add(&fLock, -1) < 1)
				if (acquire_sem(fSem) < B_OK)
					return false;
			return true;
		}
		
		void unlock() const {
			//LOCKINFO(("_event_queue_imp.unlock()\n"));
			if (atomic_add(&fLock, 1) < 0)
				release_sem(fSem);
		}
};

class _lock {
	private:		
		const _end_pt_map_imp *map;
		bool ok;
	public:
		_lock(const _end_pt_map_imp  *m) : map(m) { ok = map->lock(); }
		~_lock() { if (ok) map->unlock(); }
		bool operator!() { return !ok; }
};



void *
EndPointMap::operator new(size_t s)
{
	return rtm_alloc(NULL, s);
}

void 
EndPointMap::operator delete(void *p, size_t s)
{
	rtm_free(p);
}


EndPointMap::EndPointMap(int32 type) :
	fType(type),
	fCheckOutAllowed(false),
	fMap(new _end_pt_map_imp)
{
}


EndPointMap::EndPointMap()
{
	ClearMap();
	delete fMap;
}

void 
EndPointMap::AllowIDCheckOut()
{
	_lock l(fMap);
	fCheckOutAllowed = true;
}

void 
EndPointMap::ForbidIDCheckOut()
{
	_lock l(fMap);
	fCheckOutAllowed = true;
}

status_t 
EndPointMap::CheckOutID(int32 *id)
{
	_lock l(fMap);
	if (!fCheckOutAllowed)
		return B_NOT_ALLOWED;

	if (!id)
		return B_BAD_VALUE;
	
	/* start with next equal to the first entry and id = 0 */
	int32 theId = 0;
	endpt_map_type::iterator next = fMap->fEndPoints.begin();
	
	/* while each successive id is only 1 more than the current id */
	/* update the current id and increment */
	while ((*next).first - theId <= 1 )
	{
		theId = (*next).first;
		if (next != fMap->fEndPoints.end())
			next++;
		else break;
	}

	/* id points to the item before our first free spot */
	theId++;
	/* reserve that spot in the list! */
	fMap->fEndPoints[theId];
	*id = theId;
	return B_OK;
}

status_t 
EndPointMap::ReturnID(int32 id)
{
	_lock l(fMap);
	if (!fCheckOutAllowed)
		return B_NOT_ALLOWED;
		
	endpt_map_type::iterator found = fMap->fEndPoints.find(id);
	if (found == fMap->fEndPoints.end())
		return B_OK;
		
	if ((*found).second == NULL) {
		fMap->fEndPoints.erase(id);
		return B_OK;
	}

	return B_BAD_INDEX;
}

status_t 
EndPointMap::AddEndPoint(EndPoint *endPt)
{
	_lock l(fMap);
	/* check to see if the item is of the appropriate type */
	if (endPt->Type() != fType)
		return B_BAD_VALUE;
		
	endpt_map_type::iterator found = fMap->fEndPoints.find(endPt->ID());
	
	/* if we find an occupied slot */	
	if (found != fMap->fEndPoints.end() && (*found).second != NULL)
		return B_BAD_INDEX;
	
	/* insert the item */
	fMap->fEndPoints[endPt->ID()] = endPt;
	return B_OK;
}

status_t 
EndPointMap::EndPointAt(int32 id, EndPoint **endPt, bool remove)
{
	_lock l(fMap);
	endpt_map_type::iterator found = fMap->fEndPoints.find(id);
	/* if the slot does not contain a valid end point */
	if (found == fMap->fEndPoints.end() || (*found).second == NULL) {
		if (endPt) *endPt = NULL;
		return B_BAD_INDEX;
	}
	
	/* there is an end point at that id */
	if (endPt)
		*endPt = (*found).second;
	if (remove)
		fMap->fEndPoints.erase(id);
	return B_OK;
}

status_t 
EndPointMap::NextEndPoint(int32 id, EndPoint **endPt)
{
	_lock l(fMap);

	
	endpt_map_type::iterator found = fMap->fEndPoints.upper_bound(id);
	while (found != fMap->fEndPoints.end() && (*found).second == NULL)
		found++;
	
	if (found == fMap->fEndPoints.end())
		return B_BAD_INDEX;
	
	if (endPt)
		*endPt = (*found).second;
	return B_OK;
}

void 
EndPointMap::ClearMap()
{
	_lock l(fMap);
	fMap->fEndPoints.clear();
}
