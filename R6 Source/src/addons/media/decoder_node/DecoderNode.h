/* DecoderNode.h by Simon Clarke */

#ifndef __DecoderNode_H
#define __DecoderNode_H

#include <BufferProducer.h>
#include <BufferConsumer.h>
#include <TimedEventQueue.h>
#include <MediaNode.h>

#include "Decoder.h"

using namespace BPrivate;


class DecoderNode : public BBufferProducer, public BBufferConsumer {
public:

		DecoderNode(BMediaAddOn *myAddOn, int32 flags);
		~DecoderNode();

		/* MediaNode calls */
		
		virtual port_id ControlPort() const;
		virtual BMediaAddOn *AddOn(int32 *internal_id) const;

		virtual void Start(			bigtime_t 				performance_time);
		
		virtual void Stop(			bigtime_t 				performance_time,
									bool 					immediate);

		virtual	void Seek(			bigtime_t 				media_time,
									bigtime_t 				performance_time);
		
		virtual	void SetRunMode(	run_mode 				mode);
		
		virtual	void TimeWarp(		bigtime_t 				at_real_time,
									bigtime_t 				to_performance_time);
		
		virtual	void Preroll();
		
		virtual	status_t HandleMessage(
									int32 					message,
									const void 				*data,
									size_t 					size);
	
		/* BufferProducer calls */

		virtual	status_t AcceptFormat(
									const media_destination &dest,
									media_format 			*format);

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

		/* BufferConsumer calls */

		virtual	status_t GetNextInput(
									int32 					*cookie,
									media_input 			*out_input);

		virtual	void DisposeInputCookie(
									int32 					cookie);

		virtual	void BufferReceived(BBuffer 				*buffer);
		
		virtual	void ProducerDataStatus(
									const media_destination &for_whom,
									int32 					status,
									bigtime_t 				at_media_time);
									
		virtual	status_t GetLatencyFor(
									const media_destination &for_whom,
									bigtime_t 				*out_latency,
									media_node_id 			*out_timesource);

		virtual	status_t Connected(	const media_source 		&producer,
									const media_destination &where,
									const media_format 		&with_format,
									media_input 			*out_input);

		virtual	void Disconnected(	const media_source 		&producer,
									const media_destination &where);

		virtual	status_t FormatChanged(
									const media_source 		&producer,
									const media_destination &consumer, 
									int32 					from_change_count,
									const media_format 		&format);

protected:

		friend status_t init_node_process_thread(void *data);
		static void PrintBufferGroup(BBufferGroup *group);

		void FreeResources();
		
		void ProcessData(	BBuffer 			*buffer,
							BBuffer				*outBuffer,
							bool				droppingBuffer);

		status_t			NextChunk(void **chunkData, size_t *chunkLen, media_header *mh);
		static status_t		_NextChunk(void *userData, void **chunkData, size_t *chunkLen, media_header *mh);
		void				MainLoop();
		void				CreateBuffers();
		void				DestroyBuffers();

		bigtime_t			NextProcess();
		bigtime_t 			GuessOwnLatency(int32 imageWidth, int32 imageHeight);

		thread_id 			LaunchProcessThread();
		status_t 			ProcessLoop(uint32);

		BMediaAddOn 		*fMediaAddOn;
		
		port_id				fControlPort;
		thread_id			fProcessThread;
		bool				fRunning,fOutputConnected,fInputConnected,fOutputEnabled;

		bigtime_t			fMyLatency, fDownstreamLatency;

		bool				fAlienBufferGroup;
		bool				fClearNextBuffer;
		bool				fShouldBeSending;
		int32				fFlags;
		void *				fImageData;
		int32				fBufferSize;
		int32				fBufferCount;
		int32				fBuffersQueued;
		BBufferGroup *		fBufferGroup;
		BBuffer *			fSyncBuffer;
		bool				fNeedSyncBuffer;

		BBuffer *			fCurInBuffer;
		BBuffer *			fCurOutBuffer;
		int32				fLoopState;
		
		Decoder *			fDecoder;
		int32				fDecoderID;
		media_format		fCurFmt;

		media_input			fInput;
		media_output		fOutput;
		BTimedEventQueue	fQ;
		BTimedEventQueue	fBufferQ;
};

#endif
