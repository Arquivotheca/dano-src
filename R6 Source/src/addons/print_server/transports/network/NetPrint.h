#include <DataIO.h>
#include <String.h>
#include <print/TransportAddOn.h>

class NetPrint : public BTransportAddOn
{
 public:
				NetPrint(BNode *printer);
	virtual		~NetPrint();

	virtual ssize_t Read(void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);
	virtual uint32 GetAttributes() const { return fAttributes; }
	virtual const char *Name() const;

	virtual status_t ProbePrinters(BMessage *printers);
	virtual	status_t InitCheck() const { return fDeviceStatus; }

 private:
	long ffd;
	status_t fDeviceStatus;
	uint32 fAttributes;
	char *fDeviceID;
	BString backendSpool;
	bool hoodEnabled;
};
