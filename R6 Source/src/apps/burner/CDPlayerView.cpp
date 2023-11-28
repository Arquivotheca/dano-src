//
// CDPlayerView.cpp
//
//  by Nathan Schrenk (nschrenk@be.com)
//

#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Locker.h>
#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MessageQueue.h>
#include <MessageRunner.h>
#include <PictureButton.h>
#include <StringView.h>
#include <SoundPlayer.h>
#include <SupportDefs.h>        // for min_c()
#include <Window.h>
#include "AudioWrapperDataSource.h"
#include "BurnControlView.h"
#include "BurnerWindow.h"
#include "CDDataSource.h"
#include "CDPlayerView.h"
#include "EditWidget.h"
#include "TrackEditView.h"
#include "TrackListView.h"
#include "TransportButton.h"
#include "VolumeSlider.h"
#include "Thread.h"
#include "TimeDisplay.h"
#include "ButtonBitmaps.h"

// CD player notification messages
const uint32 kTrackPlayingMessage		= 'tPLY';
const uint32 kUpdateTimeMessage			= 'uPTM';
const uint32 kPlayerPausedMessage		= 'tPPA';
const uint32 kPlayerStoppedMessage		= 'tPST';
const uint32 kVolumeChangeMessage		= 'vCHD';

const uint32 kGainChangeMessage			= 'gchD';

// size in bytes of sound preview buffer
const size_t kPreviewBufferSize = 128 * 1024; // 128K
// size in bytes of one seek operation
const size_t kSeekQuanta		= (44100 * 2 * 2) * 3; // 3 seconds worth of CD audio data

// maximum value the volume slider will allow
const int32 kMaxVolume = 1000;

// --------------------------------------------------------------------------

class CircularBuffer
{
public:
			CircularBuffer(size_t capacity);
			~CircularBuffer();

	size_t	Read(void *writeTo, size_t size, bigtime_t timeout = B_INFINITE_TIMEOUT, bool adjustSize = false);
	size_t	Write(void *readFrom, size_t size, bigtime_t timeout = B_INFINITE_TIMEOUT/*, bool lockTriggers = false*/);
	void	Clear();
	size_t	Capacity() const;
	size_t	BytesContained() const;
	bool	CanRead(size_t bytes);
	bool	CanWrite(size_t bytes);	
//	void	AddTrigger(BMessage *msg, BHandler *target);
//	
//	BLocker fTriggerLock;

private:
//	struct triggered_message {
//		triggered_message(size_t offset, BMessage *msg, BHandler *handler) {
//			triggerOffset = offset;
//			message = msg;
//			target = handler;
//			next = NULL;
//		}
//		~triggered_message() {
//			delete message;
//		}
//		size_t triggerOffset;
//		BMessage *message;
//		BHandler *target;
//		triggered_message *next;
//	};
//
//	void ActivateTriggers();
//	void ClearTriggers();
//	
//	triggered_message *fTriggerList;
	uchar	*fBuffer;
	sem_id	fReadSem;
	sem_id	fWriteSem;
	size_t	fCapacity;
	volatile size_t	fReadOffset;
	volatile size_t	fWriteOffset;
	volatile int32	fBytesContained;
};


CircularBuffer::CircularBuffer(size_t capacity)
{
	fBuffer = (uchar *)malloc(capacity);
	fCapacity = capacity;
	fReadOffset = 0;
	fWriteOffset = 0;
	fReadSem = create_sem(0, "buffer read sem");
	fWriteSem = create_sem(fCapacity, "buffer write sem");
//	fTriggerList = NULL;
	fBytesContained = 0;
}

CircularBuffer::~CircularBuffer()
{
	delete_sem(fReadSem);
	delete_sem(fWriteSem);
	free(fBuffer);
}

// Reads the requested size into the buffer.  If timeout time passes before the read can be
// completed, no data will be read and the function will return.  If adjustSize == true,
// and there is not as much data in the buffer as requested, the requested read size will
// be adjusted downward to the largest size that can be read without blocking.
size_t CircularBuffer::Read(void *buf, size_t size, bigtime_t timeout, bool adjustSize)
{
	size_t read = 0;
	int32 contained = fBytesContained;	// store atomic var, so that it cannot change between
										// the test and the assignment
	if (adjustSize && (int32)size > contained) {
		size = contained;
	}
	// acquire the read semaphore with the size desired
	if (acquire_sem_etc(fReadSem, size, B_RELATIVE_TIMEOUT, timeout) == B_OK)
	{
//		fTriggerLock.Lock();
		// set up first read
		size_t read_size = fCapacity - fReadOffset;
		read_size = min_c(read_size, size);
		memcpy(buf, fBuffer + fReadOffset, read_size);
		fReadOffset = (fReadOffset + read_size) % fCapacity;
		read += read_size;
		
		if (read < size) { // check to see if we need a second read
			read_size = size - read;
			memcpy(((char *)buf) + read, fBuffer, read_size);
			fReadOffset = read_size;
			read += read_size;
		}

//		ActivateTriggers();
//		fTriggerLock.Unlock();
		
		// only change the atomic var and call release_sem if fWriteSem is valid
		sem_info info;
		if (get_sem_info(fWriteSem, &info) == B_OK) {
			atomic_add(&fBytesContained, -read);
			// release the write semaphore with the size read
			release_sem_etc(fWriteSem, read, B_DO_NOT_RESCHEDULE);
		}
	}
	return read;
}

size_t CircularBuffer::Write(void *buf, size_t size, bigtime_t timeout/*, bool lockTriggers*/)
{
	size_t written = 0;
	if (acquire_sem_etc(fWriteSem, size, B_RELATIVE_TIMEOUT, timeout) == B_OK)
	{
//		if (lockTriggers) {
//			fTriggerLock.Lock();
//		}
		// set up first write
		size_t write_size = fCapacity - fWriteOffset;
		write_size = min_c(write_size, size);
		memcpy(fBuffer + fWriteOffset, buf, write_size);
		fWriteOffset = (fWriteOffset + write_size) % fCapacity;
		written += write_size;
	
		if (written < size) {	// check to see if we need to do a second write		
			write_size = size - written;
			memcpy(fBuffer, ((char *)buf) + written, write_size);
			fWriteOffset = write_size;
			written += write_size;
		}
		
		// only change the atomic var and call release_sem if fReadSem is valid
		sem_info info;
		if (get_sem_info(fReadSem, &info) == B_OK) {
			atomic_add(&fBytesContained, written);
			// release the read semaphore with the size written
			release_sem_etc(fReadSem, written, 0);
		}
	}
//	if (lockTriggers && written == 0) {
//		fTriggerLock.Unlock();
//	}
	return written;
}

void CircularBuffer::Clear()
{
//	ClearTriggers();
	delete_sem(fReadSem);
	delete_sem(fWriteSem);
	atomic_and(&fBytesContained, 0); // set fBytesContained to 0
	fReadOffset = 0;
	fWriteOffset = 0;
	fReadSem = create_sem(0, "buffer read sem");
	fWriteSem = create_sem(fCapacity, "buffer write sem");
}

size_t CircularBuffer::Capacity() const
{
	return fCapacity;
}

size_t CircularBuffer::BytesContained() const
{
	return (size_t)fBytesContained;
}


// CanRead() is only safe if you have one thread reading from the buffer.
bool CircularBuffer::CanRead(size_t bytes)
{
	bool r = false;
	if (acquire_sem_etc(fReadSem, bytes, B_RELATIVE_TIMEOUT, 0) == B_OK) {
		release_sem_etc(fReadSem, bytes, 0);
		r = true;
	}
	return r;
}

// CanWrite() is only safe if you have one thread writing to the buffer.
bool CircularBuffer::CanWrite(size_t bytes)
{
	bool r = false;
	if (acquire_sem_etc(fWriteSem, bytes, B_RELATIVE_TIMEOUT, 0) == B_OK) {
		release_sem_etc(fWriteSem, bytes, 0);
		r = true;
	}
	return r;
}


//void CircularBuffer::AddTrigger(BMessage *msg, BHandler *target)
//{
//	size_t triggerPoint = fWriteOffset - 1;
//	if (triggerPoint < 0) {
//		triggerPoint = fCapacity;
//	}
//	triggered_message *node = new triggered_message(triggerPoint, msg, target);
//	triggered_message *trig;
//	if (fTriggerLock.Lock()) {
//		if (fTriggerList != NULL) {
//			trig = fTriggerList;
//			while (trig->next != NULL) {
//				trig = trig->next;
//			}
//			trig->next = node;
//		} else {
//			fTriggerList = node;
//		}
//		fTriggerLock.Unlock();
//	}
//}
//
//// ActivateTriggers goes through the trigger list and activates each
//// trigger that has an offset that is pointing into empty space.  Any
//// trigger pointing into empty space should now be activated, because
//// the read operation immediately before ActivateTriggers must have
//// tripped it.  fTriggerLock must be locked before calling this function.
//void CircularBuffer::ActivateTriggers()
//{
//	while ((fTriggerList != NULL)) {
//		triggered_message *trig = fTriggerList;	
//		bool contiguous = fReadOffset < fWriteOffset;
//		if ((contiguous && ((trig->triggerOffset < fReadOffset)
//				|| (trig->triggerOffset > fWriteOffset)))
//			|| (!contiguous && ((trig->triggerOffset > fWriteOffset)
//				&& (trig->triggerOffset < fReadOffset)))
//			|| (!contiguous && (!CanRead(1)))) // !CanRead(1) == true means the buffer is empty
//		{
////			printf("  Activating trigger: track = %ld, offset = %ld, fReadOffset = %ld, fWriteOffset = %ld\n", trig->message->FindInt32("track"), trig->triggerOffset, fReadOffset, fWriteOffset);
//			BLooper *looper = trig->target->Looper();
//			if (looper) {
//				looper->PostMessage(trig->message, trig->target);
//			}
//			fTriggerList = fTriggerList->next;
//			delete trig;
//		} else {
//			break;
//		}
//	}
//}
//
//void CircularBuffer::ClearTriggers()
//{
//	while (fTriggerList != NULL) {
//		triggered_message *trig = fTriggerList;
//		fTriggerList = fTriggerList->next;
//		delete trig;
//	}
//}

// --------------------------------------------------------------------------

class PlayBuffer {
public:
					PlayBuffer(CDTrack *track, int32 startOffset,
						size_t capacity = kPreviewBufferSize);
					~PlayBuffer();
	
	CircularBuffer	buffer;
	CDTrack*		track;
	volatile uint32	startOffset;
	volatile uint32	trackOffset;
	uint32			zeroOffset;
	volatile uint32	playedBytes;
	uint32			deadBytes;
	volatile float	gain;
	sem_id			readSem;		// created with count of 0. reader releases it when
									// starting to read, writer acquires it before going on
									// to create another PlayBuffer for the next track.  Makes
									// sure writer cannot get more than 1 PlayBuffer ahead.	
	bool			doneWriting;
};

PlayBuffer::PlayBuffer(CDTrack *track, int32 startOffset, size_t capacity)
	: buffer(capacity),
	  track(track),
	  startOffset(startOffset),
	  trackOffset(startOffset),
	  playedBytes(0),
	  doneWriting(false)
{
	readSem = create_sem(0, "PlayBuffer reading sem");
	track->PreGap((uint32*)&zeroOffset, (uint32 *)&deadBytes);
	zeroOffset = (zeroOffset + deadBytes) * track->FrameSize();
	deadBytes = deadBytes * track->FrameSize();
	
	AudioWrapperDataSource *src = dynamic_cast<AudioWrapperDataSource *>(track->DataSource());
	if (src != NULL) {
		gain = src->Gain();	
	} else {
		gain = 1.0f;
	}
}

PlayBuffer::~PlayBuffer()
{
	delete_sem(readSem);
}


// --------------------------------------------------------------------------

class AudioManager : public BRunnable
{
public:
						AudioManager(BHandler *forNotification, size_t bufferSize);
	virtual				~AudioManager();
	virtual status_t	Run();
	void				AddMessage(BMessage *msg);
	void				TrackListChanged(CDTrack *head);
	size_t				FillBuffer(void *buffer, size_t size);
	void				Quit();
	
	int32				GetPlayingOffset();
protected:
	void				Play();
	void				Stop();
	void				SeekForward(int32 bytes = kSeekQuanta);
	void				SeekBack(int32 bytes = kSeekQuanta);
	void				SkipForward();
	void				SkipBack();

	void				AdjustGain(float gain);

	size_t				ReadFromDataSource();
	void				SetPlayingTrack(CDTrack *track, int32 offset = -1);
//	void				SetCurrentTrack(CDTrack *track); // the "selected" track
	
private:

	BMessage			*CreateTrackPlayingMessage(CDTrack *track, bool isPlaying);
	
	BMessageQueue		fQueue;
	BLocker				fPlayBuffLock;
	BLocker				fWriteBuffLock;
	float				fVolume;
	BSoundPlayer		*fPlayer;
	BHandler			*fNotify;
	CDTrack* volatile	fHeadTrack;
	CDTrack* volatile	fStartPlayingTrack;
	PlayBuffer* volatile fWritingBuffer;
	PlayBuffer* volatile fPlayingBuffer;
	size_t				fBufferSize;
	volatile bool		fQuitRequested;
	volatile bool		fIsPlaying;
	volatile bool		fIsPaused;
	volatile bool		fSwitchPlayBuffers;
};

// --------------------------------------------------------------------------

void FillBuffer(void *castToAudioManager,
				void *buffer,
				size_t size,
				const media_raw_audio_format &/*format*/)
{
	AudioManager *manager = (AudioManager *)castToAudioManager;
	if (manager) {
		size_t filled = manager->FillBuffer(buffer, size);
		if (filled != size) {
//			fprintf(stderr, "Sound preview buffer not filled!\n");
			memset(buffer, 0, size); 
		}
	}
}

// --------------------------------------------------------------------------

AudioManager::AudioManager(BHandler *forNotification, size_t bufferSize)
	:   fVolume(1.0),
		fPlayer(NULL),
		fNotify(forNotification),
		fHeadTrack(NULL),
		fStartPlayingTrack(NULL),
		fWritingBuffer(NULL),
		fPlayingBuffer(NULL),
		fBufferSize(bufferSize),
		fQuitRequested(false),
		fIsPlaying(false),
		fIsPaused(false),
		fSwitchPlayBuffers(false)
{
}

AudioManager::~AudioManager()
{
	if (fPlayer) {
		fPlayer->Stop();
		delete fPlayer;
	}
	delete fWritingBuffer;
	delete fPlayingBuffer;
}

status_t AudioManager::Run()
{
	for(;;) {
		// check for quit request
		if (fQueue.Lock()) {
			if (fQuitRequested) {
				fQueue.Unlock();
				break;
			}
			
			size_t read = 0;
			fQueue.Unlock();
			
			if (fIsPlaying && !fIsPaused) {
				read = ReadFromDataSource();
			}
			bool handledMessage = true;
			fQueue.Lock();
			// look in the queue for a message
			BMessage *msg = fQueue.NextMessage();
			if (msg) {
				switch (msg->what) {
				case kPlayButtonMessage:
					Play();
					break;
				case kStopButtonMessage:
					Stop();
					break;
				case kSeekBackButtonMessage:
					SeekBack();
					break;
				case kSeekForwardButtonMessage:
					SeekForward();
					break;
				case kSkipBackButtonMessage:
					SkipBack();
					break;
				case kSkipForwardButtonMessage:
					SkipForward();
					break;
				case BurnerWindow::TRACK_SELECTION_CHANGED:
					{
						TrackListView *listView;
						if (msg->FindPointer("tracklist", (void **)&listView) == B_OK) {
							TrackRow *row = dynamic_cast<TrackRow *>(listView->CurrentSelection());
							if (row != NULL) {
								fStartPlayingTrack = row->Track();							
							} else {
								fStartPlayingTrack = fHeadTrack;
							}
						}
					}
					break;
				case kVolumeChangeMessage:
					{
						TVolumeSlider *slider(NULL);
						if (msg->FindPointer("source", (void**)&slider) == B_OK) {
							fVolume = slider->Position();
							if (fPlayer != NULL) {
								fPlayer->SetVolume(fVolume);
							}
						}
					}
					break;
				case kGainChangeMessage:
					{
						// XXX: this should not be handled in the run loop -- get it out of here.
						//      also, there should be a lock around all access to the PlayBuffers
						CDTrack *track;
						float gain;
						if ((msg->FindPointer("track", (void **)&track) == B_OK)
							&& (msg->FindFloat("gain", &gain) == B_OK))
						{
							if (fPlayBuffLock.Lock()) {
								if (fPlayingBuffer && fPlayingBuffer->track == track) {
									fPlayingBuffer->gain = gain;
								}
								fPlayBuffLock.Unlock();
							}
							
							if (fWriteBuffLock.Lock()) {
								if (fWritingBuffer && fWritingBuffer->track == track) {
									fWritingBuffer->gain = gain;
								}
								fWriteBuffLock.Unlock();
							}
						}
					}
					break;
				default: {
//					char *what = (char *)&(msg->what);
//					printf("AudioManager::Run() -- unrecognized message (%c%c%c%c) received!\n", what[0], what[1], what[2], what[3]);
//					
					break;					
					}
				}
				delete msg;
			} else {
				handledMessage = false;
			}

			fQueue.Unlock();
					
			if (!handledMessage && read <= 0) {
				snooze(30 * 1000);
			}
		} else {
//			printf("AudioManager::Run() -- fQueue.Lock() failed!!!\n");
		}
	}
//	printf("AudioManager::Run() -- exiting...\n");
	return B_OK;
}

size_t AudioManager::ReadFromDataSource()
{
	size_t written = 0;
	bool writelocked = fWriteBuffLock.Lock();
	
	if (writelocked && fWritingBuffer != NULL) {
		if (fWritingBuffer->doneWriting) {
			// try to move on to the next audio track
			if (acquire_sem_etc(fWritingBuffer->readSem, 1, B_RELATIVE_TIMEOUT, 0) == B_OK)
			{
				CDTrack *nextTrack = fWritingBuffer->track->Next();
				// skip data tracks
				while (nextTrack != NULL && nextTrack->IsData()) {
					nextTrack = nextTrack->Next();
				}
				if (nextTrack != NULL) {
					// start reading from the new track at offset zero
					fWritingBuffer = new PlayBuffer(nextTrack, 0, fBufferSize);
				} else {
					// this was the last track
					fWritingBuffer = NULL;
					fWriteBuffLock.Unlock();
					return 0;
				}
			} else { // done reading from this track, but playing has not started yet
				fWriteBuffLock.Unlock();
				return 0;
			}
		}
		fWriteBuffLock.Unlock();
		writelocked = false;
		
		// fWritingBuffer is now guaranteed to be non-NULL, so we should try to
		// read from the datasource and write to the buffer.
		int32 writeSize = fWritingBuffer->buffer.Capacity() / 2;
		if (fWritingBuffer->buffer.CanWrite(writeSize)) {
			uchar *buffer = (uchar *)malloc(writeSize);
//			uchar *buffer = (uchar *)0xc0000000;
//			int32 areaSize = (writeSize % B_PAGE_SIZE == 0) ? writeSize : ((writeSize / B_PAGE_SIZE) + 1) * B_PAGE_SIZE;
//			area_id id = create_area("soundplay data area", (void **)&buffer, B_EXACT_ADDRESS, areaSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
//			printf("Created area of size %ld, needed size %ld\n", areaSize, writeSize);

			int32 bufAvailable = writeSize;
			uchar *bufPtr = buffer;
			// write zeros into buffer for trackOffset < trackDeadBytes
			int32 zeroBytes = 0;
			if (fWritingBuffer->trackOffset < fWritingBuffer->deadBytes) {
				zeroBytes = fWritingBuffer->deadBytes - fWritingBuffer->trackOffset;
				if (zeroBytes > writeSize) {
					zeroBytes = writeSize;
				}
				memset(bufPtr, 0, zeroBytes);
				bufPtr += zeroBytes;
				bufAvailable -= zeroBytes;
			}
			
			CDDataSource *src = fWritingBuffer->track->DataSource();
			int32 len = src->Length();
			int32 srcOffset = (fWritingBuffer->trackOffset + zeroBytes) - fWritingBuffer->deadBytes;			
			status_t result = B_ERROR;

			if (bufAvailable > 0) {
				if (srcOffset + bufAvailable > len) {
					bufAvailable = len - srcOffset;					
				}
				result = src->Read((void *)bufPtr, bufAvailable, srcOffset);
			}
			// amount to write into buffer is the amount of zeros plus the
			// amount of data read, which is either the remaining amount of
			// data in the datasource, or the space available in the buffer.
			size_t amountToWrite = zeroBytes + min_c(len - srcOffset, bufAvailable);
			written = fWritingBuffer->buffer.Write(buffer, amountToWrite, B_INFINITE_TIMEOUT);
			
			if (written > 0) {
				// if data actually got written to the buffer, increment the
				// track offset and check for running over the end of the track
				fWritingBuffer->trackOffset += written;
				if (fWritingBuffer->trackOffset >= (len + fWritingBuffer->deadBytes))
				{
					fWritingBuffer->doneWriting = true;
				}
			}
			free(buffer);
//			delete_area(id);
		}
	}

	if (writelocked) {
		fWriteBuffLock.Unlock();
	}
//	printf("AudioManager::ReadFromDataSource: exiting\n");
	return written;
}


void CDPlayerView::TrackGainChanged(CDTrack *track, float gain)
{
	BMessage *msg = new BMessage(kGainChangeMessage);
	msg->AddPointer("track", track);
	msg->AddFloat("gain", gain);
	fManager->AddMessage(msg);
}


void AudioManager::TrackListChanged(CDTrack *head)
{
	if (fQueue.Lock()) {
		Stop();
		fHeadTrack = head;
		fStartPlayingTrack = head;
		fQueue.Unlock();
	}
}

// reads from fPlayingBuffer and writes into the specified buffer
size_t AudioManager::FillBuffer(void *buffer, size_t size)
{
	size_t read = 0;
//	bigtime_t this_time = system_time();
	bool playlocked = fPlayBuffLock.Lock();
	bool writelocked = fWriteBuffLock.Lock();
	
	// this is a loop so that a partial read from one PlayBuffer will be followed
	// by a read from the next PlayBuffer.  We need this for the case where we
	// play off the end of a track and into the next one.
	while (playlocked && writelocked && read < size &&
		   ((fPlayingBuffer != NULL) || (fSwitchPlayBuffers && fWritingBuffer != NULL)))
	{
		bool notify = false;
		size_t read_this_iter = 0;
		if (fSwitchPlayBuffers) {
			PlayBuffer *oldBuf = fPlayingBuffer;
			fPlayingBuffer = fWritingBuffer;
			if (fPlayingBuffer != NULL) {
				release_sem_etc(fPlayingBuffer->readSem, 1, 0);
			}

			fSwitchPlayBuffers = false;
			delete oldBuf;
			notify = true;
		}
		fWriteBuffLock.Unlock();
		writelocked = false;
		
		if (fPlayingBuffer != NULL) {
			read_this_iter = fPlayingBuffer->buffer.Read(buffer, size - read, 15 * 1000);
												// time out in 15 milliseconds
			if (read_this_iter == 0 && fPlayingBuffer->doneWriting) {
				// there is less than 'size' left in the buffer, and the writer
				// is not writing any more, so read anything left
				read_this_iter = fPlayingBuffer->buffer.Read(buffer, size - read, 0, true);
				fSwitchPlayBuffers = true;
			}
		}
		
		if (read_this_iter > 0) {
			if (fPlayingBuffer->gain != 1.0f) {
				// apply gain in a straightforward way
				int16 *samples = (int16*)(((uchar *)buffer) + read);
				float sample;
				for (uint32 i = 0; i < (read_this_iter / sizeof(int16)); i++) {
#if B_HOST_IS_BENDIAN
					B_SWAP_INT16(samples[i]);
#endif
					sample = samples[i] * fPlayingBuffer->gain;
					if (sample > 32767.0f) { sample = 32767.0f; }
					if (sample < -32767.0f) { sample = -32767.0f; }
					samples[i] = (int16)sample;
#if B_HOST_IS_BENDIAN
					B_SWAP_INT16(samples[i]);
#endif
				}
			}
			// increment bytes played counter
			fPlayingBuffer->playedBytes += read_this_iter;
			// increment read total var
			read += read_this_iter;
		}
	
		if (notify) {
			if (fNotify && fNotify->Looper()) {
				BMessage *msg;
				if (fPlayingBuffer != NULL) {
					msg = CreateTrackPlayingMessage(fPlayingBuffer->track, true);
				} else {
					msg = CreateTrackPlayingMessage(NULL, false);				
				}
				fNotify->Looper()->PostMessage(msg, fNotify);
				delete msg;
			}
		}
		fPlayBuffLock.Unlock();
		playlocked = fPlayBuffLock.Lock();
		writelocked = fWriteBuffLock.Lock();
	}
	
	if (writelocked) { fWriteBuffLock.Unlock(); }
	if (playlocked) { fPlayBuffLock.Unlock(); }

//	printf("AudioManager::FillBuffer() - took time %Ld\n", system_time() - this_time);

	return read;
}

// sets the quit flag to true
void AudioManager::Quit()
{
	if (fQueue.Lock()) {
		fQuitRequested = true;
		Stop();
		fQueue.Unlock();
	}
}

int32 AudioManager::GetPlayingOffset()
{
	int32 r = 0;
	
	if (fPlayBuffLock.Lock()) {
		if (fPlayingBuffer != NULL) {
			r = fPlayingBuffer->startOffset + fPlayingBuffer->playedBytes - fPlayingBuffer->zeroOffset;
		}
		fPlayBuffLock.Unlock();
	}
	return r;
}


void AudioManager::AddMessage(BMessage *msg)
{
	if (fQueue.Lock()) {
		fQueue.AddMessage(msg);
		fQueue.Unlock();
	}
}

void AudioManager::Play()
{
//	BMessage *msg;

	if (!fStartPlayingTrack) {
//		printf("Attempted to play without any tracks\n");
		return;
	}
	if (fPlayer == NULL) {
		// set up a format equal to the CD audio format
		media_raw_audio_format format;
		format.frame_rate		= 44100;
		format.channel_count	= 2;
		format.format 			= media_raw_audio_format::B_AUDIO_SHORT;
		format.byte_order		= B_MEDIA_LITTLE_ENDIAN;
		format.buffer_size		= 4096;
		// construct sound player
		fPlayer = new BSoundPlayer(&format, "CD Burner Preview", ::FillBuffer, NULL, this);
		fPlayer->SetVolume(fVolume);
	}
	if (fIsPlaying) {
		BMessage msg(kPlayerPausedMessage);
	 	if (!fIsPaused) { // not paused, so pause
			msg.AddBool("paused", true);
			fPlayer->SetHasData(false);
			fIsPaused = true;
		} else { // was paused, so start it up again
			msg.AddBool("paused", false);
			fIsPaused = false;
			fPlayer->SetHasData(true);
		}
		if (fNotify && fNotify->Looper()) {
			fNotify->Looper()->PostMessage(&msg, fNotify);
		}
	} else { // not already playing, so play
		SetPlayingTrack(fStartPlayingTrack);
		ReadFromDataSource(); // make sure there's some data available for the player
		fPlayer->SetHasData(true);
		fPlayer->Start();
		fIsPlaying = true;
		fIsPaused = false;
	}
}

void AudioManager::SetPlayingTrack(CDTrack *track, int32 offset)
{
	if (fWriteBuffLock.Lock()) {
		PlayBuffer *oldBuf = fWritingBuffer;
		if (track != NULL) {
			if (offset < 0) {
				// negative offset means to start after pregap
				offset = track->PreGap() * track->FrameSize();
			}
			fWritingBuffer = new PlayBuffer(track, offset, fBufferSize);
		} else { // track == NULL
			fWritingBuffer = NULL;
			if (fPlayer != NULL && fIsPlaying) {
				fPlayer->SetHasData(false);
				fIsPlaying = false;
			}
			if (fNotify && fNotify->Looper()) {
				// send stop message so GUI can update
				BMessage msg(kPlayerStoppedMessage);
				fNotify->Looper()->PostMessage(&msg, fNotify);
			}
		}
		fWriteBuffLock.Unlock();
	
		// delete the old buffer if the player has not yet started to use it
		// XXX: Should this check be better than just this pointer comparison?
		//      perhaps the readSem should be checked?
		if (fPlayingBuffer != oldBuf) {
			delete oldBuf;
		}
		fSwitchPlayBuffers = true;
	}
}

//void AudioManager::SetCurrentTrack(CDTrack *track)
//{
//	// XXX: select this track in the TrackListView?
//}


void AudioManager::Stop()
{
	SetPlayingTrack(NULL);
	fPlayBuffLock.Lock();
	fWriteBuffLock.Lock();
	delete fPlayingBuffer;
	delete fWritingBuffer;
	fPlayingBuffer = NULL;
	fWritingBuffer = NULL;
	fWriteBuffLock.Unlock();
	fPlayBuffLock.Unlock();
}

void AudioManager::SeekForward(int32 bytes)
{
	// NOTE: the implementation of this class assumes that it will always be
	// seeking forward a greater number of bytes than one full buffer.  If this
	// is not the case, improper behavior will occur.
	if (fIsPlaying) {
		bool playLocked = fPlayBuffLock.Lock();
		bool writeLocked = fWriteBuffLock.Lock();
		bool trackDone = false;
		bool trackChanged = false;
		int32 bytesDrained = 0;
		int32 bytesLeft = bytes;
		
		if (playLocked && writeLocked) {
			
			if (fPlayingBuffer != NULL) {	// drain bytes in playing buffer
				bytesDrained = fPlayingBuffer->buffer.BytesContained();
				fPlayingBuffer->buffer.Clear();
				fPlayingBuffer->playedBytes += bytesDrained;
				bytesLeft -= bytesDrained;
			}

			do {			
				if (fPlayingBuffer == fWritingBuffer) {
					// already drained fPlayingBuffer, so fWritingBuffer was also drained
					uint32 trackLen = fPlayingBuffer->track->Length() * fPlayingBuffer->track->FrameSize();
					if ((bytesLeft + fPlayingBuffer->trackOffset) < trackLen) {
						fPlayingBuffer->trackOffset += bytesLeft;
						fPlayingBuffer->playedBytes += bytesLeft;
						bytesLeft = 0;				
					} else {
						bytesLeft -= (trackLen - fPlayingBuffer->trackOffset);
						trackDone = true;
					}
				} else if (fWritingBuffer != NULL) {
					// must move playing buffer forward to writing buffer
					delete fPlayingBuffer;
					fPlayingBuffer = fWritingBuffer;
					release_sem_etc(fWritingBuffer->readSem, 1, 0);
					trackChanged = true;
										
					// this operation has already changed which track is playing, which
					// is enough of a seek -- it doesn't need to drain any more data.
					break;

//					bytesDrained = fWritingBuffer->buffer.BytesContained();
//					fWritingBuffer->buffer.Clear();
//					fWritingBuffer->playedBytes += bytesDrained;
//					bytesLeft -= bytesDrained;
//					
//					if (bytesLeft < 0) {
//						fWritingBuffer->trackOffset += bytesLeft;
//						fWritingBuffer->playedBytes += bytesLeft;
//						bytesLeft = 0;
//					} else {
//						uint32 trackLen = fPlayingBuffer->track->Length() * fPlayingBuffer->track->FrameSize();
//						if ((bytesLeft + fPlayingBuffer->trackOffset) < trackLen) {
//							fPlayingBuffer->trackOffset += bytesLeft;
//							fPlayingBuffer->playedBytes += bytesLeft;
//							bytesLeft = 0;				
//						} else {
//							trackDone = true;						
//						}
//					
				} else {
					// seeked off the end of the last track
					delete fPlayingBuffer;
					if (fHeadTrack != NULL) {
						fPlayingBuffer = fWritingBuffer = new PlayBuffer(fHeadTrack, 0, fBufferSize);
					} else {
						fPlayingBuffer = NULL;
					}
					trackChanged = true;
					break;
				}
				
				if (trackDone) {
					// must move on to next track
					CDTrack *nextTrack = (fWritingBuffer == NULL) ? NULL : fWritingBuffer->track->Next();
					while (nextTrack != NULL && nextTrack->IsData()) { // skip data tracks
						nextTrack = nextTrack->Next();
					}
					if (nextTrack == NULL) {
						nextTrack = fHeadTrack;
					}
					if (nextTrack != NULL) {
						fWritingBuffer = new PlayBuffer(nextTrack, bytesLeft, fBufferSize);
						release_sem_etc(fWritingBuffer->readSem, 1, 0);
					} else {
						fWritingBuffer = NULL;
					}
					delete fPlayingBuffer;
					fPlayingBuffer = fWritingBuffer;
					trackChanged = true;			
					trackDone = false;
				}

			} while ((bytesLeft > 0) && (fWritingBuffer != NULL));

			if (trackChanged && fNotify && fNotify->Looper()) {
				BMessage *msg = CreateTrackPlayingMessage((fPlayingBuffer != NULL) ? fPlayingBuffer->track : NULL, true);
				fNotify->Looper()->PostMessage(msg, fNotify);
				delete msg;
			}
		}
		
		// release the locks in opposite order of acquire
		if (writeLocked) { fWriteBuffLock.Unlock(); }
		if (playLocked) { fPlayBuffLock.Unlock(); }		
	} else { // !fIsplaying
		//printf("AudioManager::SeekForward() while not playing unimplemented!\n");		
	}
}

void AudioManager::SeekBack(int32 bytes)
{
	// NOTE: the implementation of this class assumes that it will always be
	// seeking forward a greater number of bytes than one full buffer.  If this
	// is not the case, improper behavior will occur.
	if (fIsPlaying) {
		bool playLocked = fPlayBuffLock.Lock();
		bool writeLocked = fWriteBuffLock.Lock();
		int32 bytesLeft = bytes;
		int32 bytesDrained = 0;
		bool skipBack = false;
		bool trackChanged = false;
		
		if (playLocked && writeLocked) {
			
			if (fPlayingBuffer != NULL) {	// drain bytes in playing buffer
				bytesDrained = fPlayingBuffer->buffer.BytesContained();
				fPlayingBuffer->buffer.Clear();
				fPlayingBuffer->trackOffset -= bytesDrained;
			}

			do {
				if (fPlayingBuffer == fWritingBuffer) {
					// already drained fPlayingBuffer, so fWritingBuffer was also drained
					if ((((int32)fPlayingBuffer->trackOffset) - bytesLeft) > 0) {
						fPlayingBuffer->trackOffset -= bytesLeft;
						fPlayingBuffer->playedBytes -= bytesLeft;
						bytesLeft = 0;				
					} else {
						bytesLeft -= fPlayingBuffer->trackOffset;
						skipBack = true;
					}
				} else {
					// must move writing buffer back to playing buffer
					if (fWritingBuffer != NULL) {
						delete fWritingBuffer;
					}
					fWritingBuffer = fPlayingBuffer;
					fWritingBuffer->doneWriting = false;
					if ((((int32)fWritingBuffer->trackOffset) - bytesLeft) > 0) {
						fWritingBuffer->trackOffset -= bytesLeft;
						fWritingBuffer->playedBytes -= bytesLeft;
						bytesLeft = 0;
					} else {
						bytesLeft -= fWritingBuffer->trackOffset;
						skipBack = true;
					}					
				}
				
				if (skipBack) {
					// must move back to previous track
					CDTrack *prevTrack = CDTrack::FindPrevious(fHeadTrack, fWritingBuffer->track, true);
					if (prevTrack == NULL) {
						prevTrack = fHeadTrack;
					}
					if (prevTrack != NULL) {
						uint32 trackLen = prevTrack->Length() * prevTrack->FrameSize();
						fWritingBuffer = new PlayBuffer(prevTrack, trackLen - bytesLeft, fBufferSize);
						release_sem_etc(fWritingBuffer->readSem, 1, 0);
					} else {
						fWritingBuffer = NULL;
					}
					delete fPlayingBuffer;
					fPlayingBuffer = fWritingBuffer;
					trackChanged = true;			
					skipBack = false;
				}

			} while ((bytesLeft > 0) && (fWritingBuffer != NULL));

			if (trackChanged && fNotify && fNotify->Looper()) {
				BMessage *msg = CreateTrackPlayingMessage((fPlayingBuffer != NULL) ? fPlayingBuffer->track : NULL, true);
				fNotify->Looper()->PostMessage(msg, fNotify);
				delete msg;
			}
		}
		
		// release the locks in opposite order of acquire
		if (writeLocked) { fWriteBuffLock.Unlock(); }
		if (playLocked) { fPlayBuffLock.Unlock(); }		
	} else { // !fIsplaying
		//printf("AudioManager::SeekBack() while not playing unimplemented!\n");		
	}
}

void AudioManager::SkipForward()
{
	CDTrack *nextTrack;
	if (fIsPlaying) {
		if (fPlayBuffLock.Lock()) {
			if (fPlayingBuffer != NULL) {
				nextTrack = fPlayingBuffer->track->Next();
				if (nextTrack == NULL) {
					nextTrack = fHeadTrack;
				}
				SetPlayingTrack(nextTrack);
			}
			fPlayBuffLock.Unlock();	
		}
	} else {
		nextTrack = fStartPlayingTrack;
		if (nextTrack != NULL) {
			nextTrack = nextTrack->Next();
		}
		if (nextTrack == NULL) {
			nextTrack = fHeadTrack;
		}
		fStartPlayingTrack = nextTrack;
//		SetCurrentTrack(nextTrack);
	}
}

void AudioManager::SkipBack()
{
	if (fPlayBuffLock.Lock()) {
		if (fPlayingBuffer != NULL) {
			// find previous track
			CDTrack *currTrack = fPlayingBuffer->track;
			fPlayBuffLock.Unlock();
			CDTrack *prevTrack = fHeadTrack;
			if (currTrack == fHeadTrack) {
				while (prevTrack->Next() != NULL) {
					prevTrack = prevTrack->Next();
				}
			} else {
				while (prevTrack && (prevTrack->Next() != currTrack)) {
					prevTrack = prevTrack->Next();
				}
			}
			SetPlayingTrack(prevTrack);
		} else {
			fPlayBuffLock.Unlock();
		}
	}
}

BMessage *AudioManager::CreateTrackPlayingMessage(CDTrack *track, bool isPlaying)
{
	BMessage *msg = new BMessage(kTrackPlayingMessage);
	if (track != NULL) {
		msg->AddInt32("track", track->Index());
		uint32 dataGap, blankGap;
		track->PreGap(&dataGap, &blankGap);
		msg->AddInt32("pregap_data", dataGap);
		msg->AddInt32("pregap_empty", blankGap);
	} else {
		msg->AddInt32("track", 0);
	}
	msg->AddBool("playing", isPlaying);
	return msg;
}

// ------------------------------ CDButton --------------------------------

// CDButton is a TransportButton that only allows keyboard invokes if the
// TrackListView has the focus.

class CDButton : public TransportButton
{
public:
	CDButton(BRect frame, const char *name,
		const unsigned char *normalBits,
		const unsigned char *pressedBits,
		const unsigned char *disabledBits,
		BMessage *invokeMessage,			// done pressing over button
		BMessage *startPressingMessage = 0, // just clicked button
		BMessage *pressingMessage = 0, 		// periodical still pressing
		BMessage *donePressing = 0, 		// tracked out of button/didn't invoke
		bigtime_t period = 0,				// pressing message period
		uint32 key = 0,						// optional shortcut key
		uint32 modifiers = 0,				// optional shortcut key modifier
		uint32 resizeFlags = B_FOLLOW_LEFT | B_FOLLOW_TOP);
	virtual ~CDButton();
	
protected:

	virtual bool IsInvokableByShortcutKey();

};

class CDPlayPauseButton : public PlayPauseButton
{
public:
	CDPlayPauseButton(BRect frame, const char *name,
		const unsigned char *normalBits,
		const unsigned char *pressedBits,
		const unsigned char *disabledBits,
		const unsigned char *normalPlayingBits,
		const unsigned char *pressedPlayingBits,
		const unsigned char *normalPausedBits,
		const unsigned char *pressedPausedBits,
		BMessage *invokeMessage,			// done pressing over button
		uint32 key = 0,						// optional shortcut key
		uint32 modifiers = 0,				// optional shortcut key modifier
		uint32 resizeFlags = B_FOLLOW_LEFT | B_FOLLOW_TOP);
	virtual ~CDPlayPauseButton();
	
protected:

	virtual bool IsInvokableByShortcutKey();

};

CDButton::CDButton(BRect frame,
					const char *name,
					const unsigned char *normalBits,
					const unsigned char *pressedBits,
					const unsigned char *disabledBits,
					BMessage *invokeMessage,
					BMessage *startPressingMessage,
					BMessage *pressingMessage,
					BMessage *donePressing,
					bigtime_t period,
					uint32 key,
					uint32 modifiers,
					uint32 resizeFlags)
	: TransportButton(frame, name, normalBits, pressedBits, disabledBits, invokeMessage,
		startPressingMessage, pressingMessage, donePressing, period, key, modifiers, resizeFlags)
{
	SetFlags(Flags() & ~B_NAVIGABLE);
}


CDButton::~CDButton()
{
}

// returns true if no view has focus or if the focused view is a TrackListView
bool CDButton::IsInvokableByShortcutKey()
{
	BWindow *win = Window();
	BView *focused = win->CurrentFocus();
	if (focused == NULL) {
		return true;
	} else {
		return (dynamic_cast<TrackListView *>(focused) != NULL);
	}
}


CDPlayPauseButton::CDPlayPauseButton(BRect frame,
									const char *name,
									const unsigned char *normalBits,
									const unsigned char *pressedBits,
									const unsigned char *disabledBits,
									const unsigned char *normalPlayingBits,
									const unsigned char *pressedPlayingBits,
									const unsigned char *normalPausedBits,
									const unsigned char *pressedPausedBits,
									BMessage *invokeMessage,
									uint32 key,
									uint32 modifiers,
									uint32 resizeFlags)
	: PlayPauseButton(frame, name, normalBits, pressedBits, disabledBits, normalPlayingBits,
		pressedPlayingBits, normalPausedBits, pressedPausedBits, invokeMessage, key,
		modifiers, resizeFlags)
{
	SetFlags(Flags() & ~B_NAVIGABLE);
}


CDPlayPauseButton::~CDPlayPauseButton()
{
}

// returns true if no view has focus or if the focused view is a TrackListView
bool CDPlayPauseButton::IsInvokableByShortcutKey()
{
	BWindow *win = Window();
	BView *focused = win->CurrentFocus();
	if (focused == NULL) {
		return true;
	} else {
		return (dynamic_cast<TrackListView *>(focused) != NULL);
	}
}

// ---------------------------- CDPlayerView ------------------------------

CDPlayerView::CDPlayerView(BRect frame, uint32 resizingMode)
	: BView(frame, CDPLAYERVIEWNAME, resizingMode, 0)/*, cdMsgFilter(NULL) */
{
	fConstructed = false;
	// Most construction is done in AttachedToWindow()
}


CDPlayerView::~CDPlayerView()
{
	//	all allocated objects are deleted in DetachedFromWindow
}

void CDPlayerView::SetCurrentTrack(int32 track, bool isPlaying)
{
	if (fIsPlaying != isPlaying) {
		// enable / disable edit controls 
		EditWidget *editWidget = dynamic_cast<EditWidget *>(fWindow->FindView("EditWidget"));
		if (editWidget != NULL) {
			editWidget->SetEnabled(!isPlaying);
		}
		TrackEditView *editView = dynamic_cast<TrackEditView *>(fWindow->FindView("TrackEditView"));
		if (editView != NULL) {
			editView->SetEnabled(!isPlaying);
		}
		BurnControlView *burnView = dynamic_cast<BurnControlView *>(fWindow->FindView("BurnControlView"));
		if (burnView != NULL) {
			burnView->GetBurnButton()->SetEnabled(!isPlaying && burnView->CanBurn());
		}
		TrackListView *tlView = dynamic_cast<TrackListView *>(fWindow->FindView("TrackListView"));
		if (tlView != NULL) {
			tlView->SetEditEnabled(!isPlaying);
		}
		fIsPlaying = isPlaying;
	}
	// change track label
	char buf[4];
	if (track) {
		sprintf(buf, "%02ld", track);
	} else {
		sprintf(buf, "--");
	}
	fTrackNumber->SetText(buf);
}


void CDPlayerView::MessageReceived(BMessage *message)
{
	TrackListView *view;
	CDTrack *track;
	bool isPlaying = false;
	
	switch (message->what) {
	case kVolumeChangeMessage:		// fall through
	case kPlayButtonMessage:		// fall through
	case kStopButtonMessage:		// fall through
	case kSeekForwardButtonMessage:	// fall through
	case kSeekBackButtonMessage:	// fall through
	case kSkipForwardButtonMessage:	// fall through
	case kSkipBackButtonMessage:
		// add button messages to the AudioManager's message queue
		Window()->DetachCurrentMessage();
		fManager->AddMessage(message);
		break;
	case kTrackPlayingMessage:
		if (message->FindInt32("track", &fPlayingIndex) == B_OK) {
			message->FindBool("playing", &isPlaying);
			if (fPlayingIndex == 0) {
				// tell audio manager to stop playing
				fManager->AddMessage(new BMessage(kStopButtonMessage));
			}
			SetCurrentTrack(fPlayingIndex, isPlaying);
		} else {
			fPlayingIndex = 0;
		}
		break;
	case kPlayerPausedMessage:
		{
			bool isPaused = message->FindBool("paused");
			if (isPaused) {
				fPlayButton->SetPaused();
			} else {
				fPlayButton->SetPlaying();
			}
		}
		break;
	case kPlayerStoppedMessage:
		fPlayButton->SetStopped();
		SetCurrentTrack(0, false);
		break;
	case kTrackInvokedMessage:
		// the message was caused by the user invoking the TrackListView
		if (message->FindPointer("tracklist", (void **)&view) == B_OK) {
			if (fIsPlaying) {
				TrackRow *row = dynamic_cast<TrackRow *>(view->CurrentSelection());
				if (row != NULL && row->Track()->Index() == fPlayingIndex) {
					// invoked track is currently playing, so pause
					fManager->AddMessage(new BMessage(kPlayButtonMessage));
				} else {
					// invoked track is not playing, so stop playing, then start again
					fManager->AddMessage(new BMessage(kStopButtonMessage));
					fManager->AddMessage(new BMessage(kPlayButtonMessage));
				}
			} else {
				// not currently playing, so play
				fManager->AddMessage(new BMessage(kPlayButtonMessage));
			}
		}
		break;
	case BurnerWindow::TRACK_ADDED:		// fall through
	case BurnerWindow::TRACK_DELETED:	// fall through
	case BurnerWindow::TRACK_MOVED:
		if (message->FindPointer("tracklist", (void **)&view) == B_OK) {
			track = view->GetTrackList();
			bool enableButtons = (track != NULL);
			if (fPlayButton->IsEnabled() != enableButtons) {
				SetControlsEnabled(enableButtons);
			}
			fManager->TrackListChanged(track);
		}
//		SetCurrentTrack(0, false);
		break;
	case BurnerWindow::TRACK_SELECTION_CHANGED:
		// add selection changed message to the AudioManager's message queue
		Window()->DetachCurrentMessage();
		fManager->AddMessage(message);
		break;
	case kUpdateTimeMessage:
		if (fTrackTime != NULL) {
			fTrackTime->SetValue(fManager->GetPlayingOffset() / 2352); // 2352 bytes per frame
		}
		break;
	default:
		BView::MessageReceived(message);
		break;
	}
}


void CDPlayerView::AttachedToWindow()
{
	fWindow = dynamic_cast<BurnerWindow *>(Window());
	if (fWindow) {
		fWindow->AddTrackListener(this);
	}

	if (!fConstructed) {
		BRect frame(Frame());
		fManager = new AudioManager(this, kPreviewBufferSize);
		fAudioManagerThread = new BThread(fManager, "CDBurner Buffer Thread");	
		fAudioManagerThread->Resume();

		BRect boxRect(frame);
		boxRect.OffsetTo(0, 0);
		BBox *box = new BBox(boxRect, "preview box", B_FOLLOW_ALL);
		box->SetLabel("Preview");
		float BORDER = 3;
		BRect rect(BORDER, BORDER, BORDER + 25, BORDER + 18);
		rect.OffsetBy(3, 12);
		SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);

		rect.right = rect.left + 20;
		fTrackNumber = new BStringView(rect, NULL, "--");
		fTrackNumber->SetAlignment(B_ALIGN_CENTER);
		BFont font;
		fTrackNumber->GetFont(&font);
		font.SetSize(14);
		font.SetFace(B_BOLD_FACE);
		fTrackNumber->SetFont(&font);
		box->AddChild(fTrackNumber);

		rect = fTrackNumber->Frame();
		rect.left = rect.right + 5;
		rect.right = rect.left + 56;
		fTrackTime = new TimeDisplay(rect, "TimeDisplay", TimeDisplay::TIME_MINUTES,
										TimeDisplay::TIME_FRAMES, true,
										B_FOLLOW_LEFT | B_FOLLOW_TOP);
		box->AddChild(fTrackTime);
		BMessenger messenger(this, fWindow);
		BMessage updateMsg(kUpdateTimeMessage);
		// construct a BMessageRunner to send an update message every 1/10 second
		fTimeRunner = new BMessageRunner(messenger, &updateMsg, 100000);
		
		// construct the buttons
const unsigned char *kPressedPlayButtonBitmapBits = kPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPlayingPlayButtonBitmapBits = kPressedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPressedPlayingPlayButtonBitmapBits = kPlayingPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPausedPlayButtonBitmapBits = kPressedPlayingPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPressedPausedPlayButtonBitmapBits = kPausedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kDisabledPlayButtonBitmapBits = kPressedPausedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;

const unsigned char *kPressedStopButtonBitmapBits = kStopButtonBitmapBits + kStopBitmapWidth * kStopBitmapHeight;
const unsigned char *kDisabledStopButtonBitmapBits = kPressedStopButtonBitmapBits + kStopBitmapWidth * kStopBitmapHeight;

const unsigned char *kPressedSeekBackBitmapBits = kSeekBackBitmapBits + kSeekBitmapWidth * kSeekBitmapHeight;
const unsigned char *kSeekingSeekBackBitmapBits = kPressedSeekBackBitmapBits + kSeekBitmapWidth * kSeekBitmapHeight;
const unsigned char *kPressedSeekingSeekBackBitmapBits = kSeekingSeekBackBitmapBits + kSeekBitmapWidth * kSeekBitmapHeight;
const unsigned char *kDisabledSeekBackBitmapBits = kPressedSeekingSeekBackBitmapBits + kSeekBitmapWidth * kSeekBitmapHeight;

const unsigned char *kPressedSeekForwardBitmapBits = kSeekForwardBitmapBits + kSeekBitmapWidth * kSeekBitmapHeight;
const unsigned char *kSeekingSeekForwardBitmapBits = kPressedSeekForwardBitmapBits + kSeekBitmapWidth * kSeekBitmapHeight;
const unsigned char *kPressedSeekingSeekForwardBitmapBits = kSeekingSeekForwardBitmapBits + kSeekBitmapWidth * kSeekBitmapHeight;
const unsigned char *kDisabledSeekForwardBitmapBits = kPressedSeekingSeekForwardBitmapBits + kSeekBitmapWidth * kSeekBitmapHeight;

const unsigned char *kPressedSkipForwardButtonBitmapBits = kSkipForwardButtonBitmapBits + kSkipBitmapWidth * kSkipBitmapHeight;
const unsigned char *kDisabledSkipForwardButtonBitmapBits = kPressedSkipForwardButtonBitmapBits + kSkipBitmapWidth * kSkipBitmapHeight;

const unsigned char *kPressedSkipBackButtonBitmapBits = kSkipBackButtonBitmapBits + kSkipBitmapWidth * kSkipBitmapHeight;
const unsigned char *kDisabledSkipBackButtonBitmapBits = kPressedSkipBackButtonBitmapBits + kSkipBitmapWidth * kSkipBitmapHeight;

const float kButtonSpacing = 3;

		rect = fTrackTime->Frame();
		rect.OffsetBy(rect.IntegerWidth() + 5, 0);
		rect.SetRightBottom(rect.LeftTop() + kSkipButtonSize);
		fSkipBackButton = new CDButton(rect, "SkipBack", kSkipBackButtonBitmapBits,
							kPressedSkipBackButtonBitmapBits, kDisabledSkipBackButtonBitmapBits,
							new BMessage(kSkipBackButtonMessage));
		box->AddChild(fSkipBackButton);
		
		rect.OffsetTo(rect.right + kButtonSpacing, rect.top);
		rect.SetRightBottom(rect.LeftTop() + kSeekButtonSize);
		fSeekBackButton = new CDButton(rect, "SeekBack", kSeekBackBitmapBits,
							kPressedSeekingSeekBackBitmapBits, kDisabledSeekBackBitmapBits,
							new BMessage(kSeekBackButtonMessage), NULL, new BMessage(kSeekBackButtonMessage),
							NULL, 200000, B_LEFT_ARROW); // send message every .2 second
		box->AddChild(fSeekBackButton);

		rect.OffsetTo(rect.right + kButtonSpacing, rect.top);
		rect.SetRightBottom(rect.LeftTop() + kStopButtonSize);
		fStopButton = new CDButton(rect, "Stop", kStopButtonBitmapBits,
							kPressedStopButtonBitmapBits, kDisabledStopButtonBitmapBits,
							new BMessage(kStopButtonMessage), NULL, NULL, NULL, 0, B_ESCAPE);
		box->AddChild(fStopButton);

		rect.OffsetTo(rect.right + kButtonSpacing, rect.top);
		rect.SetRightBottom(rect.LeftTop() + kPlayButtonSize);
		fPlayButton = new CDPlayPauseButton(rect, "Play", kPlayButtonBitmapBits,
							kPressedPlayButtonBitmapBits, kDisabledPlayButtonBitmapBits,
							kPlayingPlayButtonBitmapBits, kPressedPlayingPlayButtonBitmapBits,
							kPausedPlayButtonBitmapBits, kPressedPausedPlayButtonBitmapBits,
							new BMessage(kPlayButtonMessage), B_SPACE);
		box->AddChild(fPlayButton);

		rect.OffsetTo(rect.right + kButtonSpacing, rect.top);
		rect.SetRightBottom(rect.LeftTop() + kSeekButtonSize);
		fSeekForwardButton = new CDButton(rect, "SeekForward", kSeekForwardBitmapBits,
							kPressedSeekingSeekForwardBitmapBits, kDisabledSeekForwardBitmapBits,
							new BMessage(kSeekForwardButtonMessage), NULL, new BMessage(kSeekForwardButtonMessage),
							NULL, 200000, B_RIGHT_ARROW); // send message every .2 second
		box->AddChild(fSeekForwardButton);

		rect.OffsetTo(rect.right + kButtonSpacing, rect.top);
		rect.SetRightBottom(rect.LeftTop() + kSkipButtonSize);
		fSkipForwardButton = new CDButton(rect, "SkipForward", kSkipForwardButtonBitmapBits,
							kPressedSkipForwardButtonBitmapBits, kDisabledSkipForwardButtonBitmapBits,
							new BMessage(kSkipForwardButtonMessage));
		box->AddChild(fSkipForwardButton);

		rect.left = rect.right + 8;
		rect.top += 4;
		rect.bottom = rect.top + 10;
		rect.right = rect.left + 85;
		fVolumeSlider = new TVolumeSlider(rect, new BMessage(kVolumeChangeMessage), 0, kMaxVolume,
								TVolumeSlider::NewVolumeWidget());
		fVolumeSlider->SetModificationMessage(new BMessage(kVolumeChangeMessage));
		box->AddChild(fVolumeSlider);
		fVolumeSlider->ResizeToPreferred();
		fVolumeSlider->SetPosition(1.0);
		
		fSkipBackButton->SetTarget(this);
		fSkipForwardButton->SetTarget(this);
		fSeekBackButton->SetTarget(this);
		fSeekForwardButton->SetTarget(this);
		fStopButton->SetTarget(this);
		fPlayButton->SetTarget(this);
		
		SetControlsEnabled(false);
		
		fVolumeSlider->SetTarget(this);
		
		AddChild(box);
		fIsPlaying = false;
		fConstructed = true;
	}
}

// Set the view color of the track label properly
void CDPlayerView::AllAttached()
{
	fTrackNumber->SetHighColor(0, 255, 0);
	fTrackNumber->SetViewColor(0, 0, 0);
	fTrackNumber->SetLowColor(0, 0, 0);
}

void CDPlayerView::SetControlsEnabled(bool enabled)
{
	fSkipBackButton->SetEnabled(enabled);
	fSkipForwardButton->SetEnabled(enabled);
	fSeekBackButton->SetEnabled(enabled);
	fSeekForwardButton->SetEnabled(enabled);
	fStopButton->SetEnabled(enabled);
	fPlayButton->SetEnabled(enabled);
	fVolumeSlider->SetEnabled(enabled);	
}


void CDPlayerView::DetachedFromWindow()
{
	fWindow->RemoveTrackListener(this);
	fWindow = NULL;
	fConstructed = false;
	// shut down audio manager thread
	fManager->Quit();
	fAudioManagerThread->Wait();
	delete fManager;
	delete fAudioManagerThread;
	delete fTimeRunner;
//	delete fPlayButton;
//	delete fStopButton;
//	delete fSeekForwardButton;
//	delete fSkipForwardButton;
//	delete fSeekBackButton;
//	delete fSkipBackButton;
}
