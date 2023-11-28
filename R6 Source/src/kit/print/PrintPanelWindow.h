// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINT_PANEL_WINDOW_H_
#define _PRINT_PANEL_WINDOW_H_

#include <stdio.h>

#include <Window.h>
#include <View.h>
#include <Message.h>
#include <Rect.h>
#include <Button.h>
#include <Screen.h>
#include <TabView.h>
#include <MenuField.h>
#include <Menu.h>
#include <PopUpMenu.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Entry.h>
#include <Directory.h>
#include <MenuItem.h>
#include <StringView.h>

#include <pr_server.h>

#include <print/PrintConfigView.h>
#include <print/PrintPanel.h>

#include "NodeWatcher.h"
#include "BMPView.h"

#define BORDER 		4.0f
#define MSG_OK		'save'
#define MSG_CANCEL	'cncl'

namespace BPrivate
{

class PrinterBox;
class ControlView;
class PrintTabView;

//////////////////////////////////////////////////////////////
class PrintPanelWindow : public BWindow
//////////////////////////////////////////////////////////////
{
public:
		enum
		{
			MSG_PRINTER_SELECTED	= 'psel',
			MSG_ADD_PRINTER			= 'padd',
			MSG_PRINTER_PREFS		= 'pref'
		};

			PrintPanelWindow(BPrintPanel *panel);
		
		PrintTabView *TabView()	const;

		virtual void MessageReceived(BMessage *message);
		virtual bool QuitRequested();

		void save();
		void cancel();
		status_t Go();
		void update_size();

		void UpdatePrinterList();
		status_t Select(const char *name);

private:
	BPrintPanel *fPanel;
	PrintTabView *fTabView;
	sem_id fAsyncSem;
	status_t fReturnValue;
	PrinterBox *fPrinterBox;
	ControlView *fControlView;
};


//////////////////////////////////////////////////////////////
class PrintTabView : public BTabView
//////////////////////////////////////////////////////////////
{
public:
			PrintTabView(BRect r);
	virtual ~PrintTabView();

	virtual void AddTab(BView *target, BTab *tab = NULL);
	virtual BTab *RemoveTab(int32 tab_index);
	virtual void GetPreferredSize(float *x, float *y);
	virtual void Select(int32 tab);
private:
	BView *fContainer;
	class CView : public BView
	{ public:
		CView(BRect frame, const char *name, uint32 resizeMask, uint32 flags)
			: BView(frame, name, resizeMask, flags) { }
		BView *fTabView;
	};
};


//////////////////////////////////////////////////////////////
class PrinterBox : public BView
//////////////////////////////////////////////////////////////
{
public:
			PrinterBox(bool configPrinter);
	virtual ~PrinterBox();
	virtual void GetPreferredSize(float *w, float *h);
	virtual void AttachedToWindow();
	void UpdatePrinterList();
	status_t Select(const char *name);
	void UpdatePrinter(const char *name);

private:
	class PrinterWatcher : public NodeWatcher
	{
	public:
		friend class PrintPanelWindow;
		PrinterWatcher(PrinterBox *panel, const char *name);
		~PrinterWatcher();
		void AttributeChanged(node_ref& nref);
	private:
		node_ref fWatchNodeRef;
		PrinterBox *fPanel;
		BString fCurrentPrinter;
	};

	class NotMarkableMenuItem : public BMenuItem
	{ public:
		NotMarkableMenuItem(	const char *label,
								BMessage *message,
								char shortcut = 0,
								uint32 modifiers = 0) : BMenuItem(label, message, shortcut, modifiers) { }
		virtual void SetMarked(bool) { }
	};

	PrinterWatcher *fPrinterWatcher;
	BPopUpMenu *fPrinterMenu;
	BMenuField *fPrinterMenuField;
	BStringView *fPrinterStatus;
	BStringView *fPrinterDriver;
	BStringView *fPrinterTransport;
	BStringView *fPrinterComments;
	BMPView *fPrinterBitmap;
};

} using namespace BPrivate;

#endif
