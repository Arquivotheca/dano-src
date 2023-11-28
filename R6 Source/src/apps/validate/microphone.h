
#if !defined(microphone_h)
#define microphone_h

#include <SoundPlayer.h>
#include <MediaRecorder.h>
#include <Messenger.h>
#include <Message.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algobase.h>

#include "miniplay.h"



#define CROSSING_COUNT_LIMIT 500

static media_raw_audio_format fmt = { 44100.0f, 2, 0x2, 0, 0 };
class MicrophoneTest
{
public:
		MicrophoneTest() :
			m_player(&fmt, "mic player", MicrophoneTest::PlayBuffer, 0, this),
			m_recorder("mic recorder", 110),
			m_msgr(0),
			m_msg(0),
			m_outPhase(0.f),
			m_frameRate(fmt.frame_rate),
			m_phase(0),
			m_inBufferCount(0),
			m_channel(0),
			m_inCrossCount(0),
			m_statMax(0)
		{
			mini_set_volume(miniCapture, 0.01, 0.01);
			mini_set_adc_source(miniMicIn);
		}
		~MicrophoneTest()
		{
			m_player.Stop();
			m_recorder.Stop();
			delete m_msgr;
			delete m_msg;
		}

		void Begin(BMessenger * msgTo, BMessage * msg)
		{
			m_player.Stop();
			m_recorder.Stop();
			m_recorder.Disconnect();
			delete m_msgr;
			m_msgr = msgTo;
			delete m_msg;
			m_msg = msg;
			m_phase = 0;
			m_inBufferCount = -20;
			m_channel = 0;
			m_inCrossCount = 0;
			m_player.SetHasData(true);
			m_player.Start();
			media_raw_audio_format fmt = m_player.Format();
			m_recorder.SetBufferHook(MicrophoneTest::RecordBuffer, this);
			media_format fmt2;
			fmt2.type = B_MEDIA_RAW_AUDIO;
			(media_raw_audio_format&)fmt2.u.raw_audio = fmt;
			status_t err = m_recorder.Connect(fmt2);
			fprintf(stderr, "recorder connect status: %s\n", strerror(err));
			m_recorder.Start();
		}
		static void PlayBuffer(void * cookie, void * data, size_t size, const media_raw_audio_format & /* fmt */)
		{
			((MicrophoneTest *)cookie)->PlayBufferV(data, size);
		}
		void PlayBufferV(void * data, size_t size)
		{
			float m_outDelta = 1000.f*2*M_PI/m_frameRate;
			int cnt = size/4;
			short * ptr = (short *)data;
			while (cnt-- > 0)
			{
				short v = (short)(sin(m_outPhase)*16000);
				ptr[0] = v;
				ptr[1] = v;
				ptr += 2;
				m_outPhase += m_outDelta;
				if (m_outPhase >= 2*M_PI)
					m_outPhase -= 2*M_PI;
			}
		}
		static void RecordBuffer(void * cookie, const void * data, size_t size, const media_header & /* header */)
		{
			((MicrophoneTest *)cookie)->RecordBufferV(data, size);
		}
		void RecordBufferV(const void * data, size_t size)
		{
			if (m_inBufferCount < 0) {
				m_inBufferCount++;
				return;
			}
			short * s = (short *)data;
			int cnt = size/4;
			while (cnt-- > 0)
			{
				int v = s[m_channel];
				s += 2;
				if (m_phase <= 0)
				{
					if (v > 0)
					{
						if (-m_phase > m_statMax)
						{
							m_statMax = -m_phase;
						}
						//	SWITCH!
						if (m_inCrossCount >= 42 && m_inCrossCount <= 47)
						{
							m_inBufferCount++;
						}
						else
						{
							fprintf(stderr, "bad crossing count (+): %d\n", m_inCrossCount);
							if (m_inBufferCount > 1)
							{
								m_inBufferCount -= 2;
							}
							else
							{
								m_inBufferCount = 0;
							}
						}
						m_phase = max(m_phase, v);
						m_inCrossCount = 1;
					}
					else
					{
						m_phase = min(m_phase, v);
						m_inCrossCount++;	//	another sample
					}
				}
				else
				{
					if (v > 0)
					{
						m_phase = max(m_phase, v);
					}
					else
					{
						//	SWITCH!
						if (m_phase > m_statMax)
						{
							m_statMax = m_phase;
						}
						m_phase = min(m_phase, v);
					}
					m_inCrossCount++;	//	just keep on countin'
				}
			}
			if (m_inBufferCount > CROSSING_COUNT_LIMIT)
			{
				fprintf(stderr, "m_statMax is %d\n", m_statMax);
				m_statMax = 0;
				fprintf(stderr, "%s channel tests OK\n", m_channel ? "RIGHT" : "LEFT");
				m_channel++;
				m_inBufferCount = 0;
				if (m_channel == 2)
				{
					m_inBufferCount = INT_MIN;
					m_msgr->SendMessage(m_msg);
				}
			}
		}

private:
		BSoundPlayer m_player;
		BMediaRecorder m_recorder;
		BMessenger * m_msgr;
		BMessage * m_msg;
		float m_outPhase;
		float m_frameRate;
		int m_phase;
		int m_inBufferCount;
		int m_channel;
		int m_inCrossCount;
		int m_statMax;
};

#endif	//	microphone_h
