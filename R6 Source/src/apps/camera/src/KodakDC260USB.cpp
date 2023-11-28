#include "KodakDC260USB.h"
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
#include <Node.h>
#include <NodeInfo.h>
#include <Bitmap.h>
#include <TranslatorRoster.h>
#include <BitmapStream.h>

#define SERIAL_MSG 0

KodakDC260USB::KodakDC260USB()
{
	fd = -1;
	Run();
}

KodakDC260USB::~KodakDC260USB()
{
	if(fd >= 0) {
		close(fd);
	}
}

void KodakDC260USB::MessageReceived(BMessage *msg)
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

void KodakDC260USB::Probe(BMessage *msg)
{
	int32		cam_num;
	BMessage	message(CAM_PROBE);

	// Put the incoming camera id back into the response message
	msg->FindInt32("cam_num", &cam_num); 
	message.AddInt32("cam_num", cam_num);

	if(CmdGetCameraStatus() < B_NO_ERROR) {
		// Try it again
		if(CmdGetCameraStatus() < B_NO_ERROR) 
			goto out;
	}

	//if exists
	message.AddString("port", "USB");

out:
	msg->SendReply(&message);
}

void KodakDC260USB::Query(BMessage *msg)
{
	const char	*inStr;
	int32		cam_num;
	char		str[256];
	BMessage	message(CAM_QUERY);

//	printf("Kodak: Query\n");

	// Put the incoming camera id back into the response message
	msg->FindInt32("cam_num", &cam_num);
	message.AddInt32("cam_num", cam_num);	

	// Find the port we're supposed to look at
	if (msg->FindString("port", &inStr) < B_NO_ERROR) {
		goto out;
	}
	// We can only deal with USB here
	if(strcmp(inStr, "USB") != 0) goto out;

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
out:
	msg->SendReply(&message);
	return;
}

void KodakDC260USB::Delete(BMessage *msg)
{
	int32		i, frm;
	BMessage	message(CAM_DELETE);

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


void KodakDC260USB::Save(BMessage *msg)
{
	int32		i, frm, tot;
	const char	*name;
	char		defaultName[32], dir[512], path[512];
	entry_ref	ref;
#ifndef STAND_ALONE
	ProgressWnd	*progress;
#endif

#if SERIAL_MSG
	printf("KodakDC260USB::Save\n");
#endif
	tot = 0;
	for (i = 0; msg->FindInt32("frame", i, &frm) == B_NO_ERROR; i++)
		tot++;

	if (tot < 1)
		return;

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

void KodakDC260USB::SaveThumbnail(BMessage *msg)
{
	int32		i, frm, tot;
	const char	*name;
	const char	*path;
	entry_ref	ref;
	BMessage	message(CAM_THUMB);
	DC260_ThumbnailData *thumb = (DC260_ThumbnailData *)buffer;

#if SERIAL_MSG
	printf("KodakDC260USB::SaveThumbnail\n");
#endif
	msg->FindString("path", &path);
	msg->FindInt32("frame", &frm);

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

ssize_t KodakDC260USB::SendPacket(uint8 *buf, size_t length)
{
	ssize_t err;

	if(fd < 0){
		fd = open("/dev/camera/usb/kodak/0",O_RDWR);
		if(fd < 0) return B_ERROR;
	}
	
#if SERIAL_MSG
	printf("SendPacket sending:");
	{
		int i;
		for(i=0; i<length; i++) printf(" %02x", buf[i]);
		printf("\n");
	}
#endif
	
	err = write(fd,buf,length);
	if(err < 0){
		close(fd);
		fd = -1;
	}
#if SERIAL_MSG
	printf("SendPacket: BulkTransfer returned %d\n", err);	
#endif

	return err;
}

ssize_t KodakDC260USB::RecvPacket(uint8 *buf, size_t length)
{
	ssize_t err;

	if(fd < 0) return B_ERROR;
	
	err = read(fd, buf, length);	

	if(err < 0){
		close(fd);
		fd = -1;
	}
	
#if SERIAL_MSG
	printf("RecvPacket: BulkTransfer returned %d\n", err);
	if(err >= 0) {
		int i;

		printf("RecvPacket just got:");
		for(i=0; i<err && i<length; i++) printf(" %02x", buf[i]);
		printf("\n");
	}
#endif

	return err;
}

int KodakDC260USB::RecvMessage(DC260_message **msg)
{
	ssize_t err;
	uint32 bytes_left = 0;
	uint16 length;
	uint32 bytes_received = 0;
	uint8 *curr_ptr;
	bool done = false;
		
	// Point to the object's buffer
	curr_ptr = buffer;
	*msg = (DC260_message *)buffer;

#if SERIAL_MSG	
	printf("RecvMessage\n");
#endif
			
	// Get the incoming packet, first time
	err = RecvPacket(curr_ptr, 4*1024);
	if(err <= 0) {
		printf("RecvMessage: error getting message of length %d\n", length);
		return err;
	}

	bytes_left = B_BENDIAN_TO_HOST_INT32((*msg)->length) + 4;
	length = bytes_left;
	bytes_left -= err;
	curr_ptr += err;

	if(bytes_left > KODAKDC260USB_BUF_SIZE) {
		return B_ERROR;
	}

	while(bytes_left > 0) {	
#if SERIAL_MSG
		printf("RecvMessage: %d bytes left in message\n", bytes_left);
#endif		

		// Get the incoming packet
		err = RecvPacket(curr_ptr, 4*1024);
		if(err <= 0) {
			printf("RecvMessage: error getting message of length %d\n", length);
			return err;
		}
		bytes_left -= err;
		curr_ptr += err;
	}

	return B_NO_ERROR;
}

int KodakDC260USB::SendMessage(DC260_message *msg, void *data)
{
	ssize_t err;
	bool result;

	// Send the message
	memcpy(buffer, msg, sizeof(DC260_message));
	memcpy(buffer+sizeof(DC260_message), data,
		B_BENDIAN_TO_HOST_INT32(msg->length) + 4 - sizeof(DC260_message));
#if SERIAL_MSG
	printf("SendMessage: sending message...\n");
#endif
	err = SendPacket(buffer, B_BENDIAN_TO_HOST_INT32(msg->length) + 4);
	if((err < B_OK) && (fd < 0)){
		printf("SendMessage: retry...\n");	
		// bad fd, try once more just in case...
		err = SendPacket(buffer, B_BENDIAN_TO_HOST_INT32(msg->length) + 4);
	}
	
	if(err < B_NO_ERROR) {
		printf("SendMessage: error sending message.\n");
		return err;
	}
#if SERIAL_MSG
	printf("SendMessage: Done.\n");
#endif
	return B_NO_ERROR;
}

// calls the GetFileList command.
int  KodakDC260USB::CmdGetFileList()
{
	DC260_message 			msg;
	DC260_message 			*response_msg;
	int err;
	struct {
		int32 list_order;
		DC260_FileNameStruct filename;
	} GetFileListArgs;

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

int  KodakDC260USB::CmdEraseFile(int file_num)
{
	DC260_message 			msg;
	DC260_message 			*response_msg;
	int err;
	DC260_FileNameStruct filename;
	
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
	
	snooze(500000);

	CmdGetCameraStatus();

	return B_NO_ERROR;
}

// calls the GetFileData command.
int  KodakDC260USB::CmdGetFileData(int file_num, int fd,  bool is_thumbnail, DC260_ThumbnailData *thumb,
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
				B_BENDIAN_TO_HOST_INT32(tag->file_length));
*/
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

void KodakDC260USB::DumpFilenames()
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
int KodakDC260USB::CmdGetCameraStatus()
{
	DC260_message msg;
	DC260_message *response_msg;
	int err;

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
int  KodakDC260USB::CmdGetProductInfo()
{
	DC260_message msg;
	DC260_message *response_msg;
	int err;

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
				char *temp, *temp2;
				// Copy the name over, but remove the leading KODAK, if
				// it exists
				// At least as far as I've seen, the string returned is:
				// 'KODAK DCXXX ZOOM DIGITAL CA' or
				// 'KODAK DIGITAL SCIENCE DCXXX'
#if SERIAL_MSG
				printf("camera_string = '%s'\n", name_value[i].data);
#endif
				// If we can find a DC, followed by a number,
				// just cut that word out.
				temp = strstr(name_value[i].data, "DC");
				if(temp != NULL) {
					if((temp[2] >= '0') && (temp[2] <= '9')) {
						temp2 = strstr(temp, " ");	
						if(temp2 != NULL) {
							*temp2 = '\0';
						}						
						strcpy(camera_string, temp);
						break;
					} else {
						strcpy(camera_string, name_value[i].data);
					}
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

void KodakDC260USB::RunTest()
{
	CmdGetCameraStatus();	
	
	CmdGetProductInfo();
	
	CmdGetFileList();
	
	DumpFilenames();
}

