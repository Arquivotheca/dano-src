#include <InputServerDevice.h>

extern "C" _EXPORT BInputServerDevice* instantiate_input_device();

class AcerWPTouchDevice : public BInputServerDevice {
public:
	AcerWPTouchDevice(void);
	
	virtual status_t Start(const char* device, void* cookie);
	virtual status_t Stop(const char* device, void* cookie);
	virtual status_t Control(const char* device, void* cookie, uint32 code,
		BMessage* message);
		
private:
	static int32 AcerWPTouchPortReader(void* data);
	static int32 AcerWPToucher(void* data); // continuing the fine tradition
	
	thread_id mPortThread;
	port_id mPort;
	bool mRawMode;
	bool mRecalibrate;
	bool mTempRecalibrate;
	char mTempRecalibrationString[200];
	BMessenger mRawMessenger;
	
	int	mFileDesc;
	thread_id mThread;
	bigtime_t mClickSpeed;
};
