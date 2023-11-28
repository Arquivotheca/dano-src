#include "KodakDC260.h"
#include "CameraStrings.h"
#ifndef STAND_ALONE
#include "ProgressWnd.h"
#endif
#include "Path.h"
#include "BmpUtils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <Alert.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Bitmap.h>
#include <TranslatorRoster.h>
#include <BitmapStream.h>

#define SERIAL_MSG 0

KodakDC260::KodakDC260()
{
	is_connected = false;
	strcpy(port_name, "serial1");
	port_speed = B_115200_BPS;
	port = NULL;
	frame_size_to_dev = frame_size_to_host = 0;

	Run();
}

KodakDC260::~KodakDC260()
{
	if (is_connected)
		Disconnect();
}

void KodakDC260::MessageReceived(BMessage *msg)
{
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
		case 'test':
			RunTest();
		case 'tick':
			break;
		default:
			BLooper::MessageReceived(msg);
			break;
	}
}

void KodakDC260::Probe(BMessage *msg)
{
	int32		i;
	int32		cam_num;
	BSerialPort	serial;
	BMessage	message(CAM_PROBE);

//	printf("Kodak: Probe\n");

	// Put the incoming camera id back into the response message
	msg->FindInt32("cam_num", &cam_num);
	message.AddInt32("cam_num", cam_num);

	if (is_connected)
	{
		Disconnect();
		snooze(4000000);
	}

	port_speed = B_115200_BPS;
	for (i = 0; i < serial.CountDevices(); i++)
	{
		serial.GetDeviceName(i, port_name);
#if SERIAL_MSG
		printf("trying to connect to %s\n", port_name);
#endif
		if (Connect())
		{
			message.AddString("port", port_name);

			// Send back the speed we connected at
			{
				int32 speed;
				
				switch(port_speed) {
					case B_9600_BPS: speed = 0; break;
					case B_19200_BPS: speed = 1; break;
					case B_38400_BPS: speed = 2; break;
					case B_57600_BPS: speed = 3; break;
					case B_115200_BPS: speed = 4; break;
					default: speed = 4; break;
				}			
				
				message.AddInt32("portSpeed", speed);					
			}

			msg->SendReply(&message);
			return;
		}
	}

#if SERIAL_MSG
	printf("failed to connect\n");
#endif
	msg->SendReply(&message);
}

void KodakDC260::Query(BMessage *msg)
{
	const char	*inStr;
	int32		cam_num;
	char		str[256];
	BMessage	message(CAM_QUERY);
	int32 i;

//	printf("Kodak: Query\n");

	// Put the incoming camera id back into the response message
	msg->FindInt32("cam_num", &cam_num);
	message.AddInt32("cam_num", cam_num);	

	if (msg->FindString("port", &inStr) < B_NO_ERROR) {
		goto out;
	}
	if(strcmp(inStr, "USB") == 0) goto out;
	strcpy(port_name, inStr);
	
	// This'll be the max speed we can set
	if (msg->FindInt32("portSpeed", &i) == B_NO_ERROR)
	{
		switch (i)
		{
			case 0: port_speed = B_9600_BPS; break;
			case 1: port_speed = B_19200_BPS; break;
			case 2: port_speed = B_38400_BPS; break;
			case 3: port_speed = B_57600_BPS; break;
			case 4: port_speed = B_115200_BPS; break;
			default: port_speed = B_115200_BPS; break;
		}
	}

	if(ReconnectIfNeeded() < B_NO_ERROR) {
		goto out;
	}

	if(CmdGetProductInfo() < B_NO_ERROR) {
		goto out;
	}

	if(CmdGetFileList() < B_NO_ERROR) {
		goto out;
	}

	// Add the names of the pictures to the message
	{
		DC260_ResFileList *file;
		int i;
		char temp[64];
		
		for(i=0; i<filename_list.CountItems(); i++) {
			file = (DC260_ResFileList *)filename_list.ItemAt((long int)i);

			strcpy(temp, file->filename);
			message.AddString("filename", temp); 
		}
	}	


	strcpy(str, vendor_string);
	message.AddString("make", str);
	strcpy(str, camera_string);
	message.AddString("model", str);
	strcpy(str, "unknown");
	message.AddString("version", str);
	message.AddInt32("numFrames", filename_list.CountItems());
	PostMessage('tick');
out:
	msg->SendReply(&message);
	return;
}

void KodakDC260::Delete(BMessage *msg)
{
	int32		i, frm;
	BMessage	message(CAM_DELETE);

	if(ReconnectIfNeeded() < B_NO_ERROR) {
		BAlert	*alert = new BAlert(STR_APPNAME, STR_FAILEDCONNECT, STR_OK);
		alert->Go();
		goto out;
	}

	for (i = 0; msg->FindInt32("frame", i, &frm) == B_NO_ERROR; i++)
	{
		if(CmdEraseFile(frm) < B_NO_ERROR) {
			continue;
		}

		// delete the file from the list
		{
			DC260_ResFileList *file;
			
			file = (DC260_ResFileList *)filename_list.RemoveItem((long int)frm);
			if(file != NULL) delete file;
		}
		
		message.AddInt32("frame", frm);
	}
out:
	msg->SendReply(&message);
}


void KodakDC260::Save(BMessage *msg)
{
	int32		i, frm, tot;
	const char	*name;
	char		defaultName[32], dir[512], path[512];
	entry_ref	ref;
#ifndef STAND_ALONE
	ProgressWnd	*progress;
#endif

#if SERIAL_MSG
	printf("KodakDC260::Save\n");
#endif
	tot = 0;
	for (i = 0; msg->FindInt32("frame", i, &frm) == B_NO_ERROR; i++)
		tot++;

	if (tot < 1)
		return;

	if(ReconnectIfNeeded() < B_NO_ERROR) {
		BAlert	*alert = new BAlert(STR_APPNAME, STR_FAILEDCONNECT, STR_OK);
		alert->Go();
		return;
	}

#ifndef STAND_ALONE
	sem_id	cancelSem = create_sem(1, "cancel");
	progress = new ProgressWnd(cancelSem, tot);
#endif
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
#ifndef STAND_ALONE
		bool cancel = false;	
	
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
		} else {
			sprintf(defaultName, STR_PICTURENAME, frm + 1);
			sprintf(path, "%s/%s", dir, defaultName);
			message.AddString("curName", defaultName);
		}
		progress->PostMessage(&message);
#endif

		// Open the file to write
#if SERIAL_MSG
		printf("creating file '%s'\n", path);
#endif
		int fd = open(path, O_WRONLY | O_CREAT);
		if(fd < 0) {
			printf("error opening file '%s'\n", path);
			continue;
		}								

		{
			DC260_PartialFileTag tag;
				
#ifndef STAND_ALONE
			{
				BMessage	message(PIC_PROGRESS);

				message.AddFloat("percent", 0.0);
				progress->PostMessage(&message);
			}				
#endif
				
			tag.offset = 0;
			tag.length = 0;	
			tag.file_length = 1;
			while((tag.offset + tag.length) < tag.file_length) {
				CmdGetFileData(frm, fd, false, NULL, tag.offset + tag.length, &tag);

#ifndef STAND_ALONE
				if (acquire_sem_etc(cancelSem, 1, B_TIMEOUT, 0) == B_NO_ERROR)
				{
					release_sem(cancelSem);
					cancel = true;
					break;
				}

				{
					BMessage	message(PIC_PROGRESS);
					float 		percent = 100.0 *  (float)(tag.offset + tag.length) / (float)tag.file_length;
					
					message.AddFloat("percent", percent);
					progress->PostMessage(&message);
				}
#endif
			}
		}	

		close(fd);

#ifndef STAND_ALONE
		if(cancel == false) {
#endif
			chmod(path, 0666);
			BNode		node(path);
			BNodeInfo	ni(&node);
			ni.SetType("image/jpeg");
			SetImageIcons(path);
#ifndef STAND_ALONE

		} else {
			BEntry	entry(path);
			entry.Remove();
		}
#endif
	}
#ifndef STAND_ALONE	
	progress->PostMessage(B_QUIT_REQUESTED);
	if (acquire_sem(cancelSem) == B_NO_ERROR)
		delete_sem(cancelSem);
#endif
}

void KodakDC260::SaveThumbnail(BMessage *msg)
{
	int32		i, frm, tot;
	const char	*name;
	const char	*path;
	entry_ref	ref;
	BMessage	message(CAM_THUMB);
	DC260_ThumbnailData *thumb = (DC260_ThumbnailData *)buffer;

#if SERIAL_MSG
	printf("KodakDC260::SaveThumbnail\n");
#endif
	msg->FindString("path", &path);
	msg->FindInt32("frame", &frm);

	if(ReconnectIfNeeded() < B_NO_ERROR) {
		goto out;
	}

	if(CmdGetFileData(frm, 0, true, thumb, 0, NULL) < B_NO_ERROR) {
		goto out;
	}
	
	// Now thumb should contain the thumbnail data
	{
		BRect thumbrect(0, 0, B_BENDIAN_TO_HOST_INT32(thumb->width)-1, B_BENDIAN_TO_HOST_INT32(thumb->height)-1);
		BBitmap *bitmap = new BBitmap(thumbrect, B_RGB32);
		
		// Convert the thumbnail data to RGB
		if(YCCtoRGB32((uint8 *)thumb->data, (uint8 *)bitmap->Bits(),
			B_BENDIAN_TO_HOST_INT32(thumb->data_size)) == B_NO_ERROR)
		{
			BTranslatorRoster *roster = BTranslatorRoster::Default(); 
			BBitmapStream stream(bitmap); // init with contents of bitmap, will destroy the bitmap on exit
			BFile file(path, B_CREATE_FILE | B_WRITE_ONLY); 
			roster->Translate(&stream, NULL, NULL, &file, 0x4a504547); // GJPG

			// Clean up the thumbnail file
			chmod(path, 0666);
			{
				BNode		node(path);
				BNodeInfo	ni(&node);
				ni.SetType("image/jpeg");
			}
			message.AddString("path", path);
			message.AddInt32("frame", frm);
		} else {
			// The bitmap was not bound to the BitmapStream, so we need to delete here
			delete bitmap;
		}		
	}
out:
	msg->SendReply(&message);
}


bool KodakDC260::Connect()
{
	status_t err;
	
#if SERIAL_MSG
	printf("connect called\n");
#endif

	if (is_connected)
		return true;

	port = new BSerialPort;
#if SERIAL_MSG
	printf("opening serial port '%s'\n", port_name);
#endif

	// Set up the port
	port->SetFlowControl(0); // no flow control
	port->SetDataBits(B_DATA_BITS_8);
	port->SetStopBits(B_STOP_BITS_1);
	port->SetParityMode(B_NO_PARITY);
	port->SetDataRate(B_9600_BPS);
	port->SetBlocking(true);
	port->SetTimeout(1000000); // 1 second

	err = port->Open(port_name);
	if(err < B_NO_ERROR) {
//		printf("Kodak:couldn't open serial port %s\n", port_name);
		delete port;
		return false;
	}
	
	is_connected = true;
	
#if SERIAL_MSG
	printf("connected to port %s\n", port_name);
#endif	

	// Clear the port
	port->ClearInput();
	port->ClearOutput();

	// Send a beacon to get the attention of the camera
#if SERIAL_MSG
	printf("sending beacon...\n");
#endif
	// Wait a while before doing it
	snooze(500000);

	// Send a beacon. Drop DTR and raise it back up
	port->SetDTR(false);
	snooze(100);
	port->SetDTR(true);
	snooze(500000);
	
	{
		DC260_beacon_response resp;
		
		memset(&resp, 0, sizeof(resp));
		
#if SERIAL_MSG
		printf("waiting for response...");
#endif
		err = RecvPacket((uint8 *)&resp, DC260_BEACON_RESP_LEN);
		if(err == 1) {
#if SERIAL_MSG
			printf("timeout waiting for response.\n");
#endif
			is_connected = false;
			delete port;
			return false;
		} else if(err < B_NO_ERROR) {
#if SERIAL_MSG
			printf("error getting response.\n");
#endif
			is_connected = false;
			delete port;
			return false;
		}

		// Check the beacon response magic
		if(B_BENDIAN_TO_HOST_INT16(resp.magic) != DC260_BEACON_RESP_MAGIC) {
#if SERIAL_MSG
			printf("response failed magic check, returned 0x%x\n",
				B_BENDIAN_TO_HOST_INT16(resp.magic));
#endif
			is_connected = false;
			delete port;
			return false;
		}
		
		// Check the checksum
		if(calculate_checksum((uint8 *)&resp, 0, DC260_BEACON_RESP_LEN - 2) != resp.checksum) {
#if SERIAL_MSG
			printf("response failed checksum, returned checksum %d\n", resp.checksum);
#endif
			is_connected = false;
			delete port;
			return false;
		}
#if SERIAL_MSG
		printf("got it\n");
#endif
	}

	port->ClearInput();
	port->ClearOutput();
	
	// Wait 100ms
	snooze(100000);

	// Deal with the beacon acknowledge command
	{
		DC260_beacon_ack ack;
		
		ack.magic = B_HOST_TO_BENDIAN_INT16(DC260_BEACON_ACK_MAGIC);
		ack.if_type = 0x55;
		ack.comm_flag = 0;
		switch(port_speed) {
			case B_9600_BPS:
				ack.data_speed = B_HOST_TO_BENDIAN_INT32(9600);
				break;
			case B_19200_BPS:
				ack.data_speed = B_HOST_TO_BENDIAN_INT32(19200);
				break;
			case B_38400_BPS:
				ack.data_speed = B_HOST_TO_BENDIAN_INT32(38400);
				break;
			case B_57600_BPS:
				ack.data_speed = B_HOST_TO_BENDIAN_INT32(57600);
				break;
			case B_115200_BPS:
				ack.data_speed = B_HOST_TO_BENDIAN_INT32(115200);
				break;
			default:
				ack.data_speed = B_HOST_TO_BENDIAN_INT32(115200);
				break;
		}			
		ack.to_dev_frame_size = B_HOST_TO_BENDIAN_INT16(1024);
		ack.to_host_frame_size = B_HOST_TO_BENDIAN_INT16(1024);
		ack.checksum = calculate_checksum((uint8 *)&ack, 0, DC260_BEACON_ACK_LEN - 2);
		
#if SERIAL_MSG
		printf("sending beacon acknowledge...\n");
#endif
		err = SendPacket((uint8 *)&ack, DC260_BEACON_ACK_LEN);
		if(err <= 0) {
#if SERIAL_MSG		
			printf("error writing ack\n");
#endif
			is_connected = false;
			delete port;
			return false;
		}
	}
	
	// Wait 100ms
	snooze(100000);
	
	// Deal with the beacon completion response
	{
		DC260_beacon_comp comp;
		uint32 data_speed;

		memset(&comp, 0, sizeof(comp));
		
#if SERIAL_MSG
		printf("waiting for beacon completion signal...");
#endif	
		err = RecvPacket((uint8 *)&comp, DC260_BEACON_COMP_LEN);
		if(err == 1) {
#if SERIAL_MSG
			printf("timeout waiting for response.\n");
#endif
			is_connected = false;
			delete port;
			return false;
		} else if(err < B_NO_ERROR) {
#if SERIAL_MSG
			printf("error getting response.\n");
#endif
			is_connected = false;
			delete port;
			return false;
		}

		// Swap the bytes
		memcpy(&data_speed, comp.data_speed, sizeof(uint32));
		data_speed = B_BENDIAN_TO_HOST_INT32(data_speed);
	
		frame_size_to_host = B_BENDIAN_TO_HOST_INT16(comp.to_host_frame_size);
		frame_size_to_dev = B_BENDIAN_TO_HOST_INT16(comp.to_dev_frame_size);

#if SERIAL_MSG
		printf("beacon completion packet:\n");
		printf("\tresult = %d\n\tcomm_flag = 0x%x\n\tdata_speed = %d\n",
			comp.result, comp.comm_flag, data_speed);
		printf("\tframe_size_to_dev = %d\n\tframe_size_to_host = %d\n",
			frame_size_to_dev, frame_size_to_host);
#endif
		
		snooze(100000);
		
		// Lets set the new data rate
		switch(data_speed) {
			case 9600:
				port->SetDataRate(B_9600_BPS);
				break;
			case 14400:
#if SERIAL_MSG
				printf("no 14400\n");
#endif
				is_connected = false;
				delete port;
				return false;
			case 19200:
				port->SetDataRate(B_19200_BPS);
				break;
			case 28800:
#if SERIAL_MSG
				printf("no 28800\n");
#endif
				is_connected = false;
				delete port;
				return false;
			case 38400:
				port->SetDataRate(B_38400_BPS);
				break;
			case 57600:
				port->SetDataRate(B_57600_BPS);
				break;
			case 115200:
				port->SetDataRate(B_115200_BPS);
				break;
			default:
				break;
		}
		
		snooze(100000);
	}

#if SERIAL_MSG
	printf("done connecting\n");
#endif
	return true;
}

void KodakDC260::Disconnect()
{
	if(!is_connected) return;

	port->ClearOutput();
	port->Close();
	delete port;
	port = NULL;
	is_connected = false;
	
}

int KodakDC260::SendPacket(uint8 *buf, int length)
{
	if(!is_connected) return B_ERROR;

#if SERIAL_MSG
	printf("SendPacket sending:");
	{
		int i;
		for(i=0; i<length; i++) printf(" %02x", buf[i]);
		printf("\n");
	}
#endif

	return port->Write(buf, length);
}

int KodakDC260::RecvPacket(uint8 *buf, int length)
{
	ssize_t size_transferred;
	int bytes_received = 0;

	if(!is_connected) return B_ERROR;

	while(bytes_received < length) {
		size_transferred = port->Read(buf + bytes_received, length - bytes_received);
		if(size_transferred == 0) return 1;
		if(size_transferred < 0) return size_transferred;

#if SERIAL_MSG
		{
			int i;

			printf("RecvPacket just got:");
			for(i=bytes_received; i<(size_transferred+bytes_received); i++) printf(" %02x", buf[i]);
			printf("\n");
		}
#endif
		
		bytes_received += size_transferred;
	}

	return B_NO_ERROR;
}

int KodakDC260::SendCommandPoll(uint8 flags, uint16 length)
{
	DC260_command_poll cp;
	
	if(!is_connected) return B_ERROR;

	if(length > 1023) return B_ERROR;

	cp = 0;
	cp += (1 << 13) + (flags << 10) + (length);
	
	// Byte swap the packet
	cp = B_HOST_TO_BENDIAN_INT16(cp);

	if(SendPacket((uint8 *)&cp, DC260_COMMAND_POLL_LENGTH) <= 0) return EIO;

	return B_NO_ERROR;
}

int KodakDC260::GetCommandPoll(uint8 *flags, uint16 *length)
{
	DC260_command_poll cp;
	int err;
	
	if(!is_connected) return B_ERROR;

	err = RecvPacket((uint8 *)&cp, DC260_COMMAND_POLL_LENGTH);
	if(err != B_NO_ERROR) {
		return err;
	}		

	// Byte-swap it
	cp = B_BENDIAN_TO_HOST_INT16(cp);

	// Check to see if it looks valid (the 3 MS bits should be 001)
	if((cp & 0xe000) != 0x2000) {
		return B_ERROR;
	}

	*flags = (cp & 0x1c00) >> 10; // bits 12-10
	*length = cp & 0x03ff;		  // bits 9-0

	return B_NO_ERROR;
}

int KodakDC260::GetCommandPollResp(bool *result)
{
	DC260_command_poll_resp resp;
	int err;
	
	if(!is_connected) return B_ERROR;

//	printf("GetCommandPollResp\n");
	
	err = RecvPacket((uint8 *)&resp, DC260_COMMAND_POLL_RESP_LENGTH);
	if(err != B_NO_ERROR) {
		return err;
	}		

//	printf("resp.magic = %d\n", resp.magic);
//	printf("resp.flags = %d\n", resp.CMD);

	if(resp.magic != 0) {
		return B_ERROR;
	}
	
	if(resp.CMD & DC260_COMMAND_POLL_RESP_ACK) *result = true;
	if(resp.CMD & DC260_COMMAND_POLL_RESP_NAK) *result = false;
	
//	printf("leaving GetCommandPollResp with no error\n");
	
	return B_NO_ERROR;	
}

int KodakDC260::SendCommandPollResp(bool ack)
{
	DC260_command_poll_resp resp;

	if(!is_connected) return B_ERROR;
	
//	printf("SendCommandPollResp\n");
	
	resp.magic = 0;
	if(ack) {
		resp.CMD = 1;
	} else {
		resp.CMD = 2;
	}
	
	if(SendPacket((uint8 *)&resp, DC260_COMMAND_POLL_RESP_LENGTH) <= 0) return EIO;

	return B_NO_ERROR;
}

int KodakDC260::RecvMessage(DC260_message **msg)
{
	int err;
	uint16 length;
	uint32 bytes_received = 0;
	uint8 *curr_ptr;
	bool done = false;
	
	if(!is_connected) return B_ERROR;
	
	// Point to the object's buffer
	curr_ptr = buffer;
	
	while(!done) {	

	//	printf("RecvMessage\n");
			
		// Wait for a command poll
		{
			uint8 flags;
			
			err = GetCommandPoll(&flags, &length);
			if(err == 1) {
				printf("RecvMessage: timeout waiting for command poll\n");
				return B_ERROR;
			} else if(err < B_NO_ERROR) {
				printf("RecvMessage: error getting command poll packet\n");
				return err;
			}
	
			// if this incoming response has the end-of-buffer flag set, it's
			// we'll be done afterwards
			if(flags & DC260_COMMAND_POLL_EOB) {
				done = true;
			}
		}
	
		snooze(40000);
		
		// Check to see if the upcoming receive will overwrite the buffer
		if(bytes_received + length > KODAKDC260_BUF_SIZE) {
			printf("RecvMessage: multi-part message is getting too big! "
				"sizeof(buffer) = %d\n", KODAKDC260_BUF_SIZE);
			return B_ERROR;
		}	
			
		
		// Send the command poll response
		err = SendCommandPollResp(true);
		if(err < B_NO_ERROR) {
			printf("RecvMessage: error sending command poll response\n");
			return err;
		}
		
		// Get the incoming packet
		err = RecvPacket(curr_ptr, length);
		if(err == 1) {
			printf("RecvMessage: timeout getting message of length %d\n", length);
			return B_ERROR;
		} else if(err < B_NO_ERROR) {
			printf("RecvMessage: error getting message of length %d\n", length);
			return err;
		}
		curr_ptr += length;

	}

	// Get the two-byte null at the end
	{
		uint16 null;
		RecvPacket((uint8 *)&null, sizeof(null));
	}

	*msg = (DC260_message *)buffer;

	return B_NO_ERROR;
}

int KodakDC260::SendMessage(DC260_message *msg, void *data)
{
	int err;
	bool result;

	// Clear the port
	port->ClearInput();
	port->ClearOutput();

	// Send the command poll packet. This tells the device that you're wanting
	// to send a message. The length is the length of the message + 4.
	err = SendCommandPoll(3, B_BENDIAN_TO_HOST_INT32(msg->length) + 4);
	if(err < B_NO_ERROR) {
		printf("SendMessage: some error sending command poll.\n");
		return err;
	}

	snooze(40000);
	
	// Wait for the command poll response packet. The device will tell us if we
	// can send the message
	err = GetCommandPollResp(&result);
	if(err == 1) {
		printf("SendMessage: timeout waiting for command poll response.\n");
		return ENODEV;
	} else if(err < B_NO_ERROR) {
		printf("SendMessage: some error receiving command poll response.\n");
		return err;
	}

	if(result == false) {
		printf("SendMessage: device told us we can't send the message.\n");
		return B_ERROR;
	}
	
	snooze(40000);
	
	// Send the message
	err = SendPacket((uint8 *)msg, sizeof(DC260_message));
	if(err >= B_NO_ERROR && data != NULL) {
		err = SendPacket((uint8 *)data,
			B_BENDIAN_TO_HOST_INT32(msg->length) + 4 - sizeof(DC260_message));
	}
	if(err >= B_NO_ERROR) {
		uint16 zero = 0;
		err = SendPacket((uint8 *)&zero, sizeof(zero));
	}
	if(err < B_NO_ERROR) {
		printf("SendMessage: error sending message.\n");
		return err;
	}

	return B_NO_ERROR;
}

// calls the GetFileList command.
int  KodakDC260::CmdGetFileList()
{
	DC260_message 			msg;
	DC260_message 			*response_msg;
	int err;
	struct {
		int32 list_order;
		DC260_FileNameStruct filename;
	} GetFileListArgs;
	
	if(!is_connected) return B_ERROR;

//	printf("CmdGetFileList\n");

	// Set up the message
	memset(&msg, 0, sizeof(msg));
	msg.length = B_HOST_TO_BENDIAN_INT32(sizeof(DC260_message) - 4 + sizeof(GetFileListArgs));
	msg.version = 1;
	msg.command = B_HOST_TO_BENDIAN_INT16(0x40);
	
	// Initialize the argument.
	memset(&GetFileListArgs, 0, sizeof(GetFileListArgs));
	GetFileListArgs.filename.drive_num = B_HOST_TO_BENDIAN_INT32(2); // ram card
	GetFileListArgs.list_order = B_HOST_TO_BENDIAN_INT32(1); // ascending order
		
	err = SendMessage(&msg,&GetFileListArgs);
	if(err < B_NO_ERROR) return err;
	
	err = RecvMessage(&response_msg);
	if(err < B_NO_ERROR) return err;

	// Now we have the message
	if(B_BENDIAN_TO_HOST_INT16(response_msg->result_code) > 0) return B_ERROR;
	
	// Empty the file list
	filename_list.MakeEmpty();

	// Lets process it, shall we?
	{
		uint32 num_items;
		uint8  *curr_ptr;
		uint32 i;
		DC260_ResFileList	*file_list;
		DC260_ResFileList	*file_list_item;
		
		// Point to the data
		curr_ptr = ((uint8 *)response_msg) + sizeof(DC260_message);
		
		// Get the number of items
		num_items = B_BENDIAN_TO_HOST_INT32((uint32)*((uint32 *)curr_ptr));
//		printf("num_items = %d\n", num_items);
		curr_ptr += sizeof(num_items);
		
		file_list = (DC260_ResFileList *)curr_ptr;
		
		// Loop through all of the items returned.
		for(i=0; i<num_items; i++) {
			// Create a new copy of this item and stick it in our list
			file_list_item = new DC260_ResFileList;
			if(!file_list_item) return ENOMEM;
			
			memcpy(file_list_item, &file_list[i], sizeof(DC260_ResFileList));
			filename_list.AddItem(file_list_item);

		} 
	}
	
	return B_NO_ERROR;
}

int  KodakDC260::CmdEraseFile(int file_num)
{
	DC260_message 			msg;
	DC260_message 			*response_msg;
	int err;
	DC260_FileNameStruct filename;
	
	if(!is_connected) return B_ERROR;

	// Set up the message
	memset(&msg, 0, sizeof(msg));
	msg.length = B_HOST_TO_BENDIAN_INT32(sizeof(DC260_message) - 4 + sizeof(DC260_FileNameStruct));
	msg.version = 1;
	msg.command = B_HOST_TO_BENDIAN_INT16(0x43);
	
	// Initialize the argument.
	memset(&filename, 0, sizeof(DC260_FileNameStruct));
	// copy the file name struct from the filename list
	{
		DC260_FileNameStruct *temp;
		
		temp = (DC260_FileNameStruct *)filename_list.ItemAt(file_num);
		if(!temp) return EINVAL;
		
		memcpy(&filename, temp, sizeof(DC260_FileNameStruct));
	}
		
	err = SendMessage(&msg,&filename);
	if(err < B_NO_ERROR) return err;
	
	err = RecvMessage(&response_msg);
	if(err < B_NO_ERROR) return err;

	// Now we have the message
	if(B_BENDIAN_TO_HOST_INT16(response_msg->result_code) > 0) return B_ERROR;

	return B_NO_ERROR;
}

// calls the GetFileData command.
int  KodakDC260::CmdGetFileData(int file_num, int fd,  bool is_thumbnail, DC260_ThumbnailData *thumb,
	uint32 offset, DC260_PartialFileTag *file_tag)
{
	DC260_message 			msg;
	DC260_message 			*response_msg;
	int err;
	struct {
		DC260_FileNameStruct filename;
		uint32 				 data_selector; // 1 = get thumbnail, 0 = get file
		DC260_PartialFileTag file_tag;
	} GetFileDataArgs;
	
	if(!is_connected) return B_ERROR;

//	printf("CmdGetFileData (%d, %d, %d, %ld)\n",
//		file_num, fd, is_thumbnail, offset);

	// Set up the message
	memset(&msg, 0, sizeof(msg));
	msg.length = B_HOST_TO_BENDIAN_INT32(sizeof(DC260_message) - 4 + sizeof(GetFileDataArgs));
	msg.version = 1;
	msg.command = B_HOST_TO_BENDIAN_INT16(0x42);
	
	// Initialize the argument.
	memset(&GetFileDataArgs, 0, sizeof(GetFileDataArgs));
	// copy the file name struct from the filename list
	{
		DC260_FileNameStruct *temp;
		
		temp = (DC260_FileNameStruct *)filename_list.ItemAt(file_num);
		if(!temp) return EINVAL;
		
		memcpy(&GetFileDataArgs, temp, sizeof(DC260_FileNameStruct));
	}
	// depending on if we're getting a thumbnail or not, set up the message
	if(is_thumbnail) {
		GetFileDataArgs.data_selector = B_HOST_TO_BENDIAN_INT32(1);
	} else {
		GetFileDataArgs.file_tag.offset = B_HOST_TO_BENDIAN_INT32(offset);
	}
		
	err = SendMessage(&msg,&GetFileDataArgs);
	if(err < B_NO_ERROR) return err;
	
	err = RecvMessage(&response_msg);
	if(err < B_NO_ERROR) return err;

	// Now we have the message
	if(B_BENDIAN_TO_HOST_INT16(response_msg->result_code) > 0) return B_ERROR;
	

	// Lets process it, shall we?
	{
		uint8  *curr_ptr;
		DC260_PartialFileTag *tag;
		
		// Point to the data
		curr_ptr = ((uint8 *)response_msg) + sizeof(DC260_message);
		
		// Look at the tag
		tag = (DC260_PartialFileTag *)curr_ptr;
		curr_ptr += sizeof(DC260_PartialFileTag);

		if(file_tag != NULL) {
			// Copy it to the return structure
			file_tag->offset = B_BENDIAN_TO_HOST_INT32(tag->offset);
			file_tag->length = B_BENDIAN_TO_HOST_INT32(tag->length);
			file_tag->file_length = B_BENDIAN_TO_HOST_INT32(tag->file_length);
/*	
			printf("partial file tag:\n");
			printf("\toffset = %ld\n\tlength = %ld\n\tfilesize = %ld\n",
				B_BENDIAN_TO_HOST_INT32(tag->offset),
				B_BENDIAN_TO_HOST_INT32(tag->length),
				B_BENDIAN_TO_HOST_INT32(tag->file_length)); */
		}
		
		if(is_thumbnail && (thumb != NULL)) {
			// Copy the thumbnail data over
			memcpy(thumb, curr_ptr, sizeof(DC260_ThumbnailData) + B_BENDIAN_TO_HOST_INT32(*(uint32 *)curr_ptr) - 1);
#if SERIAL_MSG			
			printf("thumbnail info:\n");
			printf("data_size = %d\n", B_BENDIAN_TO_HOST_INT32(thumb->data_size));
			printf("\theight/width is %ld %ld\n",
				B_BENDIAN_TO_HOST_INT32(thumb->height),
				B_BENDIAN_TO_HOST_INT32(thumb->width));
			printf("\ttype is %ld\n", B_BENDIAN_TO_HOST_INT32(thumb->type));
#endif			
		} else {
			lseek(fd, B_BENDIAN_TO_HOST_INT32(tag->offset), SEEK_SET);
			write(fd, curr_ptr, B_BENDIAN_TO_HOST_INT32(tag->length));	
		}
	}
	
	return B_NO_ERROR;
}

void KodakDC260::DumpFilenames()
{
	int i;
	int num_items;
	DC260_ResFileList *file;
	
	num_items = filename_list.CountItems();
	
	printf("filename dump:\n");
	
	printf("number of filenames = %d\n", num_items);
	
	for(i=0; i<num_items; i++) {
		file = (DC260_ResFileList *)filename_list.ItemAt(i);	
		if(!file) return;
				
		// Print the drive number
		printf("%ld:", B_BENDIAN_TO_HOST_INT32(file->drive_num));

		// Print the path
		printf("%s", (char *)&file->path_name);

		// Print the filename
		printf("%s", (char *)&file->filename);

		// Print the file length
		printf("\t\tlength = %ld ", B_BENDIAN_TO_HOST_INT32(file->file_length));
		
		// Print the flags
		printf("flags = 0x%x", (unsigned int)B_BENDIAN_TO_HOST_INT32(file->file_status));
		
		printf("\n");
	}	
}

// calls the GetCameraStatus command. returns a 0 or error for now
// used to just tell if the camera is still around
int KodakDC260::CmdGetCameraStatus()
{
	DC260_message msg;
	DC260_message *response_msg;
	int err;
	
	if(!is_connected) return B_ERROR;

//	printf("CmdGetProductInfo\n");

	// Set up the message
	memset(&msg, 0, sizeof(msg));
	msg.length = B_HOST_TO_BENDIAN_INT32(sizeof(DC260_message) - 4);
	msg.version = 1;
	msg.command = B_HOST_TO_BENDIAN_INT16(0x3);
	
	err = SendMessage(&msg,NULL);
	if(err < B_NO_ERROR) return err;
	
	err = RecvMessage(&response_msg);
	if(err < B_NO_ERROR) return err;

	// Now we have the message
	if(B_BENDIAN_TO_HOST_INT16(response_msg->result_code) > 0) return B_ERROR;
	
	return B_NO_ERROR;
}

// calls the GetProductInfo command. sticks the results back in the object
int  KodakDC260::CmdGetProductInfo()
{
	DC260_message msg;
	DC260_message *response_msg;
	int err;
	
	if(!is_connected) return B_ERROR;

//	printf("CmdGetProductInfo\n");

	// Set up the message
	memset(&msg, 0, sizeof(msg));
	msg.length = B_HOST_TO_BENDIAN_INT32(sizeof(DC260_message) - 4);
	msg.version = 1;
	msg.command = B_HOST_TO_BENDIAN_INT16(0x1);
	
	err = SendMessage(&msg,NULL);
	if(err < B_NO_ERROR) return err;
	
	err = RecvMessage(&response_msg);
	if(err < B_NO_ERROR) return err;

	// Now we have the message
	if(B_BENDIAN_TO_HOST_INT16(response_msg->result_code) > 0) return B_ERROR;
	
	
	// Lets process it, shall we?
	{
		uint32 num_items;
		uint8  *curr_ptr;
		DC260_PNameTypeValueStruct	*name_value;
		uint32 i;
	
		// Point to the data
		curr_ptr = ((uint8 *)response_msg) + sizeof(DC260_message);
		
		// Get the number of items
		num_items = B_BENDIAN_TO_HOST_INT32((uint32)*((uint32 *)curr_ptr));
//		printf("num_items = %d\n", num_items);
		curr_ptr += sizeof(num_items);
		
		name_value = (DC260_PNameTypeValueStruct *)curr_ptr;
		
		// Loop through all of the items returned.
		for(i=0; i<num_items; i++) {
/*
			// Print the name
			{
				char *n = (char *)&name_value[i].name;
				
				for(j=0; j<4; j++) printf("%c", n[j]);
				printf(": ");		
			}
						
			switch(B_BENDIAN_TO_HOST_INT32(name_value[i].name_type)) {
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
					printf("0x%x", (unsigned int)name_value[i].data);
					break;
				case 6:
					for(j=0; j<4; j++) printf("%c", name_value[i].data[j]);
					break;
				case 7:
					for(j=0; j<16; j++) printf("%c", name_value[i].data[j]);
					break;				
				case 8:
					for(j=0; j<32; j++) printf("%c", name_value[i].data[j]);
					break;
				default:
					break;
			}
			printf("\n");
*/			
			// Look for the product id name/value pair
			if(B_BENDIAN_TO_HOST_INT32(name_value[i].name) == 'ptid' &&
				B_BENDIAN_TO_HOST_INT32(name_value[i].name_type) == 8) {
				char *temp;
				// Copy the name over, but remove the leading KODAK, if
				// it exists
				// At least as far as I've seen, the string returned is:
				// 'KODAK DCXXX ZOOM DIGITAL CAMERA'
#if SERIAL_MSG
				printf("camera_string = '%s'\n", name_value[i].data);
#endif
				temp = strstr(name_value[i].data, "KODAK");
				if(temp != NULL) {
					char temp_string[256];

					// Skip past 'KODAK'
					temp += strlen("KODAK") + 1;
					
					// Copy the string over
					if((strlen(temp)+1) > sizeof(temp_string)) {
						// It's really long, lets just copy and get out of here
						strcpy(camera_string, temp);
						break;
					} 
					strcpy(temp_string, temp);
					
					// If DC are the next few characters, lets just
					// copy the next word. It is most likely DCXXX, where
					// XXX is the model. Kodak may build later models that
					// do DCXXXX, though.
					// If any of this fails, it'll drop out and just copy
					// the entire string
					if(temp_string[0] == 'D' && temp_string[1] == 'C') {
						char *temp2 = strstr(temp_string, " ");
						// We found the space, stick a null there
						if(temp != NULL) {
							*temp2 = '\0';
						}						
					}
					strcpy(camera_string, temp_string);
				} else {
					strcpy(camera_string, name_value[i].data);
				}
			}

			// Look for the vendor id name/value pair
			if(B_BENDIAN_TO_HOST_INT32(name_value[i].name) == 'vdid' &&
				B_BENDIAN_TO_HOST_INT32(name_value[i].name_type) == 8) {
				// Copy the name over
				// The string I've seen is:
				// 'Eastman Kodak Corporation'
#if SERIAL_MSG
				printf("vendor_string = '%s'\n", name_value[i].data);
#endif
				// If 'Kodak' is in the string, just return 'Kodak'
				if(strstr(name_value[i].data, "Kodak") != NULL) {
					strcpy(vendor_string, "Kodak");
				} else {
					strcpy(vendor_string, name_value[i].data);
				}
			}
				
		} 
	}
	
	return B_NO_ERROR;
}

void KodakDC260::RunTest()
{
	
	strcpy(port_name, "serial2");
	ReconnectIfNeeded();
		
	if(!is_connected) return;

	CmdGetProductInfo();
	
	CmdGetFileList();
	
	DumpFilenames();
}

int KodakDC260::ReconnectIfNeeded()
{
	// See if the camera is still alive, if we think it is
	if (is_connected)
	{
			if(CmdGetCameraStatus() != B_NO_ERROR) {
			Disconnect();
			snooze(500000);
		}
	}
	
	if (!Connect())
	{
		is_connected = false;
		return B_ERROR;
	}

	is_connected = true;

	return B_NO_ERROR;
}
