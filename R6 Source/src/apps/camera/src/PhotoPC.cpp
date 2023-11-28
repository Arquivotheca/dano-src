/*
	PhotoPC.cpp
	Implementation.
*/

#include <stdio.h>
#include <string.h>
#include <NodeInfo.h>
#include <Alert.h>
#include <Path.h>
#include "PhotoPC.h"
#include "ProgressWnd.h"
#include "BmpUtils.h"
#include "CameraStrings.h"

#define SERIAL_MSG		0 // turn this on to print out data about the serial port

#define CAM_DISCONNECT	0 // turn this on to disconnect between all operations
							// communications seem more reliable if the
							// connection is left open, so this isn't recommended

#define RETRIES			3
#define WRITE_SNOOZE	2000
#define READ_SNOOZE		2000

#define NUL			0x00
#define ENQ			0x05
#define ACK			0x06
#define DC1			0x11
#define NAK			0x15

#define PKT_CMD		0x1b
#define PKT_DATA	0x02
#define PKT_LAST	0x03

#define SEQ_INITCMD	0x53
#define SEQ_CMD		0x43

#define CMD_SETINT	0
#define CMD_GETINT	1
#define CMD_ACTION	2
#define CMD_SETVAR	3
#define CMD_GETVAR	4

#define REG_FRAME		4
#define REG_NUMFRAMES	10
#define REG_IMGSIZE		12
#define REG_THUMBSIZE	13
#define REG_IMG			14
#define REG_THUMB		15
#define REG_SPEED		17
#define REG_CAMID		22
#define REG_VERSION		26
#define REG_MODEL		27
#define REG_MFG			48

#define ACT_DELALL		1
#define ACT_SHUTDOWN	4
#define ACT_DELFRAME	7

PhotoPC::PhotoPC()
{
	fConnected = false;
	fNumFrames = 0;
	strcpy(fPortName, "serial1");
	fPortSpeed = B_19200_BPS;
	fPort = NULL;
	Run();
}

PhotoPC::~PhotoPC()
{
	if (fConnected)
		Disconnect();
}

void PhotoPC::MessageReceived(BMessage *msg)
{
	int32	dummy;

	switch (msg->what)
	{
		case CAM_PROBE:
			Probe(msg);
			break;
		case CAM_QUERY:
			Query(msg);
			break;
		case CAM_DELETE:
			Delete(msg);
			break;
		case CAM_SAVE:
			Save(msg);
			break;
		case CAM_THUMB:
			SaveThumbnail(msg);
			break;
		case 'tick':
			if (fConnected)
			{
				// I don't really care - I'm just keeping the camera alive
				ReadCameraInt(REG_NUMFRAMES, &dummy);
				PostMessage('tick');
			}
			break;
		default:
			BLooper::MessageReceived(msg);
			break;
	}
}

bool PhotoPC::Connect()
{
	int32	speed, retry;
	uint8	buf[6];
	size_t	s;

	if (fConnected)
		return true;

	fPort = new BSerialPort;
	switch (fPortSpeed)
	{
		case B_9600_BPS:
			speed = 1;
#if SERIAL_MSG
			printf("Connecting to %s at 9600...\n", fPortName);
#endif
			break;
		case B_38400_BPS:
			speed = 3;
#if SERIAL_MSG
			printf("Connecting to %s at 38400...\n", fPortName);
#endif
			break;
		case B_57600_BPS:
			speed = 4;
#if SERIAL_MSG
			printf("Connecting to %s at 57600...\n", fPortName);
#endif
			break;
		case B_115200_BPS:
			speed = 5;
#if SERIAL_MSG
			printf("Connecting to %s at 115200...\n", fPortName);
#endif
			break;
		default:
			fPortSpeed = B_19200_BPS;
			speed = 2;
#if SERIAL_MSG
			printf("Connecting to %s at 19200...\n", fPortName);
#endif
			break;
	}

	if (fPort->Open(fPortName) <= 0)
	{
#if SERIAL_MSG
		printf("Couldn't open serial port\n");
#endif
		delete fPort;
		return false;
	}
	// I'm not sure I can count on this DCD stuff for all cameras
#if 0
	if (!fPort->IsDCD())
	{
#if SERIAL_MSG
		printf("No device connected\n");
#endif
		delete fPort;
		return false;
	}
#endif
	fPort->SetFlowControl(0);
	fPort->SetBlocking(true);
	fPort->SetTimeout(2000000);

	// initialize camera
	buf[0] = 0x00;
	snooze(WRITE_SNOOZE);
	fPort->Write(buf, 1);
	snooze(READ_SNOOZE);
	s = fPort->Read(buf, 1);
	if (s != 1 || buf[0] != NAK)
	{
#if SERIAL_MSG
		printf("Failed to initialize camera\n");
#endif
		delete fPort;
		return false;
	}

	fConnected = true;
	buf[0] = CMD_SETINT; // command code
	buf[1] = REG_SPEED; // register
	buf[2] = (speed) & 0xff;
	buf[3] = (speed >> 8) & 0xff;
	buf[4] = (speed >> 16) & 0xff;
	buf[5] = (speed >> 24) & 0xff;

	for (retry = RETRIES; retry; retry--)
	{
		if (WriteCameraICommand(buf, 6))
		{
			if (WaitForCamera(ACK))
			{
				fPort->SetDataRate(fPortSpeed);
#if SERIAL_MSG
				printf("Connected\n");
#endif
				return true;
			}
		}
	}

#if SERIAL_MSG
	printf("Failed to set port speed\n");
#endif
	Disconnect();
	return false;
}

void PhotoPC::Disconnect()
{
	if (fConnected)
	{
		SendCameraAction(ACT_SHUTDOWN);
		fPort->ClearOutput();
		fPort->Close();
		delete fPort;
		fPort = NULL;
		fConnected = false;
	}
}

void PhotoPC::Probe(BMessage *msg)
{
	int32		i;
	int32		cam_num;
	BSerialPort	serial;
	BMessage	message(CAM_PROBE);

	// Put the incoming camera id back into the response message
	msg->FindInt32("cam_num", &cam_num);
	message.AddInt32("cam_num", cam_num);

	if (fConnected)
	{
		Disconnect();
		snooze(4000000);
	}

	fPortSpeed = B_19200_BPS;

	for (i = 0; i < serial.CountDevices(); i++)
	{
		serial.GetDeviceName(i, fPortName);
		if (Connect())
		{
			Disconnect();
			snooze(4000000);
			message.AddString("port", fPortName);
			msg->SendReply(&message);
			return;
		}
	}

	msg->SendReply(&message);
}

void PhotoPC::Query(BMessage *msg)
{
	int32		i;
	int32		cam_num;
	const char	*inStr;
	char		str[256];
	BMessage	message(CAM_QUERY);

	// Put the incoming camera id back into the response message
	msg->FindInt32("cam_num", &cam_num);
	message.AddInt32("cam_num", cam_num);

	if (msg->FindString("port", &inStr) == B_NO_ERROR)
		strcpy(fPortName, inStr);
	
	if(strcmp(inStr, "USB") == 0) {
		msg->SendReply(&message);
		return;
	}	
		
	if (msg->FindInt32("portSpeed", &i) == B_NO_ERROR)
	{
		switch (i)
		{
			case 0: fPortSpeed = B_9600_BPS; break;
			case 1: fPortSpeed = B_19200_BPS; break;
			case 2: fPortSpeed = B_38400_BPS; break;
			case 3: fPortSpeed = B_57600_BPS; break;
			case 4: fPortSpeed = B_115200_BPS; break;
			default: fPortSpeed = B_19200_BPS; break;
		}
	}

	if (fConnected)
	{
		Disconnect();
		snooze(4000000);
	}

	if (!Connect())
	{
		msg->SendReply(&message);
		return;
	}
	if (!ReadCameraString(REG_MFG, str, 256))
		strcpy(str, STR_UNKNOWN_MAKE);
	message.AddString("make", str);
	if (!ReadCameraString(REG_MODEL, str, 256))
		strcpy(str, STR_UNKNOWN_MODEL);
	message.AddString("model", str);
	if (!ReadCameraString(REG_VERSION, str, 256))
		strcpy(str, STR_UNKNOWN_VERSION);
	message.AddString("version", str);
	if (!ReadCameraInt(REG_NUMFRAMES, &fNumFrames))
		fNumFrames = 0;
	message.AddInt32("numFrames", fNumFrames);
#if CAM_DISCONNECT
	Disconnect();
#endif
	PostMessage('tick');

	msg->SendReply(&message);
}

void PhotoPC::Delete(BMessage *msg)
{
	int32		i, frm;
	BMessage	message(CAM_DELETE);

	if (!Connect())
	{
		BAlert	*alert = new BAlert(STR_APPNAME, STR_FAILEDCONNECT, STR_OK);
		alert->Go();
		msg->SendReply(&message);
		return;
	}

	for (i = 0; msg->FindInt32("frame", i, &frm) == B_NO_ERROR; i++)
	{
		WriteCameraInt(REG_FRAME, frm + 1);
		if (SendCameraAction(ACT_DELFRAME))
			message.AddInt32("frame", frm);
	}
#if CAM_DISCONNECT
	Disconnect();
#endif
	msg->SendReply(&message);
}

void PhotoPC::Save(BMessage *msg)
{
	int32		i, frm, tot;
	const char	*name;
	char		defaultName[32], dir[512], path[512];
	entry_ref	ref;
	ProgressWnd	*progress;

	tot = 0;
	for (i = 0; msg->FindInt32("frame", i, &frm) == B_NO_ERROR; i++)
		tot++;

	if (tot < 1)
		return;

	if (!Connect())
	{
		BAlert	*alert = new BAlert(STR_APPNAME, STR_FAILEDCONNECT, STR_OK);
		alert->Go();
		return;
	}

	sem_id		cancelSem = create_sem(1, "cancel");
	progress = new ProgressWnd(cancelSem, tot);
	strcpy(dir, "/boot/home");
	if (msg->FindRef("dir", &ref) == B_NO_ERROR)
	{
		BEntry		entry(&ref, true);
		BPath		path;
		entry.GetPath(&path);
		strcpy(dir, path.Path());
	}

	for (i = 0; msg->FindInt32("frame", i, &frm) == B_NO_ERROR; i++)
	{
		if (acquire_sem_etc(cancelSem, 1, B_TIMEOUT, 0) == B_NO_ERROR)
		{
			release_sem(cancelSem);
			break;
		}
		BMessage	message(ALL_PROGRESS);
		message.AddInt32("current", i);
		message.AddInt32("total", tot);
		if (msg->FindString("name", i, &name) == B_NO_ERROR)
		{
			sprintf(path, "%s/%s", dir, name);
			message.AddString("curName", name);
		}
		else
		{
			sprintf(defaultName, STR_PICTURENAME, frm + 1);
			sprintf(path, "%s/%s", dir, defaultName);
			message.AddString("curName", defaultName);
		}
		progress->PostMessage(&message);
		WriteCameraInt(REG_FRAME, frm + 1);
		if (!ReadCameraFile(path, false, progress, cancelSem))
		{ // crap, the dowload failed - back up and try again
			i--;
#if SERIAL_MSG
			printf("Image download failed - start again\n");
#endif
		}
	}

#if CAM_DISCONNECT
	Disconnect();
#endif

	progress->PostMessage(B_QUIT_REQUESTED);
	if (acquire_sem(cancelSem) == B_NO_ERROR)
		delete_sem(cancelSem);
}

void PhotoPC::SaveThumbnail(BMessage *msg)
{
	int32		frame;
	const char	*path;
	BMessage	message(CAM_THUMB);

	if (msg->FindString("path", &path) == B_NO_ERROR &&
		msg->FindInt32("frame", &frame) == B_NO_ERROR)
	{
		if (!Connect())
			goto error;
		if (!WriteCameraInt(REG_FRAME, frame + 1))
			goto error;
		if (!ReadCameraFile(path, true, NULL, -1))
			goto error;
#if CAM_DISCONNECT
		Disconnect();
#endif
		message.AddString("path", path);
		message.AddInt32("frame", frame);
	}
error:
	msg->SendReply(&message);
}

bool PhotoPC::WaitForCamera(uint8 value)
{
	uint8	buf;
	size_t	s;

	s = fPort->Read(&buf, 1);
	if (s == 1 && buf == value)
		return true;
	else
	{
#if SERIAL_MSG
		if (buf == DC1)
			printf("Unable to execute command\n");
		else if (buf == NAK)
			printf("Negative acknowledgement\n");
		else
			printf("Acknowledgement not received\n");
#endif
		return false;
	}
}

bool PhotoPC::WriteCameraPacket(uint8 type, uint8 seq, uint8 *data, size_t len)
{
	int32	i = 0;
	uint32	j;
	uint16	crc = 0;
	uint8	buf[2054];

	if (len > (sizeof(buf) - 6))
		return false;

	snooze(WRITE_SNOOZE);

	buf[i++] = type;
	buf[i++] = seq;
	buf[i++] = (len) & 0xff;
	buf[i++] = (len >> 16) & 0xff;
	for (j = 0; j < len; j++)
	{
		crc += data[j];
		buf[i++] = data[j];
	}
	buf[i++] = (crc) & 0xff;
	buf[i++] = (crc >> 16) & 0xff;

	if (fPort->Write(buf, i) == i)
		return true;
	else
		return false;
}

bool PhotoPC::WriteCameraICommand(uint8 *data, size_t len)
{
	return WriteCameraPacket(PKT_CMD, SEQ_INITCMD, data, len);
}

bool PhotoPC::WriteCameraCommand(uint8 *data, size_t len)
{
	return WriteCameraPacket(PKT_CMD, SEQ_CMD, data, len);
}

bool PhotoPC::WriteCameraInt(int32 reg, int32 val)
{
	int32	retry;
	uint8	buf[6];

	buf[0] = CMD_SETINT;
	buf[1] = reg;
	buf[2] = (val) & 0xff;
	buf[3] = (val >> 8) & 0xff;
	buf[4] = (val >> 16) & 0xff;
	buf[5] = (val >> 24) & 0xff;

	for (retry = RETRIES; retry; retry--)
	{
		if (WriteCameraCommand(buf, 6))
		{
			if (WaitForCamera(ACK))
				return true;
		}
	}

	return false;
}

bool PhotoPC::SendCameraAction(int32 cmd)
{
	int32	retry;
	uint8	buf[3];

	buf[0] = CMD_ACTION;
	buf[1] = cmd;
	buf[2] = 0;

	for (retry = RETRIES; retry; retry--)
	{
		if (WriteCameraCommand(buf, 3))
		{
			if (WaitForCamera(ACK) && WaitForCamera(ENQ))
				return true;
		}
	}

	return false;
}

bool PhotoPC::ReadCameraPacket(uint8 *outData, size_t *len, bool *lastPacket)
{
	int32	i, rec, retries = 0;
	int16	dataLen, crc, testCrc;
	size_t	s;
	uint8	ack, header[4], data[2048], footer[2];

	snooze(READ_SNOOZE);

	//if (lastPacket != NULL)
	//	*lastPacket = true;

	goto startpoint;

retry:
	// error receiving packet:
	// it may have timed out, or the checksum may not match up
	// request a re-send
	retries++;
	if (retries > RETRIES)
		return false;
	fprintf(stderr, "packet read error - retry\n");
	snooze(READ_SNOOZE);
	ack = NAK;
	fPort->Write(&ack, 1);

startpoint:

	rec = 0;
	while (rec != 4)
	{
		s = fPort->Read(&(header[rec]), 4 - rec);
		if (s == 0)
			goto retry;
		rec += s;
	}

	dataLen = (header[3] << 8) | header[2];
	rec = 0;
	while (rec != dataLen)
	{
		s = fPort->Read(&(data[rec]), dataLen - rec);
		if (s == 0)
			goto retry;
		rec += s;
	}

	rec = 0;
	while (rec != 2)
	{
		s = fPort->Read(&(footer[rec]), 2 - rec);
		if (s == 0)
			goto retry;
		rec += s;
	}

	if (lastPacket != NULL)
	{
		if (header[0] == PKT_LAST)
			*lastPacket = true;
		else if (header[0] == PKT_DATA)
			*lastPacket = false;
		else // this shouldn't happen
			*lastPacket = true;
	}

	if ((uint16)*len < dataLen)
		goto retry;

	crc = (footer[1] << 8) | footer[0];
	testCrc = 0;
	for (i = 0; i < dataLen; i++)
	{
		testCrc += data[i];
		outData[i] = data[i];
	}

	if (testCrc != crc)
		goto retry;

	// acknowledge the packet
	snooze(READ_SNOOZE);
	ack = ACK;
	fPort->Write(&ack, 1);

	return true;
}

bool PhotoPC::ReadCameraInt(int32 reg, int32 *val)
{
	int32	retry;
	uint8	buf[4];
	size_t	len;

	*val = 0;
	buf[0] = CMD_GETINT;
	buf[1] = reg;
	for (retry = RETRIES; retry; retry--)
	{
		if (WriteCameraCommand(buf, 2))
		{
			len = 4;
			if (ReadCameraPacket(buf, &len, NULL))
			{
				*val = buf[0];
				*val |= buf[1] << 8;
				*val |= buf[2] << 16;
				*val |= buf[3] << 24;
				return true;
			}
		}
	}

	return false;
}

bool PhotoPC::ReadCameraString(int32 reg, char *str, size_t len)
{
	int32	retry;
	uint8	buf[2];

	buf[0] = CMD_GETVAR;
	buf[1] = reg;
	for (retry = RETRIES; retry; retry--)
	{
		if (WriteCameraCommand(buf, 2))
		{
			if (ReadCameraPacket((uint8 *)str, &len, NULL))
				return true;
		}
	}

	return false;
}

bool PhotoPC::ReadCameraFile(const char *path, bool thumbnail, BLooper *looper, sem_id cancelSem)
{
	int32	/*retry, */fileSize, totalSize;
	float	percent;
	uint8	buf[2048];
	size_t	len;
	bool	dataFinished;
	FILE	*f;
	bool	cancel = false;

	//for (retry = RETRIES; retry; retry--)
	//{
		buf[0] = CMD_GETVAR;
		if (thumbnail)
		{
			if (!ReadCameraInt(REG_THUMBSIZE, &totalSize))
				return false;
			buf[1] = REG_THUMB;
		}
		else
		{
			if (!ReadCameraInt(REG_IMGSIZE, &totalSize))
				return false;
			buf[1] = REG_IMG;
		}

		if (WriteCameraCommand(buf, 2))
		{
			fileSize = 0;
			f = fopen(path, "wb");
			dataFinished = false;
			while (!dataFinished)
			{
				if (cancelSem >= 0)
				{
					if (acquire_sem_etc(cancelSem, 1, B_TIMEOUT, 0) == B_NO_ERROR)
					{
						release_sem(cancelSem);
						cancel = true;
						break;
					}
				}
				len = 2048;
				if (!ReadCameraPacket(buf, &len, &dataFinished))
					break;
				if (len > 0 && f != NULL)
					fwrite(buf, len, 1, f);
				fileSize += len;
				percent = 100.0f * (float)fileSize / (float)totalSize;
				if (percent < 0.0f)
					percent = 0.0f;
				if (percent > 100.0f)
					percent = 100.0f;
				if (looper)
				{
					BMessage	message(PIC_PROGRESS);
					message.AddFloat("percent", percent);
					looper->PostMessage(&message);
				}
			} // while
			if (f != NULL)
			{
				fclose(f);
				if (cancel || !dataFinished)
				{
					BEntry	entry(path);
					entry.Remove();
					return false;
				}
				if (dataFinished)
				{
					BNode		node(path);
					BNodeInfo	ni(&node);
					ni.SetType("image/jpeg");
					SetImageIcons(path);
				}
			}
			if (dataFinished)
				return true;
		}
	//}

	return false;
}
