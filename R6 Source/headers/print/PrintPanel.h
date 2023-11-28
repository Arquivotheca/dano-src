// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINT_PANEL_H_
#define _PRINT_PANEL_H_

#include <Messenger.h>
#include <Message.h>
#include <AppDefs.h>

#define B_PRINT_PANEL_BACKGROUND	"be:background"
#define B_PRINT_PANEL_TABVIEW		"be:tabview"

class BWindow;
class BPrintConfigView;

namespace BPrivate
{
	class PrintPanelWindow;
	class PrinterWatcher;
	class OldApiPanel;
	struct private_print_panel;
}

class BPrintPanel
{
public:

	enum print_panel_action
	{
		B_CONFIG_PAGE,
		B_CONFIG_JOB,
		B_CONFIG_PRINTER	// don't use this in applications
	};

	enum
	{
		B_MODAL_PANEL = 1,
		B_HIDE_WHEN_DONE = 2,
		B_HIDE_ON_CANCEL = 4,
	};

	enum pane_type
	{
		B_DRV_PRIVATE0_PANE		= -1000,	// don't use this
		B_DRV_TOOLS_PANE		= -4,
		B_DRV_CONFIG_PAGE_PANE	= -3,
		B_DRV_CONFIG_JOB_PANE	= -2,
		B_DRV_OPTION_PANE		= -1,
		B_NO_PANE				= 0,		// don't use this
		B_APP_OPTION_PANE		= 1
	};

	enum
	{
		B_SETUP_PRINTER = _SETUP_PRINTER_,
	};

			BPrintPanel(	print_panel_action action,
							const BMessage *settings = NULL,
							const BMessenger *target = NULL,
							const BMessage *template_message = NULL,
							uint32 flags = B_HIDE_WHEN_DONE
						);

	virtual ~BPrintPanel();

	virtual	status_t SendMessage(const BMessenger& target, const BMessage& settings);

	// Adding/Removing Panes
	virtual status_t AddPane(pane_type paneType, BPrintConfigView *pane);
	virtual status_t RemovePane(BPrintConfigView *pane);
	virtual status_t AddDriverPane(pane_type paneType, BPrintConfigView *pane);
	virtual void PrinterChanged();

			// Get properties
			bool IsShowing() const;
			BWindow *Window() const;
			BString Title() const;
			print_panel_action	PanelAction() const;
			uint32 Flags() const;
			const BMessenger& Target() const;
			const BMessage& TemplateMessage() const;
			const BMessage& Settings() const;
			const char *PrinterName() const;
			const uint32 PageCount() const;		// 0 = not set/unknown
			const uint32 CurrentPage() const;	// One-based
			bool DocSelection() const;

			// Set properties
			void Show();
			void Hide();
			void SetTitle(const BString& title);
			status_t Go(BMessage& settings);
			status_t SetPanelAction(print_panel_action action);
			status_t SetFlags(uint32 flags);
			void SetTarget(const BMessenger& target);
			void SetTemplateMessage(const BMessage& template_message);
			void SetSettings(const BMessage& settings);
			void SetPageCount(uint32 page_count);	// 0 = unknown
			void SetCurrentPage(uint32 page);		// One-based
			void SetDocSelection(bool selection);
			status_t SetPrinter(const char *printer_name);

private:
	friend class BPrintJob;
	friend class BPrivate::PrintPanelWindow;
	friend class BPrivate::PrinterWatcher;
	friend class BPrivate::OldApiPanel;

	BPrintPanel(const BPrintPanel &);
	BPrintPanel& operator = (const BPrintPanel);
	virtual status_t _Reserved_BPrintPanel_0(int32 arg, ...);
	virtual status_t _Reserved_BPrintPanel_1(int32 arg, ...);
	virtual status_t _Reserved_BPrintPanel_2(int32 arg, ...);
	virtual status_t _Reserved_BPrintPanel_3(int32 arg, ...);
	virtual status_t _Reserved_BPrintPanel_4(int32 arg, ...);
	virtual status_t _Reserved_BPrintPanel_5(int32 arg, ...);
	virtual status_t _Reserved_BPrintPanel_6(int32 arg, ...);
	virtual status_t _Reserved_BPrintPanel_7(int32 arg, ...);
	virtual status_t Perform(int32 selector, void *data);

			void unload_driver_addon();
			status_t load_driver_addon();
			status_t do_async_print_panel(BMessage *);
			static	int32 _do_async_hook(BMessage *);
			int32 do_config_job(BMessage *);
			int32 do_config_page(BMessage *);
			status_t config_page(BMessage&);
			status_t config_job(BMessage&);
			status_t set_printer(const char * = NULL);
			status_t update_printer_list();
			status_t save();
			status_t cancel();
			status_t remove_panes(pane_type type);
			BMessage& merge_messages(BMessage&, const BMessage&);
			void update_doc_settings();
			status_t collect_message(BMessage&);

private:
	BPrivate::private_print_panel *_fPrivate;
	BPrivate::private_print_panel& _m_rprivate;
	uint32 reserved[4];
};


#endif
