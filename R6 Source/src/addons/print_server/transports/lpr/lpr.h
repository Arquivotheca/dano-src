#include <DataIO.h>
#include <String.h>
#include <File.h>

#include <interface/TextControl.h>

#include <print/TransportAddOn.h>
#include <print/PrintConfigView.h>

#include <net/NetAddress.h>
#include <net/NetBuffer.h>
#include <net/NetEndpoint.h>
#include <net/NetDebug.h>


class LPRPrint : public BTransportAddOn
{
 public:
				LPRPrint(BNode *printer);
	virtual		~LPRPrint();

	virtual ssize_t Write(const void *buffer, size_t size);
	virtual ssize_t Read(void *buffer, size_t size);
	virtual uint32 GetAttributes() const { return fAttributes; }
	virtual const char *Name() const;
	virtual	status_t InitCheck() const { return fDeviceStatus; }
	virtual BPrintConfigView *GetConfigView();
	virtual status_t BeginPrintjob(const BMessage& jobInfos);
	virtual status_t EndPrintjob();
	virtual status_t ProbePrinters(BMessage *printers);

 private:
	status_t ReceiveAPrinterJob(BNetEndpoint& endPoint);
	status_t PrintAnyWaitingJob(BNetEndpoint& endPoint);
	status_t Acknowledge(BNetEndpoint &);

	BNode *fPrinterNode;
	status_t fDeviceStatus;
	uint32 fAttributes;
	BPath fSpoolFilePath;

	BNetAddress fLpdAddr;
	BString fQueueName;
	BString fUserName;
	BString fJobName;
	BFile fSpoolFile;
};

class LPRConfigView : public BPrintConfigView
{
public:
		LPRConfigView(BNode *printer);
	virtual void AttachedToWindow();
	virtual status_t Save();
	virtual void MessageReceived(BMessage *message);
	virtual void GetMinimumSize(float *width, float *height);

private:
	BNode *Printer() { return fPrinter; };
	BNode *fPrinter;
	BTextControl *fTCHostName;
	BTextControl *fTCQueueName;
	BTextControl *fTCUserName;
};


