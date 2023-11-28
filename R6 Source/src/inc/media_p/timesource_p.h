
#ifndef TIMESOURCE_P
#define TIMESOURCE_P

#include "trinity_p.h"

#define N_TIME_TRANSMIT_STAMPS	128
#define N_MAX_CLIENT_COUNT		950

struct _time_transmit_client {
	port_id	port;
	int32 state;
};

struct _time_transmit_stamp {
	bigtime_t performance_time;
	bigtime_t real_time;
	float drift;
	uint32 _reserved;
};

struct _time_transmit_buf {
	int32 isStatic;
	int32 _reserved1;
	int32 _reserved2;
	int32 _reserved3;
	int32 _reserved4;
	int32 _reserved5;
	union {
		struct {
			int32 client_count;
			uint32 front_count;
			uint32 back_count;
			_time_transmit_stamp stamps[N_TIME_TRANSMIT_STAMPS];
		} dynamic_data;
		struct {
			int32 waterLine;
			int32 needToSeek;
			bigtime_t delta,stoppedAt,startTime,stopTime,seekTime,pendingSeek;
			_time_transmit_client clients[N_MAX_CLIENT_COUNT];

			/*	Note that 64-bit memory reads and writes are not strongly ordered, as
				far as I know, on 32-bit processors.  This means that we may get the high
				32-bits of the previous value and the low 32-bits of the current value, or
				vice-versa, depending on in what order the words are written.  I don't
				know whether we care... it doesn't seem like a big problem. */

			status_t AddClient(port_id port) volatile {
				for (int32 i=0;i<N_MAX_CLIENT_COUNT;i++) {
					if (!atomic_or((int32*)&clients[i].state,1)) {
						clients[i].port = port;
						if (waterLine < i) {
							ASSERT(waterLine >= (i-1));
							atomic_add((int32*)&waterLine,1);
						};
						return B_OK;
					};
				};
				return B_ERROR; // B_TOO_MANY_CLIENTS
			};

			status_t RemoveClient(port_id port) volatile {
				for (int32 i=0;i<N_MAX_CLIENT_COUNT;i++) {
					if (clients[i].port == port) {
						clients[i].port = B_BAD_VALUE;
						clients[i].state = 0;
						return B_OK;
					};
				};
				return B_BAD_VALUE; // B_CLIENT_NOT_FOUND
			};

			bool GetTime(bigtime_t *media, bigtime_t *real, float *drift) volatile {
				bool stopped;
				bigtime_t realTime = system_time();
				if (stopped = ((stopTime <= realTime) && (stopTime <= startTime))) {
					*drift = 0.0;
					if (needToSeek && (realTime > seekTime)) {
						stoppedAt = pendingSeek;
						needToSeek = 0;
					};
					*media = stoppedAt;
				} else {
					*drift = 1.0;
					if (needToSeek && (realTime > seekTime)) {
						delta = pendingSeek - seekTime;
						needToSeek = 0;
					};
					*media = realTime + delta;
				};
				*real = realTime;
				return stopped;
			};

			void Broadcast(int32 code, void *data, int32 dataSize) volatile {
				port_id port;
				for (int32 i=0;i<waterLine;i++) {
					port = clients[i].port;
					if (clients[i].state && (port != B_BAD_VALUE)) {
						if (write_port_etc(port,code,data,dataSize,B_TIMEOUT,1000000) == B_BAD_PORT_ID) {
							clients[i].port = B_BAD_VALUE;
							clients[i].state = 0;
						};
					};
				};
			};

			void BroadcastTimeWarp(bigtime_t real, bigtime_t performance) volatile {
				timewarp_q cmd;
				cmd.real_time = real;
				cmd.performance_time = performance;
				Broadcast(M_TIMEWARP,&cmd,sizeof(cmd));
			};

			void Start(bigtime_t start) volatile {
				delta = stoppedAt - start;
				startTime = start;
				BroadcastTimeWarp(start,stoppedAt);
			};

			void Stop(bigtime_t stop, bool immediate) volatile {
				if (immediate) stop = system_time();
				stoppedAt = stop + delta;
				stopTime = stop;
				BroadcastTimeWarp(stop,stoppedAt);
			};

			void Seek(bigtime_t mediaTime, bigtime_t performanceTime) volatile {
				pendingSeek = mediaTime;
				seekTime = performanceTime;
				needToSeek = 1;
				BroadcastTimeWarp(seekTime,mediaTime);
			};

			void SetRunMode(BMediaNode::run_mode mode) volatile {
				set_run_mode_q cmd;
				cmd.mode = mode;
				Broadcast(M_SET_RUN_MODE,&cmd,sizeof(cmd));
			};
		} static_data;
	} u;
};



class _BTimeSourceP : public BTimeSource
{
	public:
						_BTimeSourceP(media_node_id time_source);

		BMediaAddOn * 	AddOn(int32 * internal_id) const;
		port_id			ControlPort() const;

		/* Special stuff for clones of system time sources */
		void			ReleaseMaster();

	protected:
		status_t 		TimeSourceOp(
							const time_source_op_info & op,
							void * _reserved);
	private:
						~_BTimeSourceP();

		port_id _mControlPort;
		media_node m_node;
};


class _SysTimeSource : public BTimeSource
{
	public:
						_SysTimeSource(const char * name);
						~_SysTimeSource();

		BMediaAddOn * 	AddOn(int32 * internal_id) const;
		port_id 		ControlPort() const;
	protected:
		status_t 		TimeSourceOp(
							const time_source_op_info & op,
							void * _reserved);
};

#endif
