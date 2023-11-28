#ifndef PRINTERS_H
#define PRINTERS_H

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <ScrollView.h>
#include <ListItem.h>
#include <Window.h>
#include <Path.h>
#include <String.h>

#include <print/PrintPanel.h>

#include "BackgroundBox.h"
#include "AddPrinter.h"
#include "Watcher.h"

class TWindow;
class TPrintPanel;

#define	msg_set_default				'dflt'
#define	msg_remove_printer			'remp'
#define	msg_setup_printer			'stpp'
#define	msg_single_click			'sing'
#define	msg_double_click			'doub'
#define	msg_abort_job				'abrt'
#define	msg_ask_abort_job			'Abrt'
#define	msg_probe_printers			'prbp'
#define	msg_restart_job				'rtry'
#define	msg_config_printer			'selp'
#define	msg_config_printer_quiet	'selq'


class TPrinterItem : public BListItem
{
public:
							TPrinterItem(BLooper *looper, const char *path);
							~TPrinterItem();
						
		void				DrawItem(BView *owner, BRect frame, bool complete = false);
		void				Update(BView *owner, const BFont *font);
		void				SetIcon(const BBitmap *icon);
		void				SetIcon(bool local);
		void				SetJobCount(int32 job_count);
		void				SetFailed(bool f);
		void				SetInfos(const char *buf, const char *driver,
									 const char *transport, const char *comments,
									 bool is_checked,
									 bool *need_update, bool *need_icon);
		const char			*Name() const { return fName; }
		TWatcher			*Watcher() const { return fWatcher; }
		node_ref			Ref() const { return fRef; }

private:
		float				fv0, fdv, fdv0;
		const BBitmap		*fIcon;
		char				*fDriverName;
		char				*fTransport;
		char				*fComments;
		char 				*fName;
		TWatcher			*fWatcher;
		int32				fJobCount;
		node_ref			fRef;
		bool				fIsChecked;
		bool				fFailedJobs;
		bool				fNeedsUpdate;
};

class TPrinterBox : public BBox
{
public:
							TPrinterBox(BPath path, BRect frame);
							~TPrinterBox();
			
		void				AttachedToWindow();
			
		bool				RemovePrinter(const char* path);
		bool				RemovePrinter();
		
		status_t			NodeEvent(BMessage *m);
		void				WatcherEvent(BMessage *m);
		bool				UpdateItem(TPrinterItem *it, BEntry *entry);
		
		void				Select(const char *path);
		const TPrinterItem	*SelectedPrinter();
		const TPrinterItem	*LastSelectedPrinter();
		void				SetLastSelectedPrinter(const TPrinterItem *it);
		
		void				UpdateControls();

private:
		BPath				fPath;
		node_ref			fRef;
		
		BListView*			fPrinterList;
		BScrollView*		fListScroller;
		
		BButton*			fAddBtn;
		BButton*			fRemoveBtn;
		BButton*			fSetDefaultBtn;
		BButton*			fSetupBtn;
		BButton*			fProbeBtn;
		
		const TPrinterItem	*fLastSelectedPrinter;
		TWindow				*fWindow;
};

class TJobItem : public BListItem
{
public:
						TJobItem(const char* name, const char* app_signature, const char* path,
							const char* status, status_t errorcode, int32 page_count, float size);
						~TJobItem();
						
		void			DrawItem(BView *owner, BRect frame, bool complete = false);
		void			Update(BView *owner, const BFont *font);
		void			JobModified(BMessage *m);	
		void			GetIcon(const char *mime_type);					
		const char*		Path() const { return fPath; }
		const char*		Status() const { return fStatus; }
		status_t		ErrorCode() const { return fErrorCode; }

private:
		float			fdv0, fv0, fdv;
		char			*fName;
		char			*fPath;
		BBitmap			*fAppIcon;
		char			*fStatus;
		int32			fPageCount;
		status_t		fErrorCode;
		float			fSize;
};

class TJobBox : public BBox
{
public:
						TJobBox(BRect frame);
						~TJobBox();

		bool			AbortJob();
		bool			AskAbortJob();
		bool			RestartJob();
		void			WatcherEvent(BMessage *m);
		TJobItem		*FindItem(const char *path);
		void			SetEntry(const BEntry *entry);
		void			EmptyList();
		void			UpdateControls();
		TJobItem		*SelectedJob();

private:
		BEntry			*fEntry;

		BListView*		fJobList;
		BScrollView*	fListScroller;
		
		BButton*		fAbortBtn;
		BButton*		fRetryBtn;
};

class TWindow : public BWindow
{
public:
				TWindow(BPath);
		virtual	~TWindow();
			
		void 				MessageReceived(BMessage* m);
		void				GetPrefs();
		void				SetPrefs();
		bool 				QuitRequested();
		
		status_t			SetCurrentPrinter(const char *);
		status_t			GetCurrentPrinter(void);
		const char*			CurrentPrinter();
		void				SetupPrinter();

		void				SetJobEntry(const BEntry*);
		void				JobEvent(BMessage *m);
		void				EmptyJobList();

		status_t			configure_add_on(BMessage *msg);
		
private:
		friend class TPrintPanel;
		TJobBox				*fJobBox;
		TPrinterBox			*fPrinterBox;
		TBackgroundBox		*fBG;
	 	TAddPrinterWindow	*fAddPrinterWindow;
		BPath				fPrinterDirectory;
		BString				fCurPrinterName;
		TPrintPanel			*fPrintPanel;
};

class TPrintPanel : public BPrintPanel
{
public:
	TPrintPanel(	TWindow *printer,
					print_panel_action action,
					const BMessage *settings = NULL,
					const BMessenger *target = NULL,
					const BMessage *template_message = NULL,
					uint32 flags = B_HIDE_WHEN_DONE
				);
	virtual ~TPrintPanel();
	virtual void PrinterChanged();
	TWindow *fPrinterPanel;
};

#endif
