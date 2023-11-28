/*****************************************************************************/
/*
** "BAEMidiInput.h"
**
**	Generalized Audio Synthesis package presented in an oop fashion
**
**	\xA9 Copyright 1996-1997 Beatnik, Inc, All Rights Reserved.
**	Written by Steve Hales
**
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
** Modification History:
**
**	2/9/97		Created
**	2/10/98		Added virtual to ~BAEMidiInput
**
**	6/5/98		Jim Nitchals RIP	1/15/62 - 6/5/98
**				I'm going to miss your irreverent humor. Your coding style and desire
**				to make things as fast as possible. Your collaboration behind this entire
**				codebase. Your absolute belief in creating the best possible relationships 
**				from honesty and integrity. Your ability to enjoy conversation. Your business 
**				savvy in understanding the big picture. Your gentleness. Your willingness 
**				to understand someone else's way of thinking. Your debates on the latest 
**				political issues. Your generosity. Your great mimicking of cartoon voices. 
**				Your friendship. - Steve Hales
**
**	6/8/99		Changed BAEMidiInput::Start to be device based, and added 
**				BAEMidiInput::GetMaxDeviceCount &&  BAEMidiInput::SetCurrentDevice &&
**				BAEMidiInput::GetCurrentDevice && BAEMidiInput::GetDeviceName
**	7/13/99		Renamed HAE to BAE. Renamed BAEAudioMixer to BAEOutputMixer. Renamed BAEMidiDirect
**				to BAEMidiSynth. Renamed BAEMidiFile to BAEMidiSong. Renamed BAERMFFile to BAERmfSong.
**				Renamed BAErr to BAEResult. Renamed BAEMidiExternal to BAEMidiInput. Renamed BAEMod
**				to BAEModSong. Renamed BAEGroup to BAENoiseGroup. Renamed BAEReverbMode to BAEReverbType.
**				Renamed BAEAudioNoise to BAENoise.
**	9/2/99		Added BAEMidiInput::Setup && BAEMidiInput::Cleanup. Start now calls Setup
**
*/
/*****************************************************************************/


#ifndef BAE_AUDIO
	#include "BAE.h"
#endif

#ifndef BAE_MIDI_EXTERNAL
#define BAE_MIDI_EXTERNAL
enum
{
	BAE_MIDI_MANAGER_FAILED = 20000,
	BAE_OMS_FAILED,
	BAE_MIDI_MANGER_NOT_THERE
};

// On Macintosh only
enum
{
	BAE_MIDI_MANAGER_DEVICE = 0,
	BAE_OMS_DEVICE
};

// Midi class for direct midi input from external platform devices
class BAEMidiInput : public BAEMidiSynth
{
public:
					BAEMidiInput(BAEOutputMixer *pBAEOutputMixer,
								char *pName = 0L, void * userReference = 0);
	virtual			~BAEMidiInput();

	// number of midi input devices. ie different versions of the BAE connection. DirectSound and waveOut
	// return number of devices. ie 1 is one device, 2 is two devices.
	// NOTE: This function can be called before Start is called
	long 			GetMaxDeviceCount(void);

	// set current device. should be a number from 0 to GetMaxDeviceCount()
	// deviceParameter is a pointer to device specific info. It will
	// be NULL if there's nothing interesting. For Windows Device 1 (DirectSound) it
	// is the window handle that DirectSound will attach to. Pass NULL, if you don't know
	// what is correct.
	void 			SetCurrentDevice(long deviceID, void *deviceParameter = NULL);

	// get current device. deviceParameter is a pointer to device specific info. It will
	// be NULL if there's nothing interesting. For Windows Device 1 (DirectSound) it
	// is the window handle that DirectSound will attach to.
	long 			GetCurrentDevice(void *deviceParameter = NULL);

	// get device name
	// NOTE:	This function can be called before Open()
	//			Format of string is a zero terminated comma delinated C string.
	//	example	"OMS"
	//			"SB16 [220]"
	void			GetDeviceName(long deviceID, char *cName, unsigned long cNameLength);

	BAEResult		Start(BAE_BOOL loadInstruments,
								long deviceID, void *deviceParameter,
								long midiReference);
	void			Stop(void);

	long			GetMidiReference(void);
	long			GetTimeBaseOffset(void);
	void			SetTimeBaseOffset(long newTimeBase);

private:
	BAEResult		Setup(void);
	void			Cleanup(void);

	BAE_BOOL		mSetup;
	long			mDeviceID;
	void			*mDeviceParameter;
	long			mTimeBaseOffset;
	long			mReference;
	void			*mControlVariables;
};

#endif	// BAE_MIDI_EXTERNAL
