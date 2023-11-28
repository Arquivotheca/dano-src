/*****************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*					
******************************************************************************
* @doc INTERNAL
* @module mxr8210.c | 
*  This module contains the source code for the Mixer API.  The mixer API 
*  provides a simple interface for configuring the external audio codecs and 
*  manipulating any parameters used by these codecs.  It contains the usual 
*  set of initialization routines (init, discover, and undiscover) as well as
*  three routines (<f mxrGetControlValue>, <f mxrSetControlValue>, and 
*  <mxrGetDefaultValue>) which actually manipulate the parameters.  This
*  routine uses a value passed in at configuration time to determine which
*  type of mixer is to be configured for this chip.
*
* @iex
* Revision History:
* Version	Person		Date		Reason
* -------   ---------	----------	--------------------------------------
*  0.002	JK			Oct 21, 97	Fleshed out the overall structure and
*                                   implemented AC97 routines.
*  0.001	JK			Oct 10, 97	Initial version
* @end
******************************************************************************
*/


/************
* Includes
************/
#include "datatype.h"
#include "dbg8210.h"
#include "hal8210.h"
#include "mxr8210.h"
#include "mxr8210d.h"
#include "mxrecardp.h"

/**************************
 * Macros and definitions
 **************************/

#define MIXER_MAGIC            0xBABECADEUL
#define MAKE_MXRID(_x)         (MXRID)((0xCADEL << 16) | (_x))
#define IS_VALID_MXRID(_x)     (((_x) >> 16) == 0xCADE) && (((_x) & 0xFFFF) < mxrCount)
#define FIND_STATE(_x) \
	(IS_VALID_MXRID(_x) ? &mxrStates[(_x) & 0xFFFF] : NULL)


/*************************
 * Enumerated types
 *************************/

/* @enum enFuncValue | An enumerated listing all of the operations
 *  a mixer's control function needs to handle.
 */
typedef enum {
	MXRFUNC_CONFIG,       /* @menum Configure the mixer */
	MXRFUNC_DECONFIG,     /* @menum Deconfigure the mixer */
	MXRFUNC_SETCONTROL,   /* @menum Set one of the mixer's controls */
	MXRFUNC_GETCONTROL,   /* @menum Get the current value for a control */
	MXRFUNC_GETDEFAULT,   /* @menum Retrieve the default value for a control */
	MXRFUNC_GETDISPLAY,   /* @menum Retrieve a textual representation of the
						   *  value.  Sometimes the text is a whole lot more
						   *  useful than the DWORD-encoded value */
} enFuncValue;


/*************************
 * Structures and other fun
 *************************/



/* @struct stMXRState | Contains all of the information needed to manipulate a
 *  particular mixer instance, including its type and state indication.
 */
typedef struct mxrStateTag {
	DWORD       dwInUse;     /* @field Magic number indicating mixer in use. */
	HALID       halid;       /* @field The halid for the mixer device */
	DWORD       dwMxrData;   /* @field Mixer private data */
	DWORD       dwMixerType;
    EMUSTAT     (*ctrlHndlrFunc)(struct mxrStateTag*, enFuncValue, MXRCONTROLID,
	                             DWORD, CHAR*);
} stMXRState;


/* @typedef CtrlHndlr | A function pointer typedef for the control
 *  dispatch function.
 */
typedef EMUSTAT (*CtrlHndlr)(stMXRState *state, enFuncValue function, 
                             MXRCONTROLID ctrl, DWORD dwSettings, CHAR*);


/* @struct stMixerInfo | A structure containing the data needed to
 *  support a particular type of mixer, including its mixer id and
 *  control function.
 */
typedef struct mxrInfoTag {
	DWORD        mixerID;    /* @field The ID of this mixer class */
	CtrlHndlr    ctrlHndlrFunc;  /* @field The control handler function for this
							  *  type of mixer.  */
} stMixerInfo;


/* @struct stCtrlInfo | Contains all of the information needed to map
 *  a control ID to a particular device register.
 */
typedef struct ctrlInfoTag {
	WORD    wRegister;       /* @field The device register to manipulate */
	DWORD   dwDefaultValue;	 /* @field The default value for the register */
} stCtrlInfo;

/****************************
 * Global variables
 ****************************/

DWORD      mxrCount = 0;
stMXRState mxrStates[MAX_CHIPS];


/****************************
 * Forward references
 ****************************/
/* @globalv The AC97 control dispatch function */
EMUSTAT _AC97ctrlhndlr(stMXRState*, enFuncValue, MXRCONTROLID, DWORD, CHAR*);

/* @globalv The ECARD control dispatch function */
EMUSTAT _ECARDctrlhndlr(stMXRState*, enFuncValue, MXRCONTROLID, DWORD, CHAR*);

/* @globalv A list of all of the supported mixers */
stMixerInfo mxrInfo[] = {
	{ MXRID_AC97,		_AC97ctrlhndlr },
	{ MXRID_ECARD,		_ECARDctrlhndlr }
};


/****************************
 * Functions 
 ****************************/

/***************************************************************************
 * @func Initialize the mixer state array.
 */
EMUAPIEXPORT EMUSTAT
mxrInit()
{
	mxrCount = 0;
    memset(mxrStates, 0x0, sizeof(stMXRState) * MAX_CHIPS);
	for (mxrCount=0; mxrCount<MAX_CHIPS; mxrCount++)
		mxrStates[mxrCount].halid = 0xFFFFFFFF;

	return SUCCESS;
}


/***************************************************************************
 * @func Create a mixer instance, initialize it, and hand it back to the
 *  caller.
 */
EMUAPIEXPORT EMUSTAT
mxrDiscoverChip(HALID halid, MXRCONFIG *config, MXRID *retID)
{
	WORD    wIndex;       /* Position of new mixer in mixer array */
	WORD    wInfoIndex;	  /* Position of mixer info in info array */
	WORD    wNumMixers = sizeof(mxrInfo) / sizeof(stMixerInfo);
	
	/* Check to see if we have already been discovered */
	for (wIndex = 0; wIndex < MAX_CHIPS; wIndex++)
		if (mxrStates[wIndex].halid == halid)
			return SUCCESS;

	/* Find a free entry in the states array */
	for (wIndex = 0; wIndex < MAX_CHIPS; wIndex++)
		if (mxrStates[wIndex].dwInUse == 0)
			break;
	if (wIndex == MAX_CHIPS)
		RETURN_ERROR(MXRERR_INIT_FAILED);

	/* Initialize the state */
	mxrStates[wIndex].dwInUse = MIXER_MAGIC;
	mxrStates[wIndex].halid = halid;
	mxrStates[wIndex].dwMixerType = MXRTYPE_INVALID;

	for (wInfoIndex = 0; wInfoIndex < wNumMixers; wInfoIndex++) {
		if (mxrInfo[wInfoIndex].mixerID == config->mixerID)
			break;
	}

	if (wInfoIndex == wNumMixers)
		RETURN_ERROR(MXRERR_UNKNOWN_MIXER_ID)
    else
		mxrStates[wIndex].ctrlHndlrFunc = mxrInfo[wInfoIndex].ctrlHndlrFunc;

	mxrCount++;

	*retID = MAKE_MXRID(wIndex);

	if (!config->bInitMixer)
		return SUCCESS;
	else
		return mxrStates[wIndex].ctrlHndlrFunc(&mxrStates[wIndex], 
				MXRFUNC_CONFIG, (MXRCONTROLID) 0, 0, NULL);

}


/***************************************************************************
 * @func Deallocate the given mixer instance and return any memory allocated
 *  for that instance.
 */
EMUAPIEXPORT EMUSTAT
mxrUndiscoverChip(MXRID id)
{
	stMXRState *state = FIND_STATE(id);

	if (state == NULL)
		RETURN_ERROR(MXRERR_BAD_HANDLE);

	/* Give the mixer a chance to turn itself off */
	state->ctrlHndlrFunc(state, MXRFUNC_DECONFIG, (MXRCONTROLID) 0, 0, NULL);

	state->dwInUse = 0;
	state->halid   = 0;
	mxrCount--;

	return SUCCESS;
}


/***************************************************************************
 * @func Retrieves an array of all of the currently discovered mixer
 *  mixer instances.
 */
EMUAPIEXPORT DWORD
mxrGetHardwareInstances(DWORD numIDs, MXRID *mxrids)
{
	WORD i;
	WORD wFound = 0;

	for (i = 0; i < MAX_CHIPS && wFound < numIDs; i++) {
		if (mxrStates[i].dwInUse)
			mxrids[wFound++] = MAKE_MXRID(i);
	}

	return mxrCount;
}


/***************************************************************************
 * @func Retrieve the Mixer type.
 */
EMUAPIEXPORT DWORD
mxrGetMixerType(MXRID mxrid)
{
	stMXRState *state = FIND_STATE(mxrid);

	if (state == NULL) 
		return 0;
	else
		return state->dwMixerType;
}

		
/***************************************************************************
 * @func Set the value of a particular mixer parameter.
 */
EMUAPIEXPORT void
mxrSetControlValue(MXRID mxrid, MXRCONTROLID ctrlid, DWORD value)
{
 	stMXRState *state = FIND_STATE(mxrid);

	if (state == NULL)
		return;

	state->ctrlHndlrFunc(state, MXRFUNC_SETCONTROL, ctrlid, value, NULL);
}


/***************************************************************************
 * @func Get the value of a particular mixer parameter.
 */
EMUAPIEXPORT DWORD
mxrGetControlValue(MXRID mxrid, MXRCONTROLID ctrlid)
{
	stMXRState *state = FIND_STATE(mxrid);
	DWORD value;

	if (state == NULL)
		RETURN_ERROR(MXRERR_BAD_HANDLE);

	if (state->ctrlHndlrFunc(state, MXRFUNC_GETCONTROL, ctrlid, 
		                     (DWORD) &value, NULL) != SUCCESS) 
		return 0;
	else
		return value;
}


/***************************************************************************
 * @func Get the default value of a parameter.
 */
EMUAPIEXPORT DWORD
mxrGetControlDefaultValue(MXRID mxrid, MXRCONTROLID ctrlid)
{
	stMXRState *state = FIND_STATE(mxrid);
	DWORD value;

	if (state == NULL)
		RETURN_ERROR(MXRERR_BAD_HANDLE);

	if (state->ctrlHndlrFunc(state, MXRFUNC_GETDEFAULT, ctrlid, 
		                     (DWORD) &value, NULL) != SUCCESS)
		return 0;
	else
		return value;
}


/***************************************************************************
 * @func Get the display string representation for a specified parameter.
 */
EMUAPIEXPORT EMUSTAT
mxrGetControlDisplayString(MXRID mxrid, MXRCONTROLID ctrlid, DWORD dwNumChars,
                           CHAR *szString)
{
	stMXRState *state = FIND_STATE(mxrid);

	if (state == NULL)
		RETURN_ERROR(MXRERR_BAD_HANDLE);

	state->ctrlHndlrFunc(state, MXRFUNC_GETDISPLAY, ctrlid, 
	                     dwNumChars, szString);

	return SUCCESS;
}


/*************************************************************************
 * AC97 Implementation 
 *************************************************************************/

/* @globalv Contains a list of the controls supported by AC97.
 *   NOTE: This array must be kept in sync with the MXRCONTROLID
 *  values declared in mxr8210.h. 
 */
static const stCtrlInfo mxrAC97Controls[] = {
	{ AC97MVOL,        0x000 },
	{ AC97MVLM,        0x000 },
	{ AC97PHON,        0x008 },
	{ AC97MIC,         0x008 },
	{ AC97LINE,        0x808 },
	{ AC97CD,          0x808 },
	{ AC97VID,         0x808 },
	{ AC97AUX,         0x808 },
	{ AC97PCM,         0x808 },
	{ AC97RSEL,        0x000 },
	{ AC97RGAIN,       0x000 },
	{ AC97RGMIC,       0x000 }
};


EMUSTAT _AC97config(HALID);
static EMUSTAT _AC97deconfig(HALID);
static EMUSTAT _AC97getdefault(HALID, MXRCONTROLID ctrl, DWORD *value);


/*************************************************************************
 * @func Dispatch AC97 function requests to the appropriate place.
 */
EMUSTAT
_AC97ctrlhndlr(stMXRState *state, enFuncValue func, MXRCONTROLID ctrlid,
               DWORD value, CHAR *szParam)
{
	switch (func) {
	case MXRFUNC_CONFIG: 
		state->dwMixerType = MXRTYPE_AC97;
		return _AC97config(state->halid); 

	case MXRFUNC_DECONFIG:
		return _AC97deconfig(state->halid); 

	case MXRFUNC_SETCONTROL:
       	AC97Write(state->halid, (BYTE) mxrAC97Controls[ctrlid].wRegister, 
			      (WORD) value);
		return SUCCESS;

	case MXRFUNC_GETCONTROL:
		*(DWORD*) value = AC97Read(state->halid, (BYTE) mxrAC97Controls[ctrlid].wRegister);
		return SUCCESS;

	case MXRFUNC_GETDEFAULT:
		return _AC97getdefault(state->halid, ctrlid, (DWORD*) value);

	default:
		return MXRERR_UNKNOWN_FUNC;
	}

	/* We should never get here, but... */
	return MXRERR_UNKNOWN_FUNC;
}


/*************************************************************************
 * @func Configure an AC97-compliant mixer.
 */
EMUSTAT
_AC97config(HALID halid)
{
	WORD      wAC97Status;    /* Codec ready status read */
	DWORD     dwCount=0;         /* Codec ready check termination */
	DWORD		scratch;

	/* First, set the codec type carefully, so we dont tube someone else */
	scratch= L8010SERegRead(halid, HC);
	scratch &= ~HC_CF;			/* Clear em, dano */
	scratch |= HC_CF_AC97;		/* set the type */
	L8010SERegWrite(halid, HC,scratch);

	/* Spin waiting for the AC97 codec ready bit to be asserted */
	do {
		wAC97Status = (W8010SERegRead(halid, AC97A) & 0x80);
		dwCount++;
		halWaitWallClockCounts(halid, 3);
	} while (!wAC97Status && dwCount < 10000);

	if (!wAC97Status) {
		DPRINTF(("WARNING! AC97 didn't configure.\n"));
		RETURN_ERROR(MXRERR_INIT_FAILED);
	}

	AC97Write(halid, AC97PCM,   0x808);    /* PCM input level */
	AC97Write(halid, AC97MVOL,  0x0);      /* Don't attenuate main volume */
	AC97Write(halid, AC97RGAIN, 0x0);      /* Specify record gain */
	AC97Write(halid, AC97RSEL,  0x404);    /* Select Line input device */
	AC97Write(halid, AC97RGMIC, 0x0);      /* Unmute the microphone */

	/* Finally, enable audio output */
	dwCount = L8010SERegRead(halid, HC);
	L8010SERegWrite(halid, HC, dwCount|HC_EA);

	return SUCCESS;
}


/***********************************************************************
 * @func Deconfigure an AC97 mixer.  This function just mutes the
 *  master output.
 */
static EMUSTAT
_AC97deconfig(HALID halid)
{
	AC97Write(halid, AC97MVOL, 0x8808);
	return SUCCESS;
}


/**********************************************************************
 * @func Find the default value for a specified AC97 control.  We keep
 *  around a table of default control values for just this purpose.
 */
static EMUSTAT
_AC97getdefault(HALID halid, MXRCONTROLID ctrl, DWORD *value)
{
	WORD     w;
    WORD     wNumControls = sizeof(mxrAC97Controls) / sizeof(stCtrlInfo);

	for (w = 0; w < wNumControls; w++) {
		if (ctrl == (MXRCONTROLID) mxrAC97Controls[w].wRegister) {
			*value = mxrAC97Controls[w].dwDefaultValue;
		    return SUCCESS;
		}
	}

	return MXRERR_BAD_CONTROL;
}



/*************************************************************************
 * ECARD functional implementation
 *************************************************************************/



/* In A1 Silicon, these bits are in the HC register */
#  define HOOKN_BIT   (1L << 12)
#  define HANDN_BIT   (1L << 11)
#  define PULSEN_BIT  (1L << 10)

#define EC_GDI1 (1 << 13)
#define EC_GDI0 (1 << 14)

#define EC_NUM_CONTROL_BITS 20

#define EC_AC3_DATA_SELN  0x0001L
#define EC_EE_DATA_SEL    0x0002L
#define EC_EE_CNTRL_SELN  0x0004L
#define EC_EECLK          0x0008L
#define EC_EECS           0x0010L
#define EC_EESDO          0x0020L
#define EC_TRIM_CSN	  0x0040L
#define EC_TRIM_SCLK	  0x0080L
#define EC_TRIM_SDATA	  0x0100L
#define EC_TRIM_MUTEN	  0x0200L
#define EC_ADCCAL	  0x0400L
#define EC_ADCRSTN	  0x0800L
#define EC_DACCAL	  0x1000L
#define EC_DACMUTEN	  0x2000L
#define EC_LEDN		  0x4000L

#define EC_SPDIF0_SEL_SHIFT	15
#define EC_SPDIF1_SEL_SHIFT	17	
#define EC_SPDIF0_SEL_MASK	(0x3L << EC_SPDIF0_SEL_SHIFT)
#define EC_SPDIF1_SEL_MASK	(0x7L << EC_SPDIF1_SEL_SHIFT)
#define EC_SPDIF0_SELECT(_x) (((_x) << EC_SPDIF0_SEL_SHIFT) & EC_SPDIF0_SEL_MASK)
#define EC_SPDIF1_SELECT(_x) (((_x) << EC_SPDIF1_SEL_SHIFT) & EC_SPDIF1_SEL_MASK)


/* Most of this stuff is pretty self-evident.  According to the hardware 
 * dudes, we need to leave the ADCCAL bit low in order to avoid a DC 
 * offset problem.  Weird.
 */
#define EC_RAW_RUN_MODE	(EC_DACMUTEN | EC_ADCRSTN | EC_TRIM_MUTEN | EC_TRIM_CSN)


#define EC_DEFAULT_ADC_GAIN   0xC4C4
#define EC_DEFAULT_SPDIF0_SEL 0x0
#define EC_DEFAULT_SPDIF1_SEL 0x1

#define MDECLARE_AND_SETUP(_halid) \
		DWORD __dwHCValue = L8010SERegRead(_halid, HC) & ~(HOOKN_BIT|HANDN_BIT|PULSEN_BIT);
#define MWRITE(_halid, _x) \
		L8010SERegWrite(_halid, HC,	__dwHCValue | (_x));
		
/* Command definitions for the 93C46 EEPROM. */
#define EC_EEPROM_READ      0x2
#define EC_EEPROM_ERASE     0x3
#define EC_EEPROM_WRITE     0x1
#define EC_EEPROM_WRTENB    0x0
#define EC_EEPROM_WRTDIS    0x0


/* ECARD state structure.  This structure maintains the state
 * for various portions of the the ECARD's onboard hardware.
 */
typedef struct {
	DWORD   dwControlBits;
	WORD    wADCGain;
	WORD    wMux0Setting;
	WORD    wMux1Setting;
	WORD	wMux2Setting;
} stECARDState;


/* Private routines */
static EMUSTAT _ECARDconfig(stMXRState*);
static EMUSTAT _ECARDdeconfig(stMXRState*);
static EMUSTAT _ECARDsetcontrol(stMXRState*, MXRCONTROLID, DWORD);
static EMUSTAT _ECARDgetcontrol(stMXRState*, MXRCONTROLID, DWORD*);
static EMUSTAT _ECARDgetdefault(stMXRState*, MXRCONTROLID, DWORD*);
static EMUSTAT _ECARDgetdisplay(stMXRState*, MXRCONTROLID, DWORD, CHAR*);
static void    _ECARDsetadcgain(stMXRState*, WORD value);

/* These guys are semi-private, but they're used by other DOS tools */
void _ECARDwrite(HALID, DWORD);
WORD _ECARDread_eeprom(HALID, WORD);
void _ECARDwrite_eeprom(HALID, WORD, WORD);
void _ECARDread_serialnum(HALID,  WORD, CHAR *);
void _ECARDwrite_serialnum(HALID, WORD, CHAR *);

static void _send_eeprom_bit(HALID, BOOL);
static void _send_eeprom_command(HALID, WORD);
static void _send_eeprom_addr(HALID, WORD);


/*************************************************************************
 * @func Receive control requests for the ECARD mixer and dispatch the
 *  requests to the appropriate routines.
 */

EMUSTAT
_ECARDctrlhndlr(stMXRState *state, enFuncValue func, MXRCONTROLID ctrlid,
               DWORD value, CHAR *szParam)
{
	switch (func) {
	case MXRFUNC_CONFIG:
		return _ECARDconfig(state); 

	case MXRFUNC_DECONFIG:
        return _ECARDdeconfig(state); 

	case MXRFUNC_SETCONTROL:
		return _ECARDsetcontrol(state, ctrlid, value);

	case MXRFUNC_GETCONTROL:
		return _ECARDgetcontrol(state, ctrlid, (DWORD*) value);

	case MXRFUNC_GETDEFAULT:
		return _ECARDgetdefault(state, ctrlid, (DWORD*) value);

	case MXRFUNC_GETDISPLAY:
		return _ECARDgetdisplay(state, ctrlid, value, szParam);
	default:
		return MXRERR_UNKNOWN_FUNC;
	}
}


/*************************************************************************
 * @func Configure the Ecard codecs.
 */

static EMUSTAT
_ECARDconfig(stMXRState *state)
{
	DWORD dwHCValue;
	stECARDState *pecard;

	state->dwMixerType = MXRTYPE_ECARD;

	/* Allocate a new ECARD state structure */
	pecard = (stECARDState*) osHeapAlloc(sizeof(stECARDState), 0);
	if (pecard == NULL)
		RETURN_ERROR(MXRERR_INIT_FAILED);

	/* Set up the initial settings */
	pecard->wMux0Setting = EC_DEFAULT_SPDIF0_SEL;
	pecard->wMux1Setting = EC_DEFAULT_SPDIF1_SEL;
	pecard->wMux2Setting = 0;
	pecard->wADCGain = EC_DEFAULT_ADC_GAIN;
	pecard->dwControlBits  = EC_RAW_RUN_MODE | 
							 EC_SPDIF0_SELECT(pecard->wMux0Setting) |
							 EC_SPDIF1_SELECT(pecard->wMux1Setting);

	state->dwMxrData = (DWORD) pecard;

	/* Step 0: Set the codec type in the hardware control register 
	 * and enable audio output */
	dwHCValue = L8010SERegRead(state->halid, HC);
	L8010SERegWrite(state->halid, HC, dwHCValue|HC_EA|0x10000L);
	L8010SERegRead(state->halid, HC);

	/* Step 1: Turn off the led and deassert TRIM_CS */
	_ECARDwrite(state->halid, EC_ADCCAL | EC_LEDN | EC_TRIM_CSN);

	/* Step 2: Calibrate the ADC and DAC */
	_ECARDwrite(state->halid, EC_DACCAL | EC_LEDN | EC_TRIM_CSN);

	/* Step 3: Wait for awhile;   XXX We can't get away with this
	 * under a real operating system; we'll need to block and wait that
	 * way. */
	halWaitWallClockCounts(state->halid, 48000);

	/* Step 4: Switch off the DAC and ADC calibration.  Note
	 * That ADC_CAL is actually an inverted signal, so we assert
	 * it here to stop calibration.  */
	_ECARDwrite(state->halid, EC_ADCCAL | EC_LEDN | EC_TRIM_CSN);

	/* Step 4: Switch into run mode */
	_ECARDwrite(state->halid, pecard->dwControlBits);

	/* Step 5: Set the analog input gain */
	_ECARDsetadcgain(state, pecard->wADCGain);

	return SUCCESS;
}


/**************************************************************************
 * @func Deconfigure the ECARD mixer.
 */
static EMUSTAT
_ECARDdeconfig(stMXRState *state)
{
	osHeapFree((OSVIRTADDR) state->dwMxrData);
	return SUCCESS;
}


/**************************************************************************
 * @func Set one of the ECARD's mxr controls to the specified value.
 */
static EMUSTAT
_ECARDsetcontrol(stMXRState *state, MXRCONTROLID ctrl, DWORD value)
{
	stECARDState *pecard = (stECARDState*) state->dwMxrData;

	switch (ctrl) {
	case mxrADCGain:
		pecard->wADCGain = (WORD) value;
		_ECARDsetadcgain(state, pecard->wADCGain);
		break;

	case mxrMuxSelect0:
		ASSERT(value <= mxridDI_SPDIF1);
		pecard->wMux0Setting = (WORD) value;
		pecard->dwControlBits &= ~EC_SPDIF0_SEL_MASK;
		pecard->dwControlBits |= EC_SPDIF0_SELECT(value);
		_ECARDwrite(state->halid, pecard->dwControlBits);
		break;

	case mxrMuxSelect1:
		ASSERT(value <= mxridCD_SPDIF);
		pecard->wMux1Setting = (WORD) value;
		pecard->dwControlBits &= ~EC_SPDIF1_SEL_MASK;
		pecard->dwControlBits |= EC_SPDIF1_SELECT(value);
		_ECARDwrite(state->halid, pecard->dwControlBits);
		break;

	case mxrMuxSelect2:
		ASSERT(value <= 1);
		pecard->wMux2Setting = (WORD) value;
		pecard->dwControlBits &= ~EC_AC3_DATA_SELN;
		pecard->dwControlBits |= (value ? EC_AC3_DATA_SELN : 0);
		_ECARDwrite(state->halid, pecard->dwControlBits);
		break;
	
	case mxrBoardRev:
		RETURN_ERROR(MXRERR_READ_ONLY);
		break;

	case mxrSerialNum:
		RETURN_ERROR(MXRERR_READ_ONLY);
		break;

	default:
		RETURN_ERROR(MXRERR_BAD_CONTROL);
	}

	return SUCCESS;
}


/**************************************************************************
 * @func Retrieve the current value of one of the ECARD's mxr controls.
 */
static EMUSTAT
_ECARDgetcontrol(stMXRState *state, MXRCONTROLID ctrl, DWORD *value)
{
	stECARDState *pecard = (stECARDState*) state->dwMxrData;

	switch (ctrl) {
	case mxrADCGain:
		*value = pecard->wADCGain;
		break;

	case mxrMuxSelect0:
		*value = pecard->wMux0Setting;
		break;

	case mxrMuxSelect1:
		*value = pecard->wMux1Setting;
		break;

	case mxrMuxSelect2:
		*value = pecard->wMux2Setting;
		break;

	case mxrBoardRev:
		*value = (DWORD) _ECARDread_eeprom(state->halid, EC_BOARDREV1_ADDR);
		*value <<= 16;
		*value |= (DWORD) _ECARDread_eeprom(state->halid, EC_BOARDREV0_ADDR);
        _ECARDwrite(state->halid, pecard->dwControlBits);
		break;

	case mxrSerialNum:
		{
		char szSerial[21];

		_ECARDread_serialnum(state->halid, 21, szSerial);
        _ECARDwrite(state->halid, pecard->dwControlBits);
		*value = strtoul(&szSerial[13], NULL, 10);
		}
		break;
		

	case mxrHasAudioBay:
		if (L8010SERegRead(state->halid, HC) & EC_GDI0)
			*value = 0x1;
		else
			*value = 0x0;
		break;

	case mxrHasDigitalIfc:
		_ECARDwrite(state->halid, EC_RAW_RUN_MODE);
		if (L8010SERegRead(state->halid, HC) & EC_GDI1)
			*value = 0x1;
		else
			*value = 0x0;
		break;

    case mxrGPSPDIFStatus:
        if (LSEPtrRead(state->halid, CDSRCS) & 0x3000000L)
            *value = 0x1;
        else
            *value = 0x0;
        break;

    case mxrCDSPDIFStatus:
        if (LSEPtrRead(state->halid, GPSRCS) & 0x3000000L)
            *value = 0x1;
        else
            *value = 0x0;
        break;

	default:
		RETURN_ERROR(MXRERR_BAD_CONTROL);
	}

	return SUCCESS;
}


/**************************************************************************
 * @func Set one of the ECARD's mxr controls to the specified value.
 */
static EMUSTAT
_ECARDgetdefault(stMXRState *state, MXRCONTROLID ctrl, DWORD *value)
{
	stECARDState *pecard = (stECARDState*) state->dwMxrData;

	switch (ctrl) {
	case mxrADCGain:
		*value = EC_DEFAULT_ADC_GAIN;
		break;

	case mxrMuxSelect0:
		*value = EC_DEFAULT_SPDIF0_SEL;
		break;

	case mxrMuxSelect1:
		*value = EC_DEFAULT_SPDIF1_SEL;
		break;

	default:
		RETURN_ERROR(MXRERR_BAD_CONTROL);
	}

	return SUCCESS;
}


/**************************************************************************
 * @func Create the textual representation for a particular value.
 *  For most values, we just call _ECARDgetvalue and sprintf the value
 *  into the string.  For serial numbers, though, we actually display the entire
 *  serial number.
 */

static EMUSTAT
_ECARDgetdisplay(stMXRState *state, MXRCONTROLID ctrl, DWORD dwSize,
                 CHAR *szParam)
{
    stECARDState *pecard = (stECARDState*) state->dwMxrData;

	if (ctrl == mxrSerialNum) 
	{
		_ECARDread_serialnum(state->halid, (WORD) dwSize, szParam);
        _ECARDwrite(state->halid, pecard->dwControlBits);
	} 
	else 
	{
		DWORD dwValue;
		
		_ECARDgetcontrol(state, ctrl, &dwValue);
		sprintf(szParam, "%ld", dwValue);
	}

	return SUCCESS;
}


/**************************************************************************
 * @func Set the gain of the ECARD's CS3310 Trim/gain controller.  The
 * trim value consists of a 16bit value which is composed of two
 * 8 bit gain/trim values, one for the left channel and one for the
 * right channel.  The following table maps from the Gain/Attenuation
 * value in decibels into the corresponding bit pattern for a single
 * channel.
 */

static void
_ECARDsetadcgain(stMXRState *state, WORD wGain) 
{
	DWORD         dwCurrBit;
	HALID         halid = state->halid;
	stECARDState* pecard = (stECARDState*) state->dwMxrData;

	pecard->wADCGain = wGain;

	/* Enable writing to the TRIM registers */
	_ECARDwrite(halid, pecard->dwControlBits & ~EC_TRIM_CSN);

	/* Do it again to insure that we meet hold time requirements */
	_ECARDwrite(halid, pecard->dwControlBits & ~EC_TRIM_CSN);

	for (dwCurrBit = (1L << 15); dwCurrBit; dwCurrBit >>= 1) {

		DWORD value = pecard->dwControlBits & ~(EC_TRIM_CSN|EC_TRIM_SDATA);

		if (wGain & dwCurrBit)
		      value |= EC_TRIM_SDATA;

		/* Clock the bit */
		_ECARDwrite(halid, value);
		_ECARDwrite(halid, value | EC_TRIM_SCLK);
		_ECARDwrite(halid, value);
	}

	_ECARDwrite(halid, pecard->dwControlBits); 
}

 
/**************************************************************************
 * @func Clock bits into the Ecard's control latch.  The Ecard uses a
 *  control latch will is loaded bit-serially by toggling the Modem control
 *  lines from function 2 on the E8010.  This function hides these details
 *  and presents the illusion that we are actually writing to a distinct
 *  register.
 */
void
_ECARDwrite(HALID halid, DWORD value)
{
	WORD          wCount;
	DWORD         dwData;

	MDECLARE_AND_SETUP(halid);
	MWRITE(halid, 0x0);


	for (wCount = 0 ; wCount < EC_NUM_CONTROL_BITS; wCount++) {
	
		/* Set up the value */
		dwData = ((value & 0x1) ? PULSEN_BIT : 0);
		value >>= 1;

		MWRITE(halid, dwData);

		/* Clock the shift register */
		MWRITE(halid, dwData | HANDN_BIT);
		MWRITE(halid, dwData);
	}

	/* Latch the bits */
	MWRITE(halid, HOOKN_BIT);
	MWRITE(halid, 0x0);

}


/*****************************************************************************
 * Read a 16-bit value from the 93C46 EEPROM on the ECARD.
 */
 
WORD
_ECARDread_eeprom(HALID halid, WORD wAddr)
{
	WORD wBitCnt, wBitMask, wRead;

	
	/* Set the Chip select and make sure that the data gets steered
	 * to the PAL */

	_ECARDwrite(halid, EC_RAW_RUN_MODE | EC_EE_DATA_SEL | EC_EECS);
	
	/* Send a command */
	_send_eeprom_command(halid, EC_EEPROM_READ);
	_send_eeprom_addr(halid, wAddr);

	/* Now we start clocking the bits in, MSB first */
	for (wBitCnt = 16, wBitMask = 0x8000, wRead = 0;
	     wBitCnt > 0; wBitCnt--, wBitMask >>= 1) 
	{
		/* Clock the bit */
		_ECARDwrite(halid, EC_RAW_RUN_MODE | EC_EE_DATA_SEL | EC_EECS | 
			           EC_EECLK);
		_ECARDwrite(halid, EC_RAW_RUN_MODE | EC_EE_DATA_SEL | EC_EECS);

		if (L8010SERegRead(halid, HC) & EC_GDI1) {
			wRead |= wBitMask;
		}
	}


	/* Turn off the chip select */
	_ECARDwrite(halid, EC_RAW_RUN_MODE);

	return wRead;
}


/****************************************************************************
 * Write a 16-bit value into the EEPROM.   The EEPROM has 64 16-bit memory
 * locations, so we address it using WORD parameters.
 */

void
_ECARDwrite_eeprom(HALID halid, WORD wAddr, WORD wValue)
{
	WORD wBitMask;
	DWORD dwTimeout = 0;
	DWORD i;

	/* Enable writing */
	_ECARDwrite(halid, EC_RAW_RUN_MODE | EC_EE_DATA_SEL | EC_EECS);
	_send_eeprom_command(halid, EC_EEPROM_WRTENB);
	_send_eeprom_addr(halid, 0x30);
	_ECARDwrite(halid, EC_RAW_RUN_MODE | EC_EE_DATA_SEL);

	/* Begin the write cycle */
	_ECARDwrite(halid, EC_RAW_RUN_MODE | EC_EE_DATA_SEL | EC_EECS);
	_send_eeprom_command(halid, EC_EEPROM_WRITE);
	_send_eeprom_addr(halid, wAddr);
	
	/* Transmit the value to be written */	
	wBitMask = 0x8000;
	for (i = 0; i < 16; i++) {
		_send_eeprom_bit(halid, wValue & wBitMask);
		wBitMask >>= 1;
	}

	/* Deassert the chip select */
	_ECARDwrite(halid, EC_RAW_RUN_MODE);
	halWaitWallClockCounts(halid, 2000);
}


/**************************************************************************
 * Read and write the serial number from and to the EEPROM.  This is more
 * complicated than reading a single word, since the serial number may be
 * 24 characters in length.  
 */

#define SERIAL_LENGTH 24

void 
_ECARDread_serialnum(HALID halid, WORD wLength, CHAR *szString)
{
	char  szSerial[SERIAL_LENGTH];
	WORD i;

	memset(szString, 0x0, wLength);

	for (i = 0; i < SERIAL_LENGTH/2; i++) {
		WORD wValue = _ECARDread_eeprom(halid, (WORD) (EC_SERIALNUM_ADDR + i));

		szSerial[i*2]     = (CHAR) (wValue >> 8) & 0xFF;
		szSerial[i*2 + 1] = (CHAR) (wValue & 0xFF);
	}


	wLength = ((wLength < SERIAL_LENGTH) ? wLength : SERIAL_LENGTH);
	strncpy(szString, szSerial, wLength);
}


void
_ECARDwrite_serialnum(HALID halid, WORD wLength, CHAR *szString)
{
	WORD wChecksum;
	char szSerial[SERIAL_LENGTH];
	WORD i;

	ASSERT(wLength <= SERIAL_LENGTH);
	memset(szSerial, 0x0, SERIAL_LENGTH);

	wLength = ((wLength < SERIAL_LENGTH) ? wLength : SERIAL_LENGTH);
	strncpy(szSerial, szString, wLength);

	for (i = 0; i < SERIAL_LENGTH/2; i++) {
		WORD wValue = (szSerial[i*2] << 8) | szSerial[i*2 + 1];
		_ECARDwrite_eeprom(halid, (WORD) (EC_SERIALNUM_ADDR + i), wValue);
	}
	
	/* Update checksum */
	wChecksum = _ECARDcompute_checksum(halid);
	_ECARDwrite_eeprom(halid, EC_CHECKSUM_ADDR, wChecksum);
}


/**************************************************************************
 * Recompute the EEPROM's checksum.  This should be done whenever the contents
 * have been modified.  For purposes of efficiency, though, we don't call this
 * in _ECARDwrite_eeprom.
 */

WORD
_ECARDcompute_checksum(HALID halid)
{
	WORD w;
	WORD wSum = 0;
	WORD wValue;

	/* Calculate the checksum for the prom.  We skip the CHECKSUM
	 * address since including it will cause mismatches all the time 
	 */
	for (w = 0; w < EC_EEPROM_SIZE; w++) {

		if (w == EC_CHECKSUM_ADDR)
			continue;

		wValue = _ECARDread_eeprom(halid, w);
	
		if (wValue > 0)	
			wSum += (wSum >> 1) + wValue;
	}

	return wSum;	
}


/**************************************************************************
 * The following are utility functions used to manipulate the 93C46 EEPROM
 * on the ECARD.
 */

static void
_send_eeprom_bit(HALID halid, BOOL bit)
{
	DWORD 	dwData = EC_RAW_RUN_MODE | EC_EE_DATA_SEL | EC_EECS;

	if (bit) {
		_ECARDwrite(halid, dwData | EC_EESDO);
		_ECARDwrite(halid, dwData | EC_EESDO | EC_EECLK);
		_ECARDwrite(halid, dwData | EC_EESDO);
	} else {
		_ECARDwrite(halid, dwData);
		_ECARDwrite(halid, dwData | EC_EECLK);
		_ECARDwrite(halid, dwData);
	}
}


static void
_send_eeprom_command(HALID halid, WORD wCommand)
{
	_ECARDwrite(halid, EC_RAW_RUN_MODE | EC_EE_DATA_SEL | EC_EECS);

	/* Toggle in the start bit */
	_send_eeprom_bit(halid, TRUE);

	/* Send out the command */
	_send_eeprom_bit(halid, (wCommand & 0x2));
	_send_eeprom_bit(halid, (wCommand & 0x1));
}


static void
_send_eeprom_addr(HALID halid, WORD wAddr)
{
	WORD i;
	WORD wCurrBit = (1 << 5);

	for (i = 0; i <= 5; i++) {
		_send_eeprom_bit(halid, wAddr & wCurrBit);
		wCurrBit >>= 1;
	}
}


static BOOL
_check_eeprom_checksum(HALID halid)
{
	WORD wComputedChecksum = _ECARDcompute_checksum(halid);
	WORD wStoredChecksum = _ECARDread_eeprom(halid, EC_CHECKSUM_ADDR);

	return (wComputedChecksum == wStoredChecksum);
}
