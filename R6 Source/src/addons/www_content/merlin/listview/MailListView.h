/*
	MailListView.h
*/
#ifndef _MailListView_h
#define _MailListView_h
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <experimental/ResourceSet.h>
#include <MimeMessage.h>
#include <String.h>
#include "PostOffice.h"
	
using namespace BExperimental;
using namespace Wagner;

BResourceSet& Resources();
const char* ResourceString(int32 id);
const BBitmap* ResourceBitmap(const char* name);
void DrawFieldProperly(BRect rect, BView *parent, const char *text, const BFont *font, uint32 flags);

const bool kUse24hour = false;
const bool kUseRelative = true;
const int32 kSecondsPerDay = 86400;
const int32 kMimeMessageHashSize = 97;

enum column_mode {
	kIncomingMode = 0,
	kOutgoingMode
};

enum {
	kSelectionChanged = 'slch',
	kOpenMessage = 'open',
	kSyncList = 'sync'
};

enum icon_type {
	kNoIcon,
	kMailIcon,
	kAttachmentIcon,
	kScootIcon
};

class MailListItem;
class MailHashEntry;

class MailListView : public BColumnListView {
	public:
								MailListView(BRect frame, BMessage &parameters, atomref<GHandler> target);
		virtual 				~MailListView();
		
		virtual void 			AttachedToWindow();
		virtual void 			DetachedFromWindow();
		virtual void 			SelectionChanged();
		virtual void 			ItemInvoked();
		
		void					DeselectAllAndClear();
		void					SetColumnMode(column_mode mode);
		MailListItem *			FindMailListItem(const char *uid);
		
		void					SyncList(SummaryContainer *container, uint32 event);
		void					AddEntry(MailHashEntry *entry);
		void					RemoveEntry(MailHashEntry *entry);
		MailHashEntry *			FindEntry(const MimeMessage *message);
		inline bool				ContainsMessage(const MimeMessage *message);
		
	private:
		BMessage fState;
		MailListItem *fSelectedRow;
		column_mode fColumnMode;
		atomref<GHandler> fTarget;
		MailHashEntry *fEntries[kMimeMessageHashSize];
		
};

class MailHashEntry {
	public:
								MailHashEntry();
								MailHashEntry(MimeMessage *message);
								~MailHashEntry();
	
		const MimeMessage *		Message() const;
		MimeMessage *			Message();

	private:
		MimeMessage *fMessage;
		MailHashEntry *fHashNext;
		MailHashEntry **fHashPrevious;
		
		friend class MailListView;
};

class MailListItem : public BRow {
	public:
								MailListItem(MimeMessage *mime, column_mode mode);
		virtual					~MailListItem();
		
		void					ReloadFields();
		bool					HasAttachments() const;
		
		const char				*GetUID() const;
		const char				*GetMailbox() const;
		const char 				*GetDate() const;
		const char				*GetSubject() const;
		const char				*GetAddress() const;
				
	private:
		MimeMessage *fMessage;
		column_mode fMode;
		BString fAddress;
		bool fHasAttachments;
};

// These are a bunch of classes that extend the functionality
// of the default BFields for the column list view. In our case
// they pretty much just give us access to the Imap Flags set
// for a particular message. The appropriate BColumns draw
// the fields differently depending on the flags....
class MailField
{
	public:
								MailField(MimeMessage *mime);
		virtual					~MailField();

		uint32 					Flags();

	protected:
		MimeMessage *fMessage;
};

class ImapStringColumn : public BTitledColumn
{
	public:
								ImapStringColumn(const char *title, float width, float maxWidth,
													float minWidth, uint32 truncate);

		virtual void			DrawField(BField *field, BRect rect, BView *parent);
		virtual int 			CompareFields(BField *field1, BField *field2);
	
	private:
		uint32 fTruncate;
};

class ImapStringField : public BStringField, public MailField
{
	public:
								ImapStringField(const char *string, MimeMessage *mime);

};

class ImapDateColumn : public BTitledColumn
{
	public:
								ImapDateColumn(const char *title, float width,
												float minWidth, float maxWidth);

		virtual void 			DrawField(BField *field, BRect rect, BView *parent);
		virtual int 			CompareFields(BField *field1, BField *field2);

		void 					CalculateRelativeTime(float width, BString *out, time_t when, bool bold);
		void 					DateToString(float width,BString *output,time_t time, bool newMessage);
};

class ImapDateField : public BDateField, public MailField
{
	public:
								ImapDateField(time_t *t, MimeMessage *mime);

};

class ImapIconStatusColumn : public BTitledColumn
{
	public:
								ImapIconStatusColumn(const char *title, float width,
														float minWidth, float maxWidth, bool flag);

		virtual void 			DrawField(BField *field, BRect rect, BView *parent);
		virtual int 			CompareFields(BField *field1, BField *field2);

	private:
		bool fFlagColumn;
};

class ImapIconStatusField : public BField, public MailField
{
	public:
								ImapIconStatusField(icon_type itype, MimeMessage *message);

		const BBitmap*			Bitmap() const; 
		float 					BitmapHeight() const;
		void 					UpdateBitmap(bool force = false);
		bool 					HasAttachment();

	private:
		icon_type fIconType;
		BBitmap *fBitmap;
		uint32 fRecentFlags;
		float fBitmapHeight;
};

class ImapSizeColumn : public BTitledColumn
{
	public:
								ImapSizeColumn(const char *title, float width,
													float minWidth,	float maxWidth);

		virtual void 			DrawField(BField *field, BRect rect, BView *parent);
		virtual int 			CompareFields(BField *field1, BField *field2);
};

class ImapSizeField : public BStringField, public MailField
{
	public:
								ImapSizeField(MimeMessage *mime);

		uint32 					Size();
};


#define mail_author(s) (strstr(s, "kenny@kennyc.com") || strstr(s, "kenny@be.com") || strstr(s, "michelangelo@hopbot.com") || strstr(s, "michelangelo@be.com"))
#endif
