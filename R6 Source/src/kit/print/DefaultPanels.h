// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINT_DEFAULT_PANELS_H_
#define _PRINT_DEFAULT_PANELS_H_

#include <stdio.h>

#include <View.h>
#include <Message.h>
#include <Rect.h>
#include <Button.h>
#include <Screen.h>
#include <String.h>
#include <MessageFilter.h>
#include <CheckBox.h>
#include <StatusBar.h>

#include <print/PrintConfigView.h>
#include <print/PrintJobEditSettings.h>
#include <print/PrinterConfigAddOn.h>

class BPrintPanel;
class BRadioButton;
class BTextControl;
class BBox;

namespace BPrivate
{

class BMPView;
class MPreviewPage;


class JobOptionPanel : public BPrintConfigView, private BMessageFilter
{
public:
		JobOptionPanel(BPrinterConfigAddOn& addon);
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void GetPreferredSize(float *x, float *y);
	virtual void MessageReceived(BMessage *);
	virtual status_t Save();

private:
	friend class BPrintPanel;
	status_t PopulateQualityMenu(int32 paper);
	virtual filter_result Filter(BMessage *message, BHandler **target);

	enum
	{
		UPD_PAGES,
		UPD_SETTINGS
	};

	void update_ui(int32 code);

	static const char *fStrings[];

	BRadioButton *fRbAllPage;
	BRadioButton *fRbPages;
	BRadioButton *fRbCurrentPage;
	BRadioButton *fRbSelection;
	BTextControl *fTcPages;
	BTextControl *fTcPagesTo;
	BTextControl *fTcCopies;
	BCheckBox *fCbAssembled;
	BCheckBox *fCbReverseOrder;
	BMenu *fMPaperType;
	BMenuField *fMfPaperType;
	BMenu *fMResolution;
	BMenuField *fMfResolution;
	BRadioButton *fRbColor;
	BRadioButton *fRbBlack;

	int32 get_copy_bitmap()
	{
		int32 base = 200;
		if ((fCbReverseOrder) && (fCbReverseOrder->Value() == B_CONTROL_ON))
			base += 1;
		if ((fCbAssembled) && (fCbAssembled->Value() == B_CONTROL_OFF))
			base += 2;
		return base;
	}
	
	BMPView *fCopyBitmap;

	BPrinterConfigAddOn& fDriver;
	const BPrinterConfigAddOn::printer_mode_t *fPrinterModes;
	int32 fCountPrinterModes;
	int32 fCurrentPaper;
	int32 fCurrentPrinterMode;
};


class ConfigPagePanel : public BPrintConfigView, private BMessageFilter
{
public:
		ConfigPagePanel(BPrinterConfigAddOn& addon);
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void GetPreferredSize(float *x, float *y);
	virtual void MessageReceived(BMessage *);
	virtual status_t Save();

private:
	friend class BPrintPanel;
	status_t PopulatePaperFormatsMenu(int32 tray);
	void update_paper();
	virtual filter_result Filter(BMessage *message, BHandler **target);

	enum
	{
		UPD_PAGES,
		UPD_SETTINGS
	};

	void update_ui(int32 code);

	BPrinterConfigAddOn& fDriver;

	BMenu *fMPaperFormats;
	BMenuField *fMfPaperFormats;
	BMenu *fMPaperFeeds;
	BMenuField *fMfPaperFeeds;

	BRadioButton *fRbPortrait;
	BRadioButton *fRbLandscape;
	BCheckBox *fCbHMirror;
	BCheckBox *fCbVMirror;
	BBox *fBbOrientation;
	BBox *fBbMirror;

	int fCountPaperFormats;
	const BPrintPaper *fPaperFormats;

	int32 fCurrentTray;
	int32 fCurrentPaperFormat;
	
	MPreviewPage *fPaperPreview;
};

class MPreviewPage : public BView
{
public:
			MPreviewPage(BRect frame);
	virtual	~MPreviewPage();
	virtual	void	Draw(BRect frame);
	virtual	void	AttachedToWindow(void);
			void	Update(const BPrintPaper& paper);

private:
	BPrintPaper fPaper;
	float fFontHeight;
	float fScale;
	char fOldScaleTxt[16];
};

class InkContainer : public BView
{
public:
			InkContainer(const BPoint& pos, const rgb_color inkColor);
	virtual ~InkContainer();
	virtual void Draw(BRect frame);
	virtual void AttachedToWindow();
	void SetInkLevel(const int value);
private:
	int fInkLevel;
	BBitmap *fContainerBitmap;
	BBitmap *fContainerBitmapFull;
	BBitmap *fLevelBitmap;
	rgb_color fColor;
	BView *fView;
};

class ToolsPanel : public BPrintConfigView
{
public:
			ToolsPanel(BPrinterConfigAddOn& addon);
	virtual void AttachedToWindow();
	virtual void GetPreferredSize(float *x, float *y);
	virtual void Pulse();
	virtual void MessageReceived(BMessage *);
private:
	BPrinterConfigAddOn& fDriver;
	bigtime_t fLastTime;
	InkContainer *fInkContainers[6];
	printer_status_t fStatus;
	bool fCleanNozzle;
	bool fCheckNozzle;
	BButton *fCleanButton;
	BButton *fCheckButton;
	BStatusBar *fStatusBar;
};




class OldApiPanel : public BPrintConfigView
{
public:
			OldApiPanel(BPrintPanel& panel);
	virtual void AttachedToWindow();
	virtual void GetPreferredSize(float *x, float *y);
	virtual status_t Save(BMessage& settings);
	virtual void MessageReceived(BMessage *);
private:
	BMessage fSettings;
	BPrintPanel& fPanel;
};



} using namespace BPrivate;


#endif
