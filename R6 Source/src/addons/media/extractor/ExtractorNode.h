/* QTReaderNode.h by Simon Clarke */

#ifndef __EXTRACTOR_NODE_H
#define __EXTRACTOR_NODE_H

#include <stdlib.h>
#include <string.h>
#include <Entry.h>
#include <File.h>
#include <BufferProducer.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <MediaNode.h>
#include <TimedEventQueue.h>
#include <FileInterface.h>
#include <malloc.h>
#include "Extractor.h"

using namespace BPrivate;

struct ExtractorTrack {
		media_format		format;		
		BBufferGroup		*bufferGroup;
		BBuffer *			pendingBuffer;
		size_t				bufferSize;
		off_t				pos;
		off_t				pos_offs;
		off_t				pos_chunk;
		void *				extractor_cookie;


		media_destination	destination;
		media_source		source;
		bool				enabled,connected;
		bigtime_t			downstreamLatency;
		bigtime_t			duration;
		
		int64				frame;
		int64				frameCount;
};

struct chunk {
	BBufferGroup *	bufferGroup;
	area_id			area;
	int32			offset;
	int32			totalSize;
	uint8 *			ptr;

	int32			size;
	int64			filePos;
	bigtime_t		latestBuffer;
	int32			refCount;
	bool			wasOverlap;
	bool			seekedSinceSent;

	BBuffer * 		GrabBuffer() {
		BBuffer *buf = NULL;
		while (!buf) {
			buf = bufferGroup->RequestBuffer((size_t)0,0);
			if (!buf) {
				buffer_clone_info info;
				info.buffer = -1;
				info.area = area;
				info.offset = offset;
				info.size = totalSize;
				info.flags = 0;
				bufferGroup->AddBuffer(info);
			};
		};
		return buf;
	};
	
	void			WaitForChunk() {
		bufferGroup->WaitForBuffers();
	};
};

class ExtractorNode : public BBufferProducer, public BFileInterface {
public:

		ExtractorNode(BMediaAddOn *myAddOn, char *name);
		~ExtractorNode();

protected:

		/* MediaNode calls */
		
		virtual port_id ControlPort() const;
		virtual BMediaAddOn *AddOn(int32 *internal_id) const;

		virtual void Start(			bigtime_t 				performance_time);
		
		virtual void Stop(			bigtime_t 				performance_time,
									bool 					immediate);

		virtual	void Seek(			bigtime_t 				media_time,
									bigtime_t 				performance_time);

		virtual	status_t AddTimer(
									bigtime_t 				at_time,
									int32					cookie);
		
		virtual	void SetRunMode(	run_mode 				mode);
		
		virtual	void TimeWarp(		bigtime_t 				at_real_time,
									bigtime_t 				to_performance_time);
		
		virtual	void Preroll();
		
		virtual	status_t HandleMessage(
									int32 					message,
									const void 				*data,
									size_t 					size);
	
		/* FileInterface calls */

		virtual	status_t GetNextFileFormat(
									int32 					*cookie,
									media_file_format 		*out_format);
		
		virtual	void DisposeFileFormatCookie(
									int32 					cookie);
		
		virtual	status_t GetDuration(
									bigtime_t 				*out_time);
		
		virtual	status_t SniffRef(	const entry_ref 		&file,
									char 					*out_mime_type,
									float 					*out_quality);
		
		virtual	status_t SetRef(	const entry_ref 		&file,
									bool 					create,
									bigtime_t 				*out_time);

		virtual	status_t GetRef(	entry_ref 				*out_ref,
									char 					*out_mime_type);


		/* BufferProducer calls */

		virtual	status_t FormatSuggestionRequested(
									media_type				type,
									int32 					quality,
									media_format 			*format);

		virtual	status_t FormatProposal(
									const media_source 		&output,
									media_format 			*format);

		virtual	status_t FormatChangeRequested(
									const media_source 		&source,
									const media_destination &destination,
									media_format 			*io_format,
									int32 					*out_change_count);

		virtual	status_t GetNextOutput(
									int32 					*cookie,
									media_output 			*out_output);

		virtual	status_t DisposeOutputCookie(
									int32 					cookie);

		virtual	status_t SetBufferGroup(
									const media_source 		&for_source,
									BBufferGroup 			*group);

		virtual	status_t VideoClippingChanged(
									const media_source 		&for_source,
									int16 					num_shorts,
									int16 					*clip_data,
									const media_video_display_info &display,
									int32 					*out_from_change_count);

		virtual	status_t PrepareToConnect(
									const media_source 		&what,
									const media_destination &where,
									media_format 			*format,
									media_source 			*out_source,
									char 					*io_name);
		
		virtual	void Connect(		status_t 				error, 
									const media_source 		&source,
									const media_destination &destination,
									const media_format 		&format,
									char 					*out_name);

		virtual	void Disconnect(	const media_source 		&what,
									const media_destination &where);

		virtual	void LateNoticeReceived(
									const media_source 		&what,
									bigtime_t 				how_much,
									bigtime_t 				performance_time);

		virtual	void EnableOutput(	const media_source 		&what,
									bool 					enabled,
									int32 					*change_tag);

private:

	static	status_t			init_reader_service_thread(void *data);
			status_t			ServiceLoop();
			status_t			BuildDescriptions();

			status_t			SeekTrack(int32 in_stream, bigtime_t *inout_time);
			status_t			GetNextChunk(int32 in_stream, BBuffer **out_buffer);
			status_t			PushNextBuffer(int32 in_stream);
			void				RecyclePending();
			void				RestampPending();

			BBuffer *			GrabBuffer();
			chunk *				GrabChunk();
			chunk *				PullChunk(int64 filePos);
			BBuffer *			GrabData(int64 filePos, int32 size, bigtime_t toBePlayedAt);
	
			BMediaAddOn 		*fMediaAddOn;
			
			port_id				fControlPort;
			thread_id			fServiceThread;
			
			BFile				*fFile;
			MediaExtractor *	fExtractor;
			bool				fRunning;
			bool				fFrameSent;
			bigtime_t			fFirstFrameAt;
			entry_ref			fRef;
			bigtime_t			fMediaDuration;
			
			ExtractorTrack		*fTracks;
			int32				fTrackCount;
			
			chunk *				fChunks;
			int32				fChunkCount;
			int32				fChunkSize;
			bigtime_t			fMyLatency;
			bigtime_t			fReadAhead;
			bigtime_t			fMaxDownstreamLatency;
			bigtime_t			fMediaTime;
			bigtime_t			fPerformanceToMediaTime;
			BTimedEventQueue	fQ;
	
			area_id				fBufferArea;
			uint8 *				fBufferPtr;
			int32				fBufferSize;
			int32				fBufferFilePos;
			
			int32				fConnectionCount;
};

#endif
