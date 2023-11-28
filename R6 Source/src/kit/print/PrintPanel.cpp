// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <stdio.h>

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Message.h>
#include <Rect.h>
#include <Button.h>
#include <Screen.h>
#include <String.h>
#include <TabView.h>
#include <Node.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Autolock.h>
#include <Beep.h>

#include <pr_server.h>

#include <print/PrinterConfigAddOn.h>
#include <print/PrintConfigView.h>
#include <print/PrintPanel.h>

#include "PrintWindows.h"
#include "PrintEnv.h"
#include "PrintPanelWindow.h"
#include "NodeWatcher.h"

#include "DefaultPanels.h"

#define D(_x)	_x
#define bug		printf

#define BORDER 				4.0f
#define m					_m_rprivate

namespace BPrivate
{
	class PrinterWatcher;

	enum
	{
		ASYNC_CONFIG_JOB,
		ASYNC_CONFIG_PAGE
	};

	// ------------------------------------------------------------------
	class PrinterWatcher : public NodeWatcher
	// ------------------------------------------------------------------
	{
	public:
		friend class PrintPanelWindow;
		PrinterWatcher(BPrintPanel *panel);
		~PrinterWatcher();
		virtual	void EntryCreated(entry_ref& eref, node_ref& nref);
		virtual void EntryRemoved(node_ref& parent_nref, node_ref& nref);
	private:
		node_ref fWatchNodeRef;
		BPrintPanel *fPanel;
	};
	
	struct panel_t
	{
		panel_t(BPrintPanel::pane_type t, BTab *p, BPrintConfigView *v) : type(t), panel(p), cview(v) { }
		panel_t() : type(BPrintPanel::B_NO_PANE), panel(NULL), cview(NULL) { }
		panel_t(const panel_t& p) : type(p.type), panel(p.panel), cview(p.cview) { }
		BPrintPanel::pane_type type;
		BTab *panel;
		BPrintConfigView *cview;
	};

	// ------------------------------------------------------------------
	struct private_print_panel
	// ------------------------------------------------------------------
	{
		PrintPanelWindow *window;
		uint32 flags;
		BMessenger target;
		BMessage settings;
		BMessage message;
		BMessage document_settings;
		BNode printer;
		BString printer_name;
		BPrintPanel::print_panel_action action;
		bool new_api;
		status_t status;
		PrinterWatcher *fPrintersWatcher;
		image_id addon;
		BPrinterConfigAddOn *driver;
		BTransportIO *transport;
		BList panels;
		panel_t configjob_panel;
		panel_t configpage_panel;
		panel_t tools_panel;
		panel_t oldapi_panel;
		uint32 current_page;
		uint32 page_count;
		bool selection;
		int index_tab_page;
		int index_tab_job;
	};
	
} using namespace BPrivate;

// -----------------------------------------------------------------------------


BPrintPanel::BPrintPanel(	print_panel_action action,
							const BMessage *settings,
							const BMessenger *target,
							const BMessage *template_message,
							uint32 flags)
			:	_fPrivate(new private_print_panel),
				_m_rprivate(*_fPrivate)
{
	m.status = B_NO_INIT;
	m.action = action;
	m.flags = flags;
	m.target = (target == NULL) ? be_app_messenger : BMessenger(*target);
	m.settings = (settings == NULL) ? BMessage() : *settings;
	m.message = (template_message == NULL) ? BMessage(B_SETUP_PRINTER) : *template_message;
	m.addon = -1;
	m.new_api = false;
	m.fPrintersWatcher = NULL;
	m.driver = NULL;
	m.transport = NULL;
	m.window = new PrintPanelWindow(this);
	m.fPrintersWatcher = new PrinterWatcher(this);
	m.current_page = 0;
	m.page_count = 0;
	m.selection = false;
	m.document_settings.AddInt32("be:page_count", m.page_count);
	m.document_settings.AddInt32("be:current_page", m.current_page);
	m.document_settings.AddBool("be:selection", m.selection);

	// Make the window active, but hidden
	m.window->Hide();
	m.window->Show();

	// Make sure the printer given in the message exists
	const char *printername = NULL;
	BPrintJobSettings s(m.settings);
	BString pname = s.PrinterName(); // we must copy the string here
	if (pname.Length() > 0)
	{
		BPath path;
		find_directory(B_USER_PRINTERS_DIRECTORY, &path);
		path.Append(pname.String());	
		BEntry printer(path.Path());
		if (printer.Exists())
			printername = pname.String();
	}
	m.status = set_printer(printername);
}

						

BPrintPanel::~BPrintPanel()
{
	const bool windowStillExists = m.window->Lock();
	if (windowStillExists)
		m.window->Close();

	panel_t *a_panel;
	while ((a_panel = static_cast<panel_t *>(m.panels.FirstItem())) != NULL)
	{
		// Here we don't need (must not) to call TabView()->RemoveItem(), because the view has been deleted
		// when the window was closed.
		m.panels.RemoveItem(a_panel);
		delete a_panel;
	}

	delete m.fPrintersWatcher;

	unload_driver_addon();
	delete _fPrivate;
}


status_t BPrintPanel::set_printer(const char *printer_name)
{	// change the current printer,
	// load a new printer add-on if needed and display the appropriate panels

	if (m.window == NULL)
		return B_ERROR;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return B_ERROR;

	BString current_printer;
	status_t result;

	if (printer_name == NULL)
	{ // Get the default printer's name
		if (((result = get_default_printer(current_printer)) != B_OK)  || (current_printer.Length() == 0))
		{
			// REVISIT: here we should display something (at least, remove all tabs)
			remove_panes(B_DRV_PRIVATE0_PANE);
			remove_panes(B_DRV_TOOLS_PANE);
			remove_panes(B_DRV_OPTION_PANE);
			remove_panes(B_DRV_CONFIG_JOB_PANE);
			remove_panes(B_DRV_CONFIG_PAGE_PANE);
			remove_panes(B_APP_OPTION_PANE);
			return B_NO_PRINTER;
		}
		printer_name = current_printer.String();
	}

	// Get printer's node
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	path.Append(printer_name);	
	BNode printer(path.Path());
	if ((result = printer.InitCheck()) != B_OK)
		return B_INVALID_PRINTER;

	// This is the same printer, just return OK
	if (m.printer_name == printer_name)
		return B_OK;

	m.printer_name = printer_name;
	m.printer = printer;

	// TODO: if the driver didn't change. Then do not reload it
	if (true)
	{
		// The driver has changed or is new:
		// Remove all old driver's panels
		remove_panes(B_DRV_PRIVATE0_PANE);
		remove_panes(B_DRV_TOOLS_PANE);
		remove_panes(B_DRV_OPTION_PANE);
		remove_panes(B_DRV_CONFIG_JOB_PANE);
		remove_panes(B_DRV_CONFIG_PAGE_PANE);

		// Unload the current driver
		unload_driver_addon();
		
		// Let a chance to the user to change the currents Settings() (or anything else)
		// We call this AFTER the driver has been unloaded. That way m.driver->SetSettings()
		// won't be called from PrinterChanged() when the user will call this->SetSettings().
		PrinterChanged();

		// And load the new one
		status_t result;
		if ((result = load_driver_addon()) != B_OK) {
			// Record the error code, and set the text to be displayed in the 'state field'
			m.printer.WriteAttr("be:status", B_UINT32_TYPE, 0, &result, sizeof(uint32));
			m.printer.WriteAttr(PSRV_PRINTER_ATTR_STATE, B_STRING_TYPE, 0, strerror(result), strlen(strerror(result))+1);
			m.window->Select(m.printer_name.String());
			m.window->update_size();
			return B_NO_DRIVER;
		} else {
			// If the state of this printer was not B_OK, then mark it as free.
			status_t status;
			if (m.printer.ReadAttr("be:status", B_UINT32_TYPE, 0, &status, sizeof(uint32)) == sizeof(uint32)) {
				m.printer.WriteAttr("be:status", B_UINT32_TYPE, 0, &result, sizeof(uint32));
				if (status != B_OK)
					m.printer.WriteAttr(PSRV_PRINTER_ATTR_STATE, B_STRING_TYPE, 0, PSRV_PRINTER_ATTR_FREE, strlen(PSRV_PRINTER_ATTR_FREE)+1);
			}
		}

		if (m.settings.IsEmpty())
		{
			if (m.new_api)
			{
				BPrintJobEditSettings s(m.driver->Settings().Message());
				s.SetPrinterName(m.printer_name.String());
				m.settings = s.Message();
				m.settings.RemoveName("be:newapi");
				m.settings.AddInt32("be:newapi", 0x00000001);
			}
			else
			{
				BPrintJobEditSettings s;
				s.SetPrinterName(m.printer_name.String());
				m.settings = s.Message();
				m.settings.RemoveName("be:newapi");
			}
		}

		// Let the new driver a chance to add its own panels
		if (m.new_api)
		{
			m.driver->SetSettings(m.settings);
			m.driver->AddPanes(this);
		}
	}
	else
	{
		// The driver is the same:
		if (m.new_api)
		{
			// tell the driver that the printer just changed
			// so that it could update the tabs if needed
			delete m.transport;
			m.transport = new BTransportIO(&(m.printer));

			// Little HACK here. We don't want PrinterChanged() to call any methods
			// in the driver (because the driver state will be updated later by the m.driver->PrinterChanged()
			// call - and we don't want to update it twice in a row -
			BPrinterConfigAddOn *temp = m.driver;
			m.driver = NULL;
			PrinterChanged();
			m.driver = temp;

			if (m.settings.IsEmpty())
			{
				BPrintJobEditSettings s(m.driver->Settings().Message());
				s.SetPrinterName(m.printer_name.String());
				m.settings = s.Message();
				m.settings.RemoveName("be:newapi");
				if (m.new_api)
					m.settings.AddInt32("be:newapi", 0x00000001);
			}

			// update the driver's state
			m.driver->PrinterChanged(m.transport, &(m.printer), m.settings);
		}
	}

	// create the framework window if it doesn't exists and if we have the new driver API
	if (m.new_api)
	{
		// in B_CONFIG_PRINTER mode, we don't add the configjob/config page panels
		if (m.action != B_CONFIG_PRINTER)
		{
			// if ConfigPage() and/or ConfigJob() panel are not set by the driver
			// then, use the default panels
			if (m.configjob_panel.type == B_NO_PANE)
			{
				JobOptionPanel *options = new JobOptionPanel(*(m.driver));
				AddDriverPane(B_DRV_CONFIG_JOB_PANE, options);
			}
	
			if (m.configpage_panel.type == B_NO_PANE)
			{ // Add the default config-page panel
				ConfigPagePanel *config_page = new ConfigPagePanel(*(m.driver));
				AddDriverPane(B_DRV_CONFIG_PAGE_PANE, config_page);
			}
			
			if (m.tools_panel.type == B_NO_PANE)
			{ // Add the default tools panel
				
				if (	(m.driver->CleanPrintHeads(true) == B_OK)			||
						(m.driver->PrintNozzleCheckPattern(true) == B_OK)	||
						(m.driver->PrinterStatus(NULL) == B_OK))
				{ // Add the panel if at least one special feature is supported
					ToolsPanel *tools_panel = new ToolsPanel(*(m.driver));
					AddDriverPane(B_DRV_TOOLS_PANE, tools_panel);
				}
			}
			
			SetPanelAction(m.action);
		}
	}
	else
	{ // Add the GUI for the old API
		if (m.oldapi_panel.type == B_NO_PANE)
		{
			AddDriverPane(B_DRV_PRIVATE0_PANE, new OldApiPanel(*this));
		}
	}
	
	// select the correct tab
	m.window->TabView()->Invalidate();
	m.window->UpdateIfNeeded();

	// Select the correct printer
	m.window->Select(m.printer_name.String());
	m.window->update_size();

	return B_OK;
}

void BPrintPanel::unload_driver_addon()
{
	if (m.addon > 0)
	{
		delete m.transport;
		m.transport = NULL;

		delete m.driver;
		m.driver = NULL;

		unload_add_on(m.addon);
		m.addon = -1;
	}
}

status_t BPrintPanel::load_driver_addon()
{
	// get the driver name attribute
	char driver_name[128];
	ssize_t err = m.printer.ReadAttr(PSRV_PRINTER_ATTR_DRV_NAME, B_STRING_TYPE, 0, driver_name, 128);
	if (err < 0)
		return err;	
	BString path("Print/");
	path << driver_name;

	if ((m.addon = load_add_on(path.String())) < B_OK)
		return (status_t)(m.addon);

	// is it a new printer add-on or an old one?
	BPrinterConfigAddOn *(*instantiate_printer_config_addon)(BTransportIO *, BNode *);
	m.new_api = (get_image_symbol(m.addon, B_INSTANTIATE_PRINTER_CONFIG_ADDON_FUNCTION, B_SYMBOL_TYPE_TEXT, (void **)&instantiate_printer_config_addon) == B_OK);
	if (m.new_api)
	{
		m.transport = new BTransportIO(&(m.printer));
		m.driver = instantiate_printer_config_addon(m.transport, &(m.printer));
		return m.driver->InitCheck();
	}
	return B_OK;
}

status_t BPrintPanel::SendMessage(const BMessenger& target, const BMessage& settings)
{
	BMessage message(settings);
	return target.SendMessage(&message);
}

// #pragma mark -

BString BPrintPanel::Title() const
{
	if (m.window == NULL)
		return "";
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return "";
	return m.window->Title();
}

void BPrintPanel::Show()
{
	if (m.window == NULL)
		return;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return;

	// From BFilePanel
	// if the window is already showing, don't jerk the workspaces around,
	// just pull it to us
	// window in a different workspace, reopen in current
	const uint32 workspace = 1UL << (uint32)current_workspace();
	const uint32 windowWorkspaces = m.window->Workspaces();
	if (!(windowWorkspaces & workspace)) 		
		m.window->SetWorkspaces(workspace);

	if (m.window->IsHidden())
		m.window->Show();

	m.window->Activate();
}

status_t BPrintPanel::Go(BMessage& settings)
{
	if (m.status == B_NO_INIT)
		return m.status;

	if ((m.flags & B_MODAL_PANEL) == 0)
		return B_NOT_ALLOWED;

	status_t err;
	if ((err = m.window->Go()) != B_OK)
		return err;
	settings = m.settings;
	return B_OK;
}

void BPrintPanel::Hide()
{
	if (m.window == NULL)
		return;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return;
	if (!m.window->IsHidden())
		m.window->Hide();
}

bool BPrintPanel::IsShowing() const
{
	if (m.window == NULL)
		return false;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return false;
	return !(m.window->IsHidden());
}

BWindow *BPrintPanel::Window() const
{
	return m.window;
}

BPrintPanel::print_panel_action BPrintPanel::PanelAction() const
{
	return m.action;
}

uint32 BPrintPanel::Flags() const
{
	return m.flags;
}

const BMessenger& BPrintPanel::Target() const
{
	return m.target;
}

const BMessage& BPrintPanel::Settings() const
{
	return m.settings;
}

const BMessage& BPrintPanel::TemplateMessage() const
{
	return m.message;
}

const char *BPrintPanel::PrinterName() const
{
	return m.printer_name.String();
}

const uint32 BPrintPanel::PageCount() const
{
	return m.page_count;
}

const uint32 BPrintPanel::CurrentPage() const
{
	return m.current_page;
}

bool BPrintPanel::DocSelection() const
{
	return m.selection;
}

// #pragma mark -

void BPrintPanel::SetTitle(const BString& title)
{
	if (m.window == NULL)
		return;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return;
	m.window->SetTitle(title.String());
}

void BPrintPanel::SetTarget(const BMessenger& target)
{
	m.target = target;
}

void BPrintPanel::SetTemplateMessage(const BMessage& template_message)
{
	m.message = template_message;
}

void BPrintPanel::SetSettings(const BMessage& settings)
{
	// TODO: Is it actually a printer-change? In this case, call set_printer()
	// update the settings
	m.settings = settings;
	if ((m.new_api) && (m.driver))
		m.driver->SetSettings(m.settings);

	if (m.configjob_panel.type == B_DRV_CONFIG_JOB_PANE)
	{
		JobOptionPanel *panel = static_cast<JobOptionPanel *>(m.configjob_panel.cview);
		panel->update_ui(JobOptionPanel::UPD_SETTINGS);
	}

	if (m.configpage_panel.type == B_DRV_CONFIG_PAGE_PANE)
	{
		ConfigPagePanel *panel = static_cast<ConfigPagePanel *>(m.configpage_panel.cview);
		panel->update_ui(ConfigPagePanel::UPD_SETTINGS);
	}
}

void BPrintPanel::PrinterChanged()
{
	// Here the user may have updated the settings message in the derived method
	// And normalize the setting message (remove all non 'be:' field)
	if (!m.settings.IsEmpty())
	{
		BPrintJobEditSettings s(m.settings);
		D(bug("PrinterChanged() : \"%s\" to \"%s\"\n", s.PrinterName().String(), m.printer_name.String());)
		if (s.PrinterName() != m.printer_name)
			s.Normalize();
		s.SetPrinterName(m.printer_name.String());
		m.settings = s.Message();
		m.settings.RemoveName("be:newapi");
		if (m.new_api)
			m.settings.AddInt32("be:newapi", 0x00000001);
	}
}


status_t BPrintPanel::SetPanelAction(BPrintPanel::print_panel_action action)
{
	if (m.window == NULL)
		return B_ERROR;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return B_ERROR;
	// update the displayed panel
	if (action == B_CONFIG_PAGE) {
		m.window->TabView()->Select(m.index_tab_page);
	} else if ((action == B_CONFIG_JOB) && (m.flags & B_MODAL_PANEL)) {
		m.window->TabView()->Select(m.index_tab_job);
		BButton *button = static_cast<BButton *>(m.window->FindView("be:ok"));
		if (button  && dynamic_cast<BButton *>(button)) {	
			size_t size;
			const char *print = (const char *)GetLibbeResources()->LoadResource(B_STRING_TYPE, "be:print:print", &size);
			button->SetLabel(print);
		}
	} else if (action == B_CONFIG_PRINTER) {
		return B_NOT_ALLOWED; // Not allowed in user code
	} else {
		return B_BAD_VALUE;
	}
	m.action = action;
	return B_OK;
}

status_t BPrintPanel::SetFlags(uint32 flags)
{
	if ((Flags() & B_MODAL_PANEL) != (flags & B_MODAL_PANEL))
		return B_NOT_ALLOWED;	// Can't change the 'modalness' of the panel
	m.flags = flags;
	return B_OK;
}


void BPrintPanel::SetPageCount(uint32 page_count)
{
	if (m.page_count != page_count)
	{
		m.page_count = page_count;
		if ((m.page_count) && (m.current_page > m.page_count))
			m.current_page = 0;		
		update_doc_settings();
	}
}

void BPrintPanel::SetCurrentPage(uint32 current_page)
{
	if (m.current_page != current_page)
	{
		if ((m.page_count) && (current_page > m.page_count))
			current_page = 0;
		m.current_page = current_page;
		update_doc_settings();
	}
}

void BPrintPanel::SetDocSelection(bool selection)
{
	if (m.selection != selection)
	{
		m.selection = selection;
		update_doc_settings();
	}
}

status_t BPrintPanel::SetPrinter(const char *printer_name)
{
	return set_printer(printer_name);
}

void BPrintPanel::update_doc_settings()
{
	if (m.driver)
	{
		m.document_settings.ReplaceInt32("be:page_count", m.page_count);
		m.document_settings.ReplaceInt32("be:current_page", m.current_page);
		m.document_settings.ReplaceBool("be:selection", m.selection);
		m.driver->SetDocumentSettings(m.document_settings);
	}
	
	if (m.configjob_panel.type == B_DRV_CONFIG_JOB_PANE)
	{
		JobOptionPanel *panel = static_cast<JobOptionPanel *>(m.configjob_panel.cview);
		panel->update_ui(JobOptionPanel::UPD_SETTINGS);
		panel->update_ui(JobOptionPanel::UPD_PAGES);
	}

	if (m.configpage_panel.type == B_DRV_CONFIG_PAGE_PANE)
	{
		ConfigPagePanel *panel = static_cast<ConfigPagePanel *>(m.configpage_panel.cview);
		panel->update_ui(ConfigPagePanel::UPD_SETTINGS);
		panel->update_ui(ConfigPagePanel::UPD_PAGES);
	}
}

// #pragma mark -

status_t BPrintPanel::AddPane(pane_type paneType, BPrintConfigView *panel)
{
	if (m.window == NULL)
		return B_ERROR;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return B_ERROR;

	BRect r(m.window->TabView()->Bounds());
	r.top += m.window->TabView()->TabHeight();
	r.InsetBy(BORDER, BORDER);

	if (paneType <= 0)
		return B_NOT_ALLOWED;

	// in B_CONFIG_PRINTER, we only allow B_APP_OPTION_PANE
	if ((m.action == B_CONFIG_PRINTER) && (paneType != B_APP_OPTION_PANE))
		return B_NOT_ALLOWED;

	// Add the panel to a list, etc...
	panel_t& a_panel = *(new panel_t(paneType, new BTab, panel));
	m.panels.AddItem((void *)&a_panel);
	m.window->TabView()->AddTab(a_panel.cview, a_panel.panel);
	m.window->update_size();
	return B_OK;
}

status_t BPrintPanel::AddDriverPane(pane_type paneType, BPrintConfigView *panel)
{
	if (m.window == NULL)
		return B_ERROR;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return B_ERROR;

	BRect r(m.window->TabView()->Bounds());
	r.top += m.window->TabView()->TabHeight();
	r.InsetBy(BORDER, BORDER);

	if (paneType >= 0)
		return B_NOT_ALLOWED;

	// in B_CONFIG_PRINTER, we only allow B_DRV_OPTION_PANE
	if ((m.action == B_CONFIG_PRINTER) && (paneType != B_DRV_OPTION_PANE))
		return B_NOT_ALLOWED;
	
	// TODO: should add the config page panel first, and config job 2nd - always -
	
	// Add the panel to a list, etc...
	panel_t a_panel(paneType, new BTab, panel);
	if (paneType == B_DRV_CONFIG_JOB_PANE)			m.configjob_panel = a_panel;
	else if (paneType == B_DRV_CONFIG_PAGE_PANE)	m.configpage_panel = a_panel;
	else if (paneType == B_DRV_PRIVATE0_PANE)		m.oldapi_panel = a_panel;
	else if (paneType == B_DRV_TOOLS_PANE)			m.tools_panel = a_panel;
	else											m.panels.AddItem((void *)(new panel_t(a_panel)));

	// TODO: should be more clever here
	m.index_tab_job = 0;
	m.index_tab_page = 1;

	m.window->TabView()->AddTab(a_panel.cview, a_panel.panel);
	m.window->update_size();
	return B_OK;
}

status_t BPrintPanel::RemovePane(BPrintConfigView *panel)
{
	if (m.window == NULL)
		return B_ERROR;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return B_ERROR;

	panel_t *a_panel;
	int i = 0;
	while ((a_panel = static_cast<panel_t *>(m.panels.ItemAt(i))) != NULL)
	{
		if (a_panel->cview == panel)
		{
			m.panels.RemoveItem(a_panel);
			delete m.window->TabView()->BTabView::RemoveTab(a_panel->panel);
			delete a_panel;
			break;
		}
		i++;
	}
	return B_OK;
}


// ------------------------------------------------------------
// #pragma mark -


// these functions are just here for compatibility in BPrintJob
status_t BPrintPanel::config_page(BMessage& settings)
{
	// The following code must be called only with old API
// 	BMessage a_message(m.settings);
 	BMessage a_message(settings);
	a_message.what = 'pgst';

	// Set the current selected printer in the message
	a_message.RemoveName("printer");
	a_message.AddPointer("printer", (void*)&(m.printer));

	status_t result;
	BMessage inout, response;
	BMessage *(*config_page)(BNode *, BMessage *);
	if ((result = get_image_symbol(m.addon, "config_page", B_SYMBOL_TYPE_TEXT, (void**)&config_page)) < 0)
		return B_BAD_DRIVER;
	
	// call add-on code for print setup in a thread
	inout.AddPointer("printer_file", (void *)&(m.printer));
	inout.AddPointer("userfunc", (void *)config_page);
	inout.AddPointer("message", (void *)&a_message);
	inout.AddInt32("op", ASYNC_CONFIG_PAGE);
	result = do_async_print_panel(&inout);
	inout.FindMessage("result", &response);
	settings = response;	
	return result;	
}

status_t BPrintPanel::config_job(BMessage& settings)
{
//	BMessage a_message(m.settings);
 	BMessage a_message(settings);
	a_message.what = 'ppst';

	// Set the current selected printer in the message
	a_message.RemoveName("printer");
	a_message.AddPointer("printer", (void*)&(m.printer));

	status_t result;
	BMessage inout, response;
	BMessage *(*config_job)(BNode *, BMessage *);
	if ((result = get_image_symbol(m.addon, "config_job", B_SYMBOL_TYPE_TEXT, (void**)&config_job)) < 0)
		return B_BAD_DRIVER;

	// call add-on code for print setup in a thread
	inout.AddPointer("printer_file", (void *)&(m.printer));
	inout.AddPointer("userfunc", (void *)config_job);
	inout.AddPointer("message", (void *)&a_message);
	inout.AddInt32("op", ASYNC_CONFIG_JOB);
	result = do_async_print_panel(&inout);
	inout.FindMessage("result", &response);
	settings = response;
	return result;	
}

int32 BPrintPanel::_do_async_hook(BMessage *inout)
{
	BPrintPanel *THIS;
	inout->FindPointer("this", (void **)&THIS);
	status_t result;
	switch (inout->FindInt32("op"))
	{
		case ASYNC_CONFIG_JOB:	result = THIS->do_config_job(inout);	break;
		case ASYNC_CONFIG_PAGE:	result = THIS->do_config_page(inout);	break;
		default:
			result = B_ERROR;
	}
	delete_sem(inout->FindInt32("sem"));
	return result;
}

int32 BPrintPanel::do_config_job(BMessage *message)
{
	BMessage *(*userfunc)(BNode*, BMessage*);
	BNode *printer_file;
	BMessage *input;
	message->FindPointer("userfunc", (void **)&userfunc);
	message->FindPointer("printer_file", (void **)&printer_file);
	message->FindPointer("message", (void **)&input);
	BMessage *r = userfunc(printer_file, input);
	if (r == NULL)
		return B_ERROR;
	BMessage result(*r);
	delete r;		
	message->AddMessage("result", &result);
	if (result.what == 'stop')
		return B_CANCEL;
	return B_OK;
}

int32 BPrintPanel::do_config_page(BMessage *message)
{
	BMessage *(*userfunc)(BNode*, BMessage*);
	BNode *printer_file;
	BMessage *input;
	message->FindPointer("userfunc", (void **)&userfunc);
	message->FindPointer("printer_file", (void **)&printer_file);
	message->FindPointer("message", (void **)&input);
	BMessage *r = userfunc(printer_file, input);
	if (r == NULL)
		return B_ERROR;
	BMessage result(*r);
	delete r;		
	message->AddMessage("result", &result);
	if (result.what == 'stop')
		return B_CANCEL;
	return B_OK;
}
status_t BPrintPanel::do_async_print_panel(BMessage *msg)
{
	// Determine if the current thread is a window
	// thread. If so then call UpdateIfNeeded to keep window updated.
	BWindow *window = dynamic_cast<BWindow *>(BLooper::LooperForThread(find_thread(NULL)));

	sem_id sem = create_sem(0, "async");
	if (sem < B_OK)
		return (status_t)sem;

	msg->AddPointer("this", (void *)this);
	msg->AddInt32("sem", (int32)sem);
	thread_id tid = spawn_thread((thread_func)_do_async_hook, "printjob", B_NORMAL_PRIORITY, (void *)msg);
	if (tid < 0)
	{
		delete_sem(sem);
		return (status_t)tid;
	}

	// Launch the thread
	resume_thread(tid);

	status_t err;
	if (window)
	{ // A window is being blocked. We'll keep the window updated by calling UpdateIfNeeded.
		window_feel feel = window->Feel();
		window->SetFeel(B_NORMAL_WINDOW_FEEL);
		window->Hide();
		while (true) 
		{
			while ((err = acquire_sem_etc(sem, 1, B_TIMEOUT, 50000)) == B_INTERRUPTED)
			{ // Nothing to do
			}
			if (err == B_BAD_SEM_ID)
				break;
			window->UpdateIfNeeded();
		}
		window->Show();
		window->SetFeel(feel);
	}
	else
	{
		do
		{
			err = acquire_sem(sem);
		} while (err == B_INTERRUPTED);
	}

	wait_for_thread(tid, (int32 *)&err);
	return err;
}

status_t BPrintPanel::update_printer_list()
{
	if (m.window == NULL)
		return B_ERROR;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return B_ERROR;

	// Update the printer popup menu
	m.window->UpdatePrinterList();	

	// try to select the current selected printer. This may not work if the current printer
	// does not exist anymore
	if (set_printer(m.printer_name.String()) != B_OK)
	{
		// The printer does not exist anymore, cancel the current selection
		m.printer_name = B_EMPTY_STRING;

		// then try the default one
		status_t result;
		if ((result = set_printer()) != B_OK)
			return result;
	}

	// Eventualy select the right item in the popup menu
	m.window->Select(m.printer_name.String());
}

status_t BPrintPanel::save()
{
	BMessage settings;
	status_t err = collect_message(settings);
	if (err != B_OK)
		return err;
	m.settings = settings;
	return SendMessage(Target(), settings);	
}


status_t BPrintPanel::collect_message(BMessage& collect)
{
	if (m.window == NULL)
		return B_ERROR;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return B_ERROR;

	status_t err;
	int32 index = 0;

	if (m.new_api)
	{
		// Save the settings of all attached drivers panels
		if (m.configpage_panel.type == B_DRV_CONFIG_PAGE_PANE) {
			if ((err = m.configpage_panel.cview->Save()) != B_OK)	{
				m.window->TabView()->BTabView::Select(m.configpage_panel.panel);
				return err;
			}
			index++;
		}
	
		if (m.configjob_panel.type == B_DRV_CONFIG_JOB_PANE) {
			if ((err = m.configjob_panel.cview->Save()) != B_OK) {
				m.window->TabView()->BTabView::Select(m.configjob_panel.panel);
				return err;
			}
			index++;
		}
	
		if (m.tools_panel.type == B_DRV_TOOLS_PANE) {
			if ((err = m.tools_panel.cview->Save()) != B_OK) {
				m.window->TabView()->BTabView::Select(m.tools_panel.panel);
				return err;
			}
			index++;
		}
	}

	// Send the message back to the application
	BMessage settings(TemplateMessage());

	if (m.new_api == false)
	{
		if (m.oldapi_panel.type == B_DRV_PRIVATE0_PANE) {
			if ((err = static_cast<OldApiPanel *>(m.oldapi_panel.cview)->Save(settings)) != B_OK) {
				m.window->TabView()->BTabView::Select(m.oldapi_panel.panel);
				return err;
			}
			index++;
		}
	}

	// The other panels
	panel_t *a_panel;
	while ((a_panel = static_cast<panel_t *>(m.panels.ItemAt(index))) != NULL)
	{
		if (a_panel->type != B_NO_PANE) {
			if ((err = a_panel->cview->Save()) != B_OK) {
				m.window->TabView()->Select(index);
				return err;
			}
		}
		index++;
	}

	if ((m.new_api) && (m.driver))
	{
		// Let the driver know that we've finished with the panels.
		// it's its last chance to update the current Settings()
		if (m.driver->Save() == B_OK) {
			merge_messages(settings, m.driver->Settings().Message());
			settings.what = TemplateMessage().what;	
		}
	}

	if (Flags() & B_HIDE_WHEN_DONE)
		Hide();

	D(settings.PrintToStream();)
	collect = settings;
	return B_OK;
}

status_t BPrintPanel::cancel()
{
	if (Flags() & (B_HIDE_WHEN_DONE|B_HIDE_ON_CANCEL))
		Hide();

	// The user canceled.
	BMessage cancel(B_CANCEL);
	cancel.AddInt32("be:old_what", TemplateMessage().what);
	cancel.AddMessage("be:template", &TemplateMessage());
	status_t err;
	if ((err = SendMessage(Target(), cancel)) != B_OK)
		return err;
	return B_CANCELED;
}

status_t BPrintPanel::remove_panes(pane_type type)
{	
	if (m.window == NULL)
		return B_ERROR;
	BAutolock locker(m.window);
	if (locker.IsLocked() == false)
		return B_ERROR;

	if ((type == B_DRV_CONFIG_PAGE_PANE) && (m.configpage_panel.type == B_DRV_CONFIG_PAGE_PANE))
	{
		m.configpage_panel.type = B_NO_PANE;
		delete m.window->TabView()->BTabView::RemoveTab(m.configpage_panel.panel);
		return B_OK;
	}
	
	if ((type == B_DRV_CONFIG_JOB_PANE) && (m.configjob_panel.type == B_DRV_CONFIG_JOB_PANE))
	{
		m.configjob_panel.type = B_NO_PANE;
		delete m.window->TabView()->BTabView::RemoveTab(m.configjob_panel.panel);
		return B_OK;
	}

	if ((type == B_DRV_TOOLS_PANE) && (m.tools_panel.type == B_DRV_TOOLS_PANE))
	{
		m.tools_panel.type = B_NO_PANE;
		delete m.window->TabView()->BTabView::RemoveTab(m.tools_panel.panel);
		return B_OK;
	}

	if ((type == B_DRV_PRIVATE0_PANE) && (m.oldapi_panel.type == B_DRV_PRIVATE0_PANE))
	{
		m.oldapi_panel.type = B_NO_PANE;
		delete m.window->TabView()->BTabView::RemoveTab(m.oldapi_panel.panel);
		return B_OK;
	}

	panel_t *a_panel;
	int i = 0;
	while ((a_panel = static_cast<panel_t *>(m.panels.ItemAt(i))) != NULL)
	{
		if (a_panel->type == type)
		{
			m.panels.RemoveItem(a_panel);
			delete m.window->TabView()->BTabView::RemoveTab(a_panel->panel);
			delete a_panel;
		}
		i++;
	}

	return B_OK;
}


BMessage& BPrintPanel::merge_messages(BMessage& to, const BMessage& msg)
{
	const char* name;
	type_code type;
	long count;
	for( int32 i=0; !msg.GetInfo(B_ANY_TYPE,i,&name,&type,&count); i++ )
	{
		bool fixed_size;
		type_code dummy;
		if (msg.GetInfo(name, &dummy, &fixed_size) != B_OK )
			fixed_size = false;

		bool all_gone = false;
		for (int32 j=0; j<count; j++ )
		{
			const void* data;
			ssize_t size;
			if( msg.FindData(name,type,j,&data,&size) == B_OK )
			{
				if ( !all_gone )
				{
					if( type == B_MESSAGE_TYPE )
					{
						BMessage oldMsg;
						BMessage newMsg;
						if ( to.FindMessage(name,j,&oldMsg) == B_OK && msg.FindMessage(name,j,&newMsg) == B_OK )
						{
							merge_messages(oldMsg, newMsg);
							to.ReplaceMessage(name,j,&oldMsg);
						} else {
							all_gone = true;
						}
					}

					if ( to.ReplaceData(name,type,j,data,size) < B_OK )
					{
						long cnt=0;
						type_code mtype = type;
						if ( !to.GetInfo(name,&mtype,&cnt) )
						{
							for( int32 k=cnt-1; k>=j; k-- )
							{
								to.RemoveData(name,k);
							}
						}
						all_gone = true;
					}
				}
				if ( all_gone )
					to.AddData(name,type,data,size,fixed_size);
			}
		}
	}
	return to;
}


// #pragma mark -

status_t BPrintPanel::Perform(int32 selector, void * data) { return B_ERROR; }
status_t BPrintPanel::_Reserved_BPrintPanel_0(int32 arg, ...) { return B_ERROR; }
status_t BPrintPanel::_Reserved_BPrintPanel_1(int32 arg, ...) { return B_ERROR; }
status_t BPrintPanel::_Reserved_BPrintPanel_2(int32 arg, ...) { return B_ERROR; }
status_t BPrintPanel::_Reserved_BPrintPanel_3(int32 arg, ...) { return B_ERROR; }
status_t BPrintPanel::_Reserved_BPrintPanel_4(int32 arg, ...) { return B_ERROR; }
status_t BPrintPanel::_Reserved_BPrintPanel_5(int32 arg, ...) { return B_ERROR; }
status_t BPrintPanel::_Reserved_BPrintPanel_6(int32 arg, ...) { return B_ERROR; }
status_t BPrintPanel::_Reserved_BPrintPanel_7(int32 arg, ...) { return B_ERROR; }

// ------------------------------------------------------------
// #pragma mark -

PrinterWatcher::PrinterWatcher(BPrintPanel *panel) : NodeWatcher(be_app), fPanel(panel)
{
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	BNode node(path.Path());
	node.GetNodeRef(&fWatchNodeRef);
	StartWatching(fWatchNodeRef, B_WATCH_DIRECTORY);
};

PrinterWatcher::~PrinterWatcher()
{
	StopWatching(fWatchNodeRef);
}

void PrinterWatcher::EntryCreated(entry_ref& eref, node_ref& nref)
{
	fPanel->update_printer_list();
}

void PrinterWatcher::EntryRemoved(node_ref& parent_nref, node_ref& nref)
{
	fPanel->update_printer_list();
}
