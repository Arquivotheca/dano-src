#ifndef _QSCOPE_CONSUMER_H
#define _QSCOPE_CONSUMER_H

/* ++++++++++

   FILE:  LegoConsumer.h
   REVS:  $Revision: 1.6 $
   NAME:  William Adams
   DATE:  Mon Jun 29 18:55:07 PDT 1998

   Copyright (c) 1995-1998 by Be Incorporated.  All Rights Reserved.
	
	
+++++ */

#include "BufferConsumer.h"
#include "Controllable.h"
#include "bufferqueue.h"
#include "R3MediaDefs.h"

class BMediaAddOn;
class QScopeWindow;
class QScopeSubscriber;

class QScopeConsumer : public BBufferConsumer, public BControllable
{
public:
				QScopeConsumer(BMediaAddOn *);
	virtual 	~QScopeConsumer();

	// From BMediaNode
	virtual	BMediaAddOn* AddOn(long *) const;	/* Who instantiated you -- or NULL for app class */
	virtual	port_id ControlPort() const;
	
	virtual status_t	DirectPlayBuffer(BBuffer *buf);
	
	// From BBufferConsumer
	virtual	void BufferReceived(BBuffer * buffer);
	virtual	status_t AcceptFormat(const media_destination &dest, media_format * format);
	virtual	status_t GetNextInput(int32 * cookie, media_input * out_input);
	virtual	void DisposeInputCookie(int32 cookie);
	virtual	status_t FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 from_change_count,
				const media_format & format);

	// From BControllable
	virtual	status_t	GetParameterValue(int32 id, bigtime_t * last_change, void * value,size_t * ioSize);
	virtual	void 		SetParameterValue(int32 id, bigtime_t when, const void * value, size_t size);
	virtual	status_t 	StartControlPanel(BMessenger * out_messenger);

protected:
	// Stuff from BMediaNode
	virtual	void Start(bigtime_t performance_time);
	virtual	void Stop(bigtime_t performance_time, bool immediate);
	virtual	void TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time);

			status_t HandleMessage(int32 message, const void * data, size_t size);


	// From BBufferConsumer
	virtual	void Disconnected(		/* be sure to call BBufferConsumer::Connected()! */
				const media_source &producer,
				const media_destination &where);
	virtual	status_t Connected(		/* be sure to call BBufferConsumer::Connected()! */
				const media_source &producer,	/* here's a good place to request buffer group usage */
				const media_destination &where,
				const media_format & with_format,
				media_input * out_input);
	virtual	void ProducerDataStatus(
				const media_destination &for_whom,
				int32 status,
				bigtime_t at_media_time);
	virtual	status_t GetLatencyFor(
				const media_destination &what,
				bigtime_t * out_latency,
				media_node_id * out_timesource);

	virtual void	ConstructControlWeb();
	
private:
			void 		QScopeConsumer_Initialize();

	// Thread used for servicing control port
	static	status_t	ServiceThreadEntry(void * data);
	virtual void 		ServiceRun();

	enum {
		MAX_CONNECTIONS = 1
	};
	
	port_id			fControlPort;
	thread_id 		fControlPortThread;
	media_destination fDestination;	// only one?!

	media_format	fAudioFormat;
	media_source 	fProducerConnections[MAX_CONNECTIONS];
	media_source	fInputConnection;
	BMediaAddOn		*fAddOn;
	char msg[B_MEDIA_MESSAGE_SIZE];
		
	QScopeWindow	*fScopeWindow;
	QScopeSubscriber	*fBufferHandler;
	
	// Our running state
	bool mRunning;
	bool mStarting;
	bool mStopping;
	bigtime_t mStartTime;
	bigtime_t mStopTime;
	bigtime_t mLast;

	sem_id	fBufferPlayedSem;
	sem_id	fBufferRecordedSem;
	sem_id	fBufferReceivedSem;
	sem_id	fWriteCompletion;
};

#endif
