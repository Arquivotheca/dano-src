/*	ShowWindow.h */

#if !defined(_SHOW_WINDOW_H)
#define _SHOW_WINDOW_H

#include <Message.h>
#include <Window.h>

#include <Entry.h>

#include <TranslationDefs.h>

const uint32 msg_WindowDeleted = 'WDel';
const uint32 msg_ShowOpenPanel = 'PSho';
const uint32 msg_SetDirty = 'SETd';
const uint32 msg_SelectSlice = 'SSLc';

class BFilePanel;
class SliceDetails;
class BPrintPanel;
class BitmapView;

class ShowWindow : public BWindow
{
public:
		ShowWindow(
				BRect area,
				const char * title,
				const entry_ref & ref,
				translator_id translator,
				uint32 format,
				window_type type,
				uint32 flags);

		~ShowWindow();

		void MessageReceived(
				BMessage * message);
		bool QuitRequested();

		status_t LoadSlices();
		
		void EnableSlicing(
				bool	enabled = true);
		
		bool GetDragInfo(
				int32 * fDragFormat,
				int32 * fDragTranslator);
		void AddDNDTypes(
				BMessage * toMessage);
		bool MatchType(
				const char * mime_type,
				int32 * out_format,
				int32 * out_translator,
				const char * type_name = 0);
private:

		BFilePanel * thePanel;
		BFilePanel * saveAsPanel;

		void DoOpen(
				BMessage * message);
		void DoSaveAs(
				BMessage * message);
		
		void DoPrintSetup(BitmapView * bmv);
		void DoPrint(BitmapView * bmv, bool preview = false);
		
virtual	void MenusBeginning();

		entry_ref fRef;
		translator_id fTranslator;
		uint32 fFormat;
		
		entry_ref fSliceRef;
		
		int32 fDragFormat;
		int32 fDragTranslator;
		BMessage fDNDTypes;
		bool fDirty, fSlicesDirty;

		SliceDetails* fSliceDetails;
		
		BMessage *fPrintSettings;
		BPrintPanel *fPrintPanel;
};


#endif /* _SHOW_WINDOW_H */
