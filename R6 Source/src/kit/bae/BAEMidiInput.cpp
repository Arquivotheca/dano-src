/*****************************************************************************/
/*
** "BAEMidiInput.cpp"
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
**	6/18/97		Modified PV_MidiInputHandler to conform to Win32 API
**	6/20/97		Modified for CW 2.0. Name changes, etc
**  6/25/97     (ddz) Added Name Manager provider stuff
**	7/17/97		Added X_PLATFORM == X_WIN_BAE tests
**	7/22/97		Added the ability to selectivly connect with OMS and the Name Manager or not
**				Moved some OMS Mac stuff around to compile for intel platforms
**	8/18/97		Changed X_WIN_BAE to USE_BAE_EXTERNAL_API
**	11/10/97	Fixed a bug with PV_ConnectOMS in which the name manager was accessed, but 
**				the support drivers were not there, so it crashed.
**	11/14/97	Fixed some problems with CW 2.0
**	2/9/98		Fixed crashing OMS bug in which the BAE_UNIQUE_ID ID that is passed
**				into OMSCreateVirtualDestination caused OMS to crash. Also put in
**				some preflight memory allocations to make sure we have some extra
**				RAM before calling into OMS, because tight memory failures also
**				cause OMS to crash.
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
**	5/1/99		Added timestamp to PV_ProcessExternalMidiEvent. Tweaked PV_MidiInputHandler to
**				try to pass timestamp info along.
**	6/8/99		Changed BAEMidiInput::Start to be device based, and added 
**				BAEMidiInput::GetMaxDeviceCount &&  BAEMidiInput::SetCurrentDevice &&
**				BAEMidiInput::GetCurrentDevice && BAEMidiInput::GetDeviceName
**	7/13/99		Renamed HAE to BAE. Renamed BAEAudioMixer to BAEOutputMixer. Renamed BAEMidiDirect
**				to BAEMidiSynth. Renamed BAEMidiFile to BAEMidiSong. Renamed BAERMFFile to BAERmfSong.
**				Renamed BAErr to BAEResult. Renamed BAEMidiExternal to BAEMidiInput. Renamed BAEMod
**				to BAEModSong. Renamed BAEGroup to BAENoiseGroup. Renamed BAEReverbMode to BAEReverbType.
**				Renamed BAEAudioNoise to BAENoise.
**	9/2/99		Added BAEMidiInput::Setup && BAEMidiInput::Cleanup. Start now calls Setup
*/
/*****************************************************************************/

#include "BAE.h"
#include "BAEMidiInput.h"

#include "X_API.h"
#include "X_Formats.h"
#include "GenPriv.h"
#include "GenSnd.h"

#if X_PLATFORM == X_MACINTOSH
	#include <Types.h>
	#include <Devices.h>
	#include <Memory.h>
	#include <Resources.h>
	#include <Retrace.h>
	#include <Sound.h>
	#include <Timer.h>
	#include <LowMem.h>
	#include <Midi.h>
	#include <Folders.h>
	#include <Script.h>
	#include <Gestalt.h>

	#include "OMS.h"
	#include "OMSNameMgr.h"

	// Set global variables
	#if GENERATING68K	
		#pragma parameter __D0 GetA5
		long GetA5(void) = 0x200D;		/* MOVE.L    A5,D0 */

		#define GetGlobalsRegister()	GetA5()
		#define SetGlobalsRegister(x)	SetA5((x))
	#else
		#define GetGlobalsRegister()	0
		#define SetGlobalsRegister(x)	(x)
	#endif

// Mac specific MIDI port enums
enum
{
	MIDI_PORT_BUFFER_SIZE	= 500,
	MIDI_INPUT_ID 			= 'inpt',
	MIDI_CLIENT_ID			= 'Igor',		// application ID
	NAMES_FILE_TYPE			= 'iNmz',
	BAE_UNIQUE_ID			= 0				// suppose to be a short
};

enum
{
	USE_OMS	= 0,
	USE_OMS_NAME,
	USE_MIDI_MANAGER
};

#endif


#if (X_PLATFORM == X_WIN95) || (X_PLATFORM == X_WIN_HARDWARE)
	#include <windows.h>
	#include <mmsystem.h>
//	#define MAX_MIDI_INPUTS		32
#endif

struct XDeviceControl
{
	long				globalsVariable;
	BAEMidiInput		*pDirect;
	BAEOutputMixer		*pMixer;

#if (X_PLATFORM == X_WIN95) || (X_PLATFORM == X_WIN_HARDWARE)
	HMIDIIN				midiInDeviceHandle;
#endif
};
typedef struct XDeviceControl	XDeviceControl;

#if X_PLATFORM == X_MACINTOSH

static XBOOL				externalMidiEnabled = FALSE;
static XBOOL				registeredMIDISignOn = FALSE;
static XBOOL				registeredMIDIinPort = FALSE;
static XBOOL				usedMIDIManager = FALSE;
static XBOOL				usedOMS = FALSE;
static XBOOL				usedOMSNameManager = FALSE;

static short int 			midiInputRef = 0;
static OMSAppHookUPP		omsAppHook = NULL;
static OMSReadHook2UPP		omsReadHook = NULL;
static OMSReadHookUPP		omsDrvrReadHook = NULL;		// hook for driver, so it works with OMS 1.x
static OMSUniqueID			omsUniqueID = BAE_UNIQUE_ID;
static short int			gChosenInputID;				// uniqueID of selected input; 0 means none
static short int			gCompatMode;

static MIDIReadHookUPP		mmReadHook = NULL;
static void					*omsGlobalReference = NULL;		// used for driver->hook communication

#define USING_DRIVER
#ifdef USING_DRIVER

#define kBeanikDriverUniqueIDSelector	'BtUq'
#define kBeatnikDriverSelector			'BtDv'

static Boolean gDriverAvailable = FALSE;

pascal OSErr PV_SelectorProc(OSType selector, long *response);

#endif

#endif	// X_PLATFORM == X_MACINTOSH

static XDeviceControl		*globalDeviceControl = NULL;

#if (X_PLATFORM == X_WIN95) || (X_PLATFORM == X_WIN_HARDWARE)
static XDeviceControl		*pWinDevice = NULL;
#endif

#define BASE_TIME_OFFSET	25000

//
// Process MIDI Events
//
// This is the main enterance into the Synthesizer via an external source
//
//
//
static void PV_ProcessExternalMidiEvent(BAEMidiInput *pDirect, short int commandByte, 
										short int data1Byte, short int data2Byte, unsigned long time)
{
	if (time == 0)
	{
		time = XMicroseconds() + pDirect->GetTimeBaseOffset();
	}
	pDirect->ParseMidiData((unsigned char)commandByte,
							(unsigned char)data1Byte, (unsigned char)data2Byte,
							0, time);
}

#if (X_PLATFORM == X_WIN95) || (X_PLATFORM == X_WIN_HARDWARE)
/* PV_MidiInputHandler - Low-level callback function to handle MIDI input.
 *      Installed by midiInOpen().  The input handler takes incoming
 *      MIDI events and places them in the input buffer.   
 *
 * Param:   hMidiIn - Handle for the associated input device.
 *          wMsg - One of the MIM_***** messages.
 *          dwInstance - Points to CALLBACKINSTANCEDATA structure.
 *          dwParam1 - MIDI data.
 *          dwParam2 - Timestamp (in milliseconds)
 *
 * Return:  void
 */     
extern "C" void CALLBACK PV_MidiInputHandler(HMIDIIN hMidiIn, UINT wMsg, 
											DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
											
void CALLBACK PV_MidiInputHandler(HMIDIIN hMidiIn, UINT wMsg, 
											DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
//	XDeviceControl	*pDevice;
	char			*pBytes;
	unsigned long	time;

	hMidiIn = hMidiIn;
	dwInstance = dwInstance;
	dwParam2 = dwParam2;
//	pDevice = (XDeviceControl *)dwInstance;
	if (globalDeviceControl)
	{
		switch(wMsg)
		{
			case MIM_OPEN:
			case MIM_ERROR:
			default:
				break;


			case MIM_LONGDATA:		// system exclsive data dumps
				break;

			// The only error possible is invalid MIDI data, so just pass
			// the invalid data on so we'll see it.
			case MIM_MOREDATA:
				time = globalDeviceControl->pDirect->GetTimeBaseOffset() + (dwParam2 * 1000);
				pBytes = (char *)&dwParam1;
				PV_ProcessExternalMidiEvent(globalDeviceControl->pDirect, pBytes[0], pBytes[1], pBytes[2], time);
				break;
			case MIM_DATA:   
//				time = globalDeviceControl->pDirect->GetTimeBaseOffset() + (dwParam2 * 1000);
				time = 0;
				pBytes = (char *)&dwParam1;
				PV_ProcessExternalMidiEvent(globalDeviceControl->pDirect, pBytes[0], pBytes[1], pBytes[2], time);
				break;
		}
	}
}
#endif

#if X_PLATFORM == X_MACINTOSH
static pascal void OMS_AppHook(OMSAppHookMsg *pkt, long theRefCon)
{
	XDeviceControl	*pDevice;

	pDevice = (XDeviceControl *)theRefCon;
	if (pDevice)
	{
		theRefCon = SetGlobalsRegister(pDevice->globalsVariable);		/* Set current A5 */

		switch (pkt->msgType)
		{
			case omsMsgModeChanged:
				/* Respond to compatibility mode having changed */
				gCompatMode = pkt->u.modeChanged.newMode;
				/* this will cause side effects in the event loop */
				break;
	/*
			case omsMsgDestDeleted:
				if (gChosenOutputID == pkt->u.nodeDeleted.uniqueID) 
				{
					gOutNodeRefNum = -1;	// invalid
				}
				break;
			case omsMsgNodesChanged:
				gNodesChanged = TRUE;
				break;
	*/
		}
		theRefCon = SetGlobalsRegister(theRefCon);		/* restore previous A5 */
	}
}

// OMSMIDIPacket
// MIDIPacket

static pascal void OMS_ReadHook(OMSPacket *pkt, long theRefCon)
{
	XDeviceControl	*pDevice;

	if (!theRefCon)	// indicates we were called from Beatnik Driver (assumes PowerPC/CFM)
	{
		pDevice = (XDeviceControl *)omsGlobalReference;
	}
	else
	{
		pDevice = (XDeviceControl *)theRefCon;
	}
	if (pDevice)
	{
		theRefCon = SetGlobalsRegister(pDevice->globalsVariable);		/* Set current A5 */
		PV_ProcessExternalMidiEvent(pDevice->pDirect, pkt->data[0], pkt->data[1], pkt->data[2], 0L);
		theRefCon = SetGlobalsRegister(theRefCon);		/* restore previous A5 */
	}
}

static pascal void OMS_ReadHook2(OMSMIDIPacket *pkt, long theRefCon)
{
	XDeviceControl	*pDevice;

	if (!theRefCon)	// indicates we were called from Beatnik Driver (assumes PowerPC/CFM)
	{
		pDevice = (XDeviceControl *)omsGlobalReference;
	}
	else
	{
		pDevice = (XDeviceControl *)theRefCon;
	}
	if (pDevice)
	{
		theRefCon = SetGlobalsRegister(pDevice->globalsVariable);		/* Set current A5 */

	/*	
		Process the MIDI packet as efficiently as possible.  It is guaranteed to be MIDI
		data, not some high-level event.  The application\xD5s refCon (appRefCon) that was 
		passed to OMSOpenConnection is in the low order word of pkt->tStamp. 
		A convenient way for an application to determine the source of the MIDI data is for 
		it to pass a number identifying the source as the appRefCon to OMSOpenConnection.
		The high-order word of pkt->tStemp is the source\xD5s ioRefNum (not its uniqueID); 
		applications can also look at this to determine the source of the MIDI data. 
	*/

		PV_ProcessExternalMidiEvent(pDevice->pDirect, pkt->data[0], pkt->data[1], pkt->data[2], 0L);
		theRefCon = SetGlobalsRegister(theRefCon);		/* restore previous A5 */
	}
}

pascal OSErr PV_SelectorProc(OSType selector, long *response)
{
	selector = selector;
	*response = (long)omsDrvrReadHook;
	return 0;
}

static short PV_WriteNamesFile(OMSUniqueID deviceID, FSSpec *outFileSpec)
{
	short int 		prefVRefNum;
	long			prefDirID,count;
	static Byte *	namesFileName = "\pBeatnik Names";
	Byte *			filler = "\pFiller";
	short int		i,theErr, refNum, nameCount;
	Str255			resName,pnBuf;
	BAEOutputMixer	*pMixer;

	// assume the Folder Manager is around, put the name file in the Preferences folder
	theErr = FindFolder(kOnSystemDisk,kPreferencesFolderType,FALSE,&prefVRefNum,&prefDirID);
	if (!theErr) 
	{
		outFileSpec->vRefNum = prefVRefNum;
		outFileSpec->parID = prefDirID;
		XBlockMove(namesFileName, outFileSpec->name, namesFileName[0] + 1);
		theErr = FSpCreate(outFileSpec,MIDI_CLIENT_ID,NAMES_FILE_TYPE,smRoman);
		if (theErr && theErr != dupFNErr)
		{
			return theErr;
		}
		theErr = FSpOpenDF(outFileSpec,fsCurPerm,&refNum);
		if (theErr)
		{
			return theErr;
		}
		// write file, uniqueID followed by count of names
		count = sizeof(OMSUniqueID);
		FSWrite(refNum,&count,&deviceID);
		count = sizeof(short);
		nameCount = MAX_INSTRUMENTS; // eventually more
//		nameCount = MAX_INSTRUMENTS * MAX_BANKS; // use this one to get all names
		FSWrite(refNum,&count,&nameCount);
		for (i=0; i < 32; i++)
		{
			pnBuf[i] = 0;
		}
		// stupid test format: nameCount 32-byte pascal strings with zero padding
		
		// now pull all the names from the currently open instrument file from the mixer
		if (globalDeviceControl)
		{
			pMixer = globalDeviceControl->pMixer;
			if (pMixer)
			{
				for (i = 0; i < nameCount; i++) 
				{
					// get name in "C" string format
					if (pMixer->GetInstrumentNameFromAudioFileFromID((char *)resName, i) == BAE_NO_ERROR)
					{
						XCtoPstr(resName);
						if (resName[0] > 31)
						{
							resName[0] = 31;
						}
						BlockMove(resName, pnBuf, resName[0] + 1);
					}
					else
					{
						BlockMove(filler, pnBuf, filler[0] + 1);
					}
					count = 32;		// always write 32 bytes
					FSWrite(refNum,&count,pnBuf);
				}
			}
		}
		// close out the file
		GetFPos(refNum,&count);
		SetEOF(refNum,count);
		FSClose(refNum);
		theErr = noErr;
	}
	return theErr;
}

static short int PV_ConnectOMS(void *reference, void *omsPortName, Boolean connectWithNames)
{
	short int			theErr;
	OMSConnectionParams	conn;
	OMSIDListH			theList;
	OMSFile				nameFileSpec;
	Boolean				oms20Avail;
	OMSReadHook2UPP		*val;
	long				drvrUniqueID;
	void				*preFlight;

	preFlight = (void *)NewPtr(1024 * 256);	// try and allocate 256k of memory
	if (preFlight == NULL)
	{	// OMS will fail, but doesn't report memory failures safely
		return BAE_MEMORY_ERR;
	}
	else
	{
		DisposePtr((char *)preFlight);
	}
	theErr = BAE_NO_ERROR;
	LinkToOMSGlue();
	if (OMSVersion())
	{
// Use OMS
		omsAppHook = NewOMSAppHook(OMS_AppHook);

		usedOMS = TRUE;
		gCompatMode = TRUE;
		gChosenInputID = 0;
		gDriverAvailable = FALSE;
		usedOMSNameManager = FALSE;
		oms20Avail = ((OMSVersion() >> 16L) >= 0x200L) ? TRUE : FALSE;
		theErr = OMSSignIn(MIDI_CLIENT_ID, (long)reference, LMGetCurApName(), omsAppHook, &gCompatMode);
		if (theErr == omsNoErr)
		{
			// check and see if the driver got installed
			theErr = Gestalt(kBeatnikDriverSelector,(long *)&val);
			if (theErr == noErr) 
			{
				gDriverAvailable = TRUE;
				omsGlobalReference = reference;
				omsDrvrReadHook = NewOMSReadHook(OMS_ReadHook);
				*val = (OMSReadHook2UPP)omsDrvrReadHook;
				Gestalt(kBeanikDriverUniqueIDSelector,&drvrUniqueID);
				omsUniqueID = drvrUniqueID;
			}
			registeredMIDISignOn = TRUE;
			midiInputRef = 0;
			/*	Add an input port */
			if (oms20Avail == FALSE)	// pre 2.0 OMS, so ask for a IAC port
			{
				omsReadHook = (OMSReadHook2UPP)NewOMSReadHook(OMS_ReadHook);
				// if no driver, we'll do the IAC port
				if (gDriverAvailable == FALSE) 
				{
					theErr = OMSAddPort(MIDI_CLIENT_ID, MIDI_INPUT_ID, omsPortTypeInput, omsReadHook, 
										(long)reference, &midiInputRef);
					if (theErr == omsNoErr)
					{
						InitCursor();	// set arrow cursor for pending dialog box
						theList = OMSChooseNodes(NULL, "\pSelect one OMS input device:", (OMSBool)FALSE, 
												omsIncludeOutputs + omsIncludeReal + omsIncludeVirtual + omsIncludeSecret,
												NULL); 
						if (theList)
						{
							gChosenInputID = (**theList).id[0];
							conn.nodeUniqueID = gChosenInputID;
							conn.appRefCon = 0;
							theErr = OMSOpenConnections(MIDI_CLIENT_ID, MIDI_INPUT_ID, 1, &conn, FALSE);
							DisposeHandle((Handle)theList);
						}
					}
					else
					{
						goto error;
					}
				}
			}
			else
			{
				if (connectWithNames)
				{
					// sign into the Name Manager
					theErr = OMSNSignIn(MIDI_CLIENT_ID);
					if (theErr != omsNoErr)
					{
						goto error;
					}
					usedOMSNameManager = TRUE;
				}
				if (gDriverAvailable == FALSE) 
				{  // if no Beatnik driver, create a virtual node
					omsReadHook = NewOMSReadHook2(OMS_ReadHook2);
					theErr = OMSCreateVirtualDestination(MIDI_CLIENT_ID, MIDI_INPUT_ID,
										(OMSStringPtr)omsPortName,
										&omsUniqueID,
										&midiInputRef,
										omsReadHook,
										(long)reference, 0,
										0, NULL);
					if (theErr != omsNoErr)
					{
						goto error;
					}
				}

				// write out a file containing the patch names and uniqueID
				if (connectWithNames)
				{
					theErr = PV_WriteNamesFile(omsUniqueID,&nameFileSpec); // additional args later
				
					// tell OMS about this file
					theErr = OMSNProviderDocSaved(omsUniqueID,&nameFileSpec);
					if (gDriverAvailable)
					{
						theErr = OMSNProvideDevice(omsUniqueID, MIDI_CLIENT_ID, &nameFileSpec, omsUniqueID);
					}
				}
			}
		}
	}
	else
	{
error:
		theErr = BAE_OMS_FAILED;
	}
	return theErr;
}

static void PV_CleanupOMS(void)
{
	OMSConnectionParams	conn;
	long *val;
	short theErr;
	
	if (registeredMIDISignOn)
	{
		if (gDriverAvailable == FALSE)
		{
			if ( (OMSVersion() >> 16L) < 0x200L)	// pre 2.0 OMS, so ask for a IAC port
			{
				conn.nodeUniqueID = gChosenInputID;
				conn.appRefCon = 0;		/* not used in this example */
				OMSCloseConnections(MIDI_CLIENT_ID, MIDI_INPUT_ID, 1, &conn);
			}
			else
			{
				OMSDeleteVirtualNode(MIDI_CLIENT_ID, MIDI_INPUT_ID);
			}
		} 
		else 
		{
			theErr = Gestalt(kBeatnikDriverSelector,(long *)&val);
			if (theErr == noErr)
			{
				*val = NULL;  // clear out driver
			}
			if (omsDrvrReadHook)
			{
				DisposeRoutineDescriptor((RoutineDescriptor *)omsDrvrReadHook);
				omsDrvrReadHook = NULL;
			}
			gDriverAvailable = FALSE;
		}
		if (usedOMSNameManager)
		{
			OMSNSignOut(MIDI_CLIENT_ID);
		}
		OMSSignOut(MIDI_CLIENT_ID);
		if (omsAppHook)
		{
			DisposeRoutineDescriptor((RoutineDescriptor *)omsAppHook);
		}
		if (omsReadHook)
		{
			DisposeRoutineDescriptor((RoutineDescriptor *)omsReadHook);
		}
	}
}

// Connection through the MIDI Manager
static pascal short PV_MidiManagerReadHook(MIDIPacketPtr pkt, long theRefCon)
{
	XDeviceControl	*pDevice;

	pDevice = (XDeviceControl *)theRefCon;
	if (pDevice)
	{
		theRefCon = SetGlobalsRegister(pDevice->globalsVariable);		/* Set current A5 */
		PV_ProcessExternalMidiEvent(pDevice->pDirect, pkt->data[0], pkt->data[1], pkt->data[2], 0L);
		theRefCon = SetGlobalsRegister(theRefCon);		/* restore previous A5 */
	}
	return midiMorePacket;
}

static short int PV_ConnectMidiManager(void *reference, char *portName)
{
	MIDIPortParams	theInit;
	short int		theErr;

	theErr = BAE_NO_ERROR;

	theErr = MIDISignIn(MIDI_CLIENT_ID, (long)reference, GetResource('ICN#', 128), LMGetCurApName());
	if (theErr == BAE_NO_ERROR)
	{
		usedMIDIManager = TRUE;
		registeredMIDISignOn = TRUE;
		XSetMemory(&theInit, (long)sizeof(MIDIPortParams), 0);

		theInit.portID = MIDI_INPUT_ID;
		theInit.portType = midiPortTypeInput;
		theInit.timeBase = 0;
		theInit.offsetTime = midiGetCurrent;

		mmReadHook = NewMIDIReadHookProc(PV_MidiManagerReadHook);
		theInit.readHook = mmReadHook;
//		theInit.readHook = (MIDIReadHookProcPtr)mmReadHook;
		theInit.initClock.syncType = midiInternalSync;

		theInit.refCon = (long)reference;
		theInit.initClock.curTime = 0;
		theInit.initClock.format = midiFormatMSec;
		BlockMoveData(portName, (void *)&theInit.name, (long)portName[0]+1);
		midiInputRef = 0;
		theErr = MIDIAddPort(MIDI_CLIENT_ID, MIDI_PORT_BUFFER_SIZE, &midiInputRef, &theInit);
		if (theErr == noErr)
		{
			registeredMIDIinPort = TRUE;
		}
		else
		{
			theErr = BAE_MIDI_MANAGER_FAILED;
		}
	}
	else
	{
		theErr = BAE_MIDI_MANAGER_FAILED;
	}
	return theErr;
}

static void PV_CleanupMidiManager(void)
{
	if (registeredMIDIinPort)
	{
		MIDIRemovePort(midiInputRef);
	}
	if (registeredMIDISignOn)
	{
		MIDISignOut(MIDI_CLIENT_ID);
	}
	if (mmReadHook)
	{
		DisposeRoutineDescriptor(mmReadHook);
	}
	mmReadHook = NULL;
}

static void PV_CleanupExternalMidi(void)
{
	if (externalMidiEnabled)
	{
		externalMidiEnabled = FALSE;
		if (usedMIDIManager)
		{
			PV_CleanupMidiManager();
		}
		if (usedOMS)
		{
			PV_CleanupOMS();
		}
	}
	registeredMIDIinPort = FALSE;
	registeredMIDISignOn = FALSE;
	usedMIDIManager = FALSE;
	usedOMS = FALSE;
}


// Connect system to an external midi source. One of these types:
//		USE_OMS					-- use OMS
//		USE_OMS_NAME			-- use OMS and name manager
//		USE_MIDI_MANAGER		-- use Midi Manager

static short int PV_SetupExternalMidi(void *reference, short int connectionType)
{
	short int	theErr;
	char		portName[256];
	static char	midiManagerPortName[] = "\pInput";
	static char	defaultOMSPortName[] = "\pBeatnik";
	Handle		externalName;
	Boolean		connectWithNames;

	if (externalMidiEnabled)
	{
		PV_CleanupExternalMidi();
	}
	usedMIDIManager = FALSE;
	usedOMS = FALSE;
	registeredMIDISignOn = FALSE;
	registeredMIDIinPort = FALSE;
	externalMidiEnabled = TRUE;
	theErr = BAE_MIDI_MANGER_NOT_THERE;
	if ((connectionType == USE_OMS) || (connectionType == USE_OMS_NAME))
	{
		connectWithNames = (connectionType == USE_OMS_NAME) ? TRUE : FALSE;
		externalName = GetResource('STR ', 32000);
		if (externalName)
		{
			HLock(externalName);
			XBlockMove(*externalName, portName, *externalName[0] + 1);
			HUnlock(externalName);
		}
		else
		{
			XStrCpy(portName, defaultOMSPortName);
		}			

		theErr = PV_ConnectOMS(reference, portName, connectWithNames);
	}
	if (connectionType == USE_MIDI_MANAGER)
	{
		externalName = GetResource('STR ', 32001);
		if (externalName)
		{
			HLock(externalName);
			XBlockMove(*externalName, portName, *externalName[0] + 1);
			HUnlock(externalName);
		}
		else
		{
			XStrCpy(portName, midiManagerPortName);
		}			
		theErr = PV_ConnectMidiManager(reference, portName);
	}
	if (theErr)
	{
		PV_CleanupExternalMidi();
	}
	return theErr;
}
#endif	// X_PLATFORM == X_MACINTOSH



// Class implemention for BAEMidiInput

//#pragma mark (BAEMidiInput class)

BAEMidiInput::BAEMidiInput(BAEOutputMixer *pBAEOutputMixer,
						   char *pName,
						   void * userReference):
									BAEMidiSynth(pBAEOutputMixer, 
													pName, userReference)
{
	XDeviceControl	*pDevice;

	mSetup = FALSE;
	SetTimeBaseOffset(BASE_TIME_OFFSET);

	// most of the real variables get setup when the BAEMidiDirect
	// gets created
	mControlVariables = XNewPtr((long)sizeof(XDeviceControl));
	pDevice = (XDeviceControl *)mControlVariables;
	if (pDevice)
	{
#if X_PLATFORM == X_MACINTOSH
		pDevice->globalsVariable = GetGlobalsRegister();
#endif
		pDevice->pDirect = this;
		pDevice->pMixer = pBAEOutputMixer;
	}
	globalDeviceControl = pDevice;
	MusicGlobals->enableDriftFixer = TRUE;
	mDeviceID = 0;
}

BAEMidiInput::~BAEMidiInput()
{
	Stop();
	Cleanup();
	XDisposePtr(mControlVariables);
	globalDeviceControl = NULL;
	MusicGlobals->enableDriftFixer = FALSE;
}

BAEResult BAEMidiInput::Setup(void)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (mSetup == FALSE)
	{
		mSetup = TRUE;

		// MacOS midi input via OMS and Midi manager
		#if X_PLATFORM == X_MACINTOSH
		{
			short int	pv_connect;

			switch (mDeviceID)
			{
				default:
					err = BAE_PARAM_ERR;
					break;
				case 0:	// MIDI Manager
					pv_connect = USE_MIDI_MANAGER;
					break;
				case 1:	// OMS
					pv_connect = USE_OMS;
					break;
			}
			if (externalMidiEnabled == FALSE)
			{
				if (PV_SetupExternalMidi(mControlVariables, pv_connect))
				{
					err = BAE_DEVICE_UNAVAILABLE;
				}
			}
		}
		#endif
		
		// WinOS midi input via the midiIn API
		#if (X_PLATFORM == X_WIN95) || (X_PLATFORM == X_WIN_HARDWARE)
		XDeviceControl *pDevice;

		pDevice = (XDeviceControl *)mControlVariables;
		if (pDevice)
		{
			if (midiInOpen(&pDevice->midiInDeviceHandle,
       		             		GetCurrentDevice(),		// use default mapping
            		      		(DWORD)PV_MidiInputHandler,
								(DWORD)this,
   	        					CALLBACK_FUNCTION) == 0)
			{
				if (midiInStart(pDevice->midiInDeviceHandle) == 0)
				{
					err = BAE_NO_ERROR;
				}
				else
				{
					err = BAE_ABORTED;
				}
			}
			else
			{
				err = BAE_DEVICE_UNAVAILABLE;
			}
		}
		else
		{
			err = BAE_NOT_SETUP;
		}
		#endif
	}
	return err;
}

void BAEMidiInput::Cleanup(void)
{
	if (mSetup)
	{
		mSetup = FALSE;
	#if (X_PLATFORM == X_WIN95) || (X_PLATFORM == X_WIN_HARDWARE)
		XDeviceControl *pDevice;

		pDevice = (XDeviceControl *)mControlVariables;

		if (pDevice->midiInDeviceHandle)
		{
			midiInClose(pDevice->midiInDeviceHandle);
			pDevice->midiInDeviceHandle = NULL;
		}
	#endif
	#if X_PLATFORM == X_MACINTOSH
		if (externalMidiEnabled)
		{
			PV_CleanupExternalMidi();
		}
	#endif
	}
}

// number of midi input devices. ie different versions of the BAE connection. DirectSound and waveOut
// return number of devices. ie 1 is one device, 2 is two devices.
// NOTE: This function can be called before Start is called
long BAEMidiInput::GetMaxDeviceCount(void)
{
	#if X_PLATFORM == X_MACINTOSH
	return 2;	// MIDI Manager, OMS
	#endif
	#if (X_PLATFORM == X_WIN95) || (X_PLATFORM == X_WIN_HARDWARE)
	{
		long	wNumDevices;

		wNumDevices = (long)midiInGetNumDevs();
		return wNumDevices;
	}
	#else
	return 0;
	#endif
}

// set current device. should be a number from 0 to GetMaxDeviceCount()
// deviceParameter is a pointer to device specific info. It will
// be NULL if there's nothing interesting. For Windows Device 1 (DirectSound) it
// is the window handle that DirectSound will attach to. Pass NULL, if you don't know
// what is correct.
void BAEMidiInput::SetCurrentDevice(long deviceID, void *deviceParameter)
{
	if ( (deviceID >= 0) && (deviceID < GetMaxDeviceCount()))
	{
		mDeviceID = deviceID;
		mDeviceParameter = deviceParameter;
	}
}

// get current device. deviceParameter is a pointer to device specific info. It will
// be NULL if there's nothing interesting. For Windows Device 1 (DirectSound) it
// is the window handle that DirectSound will attach to.
long BAEMidiInput::GetCurrentDevice(void *deviceParameter)
{
	deviceParameter;
	return mDeviceID;
}

// get device name
// NOTE:	This function can be called before Open()
//			Format of string is a zero terminated comma delinated C string.
//	example	"OMS"
//			"SB16 [220]"
void BAEMidiInput::GetDeviceName(long deviceID, char *cName, unsigned long cNameLength)
{
	if (cName && cNameLength)
	{
		#if X_PLATFORM == X_MACINTOSH
		switch (deviceID)
		{
			default:
				cName[0] = 0;
				break;
			case 0:
				XStrCpy(cName, "MIDI Manager");
				break;
			case 1:
				XStrCpy(cName, "OMS");
				break;
		}
		#endif
		#if (X_PLATFORM == X_WIN95) || (X_PLATFORM == X_WIN_HARDWARE)
		{
			long		wNumDevices;
		    MIDIINCAPS	theCaps;
			MMRESULT	err;

		    wNumDevices = GetMaxDeviceCount();
		    if ((deviceID >= 0) && (deviceID < wNumDevices))
		    {
	      		err = midiInGetDevCaps(deviceID, (LPMIDIINCAPS) &theCaps, sizeof(MIDIINCAPS));
	            if (err == 0)
	            {
	            	XStrCpy(cName, theCaps.szPname);
	            }
			}
		}
		#endif
	}
}

BAEResult BAEMidiInput::Start(BAE_BOOL loadInstruments,
								long deviceID, void *deviceParameter,
								long midiReference)
{
	BAEResult		err;
	XDeviceControl	*pDevice;

	mReference = midiReference;
	SetQueue(TRUE);
	pDevice = (XDeviceControl *)mControlVariables;
	if (pDevice)
	{
		err = BAE_PARAM_ERR;
		if ( (deviceID >= 0) && (deviceID < GetMaxDeviceCount()))
		{
			SetCurrentDevice(deviceID, deviceParameter);
			err = Setup();
			// now load instruments and create live connection
			if (err == BAE_NO_ERROR)
			{
				err = Open(loadInstruments);
			}
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}

void BAEMidiInput::Stop(void)
{
	Close();
}

long BAEMidiInput::GetMidiReference(void)
{
	return mReference;
}

long BAEMidiInput::GetTimeBaseOffset(void)
{
	return mTimeBaseOffset;
}


void BAEMidiInput::SetTimeBaseOffset(long newTimeBase)
{
	mTimeBaseOffset = newTimeBase;
}


// EOF of BAEMidiInput.cpp

