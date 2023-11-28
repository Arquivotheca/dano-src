#include <Bitmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <MediaTrack.h>
#include <Locker.h>
#include <Autolock.h>
#include <SoundPlayer.h>
#include <Debug.h>
#include <Window.h>

#include "VideoDecoder.h"
#include "VideoPlayer.h"

//#pragma mark ----------------------- VideoDecoder -----------------------

VideoDecoder::VideoDecoder(VideoContentInstance * content,
							BBitmap * bitmap,
							BView * view,
							bool useoverlay,
							const media_raw_video_format &fmt,
							BMediaTrack *track) :
	m_daddy(content),
	m_videoTrack(track),
	m_bitmap(bitmap),
	m_decodeThread(-1),
	m_lock("VideoDecoder Locker"),
	m_play(false),
	m_go(false),
	m_oview(view),
	m_useoverlay(useoverlay)
{
	m_quit = 0;
	m_atime = 0;
	m_aframe = 0;
	
	m_vframe = 0;
	m_vrate = fmt.field_rate;
	
	m_bitmap->UnlockBits();
	m_decodeThread = spawn_thread(DecodeVideoEnter, "Decode Video", 10, this);
	resume_thread(m_decodeThread);
}

VideoDecoder::~VideoDecoder()
{
	//printf("~VideoDecoder\n");
	m_quit = 1;
	status_t s;
	wait_for_thread(m_decodeThread, &s);
	//printf("~VideoDecoder done\n");
}

void
VideoDecoder::Reset()
{
	//printf("VideoDecoder::Reset \n");
	ASSERT(m_decodeThread = -1);
	
	
	
	m_quit = 1;
	status_t s;
	wait_for_thread(m_decodeThread, &s);
	
	int64 ioFrame = 0;
	status_t err = m_videoTrack->SeekToFrame(&ioFrame, B_MEDIA_SEEK_CLOSEST_BACKWARD);
	if (err < 0 || ioFrame != 0)
	{
		//fprintf(stderr, "VideoDecoder::Reset() SeekToFrame() returns '%s' (%Ld)\n",
		//	strerror(err), ioFrame);
	}
	m_vframe = 0;
	m_atime = 0;
	m_aframe = 0;
	m_play = false;
	m_quit = 0;
	m_decodeThread = spawn_thread(DecodeVideoEnter, "Decode Video", 10, this);
	resume_thread(m_decodeThread);
}


void 
VideoDecoder::RequestFrame(bigtime_t time,int32 aframe,float arate)
{
	//printf("VideoDecoder::RequestFrame 1 %f\n",arate);
	m_lock.Lock();
	//printf("VideoDecoder::RequestFrame 2\n");
	m_atime = time;
	m_aframe = aframe;
	m_arate = arate;
	m_lock.Unlock();
	m_play = true;
	//printf("VideoDecoder::RequestFrame %Lu %ld %f\n",m_atime,m_aframe,m_arate);
}

status_t 
VideoDecoder::DecodeVideoEnter(void * arg)
{
	((VideoDecoder *)arg)->DecodeVideo();
	return 0;
}

void 
VideoDecoder::DecodeVideo()
{
	status_t err;
	

	float a_rate;
	float a_frame;
	bigtime_t a_time;

	m_atime = 0;
	
	
	
	while (m_quit == 0)
	{
		
		if(m_play== false)
		{
			//printf("snooze %Lu\n",system_time());
			snooze(100000);
		}
		if(m_play == true)
		{
			m_lock.Lock();
			a_rate = m_arate;
			a_frame = m_aframe;
			a_time = m_atime;
			m_lock.Unlock();
			
			//printf("Audio rate %f frame %f %Lu\n",a_rate,a_frame,a_time);
			//printf("Video rate %f frame %f %Lu\n",(float)m_vrate,(float)m_vframe,system_time());
			//printf("Timing %Lu\n",a_time);
			float r = (m_vframe/m_vrate) - (m_aframe/m_arate);
			r = r *1000000.0;
			bigtime_t timeout = a_time + (bigtime_t)(r);
			bigtime_t tnow = system_time();
		
			//printf("timing now %Lu timeout %Lu\n",tnow,timeout);
			if(tnow > (timeout + 500000) )
			{
				//printf("Late ....now:%Lu timeout%Lu\n",(uint32)tnow,(uint32)timeout);
				r = (bigtime_t)(tnow - a_time);
				r = r / 1000000.0;
				r = (r + a_frame / a_rate) * m_vrate;

				int64 toFrame = (int64)r;
				err = m_videoTrack->SeekToFrame(&toFrame);
			
				m_vframe = toFrame;
				//printf("To frame %lu\n",toFrame);
			}
			else
			{
				//printf("No Wait  \n");
				if(tnow < timeout)
				{
					//printf("Wait %Lu \n",(timeout - tnow));
					snooze(timeout - tnow);
				}
			}
		
			m_bitmap->LockBits();
			int64 frameCount = 1;
			err = m_videoTrack->ReadFrames(m_bitmap->Bits(), &frameCount);
			m_bitmap->UnlockBits();
			
			if(!m_useoverlay)
			{
				BWindow * pwin = m_oview->Window();
				if(pwin->LockWithTimeout(500000) == B_OK)
				{ 
					m_oview->DrawBitmap(m_bitmap,m_oview->Bounds());
					m_oview->Sync();
					pwin->Unlock();
				}
			}
			
			if (err < 0)
			{
				frameCount = 0;
			}
			if (frameCount == 0)
			{
				//printf("video frame %f had trouble\n",m_vframe);
				m_decodeThread = -1;
				m_daddy->Reset();
				break;
			}
			m_vframe = m_vframe + 1;	
		}
	}
}


void VideoDecoder::DecodeFrame()
{
	//printf("Decode frame \n");
	m_bitmap->LockBits();
	int64 frameCount = 1;
	status_t err = m_videoTrack->ReadFrames(m_bitmap->Bits(), &frameCount);
	m_bitmap->UnlockBits();
	
	if((err<0)||(frameCount !=1))
	{
		printf("VideoDecoder::DecodeFrame error\n");
		return;
	}
	
	if(!m_useoverlay)
	{
		BWindow * pwin = m_oview->Window();
		if(pwin->LockWithTimeout(500000) == B_OK)
		{ 
			m_oview->DrawBitmap(m_bitmap,m_oview->Bounds());
			m_oview->Sync();
			pwin->Unlock();
		}
	}
	m_vframe ++;
}

void VideoDecoder::SetPlay(bool play)
{
	if(play == false)
		m_play = play;
}

//#pragma mark ----------------------- AudioDecoder -----------------------


AudioDecoder::AudioDecoder(VideoContentInstance * content, const media_raw_audio_format &fmt, BMediaTrack *track) :
	m_daddy(content),
	m_audioTrack(track),
	m_fifo(NULL),
	m_fifoFront(-1),
	m_fifoBack(0),
	m_fifoChunk(fmt.buffer_size),
	m_fifoSize(0),
	m_fifoPos(0),
	m_frameSize(fmt.channel_count*(fmt.format & 0xf)),
	m_playTotal(0),
	m_frameRate(fmt.frame_rate),
	m_soundPlayer(0),
	m_decodeThread(-1),
	m_playing(false),
	m_lock("AudioDecoder Locker")
{

	m_soundPlayer = new BSoundPlayer(&fmt, "VideoContent SoundPlayer", PlayFunc, NULL, this);

	if (m_audioTrack)
	{
		size_t bytesPerSecond = (size_t)(fmt.frame_rate*fmt.channel_count*(fmt.format & 0xf));
		float buffersPerSecond = bytesPerSecond/(float)fmt.buffer_size;
		//	decode-ahead one-fifth of a second
		int nBuffers = 2;
		if (buffersPerSecond > 10.)
		{
			nBuffers = (int)ceil(buffersPerSecond/5.);
		}
		m_fifoSize = nBuffers*fmt.buffer_size;
		m_fifoFront = create_sem(nBuffers, "AudioDecoder::fifoFront");
		m_fifo = malloc(m_fifoSize);

		m_decodeThread = spawn_thread(DecodeAudioEnter, "Decode Audio", 17, this);
		resume_thread(m_decodeThread);
	}
	m_videodecoder = 0;
	m_soundPlayer->SetHasData(true);
	m_soundPlayer->Start();
}


AudioDecoder::~AudioDecoder()
{
	//printf("~AudioDecoder()\n");
	m_playing = false;
	m_soundPlayer->Stop(false,false);
	delete m_soundPlayer;
	//printf("deleted soundplayer\n");
	delete_sem(m_fifoFront);
	status_t s;
	wait_for_thread(m_decodeThread, &s);
	free(m_fifo);
	//printf("~AudioDecoder()\n");
}

void
AudioDecoder::Reset()
{
	//printf("AudioDecoder::Reset\n");
	//ASSERT(m_decodeThread == -1);
	//m_soundPlayer->Stop();
	BAutolock lock(m_lock);
	m_playing = false;
	m_playTotal = 0;
	
	if(m_videodecoder)
	{
		m_videodecoder->DecodeFrame();
	}
	
	if (m_audioTrack != NULL)
	{
		int64 ioFrame = 0;
		status_t err = m_audioTrack->SeekToFrame(&ioFrame, B_MEDIA_SEEK_CLOSEST_BACKWARD);
		if (err < 0 || ioFrame != 0)
		{
			printf("Audio reset error\n");
		}
	}
	
	if (m_fifo)
	{
		//fixme:	this is not really tested because we've disabled audio
		delete_sem(m_fifoFront);
		m_fifoFront = create_sem(m_fifoSize/m_fifoChunk, "AudioDecoder::fifoFront");
		m_fifoBack = 0;
		m_fifoPos = 0;
		m_decodeThread = spawn_thread(DecodeAudioEnter, "Decode Audio", 17, this);
		resume_thread(m_decodeThread);
	}
	
}

void 
AudioDecoder::SetPlay(bool playing)
{
	//printf(" AudioDecoder::SetPlay play now !!! \n");
	m_playing = playing;
	m_videodecoder->SetPlay(playing);
}

void AudioDecoder::AddVideoDecoder(VideoDecoder * videodecoder)
{
	m_videodecoder = videodecoder;
}

void AudioDecoder::RemoveVideoDecoder()
{
	m_videodecoder = NULL;
}

status_t 
AudioDecoder::DecodeAudioEnter(void *arg)
{
	((AudioDecoder *)arg)->DecodeAudio();
	return 0;
}

void 
AudioDecoder::DecodeAudio()
{
	size_t putPos = 0;

	while (true)
	{
		status_t er = acquire_sem(m_fifoFront);
		if (er < 0)
		{
			break;
		}
		int64 frameCount = 0;
		status_t err = m_audioTrack->ReadFrames(((char *)m_fifo)+putPos, &frameCount);
		if (err < 0)
		{
			//printf("AudioDecoder::ReadFrames(): %s\n", strerror(err));
			frameCount = 0;
		}
		
	
		 
		if (frameCount == 0)
		{
			//printf("AudioDecoder::ReadFrames(): had to reset %Lu\n",system_time());
			m_decodeThread = -1;
			//m_soundPlayer->Stop();
			m_daddy->Reset();
			break;
		}
		else
		{
			atomic_add(&m_fifoBack, 1);
			putPos += m_fifoChunk;
			if (putPos >= m_fifoSize)
			{
				ASSERT(putPos == m_fifoSize);
				putPos = 0;
			}
		}
	}
	//printf("AudioDecoder::DecodeAudio() done \n");
	
}

void 
AudioDecoder::PlayFunc(void *cookie, void *buf, size_t size, const media_raw_audio_format & /* fmt */)
{
	((AudioDecoder *)cookie)->Play(buf, size);
}

void 
AudioDecoder::Play(void *buf, size_t size)
{
	
	bool playing = m_playing;	//	synch point

	//printf("AudioDecoder::Play\n");

	if(m_videodecoder)
	{
	
		if(playing == true)
		{
			bigtime_t t = system_time();
			//printf("AudioDecoder::Play %Lu %ld %f \n",t,m_playTotal,m_frameRate);
			m_videodecoder->RequestFrame(t,m_playTotal,m_frameRate);
			float f = 1000000.0 *  (float)(m_playTotal) / (float)(m_frameRate);
			bigtime_t tt = (bigtime_t)f;
			m_daddy->SetTime(tt);
		}
		
	}

	BAutolock lock(m_lock);
	if (!playing || !m_playing)	//	SetCurPosition may have called Reset()
	{
		memset(buf, 0, size);
		return;
	}

	bool gobbled = true;
	if (!m_fifo)
	{
		//	no sound track: play silence and advance time
		memset(buf, 0, size);
	}
	else if (atomic_add(&m_fifoBack, -1) > 0)
	{
		//	there is data: play it
		memcpy(buf, ((char *)m_fifo)+m_fifoPos, size);
		m_fifoPos += size;
		if (m_fifoPos >= m_fifoSize)
		{
			ASSERT(m_fifoPos == m_fifoSize);
			m_fifoPos = 0;
		}
		release_sem_etc(m_fifoFront, 1, B_DO_NOT_RESCHEDULE);
	}
	else
	{
		
		//	there is a sound track, but we haven't decoded yet
		atomic_add(&m_fifoBack, 1);
		gobbled = false;
		memset(buf, 0, size);
	}
	//	if we advanced the play pointer, tell daddy
	if (gobbled)
	{
		m_playTotal += size/m_frameSize;
	}
	
}

