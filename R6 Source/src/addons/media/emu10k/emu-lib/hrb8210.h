/*****************************************************************************
*
*                             Copyright (c) 1997
*                E-mu Systems Proprietary All rights Reserved
*
*****************************************************************************/

/*****************************************************************************
*
* @doc INTERNAL
* @module hrb8210.h | 
* This file contains the public datatypes and functions used by the EMU 8010
* host recording buffer manager.
*
* @iex 
* Revision History:
*
* Person            Date			Reason
* ---------------	------------    --------------------------------------
* John Kraft		Oct 27, 97		Created.
*
*****************************************************************************
* @doc EXTERNAL 
*  This document describes the HRB8210 Host Record Buffer API.  The HRB
*  allows multiple clients to select one or more of three primary sound 
*  input sources (the microphone, Analog-to-Digital Converter, and FX 
*  Engine Outputs) and read samples from them.  The HRB will perform limited
*  sample format conversion and mixing; the client can select between 
*  8- and 16-bit inputs and control whether multiple channels get mixed into
*  a single mono channel or are presented as discrete streams.  For devices
*  which produce a large number of channels, such as the FX Output, the
*  HRB also allows clients to extract a particular set of channels from the
*  incoming data stream in an efficient manner.
*  
* @contents1 Contents |
* 
* @subindex Enumerations
* @subindex Typedefs & Structures
* @subindex Public Functions
******************************************************************************/
#ifndef __HRB8210_H
#define __HRB8210_H

/****************
 * Include files
 ****************/

#include "datatype.h" 
#include "sacommon.h"


/******************************************************************************
* @contents2 Enumerations And Public Definitions |
* @index enum |
******************************************************************************/

/* Error values */
#define HRBERR_BAD_HANDLE                 1
#define HRBERR_BAD_PARAM                  2
#define HRBERR_BAD_DEVICE                 3
#define HRBERR_BAD_SAMPLERATE             4
#define HRBERR_CANT_CHANGE_SAMPLERATE     5
#define HRBERR_BAD_CHANNELS               6
#define HRBERR_BAD_SAMPLESIZE             7
#define HRBERR_BAD_DEVINFO                8
#define HRBERR_BAD_BUFFER                 9
#define HRBERR_ALREADY_STARTED           10
#define HRBERR_INIT_FAILED               11
#define HRBERR_NO_MEM                    12
#define HRBERR_DATA_CORRUPT				 13
#define HRBERR_NOT_STARTED               14
#define HRBERR_INVALID_BUFFER_SIZE       15
#define HRBERR_INTR_FAILED               16


/* @enum HRBDEVICEVALUE |
 *  This enumeration contains all of the possible devices from which
 *  samples can be read.  Currently, three input devices are supported.
 */
typedef enum {
	hrbdevADC,        /* @emem The Analog-to-Digital Converter input */
	hrbdevMic,        /* @emem The microphone input */
	hrbdevFXOut,      /* @emem The FX Engine outputs */
} HRBDEVICEVALUE;


/* @enum HRBSAMPRATEVALUE |
 *  Enumerates all of the sample rates supported by any of the input devices.
 *  The sample rates are actually stored as distinct bits so that a user
 *  can query the HRB to determine which sample rates are supported by 
 *  a particular device.
 */
typedef enSASampleRate HRBSAMPRATEVALUE;

typedef enSASampleFormat HRBSAMPFORMATVALUE;

/***************************************************************************
 * @contents2 Types and structure definitions |
 * @index type,struct |
 ***************************************************************************/

/* @type HRBID | 
 *  An opaque handle which is used to reference a particular instance
 *  of the HRB for a particular E8010 chip.  This value is returned
 *  by <f hrbDiscoverChip>.
 */
typedef DWORD HRBID;


/* @type HRBHANDLE |
 *  An opaque handle that references a particular device
 *  which has been opened by a client.  An HRBHANDLE is returned by
 *  <f hrbOpenDevice>.
 */
typedef DWORD HRBHANDLE;

/* @type HRBCALLBACK | While recording, the HRB periodically calls back to
 *  the client and passes back an HRBBUFFER pointer which refers to a
 *  buffer which has been filled with data.  Buffers may be linked together
 *  into a list using the <t pNextBuffer> field in the <t HRBBUFFER> 
 *  structure, and the callback routine must follow the linked list to
 *  ensure that all the buffers have been processed.

 * @flag HRBBUFFER | A pointer to the head of an <t HRBBUFFER> list which 
 *  was just filled with data.  The buffers on this list were originally 
 *  passed to the HRB via <f hrbDevAddBuffers>.
 */
typedef void (*HRBCALLBACK)(stSAInputBuffer*);

/* @struct HRBDEVCONFIG |
 *  Contains a description of how a device should be configured when
 *  is is opened with the <f hrbOpenDevice> function.
 */

#define HRB_CONFIGFLAGS_LOOP            1
#define HRB_CONFIGFLAGS_SYNCABLE        2

typedef struct stDevConfigTag {
    HRBCALLBACK       callback;    /* @field Callback function which gets 
									*  invoked when a buffer is filled. */
	DWORD             dwChanSelect;/* @field A bit field indicating
									*  which channels should be extracted
									*  when the user is requesting fewer than
									*  the total number of channels supported
									*  by a device.  */
	HRBDEVICEVALUE    device;      /* @field The device to be opened */
	HRBSAMPRATEVALUE  sampleRate;  /* @field The sample rate to be used. */
	BYTE              byNumChans;  /* @field The number of channels the 
								    *  client wants to read.  If this is 
									*  set one and the number of bits set
									*  in the <t dwChannelSelect> field is
									*  greater than 1, a mono mix will occur.
									*  Otherwise, this field must be set to
									*  the number of bits set in
									*  <t dwChannelSelect>.  */
	BYTE			  bySampleSize;/* @field The size of the sample in 
									*  bytes.  We currently only support
									*  values of 1 and 2 for this field. */
    DWORD             dwFlags;     /* @field Control flags.  */
} HRBDEVCONFIG;


/* @struct HRBDEVINFO |
 *  The HRBDEVINFO structure contains fields which describe various
 *  characteristics of a device.  A client can retrieve device information
 *  by passing this structure to the <f hrbQueryDevice> function.
 */
typedef stSADevCaps HRBDEVINFO;

/* @struct HRBBUFFER | 
 *  The HRBBUFFER structure is used to convey information about the
 *  data buffers into which incoming sample data is to be placed.  Each
 *  buffer points to a contiguous range of memory to which data should
 *  be written.  In order to read data, the client must allocate at least
 *  one of these structures, set up its fields appropriately, and hand it 
 *  to the HRB using the <f hrbDevAddBuffers> function.  Once the buffer is 
 *  filled with sample  data, it will be returned to the client via the 
 *  callback function specified in the <t HRBDEVCONFIG> structure when
 *  the device was opened.
 */
typedef stSAInputBuffer HRBBUFFER;

/* @struct HRBATTRIB | Contains various attributes of the HRB that
 *  might be interesting to the client.
 */
typedef struct {
	DWORD     blah;
} HRBATTRIB;



/***************************************************************************
 * @contents2 Public Functions |
 * @index func |
 ***************************************************************************/

BEGINEMUCTYPE

/* @func This function fills an array with the IDs of all of
 *  the discovered HRB managers in the system and returns a count of the
 *  total number of sound engines.  The caller is allowed to 
 *  pass NULL for either or both of the arguments; in this case,
 *  the function will just return the total number of sound engines
 *  without attempting to dereference the array.
 *
 *  This function only returns managers for which the <f hrbDiscoverChip>
 *  function has been previously called. 
 *
 * @parm DWORD | count | The number of HRBID handles in the array.
 *  If 'count' is less than the total number of HRBS, only
 *  the first 'count' IDs will be copied into the array.  If 'count' is 0,
 *  the function will not attempt to fill the array.
 * @parm HRBID * | seids | An array of 'count' SEID handles.
 *	If NULL, the routine will not attempt to fill the array with IDs.
 * 
 * @rdesc The total number of HRB managers in the system.  If an error
 *  occurs, the function will return 0.
 */
EMUAPIEXPORT DWORD hrbGetHardwareInstances(DWORD count /* VSIZE */, 
										  HRBID *seids /* IO */);


/* @func Copy the name of the specified HRB manager into the given
 *  array.  At most 'count' characters are copied.  The system guarantees
 *  that a HRB manager name will always be fewer than 63 characters in
 *  length, so a caller can safely allocate the space for the string on the
 *  stack.
 *
 * @parm HRBID | hrbid | The ID of the sound engine whose name is being
 *  retrieved.
 * @parm DWORD | count | The size of the destination array, in characters.
 *  If the 'count' is less than the length of the name string, 'count' - 1 
 *  characters will be copied into the array and a terminating '\0' will be 
 *  placed in the last element of the array.
 * @parm char * | szName | A character array into which the device name will
 *  copied.
 *
 * @rdesc Returns SUCCESS if the retrieval completed successfully.  Otherwise,
 *  one of the following error values is returned:
 *		@flag HRBERR_BAD_HANDLE | the sound engine corresponding to seid wasn't found.  This
 *		 shouldn't happen unless an invalid seid is passed.
 *      @flag HRBERR_BAD_PARAM | A NULL or invalid pointer was passed in for 
 *       szName.
 */
EMUAPIEXPORT EMUSTAT hrbGetModuleName(HRBID id, DWORD count /* VSIZE */, 
                                      char *szName /* IO */);


/* @func Fill an attribute data structure with the attributes for the
 *  specified sound engine.
 */
EMUAPIEXPORT EMUSTAT hrbGetModuleAttributes(HRBID id, HRBATTRIB *attrib /* IO */);


/* @func Opens the specified device using the values specified in the
 *  HRBDEVCONFIG structure.  The HRBDEVCONFIG structure specifies the
 *  device to be opened, control parameters for the device (such as
 *  the sample rate), and characteristics of the incoming sample stream.
 *  Not all devices support all parameter settings.  If the client
 *  specifies an illegal set of parameters, an error will be returned.
 *
 * @parm HRBID | hrbid | The ID of the host record buffer instance whose
 *  from which the device is to be allocated.
 * @parm HRBDEVCONFIG * | config | A pointer to an HRBDEVCONFIG structure
 *  which describes how the device and the sample information is to be
 *  configured.
 * @parm HRBHANDLE * | retHandle | A pointer to the HRBHANDLE which will
 *  receive the open device reference.
 *
 * @rdesc If the device is opened without problems, SUCCESS is returned.
 *  Otherwise, one of the following values will be returned:
 *   @flag HRBERR_BAD_HANDLE | The value in <t hrbid> is invalid.
 *   @flag HRBERR_BAD_DEVICE | The configuration structure has an invalid
 *    value specified for the device.
 *   @flag HRBERR_BAD_SAMPLERATE | The sample rate specified in the
 *    configuration structure isn't supported by the specified device.
 *   @flag HRBERR_CANT_CHANGE_SAMPLERATE | Another device already has
 *    opened the device with a different sample rate.  
 *   @flag HRBERR_BAD_CHANNELS | Something is wrong with the
 *    <t dwChannelSelect> or <t byNumChans> fields.  Either they don't
 *    agree, or the client is attempting to read too many channels.
 *   @flag HRBERR_BAD_SAMPLESIZE | The value specified in <t bySampleSize>
 *    is invalid.
 *   @flag HRBERR_NO_MEM | The driver couldn't allocate space for the internal
 *    open file information.
 */
EMUAPIEXPORT EMUSTAT hrbOpenDevice(HRBID hrbid, HRBDEVCONFIG *config, 
								   HRBHANDLE *retHandle);


/* @func Closes a previously opened device.  If the device is currently
 *  recording, this function will first call <f hrbDevReset> before
 *  deallocating the device handle.  Any buffers which are queued will
 *  be discarded; the client is responsible for either calling <f hrbDevReset>
 *  before closing the device or retaining references to the queued buffers.
 *
 * @parm HRBHANDLE | hrbh | The handle of the device to close.
 * 
 * @rdesc If the device is closed without problems, SUCCESS is returned.
 *  Otherwise, one of the following will be returned:
 *	 @flag HRBERR_BAD_HANDLE | The value in <t hrbh> isn't valid.
 *	 @flag HRBERR_DATA_CORRUPT | An inconsistency was detected in the HRB's 
 *    internal data structures.  Probably indicates a bug in the driver, but it
 *    isn't immediately fatal.
 */
EMUAPIEXPORT EMUSTAT hrbCloseDevice(HRBHANDLE hrbh);


/* @func Returns the number of open handles which currently reference the
 *  specified device.  This function is useful if the client wishes to 
 *  insure that no one else currently has a device open.
 *
 * @parm HRBID | hrbid | The HRB instance to check.
 * @parm HRBDEVICEVALUE | device | The device to be checked.
 * @parm HRBDEVINFO * | devinfo | A pointer to the device information
 *  structure which will receive information about the device.
 *
 * @rdesc Returns SUCCESS if the data is retrieved successfully.  
 *  Otherwise, one of the following values is returned:
 *	@flag HRBERR_BAD_HANDLE | The HRBID is invalid.
 *  @flag HRBERR_BAD_DEVICE | The value in the <t device> parameter is 
 *   invalid.
 *  @flag HRBERR_BAD_DEVINFO | The value in the <t devinfo> parameter is
 *   invalid.
 */
EMUAPIEXPORT EMUSTAT hrbQueryDevice(HRBID hrbid, HRBDEVICEVALUE device, 
								  HRBDEVINFO *devinfo /* IO */);


/* @func Enqueues one or more buffers for incoming sample data on a 
 *  particular device.  Enqueuing a buffer makes it available to
 *  receive sample data once recording starts on the specified device. 
 *  Buffers are returned either incrementally as they are filled with 
 *  data via the callback or all at once when <f hrbDevReset> is called.
 *  The client may enqueue multiple buffers with a single call by linking
 *  the buffers together using the <t pNextBuffer> field in the buffer
 *  structures.  If only a single buffer is being passed, the client must
 *  ensure that the <t pNextBuffer> field is set to NULL.
 *
 * @parm HRBHANDLE | hrbh | The device which owns the queue onto which
 *  the buffer will be placed.
 * @parm HRBBUFFER | buffer | The actual buffer to enqueue.
 *
 * @rdesc Returns SUCCESS if the buffer is enqueued without error.
 *  Otherwise, one of the following will be returned:
 *  @flag HRBERR_BAD_HANDLE | The specified HRBHANDLE is invalid.
 *  @flag HRBERR_BAD_BUFFER | The buffer pointer is NULL or otherwise invalid.
 */
EMUAPIEXPORT EMUSTAT hrbDevAddBuffers(HRBHANDLE hrbh, HRBBUFFER *buffer);


/* @func Determines whether buffers are queued for a particular device
 *  and returns a boolean value.  This is used primarily to implement
 *  an aspect of the Windows multimedia system which requires that we
 *  check to see whether buffers are available before closing.
 *
 * @parm  * @parm HRBHANDLE | hrbh | The device which owns the queue onto which
 *  the buffer will be placed.
 *
 * @rdesc Returns TRUE if there are buffers queued, FALSE otherwise.
 */
EMUAPIEXPORT BOOL hrbDevHasBuffersQueued(HRBHANDLE hrbh);


/* @func This routine begins filling any buffers on the queue with incoming
 *  sample data from the selected device.  If no buffers have been queued,
 *  the system will just throw away audio data until the client enqueues
 *  the first buffer with <f hrbDevAddBuffer>.  
 *
 * @parm HRBHANDLE | hrbh | The device to start.
 *
 * @rdesc If input is started correctly, SUCCESS will be returned.
 *  Otherwise, one of the following will be returned:
 *	@flag HRBERR_BAD_HANDLE | The specified HRBHANDLE is invalid.
 *  @flag HRBERR_ALREADY_STARTED | The client has already started input
 */
EMUAPIEXPORT EMUSTAT hrbDevStart(HRBHANDLE hrbh);

/* @func Start multiple instances simultaneously.  Note that this
 *  function currently requires that all of the devices in the 
 *  array refer to the same device.  
 */
EMUAPIEXPORT EMUSTAT hrbDevStartSync(HRBHANDLE *ahrbh /*ARRAY 8 */, DWORD dwNumHandles);


/* @func Stops input, resets the sample counter, and dequeues all buffers
 *  from the queue.  The dequeued buffers will be returned via the 
 *  <t retBuffers> pointer.  A client should call this routine when finished
 *  reading samples for a period of time.  This routine can be called either
 *  when the device is started or when it is paused.
 *
 * @parm HRBHANDLE | hrbh | The file handle to reset.
 * @parm HRBBUFFER ** | retBufferList | A pointer to the HRBBUFFER pointer which
 *  will recieve the address of the head of the dequeued buffer list.  If
 *  retBuffer is NULL, the buffers will be dequeued but the head will
 *  not be returned; in this case, the HRB manager assumes that the client
 *  has retained pointers to the buffers.
 *
 * @rdesc If input is started correctly, SUCCESS will be returned.
 *  Otherwise, one of the following will be returned:
 *	@flag HRBERR_BAD_HANDLE | The specified HRBHANDLE is invalid.
 *  @flag HRBERR_BAD_BUFFER | The retBuffer pointer is invalid.
 */
EMUAPIEXPORT EMUSTAT hrbDevReset(HRBHANDLE hrbh, HRBBUFFER **retBufferList);


/* @func Stops reading samples and hands back to the client a pointer to 
 *  the head of a list of buffers which have been filled with data but
 *  have not yet been returned to the client.  Any unfilled buffers will 
 *  remain on the queue, and the sample counter is _not_ reset.  This
 *  function provides a convenient way of pausing input temporarily.
 *
 * @parm HRBHANDLE | hrbh | The file handle referring to the device to stop.
 * @parm HRBBUFFER ** | retBufferList | A pointer to the HRBBUFFER pointer which
 *  will recieve the address of the head of the dequeued buffer list.  If
 *  retBuffer is NULL, the buffers will be dequeued but the head will
 *  not be returned; in this case, the HRB manager assumes that the client
 *  has retained pointers to the buffers.
 *
 * @rdesc If input is started correctly, SUCCESS will be returned.
 *  Otherwise, one of the following will be returned:
 *	@flag HRBERR_BAD_HANDLE | The specified HRBHANDLE is invalid.
 *  @flag HRBERR_BAD_BUFFER | The retBuffer pointer is invalid.
 */
EMUAPIEXPORT EMUSTAT hrbDevStop(HRBHANDLE hrbh, HRBBUFFER **retBufferList);


/* @func Stop multiple open instances simultaneously.  Note that
 *  this function is currently only works if all of the handles
 *  passed refer to the same device on the same board.  
 */
EMUAPIEXPORT EMUSTAT hrbDevStopSync(HRBHANDLE *ahrbh /* ARRAY 8 */, DWORD dwNumHandles);

/* @func Returns the sample number of sample most recently DMA'ed into
 *  memory.  This can be used to provide a rough correspondence between
 *  the current time and a sample number.  This function may only be
 *  called if the device has been started.
 *
 * @parm HRBHANDLE | hrbh | A handle to a previously opened device.
 * 
 * @rdesc Returns the sample number.  If the device has not yet been started
 *  or is paused, the sample number number of the next sample to be read will
 *  be returned.
 */
EMUAPIEXPORT DWORD hrbDevGetCurrentSampleFrame(HRBHANDLE hrbh);


EMUAPIEXPORT EMUSTAT hrbDevFlushBuffers(DWORD dwNumHandles, HRBHANDLE *ahrbh);


EMUAPIEXPORT void hrbpInvokeCallback(HRBID, HRBDEVICEVALUE);

ENDEMUCTYPE

#endif /* __HRB8210_H */
