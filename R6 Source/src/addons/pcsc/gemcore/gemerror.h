/*******************************************************************************
*                    Copyright (c) 1995 Gemplus Development
*
* Name        : GemError.h
*
* Description : Errors and Warning definitions for all GEMPLUS API (GCR/GPS).
*
* Release     : 4.31.001
*
* Last Modif  : 13/10/97: V4.31.001  
*               17/01/97: V4.10.003 - Add GE_IFD_BUSY, GW_ALREADY_DONE and
*                                     GW_ICC_STATUS.
*               04/01/96: V4.10.002 - Replace LPSTR by char G_FAR *.
*               01/12/95: V4.10.001 - Update to new Gemplus 4.10 Version.
*               08/09/95: V4.01.005
*               06/07/94: V4.00.000
*
********************************************************************************
*
* Warning     :
*
* Remark      : The following convention applies to all definitions:
*                 OK      is coded by a null value,
*                 Error   are coded with negative values,
*                 Warning are coded with positive values.
*
*               The following definitions are used:
*                 ICC : Integrated Circuit Card
*                 IFD : InterFace Device
*                 GPS : Gemplus Personalization System.
*                 Host: Machine which drives the reader.
*
*******************************************************************************/


/*------------------------------------------------------------------------------
Name definition:
   _GEMERROR_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _GEMERROR_H
#define _GEMERROR_H


/*------------------------------------------------------------------------------
Successful operation:
   G_OK (0) if everything is under control.
------------------------------------------------------------------------------*/
#define G_OK                      0


/*------------------------------------------------------------------------------
Errors definitions:
   Error codes from ICC.
      GE_ICC_ABSENT    (- 1)                                                (FB)
         No ICC is present in IFD.
      GE_ICC_MUTE      (- 2)                                                (A2)
         ICC is mute.
      GE_ICC_UNKNOWN   (- 3)                                                (14)
         ICC is not supported.
      GE_ICC_PULL_OUT  (- 4)                                                (F7)
         ICC has been removed during command execution. Database may be
         corrupted.
      GE_ICC_NOT_POWER (- 5)                                                (15)
         ICC not reveiving power or has been removed-inserted between commands.
      GE_ICC_INCOMP    (- 6)
         ICC has caused a short circuit or is physically incompatible with IFD.
      GE_ICC_ABORT     (-10)                                                (A4)
          ICC sends an abort block (T=1 only).
------------------------------------------------------------------------------*/
#define GE_ICC_ABSENT            (-1)
#define GE_ICC_MUTE              (-2)
#define GE_ICC_UNKNOWN           (-3)
#define GE_ICC_PULL_OUT          (-4)
#define GE_ICC_NOT_POWER         (-5)
#define GE_ICC_INCOMP            (-6)
#define GE_ICC_ABORT            (-10)

/*------------------------------------------------------------------------------
   Error codes between ICC and IFD (II).
      GE_II_COMM      (-100)                                                (13)
         Communication is not possible between ICC and IFD.
      GE_II_PARITY    (-101)                                                (A3)
         Character parity error between ICC and IFD.
      GE_II_EDC       (-102)
         Error detection code raised.

      GE_II_ATR       (-110) 
         Incoherence in ATR.
      GE_II_ATR_TS    (-111)                                                (10)
         Bad TS in ATR.
      GE_II_ATR_TCK   (-112)                                                (1D)
         Bad TCK in ATR.
      GE_II_ATR_READ  (-113)                                                (03)
         Impossible to read some ATR bytes.
      GE_II_PROTOCOL  (-120)
         Inconsistent protocol.
      GE_II_UNKNOWN   (-121)                                                (17)
         Unknown protocol.
      GE_II_PTS       (-122)                                                (18)
         PTS is required for this choice.
      GE_II_IFSD_LEN  (-123)
         Received block length > IFSD (T=1 only).
      GE_II_PROC_BYTE (-124)                                                (E4)
         Bad procedure byte from ICC (T=0 only).
      GE_II_INS       (-125)                                                (11)
         Bad INS in a command (6X or 9X) (T=0 only).
      GE_II_RES_LEN   (-126)
         Message length from ICC not supported.
      GE_II_RESYNCH   (-127)
         3 failures ! Resynch required by ICC.
------------------------------------------------------------------------------*/
#define GE_II_COMM             (-100)
#define GE_II_PARITY           (-101)
#define GE_II_EDC              (-102)
#define GE_II_ATR              (-110)
#define GE_II_ATR_TS           (-111)
#define GE_II_ATR_TCK          (-112)
#define GE_II_ATR_READ         (-113)
#define GE_II_PROTOCOL         (-120)
#define GE_II_UNKNOWN          (-121)
#define GE_II_PTS              (-122)
#define GE_II_IFSD_LEN         (-123)
#define GE_II_PROC_BYTE        (-124)
#define GE_II_INS              (-125)
#define GE_II_RES_LEN          (-126)
#define GE_II_RESYNCH          (-127)

/*------------------------------------------------------------------------------
   Error codes from IFD.
      GE_IFD_ABSENT     (-200)
         No IFD connected.
      GE_IFD_MUTE       (-201)
         No response from IFD/GPS.
      GE_IFD_UNKNOWN    (-202)
         IFD is not supported.
      GE_IFD_BUSY       (-203)
         IFD is busy by an other process.
      GE_IFD_FN_PROG        (-210)                                          (16)
         Insufficient power for programming.
      GE_IFD_FN_UNKNOWN     (-211)                                          (1C)
         The function is not available in the IFD.
      GE_IFD_FN_FORMAT      (-212)                                    (1B or 04)
         Incoherence in the argument number or type.
      GE_IFD_FN_DEF         (-213)                              (19, 1E, 1F, 20)
         A macro definition generates an internal problem.
      GE_IFD_FN_FAIL        (-214)
         The called IFD function has failed.
      
      GE_IFD_MEM_PB         (-215)
         This memory option is not available.
      GE_IFD_MEM_ACCESS     (-216)
         This  memory access is forbidden.
      GE_IFD_MEM_ACTIVATION (-217)
         Impossible to activate the selected code.

      GE_IFD_ABORT          (-220)                                          (A5)
         IFD sends an abort block.Buffer istoo small to receive data from card
         (T=1 only).
      GE_IFD_RESYNCH        (-221)                                          (A6)
         IFD has done a resynchronisation. Data are lost (T=1 only).
      GE_IFD_TIMEOUT        (-290)                                          (04)
         Returned by the IFD keyboard when no key has been pressed during the
         given time.
      GE_IFD_OVERSTRIKED    (-291)                                          (CF)
         Returned by the IFD keyboard when two keys are pressed at the same
         time.
------------------------------------------------------------------------------*/
#define GE_IFD_ABSENT          (-200)
#define GE_IFD_MUTE            (-201)
#define GE_IFD_UNKNOWN         (-202)
#define GE_IFD_BUSY            (-203)
#define GE_IFD_FN_PROG         (-210)
#define GE_IFD_FN_UNKNOWN      (-211)
#define GE_IFD_FN_FORMAT       (-212)
#define GE_IFD_FN_DEF          (-213)
#define GE_IFD_FN_FAIL         (-214)
#define GE_IFD_MEM_PB          (-215)
#define GE_IFD_MEM_ACCESS      (-216)
#define GE_IFD_MEM_ACTIVATION  (-217)
#define GE_IFD_ABORT           (-220)
#define GE_IFD_RESYNCH         (-221)
#define GE_IFD_TIMEOUT         (-290)
#define GE_IFD_OVERSTRIKED     (-291)

/*------------------------------------------------------------------------------
   Error codes between Host and IFD (HI).
      GE_HI_COMM     (-300)
         Communication error between IFD and Host.
      GE_HI_PARITY   (-301)
         Character parity error between IFD and Host.
      GE_HI_LRC      (-302)
         Longitudinal redundancy code error.
      
      GE_HI_PROTOCOL (-310)
         Frame error in the Host-IFD protocol.
      GE_HI_LEN      (-311)                                                 (1A)
         Bad value for LN parameter in header.
      GE_HI_FORMAT   (-312)                                                 (09)
         Header must contain ACK or NACK in TLP protocol.
         No I/R/S-Block has been detected in Gemplus Block Protocol.
      GE_HI_CMD_LEN  (-313)                                                 (12)
         Message length from Host not supported.
      GE_HI_NACK     (-314)
         IFD sends a NACK /R-Block.
      GE_HI_RESYNCH  (-315)
         IFD sends a S-Block.
      GE_HI_ADDRESS  (-316)
         A bad Source/Target address has been detected in Gemplus Block Protocol
      GE_HI_SEQUENCE (-317)
         A bad sequence number has been detected in Gemplus Block Protocol.
------------------------------------------------------------------------------*/
#define GE_HI_COMM             (-300)
#define GE_HI_PARITY           (-301)
#define GE_HI_LRC              (-302)
#define GE_HI_PROTOCOL         (-310)
#define GE_HI_LEN              (-311)
#define GE_HI_FORMAT           (-312)
#define GE_HI_CMD_LEN          (-313)
#define GE_HI_NACK             (-314)
#define GE_HI_RESYNCH          (-315)
#define GE_HI_ADDRESS          (-316)
#define GE_HI_SEQUENCE         (-317)

/*------------------------------------------------------------------------------
   Error codes from host.
      GE_HOST_PORT       (-400)
         Port not usable.
      GE_HOST_PORT_ABS   (-401)
         Port absent.
      GE_HOST_PORT_INIT  (-402)
         Port not initialized.
      GE_HOST_PORT_BUSY  (-403)
         Port is always busy.
      GE_HOST_PORT_BREAK (-404)
         Port does not sent bytes.
      GE_HOST_PORT_LOCKED (-405)
         Port access is locked.

      GE_HOST_PORT_OS    (-410)
         Unexpected error for operating system.
      GE_HOST_PORT_OPEN  (-411)
         The port is already opened.
      GE_HOST_PORT_CLOSE (-412)
         The port is already closed.

      GE_HOST_MEMORY     (-420)
         Memory allocation fails.
      GE_HOST_POINTER    (-421)
         Bad pointer.
      GE_HOST_BUFFER_SIZE(-422)

      GE_HOST_RESOURCES  (-430)
         Host runs out of resources.

      GE_HOST_USERCANCEL (-440)
         User cancels operation.
         
      GE_HOST_PARAMETERS (-450)
         A parameter is out of the allowed range.
      GE_HOST_DLL_ABS    (-451)
         A dynamic call to à library fails because the target library was not 
         found.
      GE_HOST_DLL_FN_ABS (-452)
         A dynamic call to a function fails because the target library does not
         implement this function.
------------------------------------------------------------------------------*/
#define GE_HOST_PORT           (-400)
#define GE_HOST_PORT_ABS       (-401)
#define GE_HOST_PORT_INIT      (-402)
#define GE_HOST_PORT_BUSY      (-403)
#define GE_HOST_PORT_BREAK     (-404)
#define GE_HOST_PORT_LOCKED    (-405)
#define GE_HOST_PORT_OS        (-410)
#define GE_HOST_PORT_OPEN      (-411)
#define GE_HOST_PORT_CLOSE     (-412)
#define GE_HOST_MEMORY         (-420)
#define GE_HOST_POINTER        (-421)
#define GE_HOST_BUFFER_SIZE    (-422)
#define GE_HOST_RESOURCES      (-430)
#define GE_HOST_USERCANCEL     (-440)
#define GE_HOST_PARAMETERS     (-450)
#define GE_HOST_DLL_ABS        (-451)
#define GE_HOST_DLL_FN_ABS     (-452)

/*------------------------------------------------------------------------------
   Error codes from APDU layer.
      GE_APDU_CHAN_OPEN   (-501)
         Channel is already opened.
      GE_APDU_CHAN_CLOSE  (-502)
         Channel is already closed.
      GE_APDU_SESS_CLOSE  (-503)
         OpenSession must be called before.
      GE_APDU_SESS_SWITCH (-504)
         The switch session is not possible (ICC type has change or function is
         not available on the selected reader).

      GE_APDU_LEN_MAX     (-511)
         ApduLenMax greater than the maximum value 65544.
      GE_APDU_LE          (-512)
         Le must be < 65536 in an APDU command.
      GE_APDU_RECEIV      (-513)
         The response must contained SW1 & SW2.
         
      GE_APDU_IFDMOD_ABS   (-520)
         The selected IFD module is absent from the system.
      GE_APDU_IFDMOD_FN_ABS(-521)
         The selected function is absent from the IFD module.
------------------------------------------------------------------------------*/
#define GE_APDU_CHAN_OPEN      (-501)
#define GE_APDU_CHAN_CLOSE     (-502)
#define GE_APDU_SESS_CLOSE     (-503)
#define GE_APDU_SESS_SWITCH    (-504)
#define GE_APDU_LEN_MAX        (-511)
#define GE_APDU_LE             (-512)
#define GE_APDU_RECEIV         (-513)
#define GE_APDU_IFDMOD_ABS     (-520)
#define GE_APDU_IFDMOD_FN_ABS  (-521)

/*------------------------------------------------------------------------------
   Error codes from TLV exchanges.
      GE_TLV_WRONG     (-601)
         A TLV type is unknown.
      GE_TLV_SIZE      (-602)
         The value size is not sufficient to  hold the data according to TLV
         type.
      GE_TLV_NO_ACTION (-603)
         The TLV function call has not realized the requested operation.
------------------------------------------------------------------------------*/
#define GE_TLV_WRONG           (-601)
#define GE_TLV_SIZE            (-602)
#define GE_TLV_NO_ACTION       (-603)

/*------------------------------------------------------------------------------
   Error codes from file operations:
      GE_FILE_OPEN      (-700)
         A file is already opened or it is impossible to open the selected
         file.
      GE_FILE_CLOSE     (-701)
         No file to close or it is impossible to close the selected file.

      GE_FILE_WRITE     (-710)
         Impossible to write in file.

      GE_FILE_READ      (-720)
         Impossible to read in file.

      GE_FILE_FORMAT    (-730)
         The file format is invalid.
      GE_FILE_HEADER    (-731)
         No header found in file.
      GE_FILE_QUOT_MARK (-732)
         Unmatched quotation mark founds in file.
      GE_FILE_END       (-733)
         File end encountered.
      GE_FILE_CRC   (-734)
         Invalide CRC value for file.

      GE_FILE_VERSION   (-740)
         File version not supported.

      GE_FILE_CONFIG    (-750)
         The read config is invalid.
------------------------------------------------------------------------------*/
#define GE_FILE_OPEN           (-700)
#define GE_FILE_CLOSE          (-701)
#define GE_FILE_WRITE          (-710)
#define GE_FILE_READ           (-720)
#define GE_FILE_FORMAT         (-730)
#define GE_FILE_HEADER         (-731)
#define GE_FILE_QUOT_MARK      (-732)
#define GE_FILE_END            (-733)
#define GE_FILE_CRC            (-734)
#define GE_FILE_VERSION        (-740)
#define GE_FILE_CONFIG         (-750)


/*------------------------------------------------------------------------------
   Error codes from multi-task system. (SYS)
      GE_SYS_WAIT_FAILED       (-800)
         A wait for an object is failed (the object cannot be free or 
                     the system is instable)
      GE_SYS_SEMAP_RELEASE     (-801)
         The API cannot release a semaphore
------------------------------------------------------------------------------*/
#define GE_SYS_WAIT_FAILED     (-800)
#define GE_SYS_SEMAP_RELEASE   (-801)


/*------------------------------------------------------------------------------
   Unknown error code.
      GE_UNKNOWN_PB (-1000)
         The origine of error is unknown !!
      When an unexpected error code has been received, the returned value is 
      calculated by GE_UNKNOWN_PB - Error Code.
------------------------------------------------------------------------------*/
#define GE_UNKNOWN_PB         (-1000)


/*------------------------------------------------------------------------------
                Errors definitions Generic GPS
------------------------------------------------------------------------------*/
#define GE_GPS_CARDABSENT      (-1001) /* CARD is ABSENT in the station       */

#define GE_GPS_MUTE            (-1201) /* GPS is MUTE                         */

#define GE_GPS_COMMAND         (-1211) /* COMMAND not available in this GPS   */
#define GE_GPS_FORMAT          (-1212) /* incorrect command FORMAT            */

#define GE_GPS_COMM            (-1300) /* COMMunication error with GPS        */

#define GE_GPS_OPEN            (-1411) /* The port is already opened.         */
#define GE_GPS_CLOSE           (-1412) /* The port is already closed.         */

#define GE_GPS_SYSTEM          (-1230) /* SYSTEM problems on GPS              */

#define GE_GPS_MECHANIC        (-1240) /* MECHANIC problems on GPS            */
#define GE_GPS_BUSY            (-1241) /* the station is BUSY                 */
#define GE_GPS_CARDOUTPUT      (-1242) /* CARDOUTPUT is full                  */
#define GE_GPS_CARDINPUT       (-1243) /* CARDINPUT is empty                  */

#define GE_GPS_GRAPHIC         (-1250) /* error with GRAPHIC head             */
#define GE_GPS_RIBBON          (-1251) /* error with RIBBON on graphic head   */
#define GE_GPS_OVERHEAT        (-1252) /* OVERHEAT  on graphic head           */
#define GE_GPS_NOTHING2PRINT   (-1253) /* Nothing to print on card !!         */

#define GE_GPS_ELECTRIC        (-1260) /* error with ELECTRIC head            */

#define GE_GPS_MAGNETIC        (-1270) /* error with MAGNETIC head            */

#define GE_GPS_PCMCIA          (-1280) /* error with PCMCIA station           */

#define GE_GPS_DOWNLOAD        (-1303) /* font or bitmap DOWNLOAD error       */

#define GE_GPS_LENGTH          (-1500) /* Data length > Data max or Data dot  */
                                       /* length.                             */
#define GE_GPS_LOGO_FORMAT     (-1501) /* Only PCX file are supported today   */
                                       /* on GPS1X0.                          */
#define GE_GPS_BMP_FORMAT      (-1501) /* Only BMP 24 bits per pixel are      */
                                       /* supported today on GPS2X0.          */
#define GE_GPS_BAD_OBJECT      (-1502) /* The object is not supported by the  */
                                       /* GPS.                                */
#define GE_GPS_BAD_VERSION     (-1503) /* The driver is not adapted to the    */
                                       /* GPS.                                */
#define GE_GPS_RESTART         (-1550) /* The restart command must been sent. */
#define GE_GPS_RESET           (-1551) /* The GPS must be reseted.            */

#define GE_GPS_FONT            (-1560) /* The selected font is not available. */

#define GE_GPS_WIDTH           (-1570) /* The object width is too small !     */
#define GE_GPS_HEIGHT          (-1571) /* The object height is too small !    */

/*------------------------------------------------------------------------------
                                 File codes
-----------------------------------------------------------------------------*/
#define GE_GPS_FILE_OPEN       (-1600)
#define GE_GPS_FILE_END        (-1601)
#define GE_GPS_FILE_READ       (-1602)
#define GE_GPS_FILE_WRITE      (-1603)
#define GE_GPS_FILE_NOT_OPEN   (-1604)   

#define GE_GPS_FILE_FORMAT     (-1610)
#define GE_GPS_FILE_HEADER     (-1611)
#define GE_GPS_FILE_CARD_INFO  (-1612)
#define GE_GPS_FILE_DEF_OBJ    (-1613)
#define GE_GPS_FILE_OBJECT     (-1614)

#define GE_GPS_FILE_CONFIG     (-1620)
#define GE_GPS_FILE_QUOT_MARK  (-1621)
#define GE_GPS_FILE_LOGO_PARAM (-1622)

#define GE_GPS_FILE_VERSION    (-1630)

#define GE_GPS_FILE_PRINT_PARAM (-1640)
#define GE_GPS_FILE_CRC         (-1641)

/*------------------------------------------------------------------------------
                               TLV codes for GPS
------------------------------------------------------------------------------*/
#define GE_TLV_TYPE            (-1700)
#define GE_TLV_LEN             (-1701)

/*------------------------------------------------------------------------------
Warning definitions
      GW_ATR          (1)                                                   (A0)
         The card is not fully supported.
      GW_APDU_LEN_MAX (2)
         APDU cannot be transported with this value.
      GW_APDU_LE      (3)                                                   (E5)
         Le != Response length. For example, data are requested but are not
         available.
      GW_ALREADY_DONE (4)                                                   
         The action is already perform.
      GW_ICC_STATUS   (5)                                                   (E7)                                
         The ICC status is different from 0x9000.
      GW_HI_NO_EOM  (300)
         The decoded message has no EOM character.
------------------------------------------------------------------------------*/
#define GW_ATR                    1
#define GW_APDU_LEN_MAX           2
#define GW_APDU_LE                3
#define GW_ALREADY_DONE           4
#define GW_ICC_STATUS             5
#define GW_HI_NO_EOM            300

#define GW_GPS_FONT_DISAGREE    1001
#define GW_GPS_BARCODE_DISAGREE 1002

/*------------------------------------------------------------------------------
GTV100 Warning

      GW_GTV_ERASE_NEEDED  (1101)
         The down loader has detected that the programming area is already 
         programmed. A preliminary erase is needed.
      GW_GTV_DOWNLOADING  (1102)
		 the firmware's downloading processus is still active
------------------------------------------------------------------------------*/
#define GW_GTV_ERASE_NEEDED		1101
#define GW_GTV_DOWNLOADING		1102

/*------------------------------------------------------------------------------
                               Prototypes section
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

INT16 G_DECL G_SetReaderName    (const char   G_FAR *lpszReaderName);
INT16 G_DECL G_SetGpsName       (const char   G_FAR *lpszGpsName);
INT16 G_DECL G_GetError         (const INT16         nErrCode,
                                       char   G_FAR *lpszErrMsg
                                );

#ifdef __cplusplus
}
#endif


#endif

