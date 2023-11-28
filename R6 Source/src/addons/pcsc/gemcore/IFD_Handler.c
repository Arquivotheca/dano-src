/*****************************************************************
/
/ File   :   IFD_Handler.cpp
/ Author :   Atul Pandit
/ Date   :
/ Purpose:
/
******************************************************************/

#include "config.h"

#define G_UNIX

#include <string.h>

#include "gemplus.h"
#include "gemgcr.h"
#include "gtser.h"
#include "gtgbp.h"
#include "gemcard.h"

#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#	include "gemansi.h"
#endif

#include "or3gll.h"
#include "IFD_Handler_private.h"

WORD16	rlen;
WORD32	slen;
WORD8	rbuff[HOR3GLL_BUFFER_SIZE];
WORD8	sbuff[HOR3GLL_BUFFER_SIZE];

const char *vendor_name	= "Gemplus";
const char *ifd_type	= "Gemcore Based Reader";
const char *ifd_serial	= "Gemcore Serial No. Not used";


BEOS_DEVICE BEOS_SerialPort[M_NB_MAX_DEVICES];


RESPONSECODE IO_Create_Channel(DWORD channelId)
{
	int dataChannelType;
	int channelNumber;
	RESPONSECODE lRetVal = IFD_SUCCESS;
	INT16 response;
	int major_ver, minor_ver;
	char *tmp = NULL;
	char os_string[HOR3GLL_OS_STRING_SIZE+1];
	WORD16 os_length = HOR3GLL_OS_STRING_SIZE;
	WORD16 rlen;
	WORD8 rbuff[HOR3GLL_BUFFER_SIZE];
	char *oros_info=NULL;
	int iPort;
	
	D(bug("IFD_Handler: IO_Create_Channel(\"%s\", %08lx)\n", devicename, flags));

	dataChannelType	= channelId >> 16;
	channelNumber	= channelId & 0x00FF;

	// We only support serial port
	// 0x02 :Parallel, 0x04 :PS2, 0x08 :SCSI, 0x10 IDE, 0x20 : USB Port
	if ((dataChannelType) != 0x01)
		return IFD_NOT_SUPPORTED;

	switch (channelNumber)
	{ // The spec' is not clear at all about the "channel number", is it the address or not?
        case 0x3F8:	channelNumber=1;	break;
        case 0x2F8:	channelNumber=2;	break;
        case 0x3E8:	channelNumber=3;	break;
        case 0x2E8:	channelNumber=4;	break;
	}

	// devid = serial + flags (beos specific) + channel_number
	iPort = 0x00010000 | (channelId & 0x0000FF00) | (channelNumber & 0x00FF);
	response = G_Oros3OpenComm(iPort, 9600);
	if (response < 0)
		return IFD_COMMUNICATION_ERROR;
	
	response = G_Oros3String(&os_length, os_string);
	if (response < G_OK)
	{
		G_Oros3CloseComm();
		return IFD_COMMUNICATION_ERROR;
	}
	
	if (strncmp(os_string+1, IFD_OS_VERSION, strlen(IFD_OS_VERSION)) != 0)
	{
		G_Oros3CloseComm();
		return (IFD_NOT_SUPPORTED);
	}
	
	os_string[os_length] = 0;
	D(bug("OS String : %s\n", os_string+1));
	if (tmp = strstr(os_string+1, IFD_OS_VERSION))
	{
		tmp += strlen(IFD_OS_VERSION);
		sscanf(tmp, "%1u%2u", &major_ver, &minor_ver);
	}
	
	memset(&Device, 0, sizeof(Device));
	memset(&ICC, 0, sizeof(ICC));
	
	Device.IFD_Version = major_ver << 24;		// Major version
	Device.IFD_Version |= (minor_ver << 16);	// Minor version
	Device.IFD_Version |= 0x0001;				// Build number (TODO:)
	
	/*
	// Should we change baud rate of reader comm-n here..
	if ( G_ChangeIFDBaudRate(iPort, 38400) != G_OK )
	{
		G_Oros3CloseComm();
		D(bug("38400 baud setting problem. exiting\n"));
		return (IFD_COMMUNICATION_ERROR);
	}
	*/
	
	// Removes TLP Compatibility here..
	rlen = HOR3GLL_BUFFER_SIZE;
	response = G_Oros3SetMode(HOR3GLL_LOW_TIME, 0x00, &rlen, rbuff);
	if (response < G_OK)
	{
		G_Oros3CloseComm();
		return (IFD_COMMUNICATION_ERROR);
	}
	
	Device.Vendor_Name	= (char *)vendor_name;
	Device.IFD_Type		= (char *)ifd_type;
	Device.IFD_Serial	= (char *)ifd_serial;
	Device.IFD_Channel_ID	= iPort & 0x00FF00FF;
	Device.Asynch_Supported	= 0x00000003; // Both T=0/T=1 Supported
	Device.Default_Clock	= 3680;
	Device.Max_Clock		= 3680;
	Device.Default_Data_Rate= 9600;
	Device.Max_Data_Rate	= 115000;
	Device.Synch_Supported	= 0;
	Device.Power_Mgmt		= 1;		// IFD Supports powerdown if ICC present;
	
	D(bug("Vendor Name		= %s\n", Device.Vendor_Name));
	D(bug("IFD Type			= %s\n", Device.IFD_Type));
	D(bug("IFD Version		= 0x%08lX\n", Device.IFD_Version));
	D(bug("IFD Serial		= %s\n", Device.IFD_Serial));
	D(bug("IFD Channel ID	= 0x%04x\n", Device.IFD_Channel_ID));

	// All Others not supported..
	return IFD_SUCCESS;
}


RESPONSECODE IO_Close_Channel()
{
	RESPONSECODE lRetVal = IFD_SUCCESS;
	INT16 response;
	
	D(bug("IFD Handler : IO_close_channel()\n"));
	response = G_Oros3CloseComm();
	if (response < 0)
	{
		D(bug("IFD Handler : IO_Close_Channel() : IFD_COMM_ERROR\n"));
		return IFD_COMMUNICATION_ERROR;
	}

	return IFD_SUCCESS;
}


RESPONSECODE IFD_Get_Capabilities (DWORD Tag, BYTE Value[])
{	
	RESPONSECODE lRetVal = IFD_SUCCESS;
	int HighNibble = (Tag >> 8) & 0xFF;
	int LowNibble = Tag & 0xFF;
	
	if (HighNibble == 0x01)
	{ // Enumerating IFD
		switch (LowNibble)
		{
			case 0x00:	memcpy(Value, Device.Vendor_Name, strlen(Device.Vendor_Name));	break;	// 32 bytes
			case 0x01:	memcpy(Value, Device.IFD_Type, strlen(Device.IFD_Type));		break;	// 32 bytes
			case 0x02:	*(DWORD*)Value = Device.IFD_Version;							break;	// 4 bytes
			case 0x03:	memcpy(Value, Device.IFD_Serial, strlen(Device.IFD_Serial));	break;	// 32 bytes
			case 0x10:	*(DWORD*)Value = Device.IFD_Channel_ID;							break;	// 4 bytes 0xDDDDCCCC
			case 0x20:	*(DWORD*)Value = Device.Asynch_Supported;						break;	// 4 bytes
			case 0x21:	*(DWORD*)Value = Device.Default_Clock;							break;	// 4 bytes
			case 0x22:	*(DWORD*)Value = Device.Max_Clock;								break;	// 4 bytes
			case 0x23:	*(DWORD*)Value = Device.Default_Data_Rate;						break;	// 4 bytes
			case 0x24:	*(DWORD*)Value = Device.Max_Data_Rate;							break;	// 4 bytes
			case 0x25:	*(DWORD*)Value = Device.Max_IFSD;								break;	// 4 bytes
			case 0x26:	*(DWORD*)Value = Device.Synch_Supported;						break;	// 4 bytes
			case 0x31:	*(DWORD*)Value = Device.Power_Mgmt;								break;	// 4 bytes
			case 0x40:	*(DWORD*)Value = Device.Card_Auth_Devices;						break;	// 4 bytes
			case 0x42:	*(DWORD*)Value = Device.User_Auth_Device;						break;	// 4 bytes
			case 0x50:	*(DWORD*)Value = Device.Mechanics_Supported;					break;	// 4 bytes
			default:	lRetVal = IFD_ERROR_TAG;
		}
	}
	else if (HighNibble == 0x02)
	{ // Enumerating IFD-ICC Interface
		switch (LowNibble)
		{
			case 0x01:	*(DWORD*)Value = Protocol.Protocol_Type;		break;	// 4 bytes
			case 0x02:	*(DWORD*)Value = Protocol.Current_Clock;		break;	// 4 bytes
			case 0x03:	*(DWORD*)Value = Protocol.Current_F;			break;	// 4 bytes
			case 0x04:	*(DWORD*)Value = Protocol.Current_D;			break;	// 4 bytes
			case 0x05:	*(DWORD*)Value = Protocol.Current_N;			break;	// 4 bytes	
			case 0x06:	*(DWORD*)Value = Protocol.Current_W;			break;	// 4 bytes
			case 0x07:	*(DWORD*)Value = Protocol.Current_IFSC;			break;	// 4 bytes
			case 0x08:	*(DWORD*)Value = Protocol.Current_IFSD;			break;	// 4 bytes
			case 0x09:	*(DWORD*)Value = Protocol.Current_BWT;			break;	// 4 bytes
			case 0x0a:	*(DWORD*)Value = Protocol.Current_CWT;			break;	// 4 bytes
			case 0x0b:	*(DWORD*)Value = Protocol.Current_EBC;			break;	// 4 bytes
			default:	lRetVal = IFD_ERROR_TAG;
		}
	}
	else if (HighNibble == 0x03)
	{ // This is the ICC_STATE

		IFD_Is_ICC_Present(); // Call this function which update params
		
		switch (LowNibble)
		{				
			case 0x00:	*(BYTE*)Value = ICC.ICC_Presence;			break;	// 1 byte
			case 0x01:	*(BYTE*)Value = ICC.ICC_Interface_Status;	break;	// 1 byte
			case 0x03:														// 32 bytes
				Value[0] = 0;
				if ((ICC.ICC_Presence == 2) && (ICC.ICC_Interface_Status))
					memcpy(Value, ICC.ATR, sizeof(ICC.ATR));
				break;				
			case 0x04:	*(BYTE*)Value = ICC.ICC_Type;				break;	// 1 byte

			// This is an extension to the specification. Please see it's
			//  definition in the header file.
			
			case 0x50:	*(DWORD*)Value = ICC.ICC_LOCK_ID; 			break;	// 4 bytes
			case 0x51:	*(DWORD*)Value = ICC.ICC_LOCK; 				break;	// 4 bytes

			default:	lRetVal = IFD_ERROR_TAG;
		}	
	}

	return lRetVal;	
}

RESPONSECODE IFD_Set_Capabilities(DWORD Tag, BYTE Value[])
{
	RESPONSECODE lRetVal = IFD_SUCCESS;
	int HighNibble = (Tag >> 8) & 0xFF;
	int LowNibble = Tag & 0xFF;

	// TODO : IFD_Set_Capabilities

	if (HighNibble == 0x03)
	{ // This is the ICC_STATE
		switch (LowNibble)
		{							
			// This is an extension to the specification. Please see it's
			// definition in the header file.
			case 0x50:	ICC.ICC_LOCK_ID = *(DWORD*)Value;	break;
			case 0x51:	ICC.ICC_LOCK = *(DWORD*)Value;		break;
			default:	lRetVal = IFD_ERROR_TAG;
		}
	}

	return lRetVal;
}

RESPONSECODE IFD_Set_Protocol_Parameters(	DWORD ProtocolType,
											BYTE SelectionFlags,
											BYTE PTS1, BYTE PTS2, BYTE PTS3 )
{	
	RESPONSECODE lRetVal = IFD_SUCCESS;

	if (ProtocolType & 0x7FFFFFFE)
		return IFD_PROTOCOL_NOT_SUPPORTED;

	ICC.ProtocolType = ProtocolType;
	ICC.SelectionFlags = SelectionFlags;
	ICC.PTS1 = PTS1;
	ICC.PTS2 = PTS2;
	ICC.PTS3 = PTS3;
	
	return lRetVal;	
}

RESPONSECODE IFD_Power_ICC(DWORD ActionRequested)
{	
	RESPONSECODE lRetVal;
	RESPONSECODE TestRsp;
	int i;
	int lc;
	unsigned int lr;
	unsigned int iTerm;
	INT16 Clock = 0;
	INT16 response;
	INT16 offset;
	INT16 n;
	INT16 l;
	WORD8 k;	
	BYTE ptsmode, pts0,	pts1, pts2,	pts3;
	
	if (ActionRequested == IFD_POWER_UP)
	{
		D(bug("\nIFD_Power_ICC: IFD_PowerUP_ICC.\n"));
		rlen = HOR3GLL_BUFFER_SIZE;
		response = G_Oros3IccPowerDown(	HOR3GLL_DEFAULT_TIME, &rlen, rbuff);
		
		if (response >= G_OK)
		{
			response = GE_Translate(rbuff[0]);
		}
		else
		{	D(bug("ICC Power up-dn returing error\n"));
			return IFD_ERROR_POWER_ACTION;
		}
		
		
		if (ICC.SelectionFlags != 0)
		{
			ptsmode = IFD_NEGOTIATE_PTS_MANUALLY;
			pts0 = ICC.SelectionFlags;
			pts1 = ICC.PTS1;
			pts2 = ICC.PTS2;
			pts3 = ICC.PTS3;
		}
		else
		{
			ptsmode = pts0 = pts1 = pts2 = pts3 = 0;
		}
		
		pts0 &= 0x07;		
		rlen = HOR3GLL_BUFFER_SIZE;
		response = G_Oros3IccPowerUp(	HOR3GLL_DEFAULT_TIME, ICC_VCC_5V,
										ptsmode, pts0, pts1, pts2, pts3,
										&rlen, rbuff);
		
		if (response >= G_OK)
		{
			response = GE_Translate(rbuff[0]);
		}
		else
		{	D(bug("ICC Power up returing error\n"));
			return IFD_ERROR_POWER_ACTION;
		}
		
		memset(ICC.ATR, 0, sizeof(ICC.ATR));
		memcpy(ICC.ATR, rbuff+1, rlen-1);
		
		l = 1;
		while (ICC.ATR[l] & 0x80)
		{
			offset = 0;
			for (k=0x10 ; k>0 ;  k<<=1)
			{
				if (ICC.ATR[l] & k)
					offset++;
			}
			l += offset;
			Protocol.Protocol_Type = (WORD16)(ICC.ATR[l] & 0x0F);
		}
	}
	else if (ActionRequested == IFD_POWER_DOWN)
	{
		D(bug("\nIFD_Power_ICC: IFD_PowerDN_ICC.\n"));
		rlen = HOR3GLL_BUFFER_SIZE;	
		response = G_Oros3IccPowerDown(HOR3GLL_DEFAULT_TIME, &rlen, rbuff);		
		if (response >= G_OK)
		{
			response = GE_Translate(rbuff[0]);
		}
		else
		{	D(bug("ICC Power down returing error\n"));
			return IFD_ERROR_POWER_ACTION;
		}
	}
	else if (ActionRequested == IFD_RESET)
	{
		D(bug("\nIFD_Power_ICC: IFD_RESET.\n"));
		if (ICC.SelectionFlags != 0)
		{
			ptsmode = IFD_NEGOTIATE_PTS_MANUALLY;
			pts0 = ICC.SelectionFlags;
			pts1 = ICC.PTS1;
			pts2 = ICC.PTS2;
			pts3 = ICC.PTS3;
		}
		else
		{
			ptsmode = pts0 = pts1 = pts2 = pts3 = 0;
		}
		
		pts0 &= 0x07;

		
		rlen = HOR3GLL_BUFFER_SIZE;		
		response = G_Oros3IccPowerUp(	HOR3GLL_DEFAULT_TIME, ICC_VCC_5V,
										ptsmode, pts0, pts1, pts2, pts3,
										&rlen, rbuff);
		if (response >= G_OK)
		{
			response = GE_Translate(rbuff[0]);
		}
		else
		{	D(bug("ICC Power up-reset returing error\n"));
			return IFD_ERROR_POWER_ACTION;
		}
		
		memset(ICC.ATR, 0, sizeof(ICC.ATR));
		memcpy(ICC.ATR, rbuff+1, rlen-1);
		
		l = 1;
		while (ICC.ATR[l] & 0x80)
		{
			offset = 0;
			for(k = 0x10; k > 0;  k <<=1)
			{
				if ( ICC.ATR[l] & k)
				{
					offset++;
				}
			}
			l += offset;
			Protocol.Protocol_Type = (WORD16)(ICC.ATR[l] & 0x0F);
		}
	}
	else
	{
		return IFD_ERROR_NOT_SUPPORTED;
	}
	
	if ((ActionRequested == IFD_POWER_UP) || (ActionRequested == IFD_RESET))
	{
		IFD_Is_ICC_Present(); // Call this fun which updates params
		Protocol.Current_Clock = 0x3680;
		GetAtrParams(ICC.ATR, &Protocol);
		rlen = HOR3GLL_BUFFER_SIZE;
		if (G_Oros3BufferSize(&rlen, rbuff) == G_OK)
			Protocol.Current_IFSC = (WORD32)rbuff[1];		
	}
	return IFD_SUCCESS;
}

RESPONSECODE IFD_Swallow_ICC()
{	
	return IFD_ERROR_NOT_SUPPORTED;        // This is not supported.	
}

RESPONSECODE IFD_Eject_ICC()
{	
	return IFD_ERROR_NOT_SUPPORTED;        // This is not supported.	
}

RESPONSECODE IFD_Confiscate_ICC()
{	
	return IFD_ERROR_NOT_SUPPORTED;        // This is not supported.		
}

RESPONSECODE IFD_Transmit_to_ICC(	const struct SCARD_IO_HEADER SendPci,
									const BYTE CommandData[], DWORD CommandSize,
									BYTE ResponseData[], DWORD *ResponseSize,
									struct SCARD_IO_HEADER *RecvPci)
{
	
	RESPONSECODE lRetVal;
	int i;
	DWORD lc, Prot;
	const BYTE *CmdData;
	BYTE data_in[280], data_out[280];
	
	G4_APDU_COMM ApduComm;
	G4_APDU_RESP ApduResp;
	
	INT16 response;
	BYTE *tmp;
	
	lc = CommandSize;
	Prot = (SendPci.Protocol & 0x3)-1; // convert protocol from libpcsc to ATR-style
	CmdData = CommandData;
	*ResponseSize = 0;
	
	// IFD_Is_ICC_Present();	// The linux driver does this, it's silly!?

	if (Prot != Protocol.Protocol_Type)
	{
		D(bug("Prot not match\n"));
		return IFD_COMMUNICATION_ERROR;
	}
	
	if (lc < 4)
	{	D(bug("Lc < 4 !\n"));
		return IFD_COMMUNICATION_ERROR;
	}
	
	tmp = (BYTE*)&ApduComm;		memset(tmp, 0, sizeof(G4_APDU_COMM));	
	tmp = (BYTE*)&ApduResp;		memset(tmp, 0, sizeof(G4_APDU_RESP));
	
	// COMMAND_LEN is defined as 4
	memcpy(ApduComm.Command, CmdData, COMMAND_LEN);
	
	ApduComm.DataIn = data_in;
	ApduResp.DataOut = data_out;
	
	if (lc == 4)
	{ // Case1
		ApduComm.LengthIn = ApduComm.LengthExpected = 0;
	}
	else if (lc == 5)
	{ //	Case2..ie Lin=0, Le > 0
		ApduComm.LengthIn = 0;
		ApduComm.LengthExpected = CmdData[COMMAND_LEN];
	}
	else
	{ // Case3 & Case4
		ApduComm.LengthIn = CmdData[COMMAND_LEN];
		ApduComm.LengthExpected = 0;
		if (ApduComm.LengthIn > 0)
		{ // Case3
			D(bug("Copying ApduComm.Datain %d bytes from cmdata\n", ApduComm.LengthIn));
			memcpy(ApduComm.DataIn, CmdData+COMMAND_LEN+1, ApduComm.LengthIn);
		}
		
		if (lc > COMMAND_LEN+1+ApduComm.LengthIn)
		{ // Case4
			D(bug("Len expected = %d\n", ApduComm.LengthExpected));
			ApduComm.LengthExpected = *(CmdData+COMMAND_LEN+1+ApduComm.LengthIn);
		}
	}
	
	
	if (Prot == 0)
	{ // T=0 protocol
		// This will prevent execution of both IsoIn/IsoOut commands at a time..
		if ((ApduComm.LengthIn > 0) && (ApduComm.LengthExpected > 0))
			return (IFD_COMMUNICATION_ERROR);
		response = ApduSpliter( HOR3GLL_DEFAULT_TIME, &ApduComm, &ApduResp, G_Oros3IccIsoInput,	G_Oros3IccIsoOutput);		
		if (response < G_OK)
		{	D(bug("ApduSpliter returning < G_OK\n"));
			return IFD_COMMUNICATION_ERROR;
		}
		
		*ResponseSize = 2 + ApduResp.LengthOut; // Added by Dave C		
		for (i=0 ; i<ApduResp.LengthOut ; ++i)
			ResponseData[i] = ApduResp.DataOut[i];
		
		ResponseData[i] = ((ApduResp.Status & 0xff00) >> 8) & 0xFF;
		ResponseData[i+1] = (ApduResp.Status & 0x00ff);
	}
	else if (Prot == 1)
	{ // T=1 protocol
		if ((ApduComm.LengthExpected > (HOR3GLL_BUFFER_SIZE - 5)) ||
			((ApduComm.LengthExpected == 0) && (ApduComm.LengthIn > (HOR3GLL_BUFFER_SIZE - 6))) ||
			((ApduComm.LengthExpected != 0) && (ApduComm.LengthIn > (HOR3GLL_BUFFER_SIZE - 7))))
		{	D(bug("Parameters not matching\n"));
			return(IFD_COMMUNICATION_ERROR);
		}
		
		slen = HOR3GLL_BUFFER_SIZE;		
		response = ApduBuilder(&ApduComm, sbuff, &slen);
		if (response < G_OK)
		{	D(bug("ApduBuilder returning %d\n", response));
			return (IFD_COMMUNICATION_ERROR);
		}

		rlen = HOR3GLL_BUFFER_SIZE;	
		response = G_Oros3IccIsoT1(HOR3GLL_DEFAULT_TIME, slen, sbuff, &rlen, rbuff);
		
		if (response > G_OK)
		{	D(bug("G_Oros3IccIsoT1 returning %d\n", response));
			response = GE_Translate(rbuff[0]);
		}

		if ((response < G_OK) || (rlen < 3))
			return IFD_COMMUNICATION_ERROR;
		
		ApduResp.LengthOut = rlen - 3;
		D(bug("G_Oros3IccIsoT1 returning rlen=%d\n", rlen));
		
		// Added by Atul..(two byte status word response..)
		*ResponseSize = 2 + ApduResp.LengthOut;
		
		if (ApduResp.LengthOut > 0)		memcpy(ResponseData, rbuff+1, ApduResp.LengthOut);
		else							ApduResp.LengthOut = 0;
		
		ResponseData[ApduResp.LengthOut] = rbuff[rlen-2];
		ResponseData[ApduResp.LengthOut+1] = rbuff[rlen-1];
	}
	
	if (RecvPci)  // If the user asked for a response protocol header
	{ // Actually, this is _really_ useless
		RecvPci->Protocol = SendPci.Protocol;
		RecvPci->Length = sizeof(*RecvPci);
	}

	return IFD_SUCCESS;
}

RESPONSECODE IFD_Is_ICC_Present()
{	
	INT16 response;
	WORD8 cmd[2] = { HOR3GLL_IFD_CMD_ICC_STATUS };

	rlen = MAX_IFD_STRING;	
	response = G_Oros3Exchange(HOR3GLL_LOW_TIME, 1, cmd, &rlen, rbuff);
	
	if (response != G_OK)
		return IFD_NOT_SUPPORTED; // should not return this
	
	ICC.ICC_Presence = 0;
	ICC.ICC_Interface_Status = 0;
	ICC.ICC_Type = 0;
	
	if (rlen == 7)
	{
		if ((rbuff[1] & 0x04) == 0)
		{ // ICC ABSENT
			ICC.ICC_Presence = 0;
			ICC.ICC_Interface_Status = 0;
			ICC.ICC_Type = 0;
			memset(ICC.ATR, 0, sizeof(ICC.ATR));
			return IFD_ICC_NOT_PRESENT;
		}
		else if ((rbuff[1] & 0x02) == 0)
		{ // ICC PRESENT. POWER-DN
			ICC.ICC_Presence = 2;
			ICC.ICC_Interface_Status = 0; // Contact Inactive..
			return IFD_SUCCESS;
		}
		else if ((rbuff[1] & 0x08) == 0)
		{ // ICC PRESENT. POWERUP
			ICC.ICC_Presence = 2;
			ICC.ICC_Interface_Status = 1; // Contact Active..
			Protocol.Protocol_Type = 0;
			if ((rbuff[2] != ISOCARD) && (rbuff[2] != FASTISOCARD))
				ICC.ICC_Type = 0; // Non ISO Card
			else
				ICC.ICC_Type = 1; // ISO Card
			return IFD_SUCCESS;
		}
		else if ((rbuff[2] == ISOCARD) || (rbuff[2] == FASTISOCARD))
		{
			ICC.ICC_Presence = 2;
			ICC.ICC_Interface_Status = 1; // Contact Active..
			Protocol.Protocol_Type = 1;
			ICC.ICC_Type = 1;
			return IFD_SUCCESS;
		}
	}
	
	return IFD_SUCCESS;
}

RESPONSECODE IFD_Is_ICC_Absent()
{
	if (IFD_Is_ICC_Present() == IFD_SUCCESS)
		return IFD_ICC_PRESENT;		// This is an error code, not an answer
	return IFD_SUCCESS;
}
