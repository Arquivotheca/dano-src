
/*********************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*/

/******************************************************************************
*
* @doc INTERNAL
* @module mxr8010.h |
* EMU8010 hardware resource manager interface to the external mixer 
* 
*
* @iex
* Revision History:
*
* Version    Person        Date         Reason
* -------    ---------   -----------  --------------------------------------- 
*   
*  0.001        MG       Oct 10, 97   Initial version
*
******************************************************************************
* @doc EXTERNAL
* @contents1 EMU MXR8210 Programmer's Manual |
* MXR8210 is the interface mixer devices which are programmed through the EMU8010. 
* Since these devices are optional, this is a general purpose interface which
* are resolved with real implementations depending on the hardware. Such real
* implementations have different numbers of controls, and must be programmed through
* different PCI functions. It is expected that the client not only knows about 
* what mixer configurations exist, but everything about any one mixer configuration,
* including how many controls there are, how they should be programmed, and what settings
* actually mean. 
*/

#ifndef __MXR8210_H
#define __MXR8210_H
/************

* Includes
************/

#include "datatype.h" /* Data types */ 
#include "aset8210.h" 

/****************
 * Definitions
 ****************/

/* Error codes */
#define MXRERR_BAD_HANDLE	1
#define MXRERR_INIT_FAILED  2
#define MXRERR_UNKNOWN_MIXER_ID 3
#define MXRERR_BAD_CONTROL  4
#define MXRERR_UNKNOWN_FUNC 5
#define MXRERR_READ_ONLY 6

/* Supported Mixer types */
#define MXRTYPE_INVALID			0x0
#define MXRTYPE_AC97			0x1
#define MXRTYPE_ECARD			0x2


/********************
 * Enumerated types
 *******************/

/* @enum MXRCONTROLID | An enumeration of all of the valid mixer control
 *  parameters.  Not all of these are valid for all types of hardware mixers,
 *  and the actual encodings of the settings for these controls is
 *  hardware-specific.
 */
typedef enum {
	mxrMasterVolume,   /* @emem Volume of the main outputs of the mixer */
	mxrMonoVolume,     /* @emem Volume of the main mono mixer output */
	mxrPhoneVolume,    /* @emem Volume of the telephone/modem input */
	mxrMICVolume,     /* @emem Volume for microphone input 1 */
	mxrLineInVolume,   /* @emem Volume for the line input */
	mxrCDVolume,       /* @emem Volume for the compact disk audio input */
	mxrVideoVolume,    /* @emem Volume for the video audio input */
	mxrAuxVolume,      /* @emem Volume for the auxilliary input */
	mxrPCMOutVolume,   /* @emem Volume for the PCM data stream from the E8010 */
	mxrRecordSelect,   /* @emem Select the device to record from */
	mxrRecordGain,     /* @emem Gain for the input device */
	mxrRecordGainMic,  /* @emem Gain for the audio input */

	/* ECARD specific params */
	mxrADCGain,        /* @emem Specify the ADC gain control */
	mxrMuxSelect0,	   /* @emem Select input device from the EMUX0 */
	mxrMuxSelect1,     /* @emem Select input device from the EMUX1 */
	mxrMuxSelect2,     /* @emem Select between the DI and Audio drive */
	mxrBoardRev,	   /* @emem Current board revision level */
	mxrHasAudioBay,    /* @emem Read-only flag indicating presence of 
			    *  the audio bay.  */
	mxrHasDigitalIfc,  /* @emem Read-only flag indicating presence of
			    *  the digitial interface board */
	mxrSerialNum,
	mxrGPSPDIFStatus,  /* @emem A boolean value indicating whether a device
                            *  is connected to the currently selected input */
	mxrCDSPDIFStatus,  /* @emem A boolean value indicating whether a device
                            *  is connected to the input */
} MXRCONTROLID;

/* @enum MXRINPUTID | An enumeration of all of the supported audio input
 *  devices.  These input devices are used as a parameter for the mxrMuxSelct
 *  controls.  NOTE:  These IDs are specific to the E-card and should be
 *  kept in order.  Do not insert new devices in the middle.
 */
typedef enum {
	mxridEC_SPDIF   = 0,    /* @emem ECARD SPDIF in */
	mxridAD_SPIF    = 1,    /* @emem Audio Drive SPDIF in */
	mxridDI_SPDIF0  = 2,    /* @emem Digital Interface SPDIF in 0 */
	mxridDI_SPDIF1  = 3,    /* @emem Digital Interface SPDIF in 1 */
	mxridCD_SPDIF   = 4,    /* @emem Compact Disk SPDIF in */
} MXRECARDINPUTID;

/****************
 * Structures
 ***************/

BEGINEMUCTYPE

/* @type MXRID | An opaque handle used to reference a particular EMU8010
 * programmed external mixer
 */
typedef DWORD MXRID;


/*************************************
 * Public functions
 *************************************/

/* @func This function fills an array with the IDs of all of
 *  the discovered mixer IDs in the system and returns a count of the
 *  total number of mixer instances.  The caller is allowed to 
 *  pass NULL for either or both of the arguments; in this case,
 *  the function will just return the total number of mixer instances
 *  without attempting to dereference the array.
 *
 *  This function only returns sound engines for which the seDiscoverChip
 *  function has been previously called. 
 *
 * @parm DWORD | count | The number of MXRID handles in the array.
 *  If 'count' is less than the total number of mixer instances, only
 *  the first 'count' IDs will be copied into the array.  If 'count' is 0,
 *  the function will not attempt to fill the array.
 * @parm MXRID * | seids | An array of 'count' MXRID handles.
 *	If NULL, the routine will not attempt to fill the array with IDs.
 * 
 * @rdesc The total number of sound engines in the system.  If an error
 *  occurs, the function will return 0.
 */
EMUAPIEXPORT DWORD mxrGetHardwareInstances(DWORD count /* VSIZE */, 
                                           MXRID *mxrids /* IO */);


/* @func Retrieve the ID of the mixer associated with the given MXRID.
 *  This ID is unique between different versions of the mixer and can
 *  be used to differentiate between ECARD and AC97 mixers.
 *
 * @parm MXRID | mxrID | The ID of the mixer whose ID is to be retrieved.
 * @rdesc Returns the Mixer's Type.  Valid return values are MXRTYPE_ECARD
 *  and MXRTYPE_AC97.
 */
EMUAPIEXPORT DWORD mxrGetMixerType(MXRID mxrid);


/* @func Set the value of a control
 *
 * @parm MXRID | mxrID | 'ID' value returned by mxrDiscoverChip 
 * @parm MXRCONTROL | controlID | ID value for a paricular control which is known
 * by the client to work with a particular mixer configuration
 * @parm DWORD | dwSetting | The setting for that control.
 *
 * @rdesc Returns the current setting
 */
EMUAPIEXPORT void   mxrSetControlValue(MXRID id, MXRCONTROLID controlID, 
                                       DWORD dwSetting);


/* @func Return the current setting of a control
 *
 * @parm MXRID | hrmID | 'ID' value returned by mxrDiscoverChip 
 * @parm MXRCONTROL | controlID | ID value for a paricular control which is 
 *  known by the client to work with a particular mixer configuration
 *
 * @rdesc Returns the current setting
 */
EMUAPIEXPORT DWORD   mxrGetControlValue(MXRID id, MXRCONTROLID controlID);


/* @func Return the default value, or the value which was set upon mixer discovery
 *
 * @parm MXRID | hrmID | 'ID' value returned by mxrDiscoverChip 
 * @parm MXRCONTROL | controlID | ID value for a paricular control which is known
 * by the client to work with a particular mixer configuration
 *
 * @rdesc Returns the value
 */
EMUAPIEXPORT DWORD   mxrGetControlDefaultValue(MXRID id, 
                                               MXRCONTROLID controlID);


/* @func Retrieve the serial number string.
 *
 * @parm MXRID | mxrid | 'ID' value from mxrDiscoverChip.
 * @parm MXRCONTROLID | controlID | The control whose display string is to be
 *  acquired.
 * @parm DWORD | dwNumChars | The size of the returned string.
 * @parm CHAR * | retString | A pointer to an array of characters which will
 *  recieve the serial number.   The string will be NULL-terminated.
 *
 * @rdesc Returns SUCCESS if the serial number is successfully retrieved.
 */
EMUAPIEXPORT EMUSTAT mxrGetControlDisplayString(MXRID mxrid, 
                                                MXRCONTROLID ctrlid, 
                                                DWORD dwNumChars /* VSIZE */, 
                                                CHAR *retString /* IO */); 

ENDEMUCTYPE

#endif
