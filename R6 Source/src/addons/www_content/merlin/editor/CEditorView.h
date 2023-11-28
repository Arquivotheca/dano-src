/*
	CEditorView.h
*/
#ifndef _CEditorView_h
#define _CEditorView_h
#include <ScrollView.h>
#include "CTextView.h"

enum text_style {
	kPlainStyle,
	kBoldStyle
};
	
class CEditorView : public BView
{
	public:
								CEditorView(BRect frame);
								~CEditorView();

		status_t 				SendMessage(BMessage *args);
		status_t				AddAttachment(const char *path, const char *name);
		status_t				RemoveAttachment(const char *path);
		status_t				RemoveAllAttachments();
		status_t 				StreamTextBody(BDataIO *stream, int32 thePoliceMan, volatile int32 *textViewPolice,
												BLocker *locker, int32 mode);
		
		CTextView 				*TextView();
		void 					SetSelectedTextColor(const rgb_color *color);
		void 					SetSelectedTextSize(int delta);
		void 					SetSelectedTextStyle(text_style style);

	private:
		CTextView *fTextView;
		BList fAttachmentList;
};

#endif
