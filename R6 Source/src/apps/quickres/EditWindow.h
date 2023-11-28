#ifndef QUICKRES_EDIT_WINDOW_H
#define QUICKRES_EDIT_WINDOW_H

#include <ResourceEditor.h>
#include <ResourceRoster.h>

#include <View.h>
#include <Window.h>
#include <String.h>

#include <ToolTipHook.h>

class BView;
class BMenuItem;
class BMessageRunner;

enum {
	EDITWIN_ACTIVATE = 'Eact',
	EDITWIN_APPLYCHANGES = 'Eacg'
};

class EditWindowController : public BResourceAddonBase
{
public:
	EditWindowController(BWindow& window,
						 BResourceRoster& roster,
						 BResourceAddonArgs args);

	void				SetContainer(BView* container);
	void				SetMenus(BMenuBar* bar, BMenu* editMenu,
								 BMenu* customPosition);
	
	void				SetShowAllUndos(bool state = true);
	bool				ShowAllUndos() const;
	
	BList*				GetContext(const BResourceCollection* collection,
								   BList* context);
	
	status_t			ExecMessageReceived(BMessage *msg);
	
	void				UpdateUndoMenus(BMenuItem* undo, BMenuItem* redo);
	void				PerformUndo();
	void				PerformRedo();
	
	bool				ApplyChanges(BMessage* reply = 0);
	
	enum {
		T_UPDATE_NOW		= (1<<0),
		T_UPDATE_FOCUS		= (1<<1)
	};
	void				ForgetEditor();
	void				UpdateEditor(BResourceHandle it, uint32 flags=0);
	void				ShowEditor(uint32 flags=0);

private:
	void				DeleteMenus();
	
	BWindow&			fWindow;
	BResourceRoster&	fRoster;
	bool				fShowAllUndos;
	
	BResourceHandle		fEditItem;
	
	BView*				fContainer;
	BMenuBar*			fMenuBar;
	BMenu*				fEditMenu;
	BMenu*				fCustomPosition;
	
	BResourceAddon*		fEditAddon;
	BString				fEditName;
	BFullItemEditor*	fFullEditor;
	
	BMessageRunner*		fEditorShow;
	
	BView*				fEditorView;
	BList				fEditorMenuItems;
	BList				fEditorMenus;
};

class EditWindow : public BWindow, public BResourceAddonBase,
				   public BToolTipFilter
{
public:
	EditWindow(const BResourceAddonArgs& args, BResourceHandle handle,
			   BResourceRoster& roster,
			   BPoint center, const char* doc_name,
			   window_look look = B_DOCUMENT_WINDOW_LOOK,
			   window_feel feel = B_NORMAL_WINDOW_FEEL,
			   uint32 flags = B_ASYNCHRONOUS_CONTROLS,
			   uint32 workspace = B_CURRENT_WORKSPACE);

	virtual void	MessageReceived(BMessage *msg);
	
	virtual void	Quit();
	virtual bool	QuitRequested();
	virtual void	MenusBeginning();
	
	virtual void	DataChanged(BResourceHandle& item);
	
private:
	void			UpdateTitle();
	
	EditWindowController	fController;
	
	BResourceHandle			fEditItem;
	
	BString					fDocName;
	
	BView*					fContainer;
	
	BMenuItem*				fUndoItem;
	BMenuItem*				fRedoItem;
	BMenuItem*				fCutItem;
	BMenuItem*				fCopyItem;
	BMenuItem*				fPasteItem;
	BMenuItem*				fClearItem;
};

#endif
