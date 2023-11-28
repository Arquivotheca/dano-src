/*
	PhotoPC.h
	PhotoPC is a BLooper-derived object that handles the actual
	communication with the camera. This consists of four messages
	described below. Some of the messages send back responses which
	are also described below.
*/

#ifndef PHOTOPC_H
#define PHOTOPC_H

#include <Looper.h>
#include <SerialPort.h>

// messages:
// 'prob' - probes all serial ports until it finds a camera
// 'qury' - connect and get the make/model and # of pictures
//			"port" is a string with the serial port name to use
//			"portSpeed" is an int32 indicating the serial port speed
//			0->4 translates to 9600, 19200, 38400, 57600, 115200
// 'dlet' - delete a list of pictures
//			"frame" is a list of int32's for the frames to delete (zero-based)
//			the frames should be listed from highest to lowest,
//			so that they can be deleted correctly
// 'save' - saves a list of pictures
//			"dir" is an entry_ref that points to a folder to save them in
//			"frame" is a list of int32's for the frames to save (zero-based)
//			"name" is a list of filenames that the images should be saved as
// 'thmb' - saves a thumbnail image
//			"path" is the path to save it to
//			"frame" is an int32 for the frame to save (zero-based)
// 'tick' - a dummy message the camera sends itself to keep alive

// responses: if the camera responds, it will use the same code as the original message
// 'prob' - results of the probe
//			"port" string - name of the serial port with a camera
// 'qury' - results of the camera query
//			"make" string
//			"model" string
//			"version" string
//			"numFrames" int32
// 'dlet' - results of the delete request
//			"frame" is a list of int32's for the frames that were successfully deleted (zero-based)
//			the frames should be listed from highest to lowest,
//			so that they can be deleted correctly
// 'thmb' - results of the thumbnail fetch
//			"path" is the path to the thumbnail (should be the same as the requested path
//			"frame" is the frame id # of the thumbnail

#include "SharedCamera.h"

class PhotoPC : public BLooper {
public:
	PhotoPC();
	~PhotoPC();

	void MessageReceived(BMessage *msg);
private:
	bool Connect();
	void Disconnect();

	void Probe(BMessage *msg);
	void Query(BMessage *msg);
	void Delete(BMessage *msg);
	void Save(BMessage *msg);
	void SaveThumbnail(BMessage *msg);

	bool WaitForCamera(uint8 value);
	bool WriteCameraPacket(uint8 type, uint8 seq, uint8 *data, size_t len);
	bool WriteCameraICommand(uint8 *data, size_t len);
	bool WriteCameraCommand(uint8 *data, size_t len);
	bool WriteCameraInt(int32 reg, int32 val);
	bool SendCameraAction(int32 cmd);
	bool ReadCameraPacket(uint8 *outData, size_t *len, bool *lastPacket);
	bool ReadCameraInt(int32 reg, int32 *val);
	bool ReadCameraString(int32 reg, char *str, size_t len);
	bool ReadCameraFile(const char *path, bool thumbnail, BLooper *looper, sem_id cancelSem);

	bool			fConnected;
	int32			fNumFrames;
	char			fPortName[B_OS_NAME_LENGTH];
	data_rate		fPortSpeed;
	BSerialPort		*fPort;
};

#endif
