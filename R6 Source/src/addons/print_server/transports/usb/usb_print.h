#include <DataIO.h>
#include <String.h>
#include <print/TransportAddOn.h>


#define USB_DEVICE_DIRECTORY "/dev/printer/usb"


class USBPrint : public BTransportAddOn
{
 public:
				USBPrint(BNode *printer);
	virtual		~USBPrint();

	virtual status_t SetAccessMode(int access_mode);
	virtual ssize_t Read(void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);
	virtual uint32 GetAttributes() const { return fAttributes; }
	virtual const char *Name() const;

	virtual status_t ProbePrinters(BMessage *printers);
	virtual status_t GetDeviceID(BString& device_id) const;
	virtual status_t GetPortStatus(uint8 *status) const;
	virtual status_t SoftReset() const;
	virtual	status_t InitCheck() const { return fDeviceStatus; }

 private:
	virtual status_t Perform(int32 selector, void *data);
	status_t GetDeviceID(int ffd, BString& device_id) const;
	status_t find_device_path(const char *dev_name, BPath *devpath);
	status_t probe_printers(const char *device_directory, BMessage *inout);

	long ffd;
	status_t fDeviceStatus;
	uint32 fAttributes;
	char *fDeviceID;
	BPath fDevicePath;
	int fCurrentAccessMode;
};
