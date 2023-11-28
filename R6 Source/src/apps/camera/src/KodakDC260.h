#ifndef KODAK260_H
#define KODAK260_H

#include <Looper.h>
#include <SerialPort.h>
#include <Message.h>
#include <ByteOrder.h>

#include "SharedCamera.h"

#ifndef KODAK260STRUCTS_H
#define KODAK260STRUCTS_H

/* Big-Endian */

/* Beacon response signal */
#define DC260_BEACON_RESP_MAGIC (0xA55A)
#define DC260_BEACON_RESP_LEN   7

typedef struct {
	uint16 magic; // Should be 0xA55A
	uint16 vendor_id ;
	uint16 device_id;
	uint8  checksum;
} _PACKED DC260_beacon_response;

/* Beacon ack signal */
#define DC260_BEACON_ACK_MAGIC (0x5AA5)
#define DC260_BEACON_ACK_LEN   13

typedef struct {
	uint16 magic; // Should be 0x5AA5
	uint8  if_type; // should be 0x55
	uint8  comm_flag; // should be 00 for normal use
	uint32 data_speed;
	uint16 to_dev_frame_size;
	uint16 to_host_frame_size;
	uint8  checksum;
} _PACKED DC260_beacon_ack;

/* Beacon completion */
#define DC260_BEACON_COMP_LEN  10

typedef struct {
	uint8  result;
	uint8  comm_flag;
	uint16 data_speed[2]; // to make the structure align on PPC
	uint16 to_dev_frame_size;
	uint16 to_host_frame_size;
} _PACKED DC260_beacon_comp;

/* Command poll packet
   This packet is sent either by the camera or the host before
   either a command or a response is sent. The target will then
   reply with a Command poll ack packet.
	
   bits:
	15-13: always 001
	12-10: bit 0: Beginning of Block
		   bit 1: End of Block
		   bit 2: 0=command, 1=response
	9-0:   length of the upcoming packet
*/
typedef uint16 DC260_command_poll;

#define DC260_COMMAND_POLL_BOB	0
#define DC260_COMMAND_POLL_EOB	2
#define DC260_COMMAND_POLL_RESP 4
#define DC260_COMMAND_POLL_LENGTH 2

/* Command poll response packet
   This packet is sent as a response to the command poll packet.
   It basically gives the go-ahead to send the real command or response. */
typedef struct {
	uint8	magic; // Always 0x00
	uint8	CMD; /* flags:
					bit 0:ACK. Send the packet or not
					bit 1:NAK. Back off! */					
} DC260_command_poll_resp;

#define DC260_COMMAND_POLL_RESP_ACK	1
#define DC260_COMMAND_POLL_RESP_NAK	2
#define DC260_COMMAND_POLL_RESP_LENGTH 2

/* Message structure
   This is the header to a message packet. Basically a message is
   this header, and a arbitrary amount of data. The Command poll
   negotiation decides how long this message is gonna be. */
typedef struct  {
	uint32 length;
	uint8  version;
	uint8  unused1, unused2, unused3;
	uint16 command;
	uint16 result_code;
} _PACKED DC260_message;

//----------------------------------------------------
// function calling structs and var types

typedef uint32	PName;
typedef uint32	PNameType;

// This struct is used as a response to the GetProductInfo
typedef struct {
	PName		name;
	PNameType 	name_type;
	char		data[32]; // Can be all sorts of types
} DC260_PNameTypeValueStruct;

// This struct is used as an arg to GetFileList
typedef struct {
	int32		drive_num;
	char		path_name[32];
	char		filename[16];
} DC260_FileNameStruct;

typedef struct {
	int32		drive_num;
	char		path_name[32];
	char		filename[16];
	int32		file_length;
	int32		file_status;
} DC260_ResFileList;

typedef struct {
	uint32		offset;
	uint32		length;
	uint32		file_length;
} DC260_PartialFileTag;

typedef struct {
	uint32		data_size;
	uint32		height;
	uint32		width;
	uint32		type;
	char		data[1];
} DC260_ThumbnailData;

#endif

//----------------------------------------------------

#define KODAKDC260_BUF_SIZE 64*1024

class KodakDC260 : public BLooper {
public:
	KodakDC260();
	~KodakDC260();

	void MessageReceived(BMessage *msg);
	
private:
	bool Connect();
	void Disconnect();
	
	// high-level message handling funcs
	void Probe(BMessage *msg);
	void Query(BMessage *msg);
	void Delete(BMessage *msg);
	void Save(BMessage *msg);
	void SaveThumbnail(BMessage *msg);

	// Kodak commands
	int CmdGetProductInfo();
	int CmdGetCameraStatus();
	int CmdGetFileList();
	int	CmdEraseFile(int file_num);
	int CmdGetFileData(int file_num, int fd,  bool is_thumbnail,
		DC260_ThumbnailData *thumb,
		uint32 offset, DC260_PartialFileTag *file_tag);

	// Message-passing stuff
	int SendMessage(DC260_message *msg, void *data);
	int RecvMessage(DC260_message **msg);
	int SendCommandPoll(uint8 flags, uint16 length);
	int GetCommandPoll(uint8 *flags, uint16 *length);
	int GetCommandPollResp(bool *result);
	int SendCommandPollResp(bool ack);
	int SendPacket(uint8 *buf, int length);
	int RecvPacket(uint8 *buf, int length);

	// misc
	void DumpFilenames();
	int ReconnectIfNeeded();
	void RunTest();
	
	// private vars
	bool			is_connected;
	char			port_name[B_OS_NAME_LENGTH];
	char			camera_string[33];
	char			vendor_string[33];
	uint8			buffer[KODAKDC260_BUF_SIZE];
	data_rate		port_speed;
	BSerialPort		*port;
	int				frame_size_to_dev;
	int				frame_size_to_host;
	FilenameList	filename_list;
};

#endif
