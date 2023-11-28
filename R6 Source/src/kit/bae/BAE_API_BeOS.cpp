/*****************************************************************************/
/*
**	HAE_API_BeOS.c
**
**	This provides platform specfic functions for BeOS. This interface
**	for HAE uses a BSoundPlayer to play the audio.
**
**	Copyright 2000 Be, Inc, All Rights Reserved.
**	Written by Jon Watte
**
**	Not based on a previous implementation which carried the legend:
**	Beatnik products contain certain trade secrets and confidential and
**	proprietary information of Beatnik.  Use, reproduction, disclosure
**	and distribution by any means are prohibited, except pursuant to
**	a written license from Beatnik. Use of copyright notice is
**	precautionary and does not imply publication or disclosure.
**
**	Restricted Rights Legend:
**	Use, duplication, or disclosure by the Government is subject to
**	restrictions as set forth in subparagraph (c)(1)(ii) of The
**	Rights in Technical Data and Computer Software clause in DFARS
**	252.227-7013 or subparagraphs (c)(1) and (2) of the Commercial
**	Computer Software--Restricted Rights at 48 CFR 52.227-19, as
**	applicable.
**
**	Confidential-- Internal use only
**
**	History	-
**
**	2000-01-17	Created new by ripping out all old code
**
*/
/*****************************************************************************/

#include "BAE_API.h"

#include <kernel/OS.h>
#include <media/SoundPlayer.h>
#include <support/ByteOrder.h>
#include <support/Locker.h>
#include <support/Autolock.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "GenPriv.h"

#if DEBUG
	#define FPRINTF fprintf
#else
	static inline void FPRINTF(FILE*, const char*, ...) { }
#endif

//	Define this to how many percent of available CPU time is
//	OK to use to generate MIDI content, before the overload
//	hook is called (if any).
//	Warning: every buffer that takes more than this to generate
//	will be somewhat snooze()-ed to give some CPU to non-real-
//	time threads, so it'll sound bad.
#define LOAD_PERC 60

#define MAX_FILES 32
static BLocker fileTableLock("BAE: File Table Lock");
static FILE * fileTable[MAX_FILES];
static char * fileNames[MAX_FILES];
static BLocker soundPlayerLock("BAE: Sound Player Lock");
static BSoundPlayer * bsp;
static media_raw_audio_format fmt;
static int32 framesPlayed;


static long AddToFileTableOrClose(FILE * f, const char * name)
{
	if (!f) return -1;
	BAutolock lock(fileTableLock);
	for (int ix=0; ix<MAX_FILES; ix++) {
		if (!fileTable[ix]) {
			fileTable[ix] = f;
			fileNames[ix] = strdup(name);
			return ix+1;
		}
	}
	fclose(f);
	return -1;
}

static FILE * FileTableAt(long ix)	//	this is a thread-unsafe reference returned...
{
	if (ix < 1 || ix > MAX_FILES) return NULL;
	return fileTable[ix-1];
}

static long CloseFileTableAt(long ix)
{
	if (ix < 1 || ix > MAX_FILES) return -1;
	BAutolock lock(fileTableLock);
	fclose(fileTable[ix-1]);
	fileTable[ix-1] = 0;
	free(fileNames[ix-1]);
	fileNames[ix=1] = 0;
	return 0;
}


// HAE_Setup()
// -----------
// Setup function. Called before memory allocation, or anything serious. Can be used to 
// load a DLL, library, etc.
// return 0 for ok, or -1 for failure
int HAE_Setup(void)
{
	return 0;
}

// HAE_Cleanup()
// -------------
// Cleanup function. Called after all memory and the last buffers are deallocated, and
// audio hardware is released. Can be used to unload a DLL, library, etc.
// return 0 for ok, or -1 for failure
int HAE_Cleanup(void)
{
	return 0;
}

// HAE_Allocate()
// --------------
// allocate a block of locked, zeroed memory. Return a pointer
void * HAE_Allocate(unsigned long size)
{
thread_info info;
get_thread_info(find_thread(NULL), &info);
if (info.priority >= 100) {
static int cnt = 0;
	if (cnt++ > 100) {
		debugger("Allright, that's enough!");
	}
}
	void * ret = malloc(size+sizeof(long));
	if (ret) {
		memset(ret, 0, size+sizeof(long));
		*(long *)ret = size;
	}
	return ret ? ((char *)ret)+sizeof(long) : NULL;
}

// HAE_Dellocate()
// ---------------
// dispose of memory allocated with HAE_Allocate
void HAE_Deallocate(void *memoryBlock)
{
	if (memoryBlock) {
		free(((char *)memoryBlock)-sizeof(long));
	}
}

// HAE_IsBadReadPointer()
// ----------------------
// Given a memory pointer and a size, validate of memory pointer is a valid memory address
// with at least size bytes of data avaiable from the pointer.
// This is used to determine if a memory pointer and size can be accessed without 
// causing a memory protection
// fault.
// return 0 for valid, or 1 for bad pointer, or 2 for not supported. 
int HAE_IsBadReadPointer(void */*memoryBlock*/, unsigned long /*size*/)
{
	return 2;	// not supported, so this assumes that we don't have memory protection and will
				// not get an access violation when accessing memory outside of a valid memory block
}

// HAE_SizeOfPointer()
// -------------------
// this will return the size of the memory pointer allocated with HAE_Allocate. Return
// 0 if you don't support this feature
unsigned long HAE_SizeOfPointer(void * memoryBlock)
{
	return ((long *)memoryBlock)[-1];
}

// HAE_BlockMove()
// ---------------
// block move memory. This is basically memcpy, but it's exposed to take advantage of
// special block move speed ups, various hardware has available.
void HAE_BlockMove(void * source, void * dest, unsigned long size)
{
	if (source && dest && size) {
		memmove(dest, source, size);
	}
}


// ---------------------------------------------------------------
// -- TIMING SERVICES
// ---------------------------------------------------------------
#if 0
	#pragma mark -
	#pragma mark TIMING SERVICES
#endif

// HAE_Microseconds
// ----------------
// return microseconds
unsigned long HAE_Microseconds(void)
{
	static bigtime_t betick = 0;

	if (betick == 0) {
		betick = system_time();
	}
	return (unsigned long) (system_time() - betick);
}

// HAE_WaitMicroseocnds()
// ----------------------
// wait or sleep this thread for this many microseconds
void HAE_WaitMicroseocnds(unsigned long waitAmount)
{
	snooze(waitAmount);		// wait microseconds
}


// ---------------------------------------------------------------
// -- FILE SUPPORT
// ---------------------------------------------------------------
#if 0
	#pragma mark -
	#pragma mark FILE SYSTEM SERVICES
#endif


// HAE_CopyFileNameNative()
// ------------------------
// Given the fileNameSource that comes from the call HAE_FileXXXX, copy the name
// in the native format to the pointer passed fileNameDest.
void HAE_CopyFileNameNative(void *fileNameSource, void *fileNameDest)
{
	strcpy((char *)fileNameDest, (const char *)fileNameSource);
}


// HAE_FileCreate()
// ----------------
// Create a file, delete orignal if duplicate file name.
// Return -1 if error
long HAE_FileCreate(void *fileName)
{
	if (fileName) {
		int fd = open((const char *)fileName, O_RDWR | O_CREAT | O_TRUNC, 0666);
		if (fd >= 0) {
			close(fd);
			return 0;
		}
	}
	return -1L;
}


// HAE_FileDelete()
// ----------------
// Return -1 if error, 0 if ok.
long HAE_FileDelete(void *fileName)
{
	return unlink((const char *)fileName) < 0 ? -1 : 0;
}

// HAE_FileOpenForRead()
// ---------------------
// Open a file
// Return -1 if error, otherwise file handle
long HAE_FileOpenForRead(void *fileName)
{
	if (fileName) {
		return AddToFileTableOrClose(fopen((const char *)fileName, "r"), (const char *)fileName);
	}
	return -1L;
}

// HAE_FileOpenForWrite()
// ----------------------
//
long HAE_FileOpenForWrite(void *fileName)
{
	if (fileName) {
		return AddToFileTableOrClose(fopen((const char *)fileName, "r+"), (const char *)fileName);
	}
	return -1L;
}

// HAE_FileOpenForReadWrite()
// --------------------------
//
long HAE_FileOpenForReadWrite(void *fileName)
{
	if (fileName) {
		return AddToFileTableOrClose(fopen((const char *)fileName, "r+"), (const char *)fileName);
	}
	return -1L;
}

// HAE_FileClose()
// ---------------
// Close a file
void HAE_FileClose(long fileReference)
{
	(void)CloseFileTableAt(fileReference);
}

// HAE_ReadFile()
// --------------
// Read a block of memory from a file.
// Return -1 if error, otherwise length of data read.
long HAE_ReadFile(long fileReference, void *pBuffer, long bufferLength)
{
	FILE * f = FileTableAt(fileReference);
	if (f == 0) return -1;
	return fread(pBuffer, 1, bufferLength, f);
}

// HAE_WriteFile()
// ---------------
// Write a block of memory from a file
// Return -1 if error, otherwise length of data written.
long HAE_WriteFile(long fileReference, void *pBuffer, long bufferLength)
{
	FILE * f = FileTableAt(fileReference);
	if (f == 0) return -1;
	return fwrite(pBuffer, 1, bufferLength, f);
}

// HAE_SetFilePosition()
// ---------------------
// set file position in absolute file byte position
// Return -1 if error, otherwise 0.
long HAE_SetFilePosition(long fileReference, unsigned long filePosition)
{
	FILE * f = FileTableAt(fileReference);
	if (f == 0) return -1;
	return fseek(f, filePosition, 0);
}

// HAE_GetFilePosition()
// ---------------------
// get file position in absolute file bytes
unsigned long HAE_GetFilePosition(long fileReference)
{
	FILE * f = FileTableAt(fileReference);
	if (f == 0) return 0;
	return ftell(f);
}

// HAE_GetFileLength()
// -------------------
// get length of file
unsigned long HAE_GetFileLength(long fileReference)
{
	FILE * f = FileTableAt(fileReference);
	if (f == 0) {
		return 0;
	}
	long pos = ftell(f);
	fseek(f, 0, 2);
	long size = ftell(f);
	fseek(f, pos, 0);
	return size;
}

// HAE_SetFileLength()
// -------------------
// set the length of a file. Return 0, if ok, or -1 for error
int HAE_SetFileLength(long fileReference, unsigned long newSize)
{
	FILE * f = FileTableAt(fileReference);
	if (f == 0) return -1;
	fflush(f);
	int err = ftruncate(fileno(f), newSize);
	if (err < 0) return -1;
	rewind(f);	//	try to flush the FILE buffer
	fseek(f, 0, 2);
	return 0;
}


// ---------------------------------------------------------------
// -- PROCESS THREAD SERVICES
// ---------------------------------------------------------------
#if 0
	#pragma mark -
	#pragma mark PROCESS THREAD SERVICES
#endif

//int HAE_CreateFrameThread(void* context, HAE_FrameThreadProc proc)
//{
//}
//
//int HAE_DestroyFrameThread(void* context)
//{
//}
//
//int HAE_SleepFrameThread(void* context, long msec)
//{
//}


// ---------------------------------------------------------------
// -- AUDIO OUTPUT DEVICE ALLOCATION SERVICES
// ---------------------------------------------------------------
#if 0
	#pragma mark -
	#pragma mark AUDIO OUTPUT DEVICE ALLOCATION SERVICES
#endif


static char tempData[MAX_CHUNK_SIZE*4];
static int tempFrames = 0;
bool _bae_overload = false;
void (*_bae_overloadHook)(void * cookie);
void * _bae_overloadCookie;

static void
BSP_PlayBuffer(
	void * context,
	void * buffer,
	size_t size,
	const media_raw_audio_format & f)
{
bigtime_t start = system_time();
	int frs = size/((f.format & 0xf)*f.channel_count);
	framesPlayed += frs;
	int msps = HAE_GetMaxSamplePerSlice();
	int bpf = (f.format & 0xf)*f.channel_count;
	if (frs < msps) {
		//	we need to re-buffer from the synth
		if (tempFrames <= 0) {
			HAE_BuildMixerSlice(context, tempData, msps*bpf, msps);
			tempFrames += msps;
		}
		memcpy(buffer, &tempData[(msps-tempFrames)*bpf], bpf*frs);
		tempFrames -= frs;
	}
	else while (frs > 0) {
		//	we generate more than one slice at a time
		HAE_BuildMixerSlice(context, buffer, size, msps);
		int d = msps*bpf;
		buffer = ((char *)buffer)+d;
		size -= d;
		frs -= msps;
	}
bigtime_t stop = system_time();
	//	don't allow more than X% of time to go to generating (fail-safe)
	if (stop-start > bigtime_t(LOAD_PERC*10000.0*(f.buffer_size/bpf)/f.frame_rate)) {
#if DEBUG
		debugger("Generating took too long!");
#endif
		if (!_bae_overload) {
			_bae_overload = true;
			void (*func)(void *) = _bae_overloadHook;
			if (func) {
				(*func)(_bae_overloadCookie);
			}
		}
		snooze(10000LL);
	}
}


// HAE_AquireAudioCard()
// ---------------------
// Aquire and enabled audio card
// return 0 if ok, -1 if failed
int HAE_AquireAudioCard(void *context, unsigned long sampleRate, unsigned long channels, unsigned long bits)
{
	BAutolock lock(soundPlayerLock);
	if (bsp) return -1;
	media_raw_audio_format fmt(media_raw_audio_format::wildcard);
	fmt.frame_rate = sampleRate;
	fmt.channel_count = channels;
	fmt.byte_order = B_MEDIA_HOST_ENDIAN;
	fmt.format = (bits == 8) ? 0x11 : 0x2;
#if BAE_WORKAROUND_FOR_DOOM
	extern bool MIDIisInitializingWorkaroundForDoom;
	MIDIisInitializingWorkaroundForDoom=true;
#endif
	bsp = new BSoundPlayer(&fmt, "Soft Synth", BSP_PlayBuffer, 0, context);
#if BAE_WORKAROUND_FOR_DOOM
	MIDIisInitializingWorkaroundForDoom=false;
#endif
	if (bsp->InitCheck() < 0) {
fprintf(stderr, "BSoundPlayer::InitCheck(): %s\n", strerror(bsp->InitCheck()));
		delete bsp;
		bsp = 0;
		return -1;
	}
	memset(tempData, 0, sizeof(tempData));
	bsp->Start();
	fmt = bsp->Format();
	return 0;
}

// HAE_ReleaseAudioCard()
// ----------------------
// Release and free audio card.
// return 0 if ok, -1 if failed.
static BLocker sQuitLock("BAE QuitLock");

int HAE_ReleaseAudioCard(void *context)
{
//	This lock causes a deadlock with the player thread, because
//	HAE calls into our other functions below (which lock)
//	BAutolock lock(soundPlayerLock);
//	However, the only use for the lock is to guard the deletion
//	and assignment of bsp, which we can do in antoher way.

	sQuitLock.Lock();	//	don't allow more than one quit at the same time
	BSoundPlayer * pp = bsp;
	bsp = 0;	//	from here on, we can suddenly get called in from other places.
	delete pp;
	sQuitLock.Unlock();
	return 0;
}


// ---------------------------------------------------------------
// -- AUDIO OUTPUT DEVICE MANAGEMENT SERVICES
// ---------------------------------------------------------------
#if 0
	#pragma mark -
	#pragma mark AUDIO OUTPUT DEVICE MANAGEMENT SERVICES
#endif

// HAE_MaxDevices()
// ----------------
// number of devices. ie different versions of the HAE connection. DirectSound and waveOut
// return number of devices. ie 1 is one device, 2 is two devices.
// NOTE: This function needs to function before any other calls may have happened.
long HAE_MaxDevices(void)
{
	return 1;
}

// HAE_SetDeviceID()
// -----------------
// set the current device. device is from 0 to HAE_MaxDevices()
// NOTE:	This function needs to function before any other calls may have happened.
//			Also you will need to call HAE_ReleaseAudioCard then HAE_AquireAudioCard
//			in order for the change to take place.
void HAE_SetDeviceID(long /*deviceID*/, void */*deviceParameter*/)
{
	// Do nothing.
}

// HAE_GetDeviceID()
// -----------------
// return current device ID
// NOTE: This function needs to function before any other calls may have happened.
long HAE_GetDeviceID(void */*deviceParameter*/)
{
	return 0;
}

// HAE_GetDeviceName()
// -------------------
// get deviceID name 
// NOTE:	This function needs to function before any other calls may have happened.
//			Format of string is a zero terminated comma delinated C string.
//			"platform,method,misc"
//	example	"MacOS,Sound Manager 3.0,SndPlayDoubleBuffer"
//			"WinOS,DirectSound,multi threaded"
//			"WinOS,waveOut,multi threaded"
//			"WinOS,VxD,low level hardware"
//			"WinOS,plugin,Director"
void HAE_GetDeviceName(long deviceID, char *cName, unsigned long cNameLength)
{
static const char * id = "BeOS,Mixer,BSoundPlayer";

	if (cName && cNameLength) {
		if (deviceID == 0) {
			cName[0] = 0;
		}
		else {
			strncpy(cName, id, cNameLength-1);
		}
		cName[cNameLength-1] = 0;
	}
}


// ---------------------------------------------------------------
// -- AUDIO HARDWARE VOLUME CONTROL SERVICES
// ---------------------------------------------------------------
#if 0
	#pragma mark -
	#pragma mark AUDIO HARDWARE VOLUME CONTROL SERVICES
#endif

// HAE_GetHardwareVolume()
// -----------------------
// returned volume is in the range of 0 to 256
short int HAE_GetHardwareVolume(void)
{
	BAutolock lock(soundPlayerLock);
	if (bsp) {
		return (int)(bsp->Volume()*256);
	}
	return 256;
}

// HAE_SetHardwareVolume()
// -----------------------
// theVolume is in the range of 0 to 256
void HAE_SetHardwareVolume(short int theVolume)
{
	BAutolock lock(soundPlayerLock);
	if (bsp) {
		bsp->SetVolume(theVolume/256.0);
	}
}

// HAE_GetHardwareBalance()
// ------------------------
// returned balance is in the range of -256 to 256. Left to right. If you're hardware doesn't support this
// range, just scale it.
short int HAE_GetHardwareBalance(void)
{
	return 0;
}

// HAE_SetHardwareBalance()
// ------------------------
// 'balance' is in the range of -256 to 256. Left to right. If you're hardware doesn't support this
// range, just scale it.
void HAE_SetHardwareBalance(short int balance)
{
	// Do nothing.
}

// ---------------------------------------------------------------
// -- AUDIO CONFIGURATION INFORMATION SERVICES
// ---------------------------------------------------------------
#if 0
	#pragma mark -
	#pragma mark AUDIO CONFIGURATION INFORMATION SERVICES
#endif

// HAE_IsStereoSupported()
// -----------------------
// Return 1 if stereo hardware is supported, otherwise 0.
int HAE_IsStereoSupported(void)
{
	return 1;
}

// HAE_Is16BitSupported()
// ----------------------
// Return 1, if sound hardware support 16 bit output, otherwise 0.
int HAE_Is16BitSupported(void)
{
	return 1;
}

// HAE_Is8BitSupported()
// ---------------------
// Return 1, if sound hardware support 8 bit output, otherwise 0.
int HAE_Is8BitSupported(void)
{
	return 1;
}

// HAE_GetAudioBufferCount()
// -------------------------
// Return the number of 11 ms buffer blocks that are built at one time.
int HAE_GetAudioBufferCount(void)
{
	int b = (int)floor(1000.0*(fmt.buffer_size/((fmt.format&0xf)*(fmt.channel_count)))/fmt.frame_rate/11.0+0.5);
fprintf(stderr, "numblocks = %d\n", b);	//fixme:	DEBUG
	if (b < 1) b = 1;
	return b;
}

// HAE_GetAudioByteBufferSize()
// ----------------------------
// Return the number of bytes used for audio buffer for output to card
long HAE_GetAudioByteBufferSize(void)
{
	return fmt.buffer_size;
}

// HAE_GetDeviceSamplesPlayedPosition()
// ------------------------------------
// return number of audio frames sent to hardware
unsigned long HAE_GetDeviceSamplesPlayedPosition(void)
{
	BAutolock lock(soundPlayerLock);
	if (!bsp) return 0;
	return framesPlayed;
}

// EOF of BAE_API_BeOS.cpp
