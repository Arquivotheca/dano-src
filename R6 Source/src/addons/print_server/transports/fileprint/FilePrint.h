#include <File.h>
#include <Looper.h>
#include <Handler.h>
#include <FilePanel.h>
#include <String.h>
#include <DataIO.h>
#include <print/TransportAddOn.h>


class FilePrint : public BHandler, public BTransportAddOn
{
 public:
				FilePrint(BNode *printer);
	virtual		~FilePrint();

	virtual status_t BeginPrintjob(const BMessage& jobInfo);
	virtual ssize_t Read(void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);
	virtual uint32 GetAttributes() const { return fAttributes; }
	virtual const char *Name() const;

	virtual status_t ProbePrinters(BMessage *printers);
	virtual	status_t InitCheck() const { return fDeviceStatus; }

 private:
	virtual void MessageReceived(BMessage *msg);
			const char *OutputPath();
			bool CancelRequested();

 private:
	long ffd;
	status_t fDeviceStatus;
	uint32 fAttributes;
	BFile fFile;

	BString		output_path;
	bool		cancel_requested;
	bool 		valid_output;
	sem_id		printsem;
};
