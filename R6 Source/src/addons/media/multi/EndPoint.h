#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <String.h>

class EndPoint;
class BBufferGroup;
class BBuffer;

class _end_pt_map_imp;

class EndPointMap {
	public:
		void * 						operator new(size_t s);
		void 						operator delete(void * p, size_t s);

									EndPointMap(int32 type);
									EndPointMap(); // note this is non-virtual!

		/* EndPoint ID Management */
		void						AllowIDCheckOut();
		void						ForbidIDCheckOut();
		
		status_t					CheckOutID(int32 *id);
		status_t					ReturnID(int32 id);
		
		status_t					AddEndPoint(EndPoint *endPt);
		status_t					EndPointAt(int32 id, EndPoint **endPt, bool remove = false);
		status_t					NextEndPoint(int32 id, EndPoint **endPt);
		
		void						ClearMap();

	private:
		int32						fType;
		bool						fCheckOutAllowed;
		_end_pt_map_imp *			fMap;
};

class EndPoint {
	public:
		enum endpoint_type {
			B_NO_TYPE = 0,
			B_INPUT,	
			B_OUTPUT
		};
		virtual						~EndPoint();
									EndPoint(int32 id, endpoint_type type, const char *name = NULL);
	
	
		// Setters
		status_t					SetOutput(const media_output *output);
		status_t					SetInput(const media_input *input);
		status_t					SetSource(const media_source &source);
		status_t					SetDestination(const media_destination &dest);
		status_t					SetFormat(const media_format *format);
		void						SetDataStatus(int32 dataStatus);
		void						SetOutputEnabled(bool enabled);
		void						SetBufferGroup(BBufferGroup *buffers, bool endPtWillOwn);
		void						SetNextBuffer(BBuffer *buffer);
		void						SetLatencies(const bigtime_t *process, const bigtime_t *downstream);
		
		//	Accessors
		media_input					Input();
		media_output				Output();
		
		const endpoint_type			Type() const;
		const char *				Name() const;
		const int32					ID() const;
		const media_node &			Node() const;
		const media_source &		Source() const;	
		const media_destination &	Destination() const;
		const media_format &		Format() const;
		int32						DataStatus() const;
		bool						OutputEnabled() const;
		bool						BuffersOwned() const;
		BBufferGroup *				BufferGroup() const;
		BBuffer *					NextBuffer() const;
		bigtime_t					ProcessLatency() const;
		bigtime_t					DownstreamLatency() const;
		const EndPointMap &			Spouses() const;
		void						GetLatencies(bigtime_t *process, bigtime_t *downstream);
		
	private:
		int32						fId;
		endpoint_type				fType;
		BString						fName;
		media_node					fNode;
		media_source				fSource;
		media_destination			fDestination;
		media_format				fFormat;
		int32						fDataStatus;
		bool						fOutputEnabled;
		bool						fBuffersOwned;
		BBufferGroup *				fBuffers;
		BBuffer *					fNextBuffer;
		bigtime_t					fProcessLatency;		
		bigtime_t					fDownstreamLatency;
		EndPointMap					fSpouses;
};

#endif //ENDPOINT_H
