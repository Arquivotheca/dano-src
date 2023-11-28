#include <DataIO.h>
#include <SerialPort.h>
#include <Window.h>
#include <Box.h>
#include <StringView.h>
#include <Button.h>
#include <MenuField.h>
#include <String.h>

class SerialPrint : public BDataIO
{
 public:
				SerialPrint(BNode& printer, BMessage* msg);
				~SerialPrint();

	ssize_t		Read(void* buffer, size_t numBytes);	
	ssize_t		Write(const void* buffer, size_t numBytes);
	
	bool		InitCheck();
	
 private:

	BSerialPort* bsp;	
	char		deviceName[B_OS_NAME_LENGTH];
	bool		isValid;
	bool		shouldWriteEOT;
};


class SerialConfigWindow : public BWindow
{
 public:
	SerialConfigWindow(BRect, const char *printer_file);
	~SerialConfigWindow();
	
	void MessageReceived(BMessage*);
	bool		QuitRequested();
	
 private:

	BEntry *printerEntry;

	bool SetSettings();
	void GetCurrentSettings();
	void PopulateDeviceList(BPopUpMenu *);

	BBox *backBox;
	BStringView *textString;

	BString curDevice;
	uint32 curDataRate;
	uint32 curStopBits;
	uint32 curDataBits;
	uint32 curParity;
	uint32 curFlowControl;
	
	BString newDevice;
	uint32 newDataRate;
	uint32 newDataBits;
	uint32 newStopBits;
	uint32 newParity;
	uint32 newFlowControl;
	
	BMenuField *deviceNames;
	BMenuField *dataRate;
	BMenuField *dataBits;
	BMenuField *stopBits;
	BMenuField *parity;
	BMenuField *flowControl;

	BButton *saveBtn;
	BButton *cancelBtn;

	bool dirty;
};
