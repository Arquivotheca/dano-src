//--------------------------------------------------------------------
//	
//	Header.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <StorageKit.h>
#include <InterfaceKit.h>
#include <StringView.h>
#include <E-mail.h>
#include <String.h>
#include <fs_index.h>
#include <fs_info.h>
#include <map>

#include "Mail.h"
#include "Header.h"
#include "Utilities.h"
#include "QueryMenu.h"
#include "FieldMsg.h"

const char	*kDateLabel = "Date:";
	
ssize_t	FetchAttribute( const BString &attribute, const BNode &file, BString &result );	

class QPopupMenu : public QueryMenu
{
	public:
		QPopupMenu( const char *title );
		
	protected:
		virtual void EntryCreated( const entry_ref &ref, ino_t node );
		virtual void EntryRemoved( ino_t node );
		
		int32 fGroups;
};

//====================================================================

struct evil {
	bool operator()(const BString *s1, const BString *s2) const {
		return (s1->Compare(*s2) < 0);
	}
};

//====================================================================

THeaderView::THeaderView(BRect rect, BRect wind_rect, bool incoming,
						 BFile *file, bool resending)
			:BBox(rect, "m_header", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW, B_PLAIN_BORDER),
			fFile(NULL),
			fBccMenu(NULL),
			fCcMenu(NULL),
			fToMenu(NULL),
			fDate(NULL),
			fBcc(NULL),
			fCc(NULL),
			fResending(resending),
			fIncoming(incoming)
{
	char		string[20];
	float		offset;
	BMenuField	*field;
	BRect		r;
	BMessage	*msg;


	BFont		font = *be_plain_font;
	font.SetSize(FONT_SIZE);
	SetFont(&font);
	offset = font.StringWidth("Enclosures: ") + 12;
	
	if (!fIncoming)	{
		InitEmailCompletion();
		InitGroupCompletion();
	}
	if ((fIncoming) && (!resending)) {
		r.Set(offset - font.StringWidth(FROM_TEXT) - 11, TO_FIELD_V,
			  wind_rect.Width() - SEPERATOR_MARGIN,
			  TO_FIELD_V + TO_FIELD_HEIGHT);
			sprintf(string, FROM_TEXT);
	}
	else {
		r.Set(offset - 11, TO_FIELD_V,
			  wind_rect.Width() - SEPERATOR_MARGIN,
			  TO_FIELD_V + TO_FIELD_HEIGHT);
		string[0] = 0;
	}
	fTo = new TTextControl(r, string, new BMessage(TO_FIELD),
		fIncoming, resending, B_FOLLOW_LEFT_RIGHT);

	if (!fIncoming) {
		fTo->SetChoiceList( &emailList );
		fTo->SetAutoComplete( true );
	}
	AddChild(fTo);
	(msg = new BMessage( FIELD_CHANGED ))->AddInt32( "bitmask", FIELD_TO );
	fTo->SetModificationMessage( msg );
	
	if ((!fIncoming) || (resending)) {
		r.right = r.left + 8;
		r.left = r.right - be_plain_font->StringWidth(TO_TEXT) - 30;
		r.top -= 1;
		fToMenu = new QPopupMenu(TO_TEXT);
		field = new BMenuField(r, "", "", fToMenu, B_FOLLOW_LEFT | B_FOLLOW_TOP,
													B_WILL_DRAW);
		field->SetDivider(0.0);
		field->SetEnabled(true);
		AddChild(field);
	}

	r.Set(offset - font.StringWidth(SUBJECT_TEXT) - 11, SUBJECT_FIELD_V,
		  wind_rect.Width() - SEPERATOR_MARGIN,
		  SUBJECT_FIELD_V + TO_FIELD_HEIGHT);
	fSubject = new TTextControl(r, SUBJECT_TEXT, new BMessage(SUBJECT_FIELD),
				fIncoming, false, B_FOLLOW_LEFT_RIGHT);
	AddChild(fSubject);
	(msg = new BMessage( FIELD_CHANGED ))->AddInt32( "bitmask", FIELD_SUBJECT );
	fSubject->SetModificationMessage( msg );
	
	if( fResending )
		fSubject->SetEnabled( false );
	
	if (!fIncoming) {
		r.Set(offset - 11, CC_FIELD_V,
			  CC_FIELD_H + CC_FIELD_WIDTH,
			  CC_FIELD_V + CC_FIELD_HEIGHT);
		fCc = new TTextControl(r, "", new BMessage(CC_FIELD),
											fIncoming, false);
		fCc->SetChoiceList( &emailList );
		fCc->SetAutoComplete( true );
		AddChild(fCc);
		(msg = new BMessage( FIELD_CHANGED ))->AddInt32( "bitmask", FIELD_CC );
		fCc->SetModificationMessage( msg );
	
		r.right = r.left + 9;
		r.left = r.right - be_plain_font->StringWidth(CC_TEXT) - 30;
		r.top -= 1;
		fCcMenu = new QPopupMenu(CC_TEXT);
		field = new BMenuField(r, "", "", fCcMenu, B_FOLLOW_LEFT | B_FOLLOW_TOP,
									B_WILL_DRAW);

		field->SetDivider(0.0);
		field->SetEnabled(true);
		AddChild(field);

		r.Set(BCC_FIELD_H + be_plain_font->StringWidth(BCC_TEXT), BCC_FIELD_V,
		  wind_rect.Width() - SEPERATOR_MARGIN, BCC_FIELD_V + BCC_FIELD_HEIGHT);
		fBcc = new TTextControl(r, "", new BMessage(BCC_FIELD),
						fIncoming, false, B_FOLLOW_LEFT_RIGHT);
		fBcc->SetChoiceList( &emailList );
		fBcc->SetAutoComplete( true );
		AddChild(fBcc);
		(msg = new BMessage( FIELD_CHANGED ))->AddInt32( "bitmask", FIELD_BCC );
		fBcc->SetModificationMessage( msg );
		
		r.right = r.left + 9;
		r.left = r.right - be_plain_font->StringWidth(BCC_TEXT) - 30;
		r.top -= 1;
		fBccMenu = new QPopupMenu(BCC_TEXT);
		field = new BMenuField(r, "", "", fBccMenu, B_FOLLOW_LEFT | 
		  B_FOLLOW_TOP, B_WILL_DRAW);
		field->SetDivider(0.0);
		field->SetEnabled(true);
		AddChild(field);
	} else {
		r.left = offset - font.StringWidth(kDateLabel) - 10;
		r.OffsetBy(0.0, fSubject->Frame().Height() + 1);
		fDate = new BStringView(r, "", "");
		AddChild(fDate);	
		fDate->SetFont(&font);	
		fDate->SetHighColor(0, 0, 0);

		LoadMessage( file );
	}
}

//--------------------------------------------------------------------

void 
THeaderView::InitEmailCompletion()
{
	// get boot volume
	BVolumeRoster roster;
	BVolume v;
	roster.GetBootVolume( &v );

	// make sure "META:email" is indexed
	fs_create_index( dev_for_path( "/boot" ), "META:email", B_STRING_TYPE, 0 );

	BQuery query;
	query.SetVolume( &v );
	query.SetPredicate( "META:email=**" );
	query.Fetch();
	entry_ref ref;
	BNode file;
	BString email;
	
	// The leash it a work-around for when the something in the query breaks and prevents
	// GetNextRef() from ever returning B_ENTRY_NOT_FOUND
	for ( int32 leash=0; (query.GetNextRef(&ref) != B_ENTRY_NOT_FOUND)&&(leash < 1024); leash++) {
		if (file.SetTo(&ref) == B_OK) {
			FetchAttribute("META:email", file, email);
			emailList.AddChoice(email.String());
		}
	}
}

void 
THeaderView::InitGroupCompletion()
{
	// get boot volume
	BVolumeRoster roster;
	BVolume v;
	roster.GetBootVolume( &v );

	// make sure "META:group" is indexed
	fs_create_index(dev_for_path("/boot"), "META:group", B_STRING_TYPE, 0);

	// build a list of all unique groups and the addresses
	// they expand to
	BQuery query;
	query.SetVolume(&v);
	query.SetPredicate("META:group=**");
	query.Fetch();

	map<BString *, BString *, evil> group_map;
	entry_ref ref;
	BNode file;
	while (query.GetNextRef(&ref) == B_OK) {
		if (file.SetTo(&ref) != B_OK)
			continue;
		
		BString groups;
		FetchAttribute("META:group", file, groups);

		BString address;
		FetchAttribute("META:email", file, address);

		// avoid adding an empty address
		if (address.Length() == 0)
			continue;
			
		char *grp = groups.LockBuffer(groups.Length());
		char *next = strchr(grp, ',');
		
		for (;;) {
			if (next) *next = 0;
			while (*grp && *grp == ' ') grp++;
		
			BString *group = new BString(grp);
			
			// nobody is in this group yet, start it off
			if (group_map[group] == NULL) {
				BString *addresses = new BString(*group);
				addresses->Append(" <");
				addresses->Append(address);
				group_map[group] = addresses;
			} else {
				BString *a = group_map[group];
				a->Append(", ");
				a->Append(address);
				
				delete group;
			}

			if (!next)
				break;
				
			grp = next+1;
			next = strchr(grp, ',');
		}
	}
	
	map<BString *, BString *, evil>::iterator iter;
	for (iter = group_map.begin(); iter != group_map.end(); ) {
		BString *grp = iter->first;
		BString *addr = iter->second;
		addr->Append(">");
		emailList.AddChoice(addr->String());
		++iter;
		group_map.erase(grp);
		delete grp;
		group_map.erase(addr);
		delete addr;
	}

}

//--------------------------------------------------------------------

void THeaderView::MessageReceived(BMessage *msg)
{
	BMessage	*message;
	BTextView	*text_view;

	switch (msg->what) {
		case B_SIMPLE_DATA:
			text_view = (BTextView *)fTo->ChildAt(0);
			if (text_view->IsFocus())
				fTo->MessageReceived(msg);
			else if (!fIncoming) {
				text_view = (BTextView *)fCc->ChildAt(0);
				if (text_view->IsFocus())
					fCc->MessageReceived(msg);
				else {
					text_view = (BTextView *)fBcc->ChildAt(0);
					if (text_view->IsFocus())
						fBcc->MessageReceived(msg);
					else {
						message = new BMessage(*msg);
						message->what = REFS_RECEIVED;
						Window()->PostMessage(message, Window());
						delete message;
					}
				}
			}
			break;
	}
}

void THeaderView::AttachedToWindow(void)
{
	if( fToMenu )
	{
		fToMenu->SetTargetForItems( fTo );
		fToMenu->SetPredicate( "META:email=**" );
	}
	if( fCcMenu )
	{
		fCcMenu->SetTargetForItems( fCc );
		fCcMenu->SetPredicate( "META:email=**" );
	}
	if( fBccMenu )
	{
		fBccMenu->SetTargetForItems( fBcc );
		fBccMenu->SetPredicate( "META:email=**" );
	}
	if( fTo )
		fTo->SetTarget( Looper() );
	if( fSubject )
		fSubject->SetTarget( Looper() );
	if( fCc )
		fCc->SetTarget( Looper() );
	if( fBcc )
		fBcc->SetTarget( Looper() );
	BBox::AttachedToWindow();
}

//--------------------------------------------------------------------

void THeaderView::BuildMenus()
{
	/*
	BMenuItem *item;
	while (item = fToMenu->ItemAt(0)) {
		fToMenu->RemoveItem(item);
		delete item;
	}
	if (!fResending) {
		while (item = fCcMenu->ItemAt(0)) {
			fCcMenu->RemoveItem(item);
			delete item;
		}
		while (item = fBccMenu->ItemAt(0)) {
			fBccMenu->RemoveItem(item);
			delete item;
		}
	}

	// do a query to find all People files where there is a group
	BVolumeRoster	volume;
	BVolume			vol;
	volume.GetBootVolume(&vol);

	BQuery			query;
	query.SetVolume(&vol);
	query.SetPredicate( "META:email=**" );
	//query.PushAttr("META:email");
	//query.PushString("**");
	//query.PushOp(B_EQ);
	query.Fetch();

	BEntry	entry;
	entry_ref ref;
	BFile	file;
	int		groups=0;
	
	printf( "Build Menus\n" );
	status_t status;
	
	while ((status = query.GetNextRef(&ref)) == B_OK) {
		entry.GetRef( &ref );
		printf( "Found: %s\n", ref.name );
		file.SetTo(&ref, B_READ_ONLY);
		if (file.InitCheck() == B_NO_ERROR) {
			printf( "Init Ok\n" );
			BString	name;
			FetchAttribute("META:name", file, name);
			BString email;
			FetchAttribute("META:email", file, email);
			BString grouplist;
			FetchAttribute("META:group", file, grouplist);
			
			BString label;
			BString address;
			// if we have no Name, just use the email address 
			if (name.Length() == 0) {
				address = email;
				label = email;
			// otherwise, pretty-format it 
			} else {
				address.SetTo("") << "\"" << name << "\" <" << email << ">";
				label.SetTo("") << name << " (" << email << ")";
			}	

			if (grouplist.Length() == 0) {
			// just put it in the top level (contains no groups)  
				BMessage msg;
				msg.AddString("address", address.String());
				
				int index=groups;
				while (item = fToMenu->ItemAt(index)) {
					if(strcasecmp(item->Label(), label.String()) > 0) break;
					index++;
				}
				msg.what = M_TO_MENU;
				fToMenu->AddItem(new BMenuItem(label.String(), new BMessage(msg)), index);
				
				if(!fResending) {
					msg.what = M_CC_MENU;
					fCcMenu->AddItem(new BMenuItem(label.String(), new BMessage(msg)), index);
					
					msg.what = M_BCC_MENU;
					fBccMenu->AddItem(new BMenuItem(label.String(), new BMessage(msg)), index);
				}				
			} else {
				
			// this name is in one or more groups 
			grouplist.Append(","); 
			BString group;
			int group_end=0;
			
			while(grouplist.Length() > 0) {
				grouplist.CopyInto(group, 0, group_end = grouplist.FindFirst(",", 0));
				grouplist.Remove(0, group_end +1);

				// here we check to see if the submenu has been created
				if((item = fToMenu->FindItem(group.String())) == NULL) {
					int index=0;
					item = fToMenu->ItemAt(index);
					while (item != NULL && index < groups) {
						if(strcasecmp(item->Label(), group.String()) > 0) break;
						index++;
						item = fToMenu->ItemAt(index);
					}
					
					BMessage msg;
					msg.AddString("group", group.String());
									
					BMenu *submenu = new BMenu(group.String());
					submenu->SetFont(be_plain_font);
					msg.what = M_TO_MENU;
					fToMenu->AddItem(new BMenuItem(submenu, new BMessage(msg)), index);
					
					if(!fResending) {
						submenu = new BMenu(group.String());
						submenu->SetFont(be_plain_font);
						msg.what = M_CC_MENU;
						fCcMenu->AddItem(new BMenuItem(submenu, new BMessage(msg)), index);
		
						submenu = new BMenu(group.String());
						submenu->SetFont(be_plain_font);
						msg.what = M_BCC_MENU;
						fBccMenu->AddItem(new BMenuItem(submenu, new BMessage(msg)), index);
					}
					groups++;
				}
					
					// the submenu either exists or was just created 
					item = fToMenu->FindItem(group.String());
					BMenu *submenu = item->Submenu();
		
					int index=0;
					while (item = submenu->ItemAt(index)) {
						if(strcasecmp(item->Label(), label.String()) > 0) break;
						index++;
					}
					
					BMessage msg;
					msg.AddString("address", address.String());
					
					msg.what=M_TO_MENU;
					submenu->AddItem(new BMenuItem(label.String(), new BMessage(msg)), index);
					
					if (!fResending) {
						msg.what=M_CC_MENU;
						item = fCcMenu->FindItem(group.String());
						submenu = item->Submenu();
						submenu->AddItem(new BMenuItem(label.String(), new BMessage(msg)), index);
						
						msg.what=M_BCC_MENU;
						item = fBccMenu->FindItem(group.String());
						submenu = item->Submenu();
						submenu->AddItem(new BMenuItem(label.String(), new BMessage(msg)), index);
					}
				}
			}
		}
		else
			printf( "Init Check Failed\n" );
	}
	printf( "Status Say: %s\n", strerror( status ) );
	// add the seperator bars
	if (groups > 0) {
		fToMenu->AddItem(new BSeparatorItem(), groups);
		if (!fResending) {
			fCcMenu->AddItem(new BSeparatorItem(), groups);
			fBccMenu->AddItem(new BSeparatorItem(),  groups);
		}
	}
	*/
}

//--------------------------------------------------------------------

ssize_t FetchAttribute( const BString &attribute, const BNode &file, BString &result )
{
	attr_info info;
	ssize_t status;
	
	status = file.GetAttrInfo(attribute.String(), &info);
	if(status == B_NO_ERROR) {
		file.ReadAttr(attribute.String(), B_STRING_TYPE, 0, result.LockBuffer(info.size+1), info.size);
		result.UnlockBuffer();
	} else {
		result.SetTo("");
	}
	return status;
}


//--------------------------------------------------------------------

void THeaderView::SetAddress(BMessage *msg)
{
	bool			group = false;
	const char		*msgStr;
	int32			end;
	int32			start;
	BEntry			entry;
	BFile			file;
	BQuery			query;
	BTextView		*text;
	BVolume			vol;
	BVolumeRoster	volume;
	attr_info		info;

	switch (msg->what) {
		case M_TO_MENU:
			text = fTo->TextView();
			if (msg->HasString("group")) {
				msg->FindString("group", &msgStr);
				group = true;
			}
			else
				msg->FindString("address", &msgStr);
			break;
		case M_CC_MENU:
			text = fCc->TextView();
			if (msg->HasString("group")) {
				msg->FindString("group", &msgStr);
				group = true;
			}
			else
				msg->FindString("address", &msgStr);
			break;
		case M_BCC_MENU:
			text = fBcc->TextView();
			if (msg->HasString("group")) {
				msg->FindString("group", &msgStr);
				group = true;
			}
			else
				msg->FindString("address", &msgStr);
			break;
	}

	text->GetSelection(&start, &end);
	if (start != end)
		text->Delete();

	if (group) {
		volume.GetBootVolume(&vol);
		query.SetVolume(&vol);

		BString predicate;
		predicate << "META:group='*" << msgStr << "'";
		query.SetPredicate(predicate.String());
		query.Fetch();

		while (query.GetNextEntry(&entry) == B_NO_ERROR) {
			file.SetTo(&entry, O_RDONLY);
			BString	name;
			FetchAttribute("META:name", file, name);
			BString email;
			FetchAttribute("META:email", file, email);

			BString address;
			/* if we have no Name, just use the email address */
			if (name.Length() == 0) {
				address = email;
			/* otherwise, pretty-format it */
			} else {
				address.SetTo("") << "\"" << name << "\" <" << email << ">";
			}
			
			if (end = text->TextLength()) {
				text->Select(end, end);
				text->Insert(", ");
			}	
			text->Insert(address.String());
		}
	}

	else {
		if (end = text->TextLength()) {
			text->Select(end, end);
			text->Insert(", ");
		}
		text->Insert(msgStr);
	}

	text->Select(text->TextLength(), text->TextLength());
}

status_t THeaderView::LoadMessage( BFile *file )
{
	char		dateStr[129] = "Unknown";
	char		theString[257] = "";
	int32		headerLen;
	attr_info	aInfo;

	//
	//	Note: I didn't delete the old file because it's not mine to delete.
	//	It was handed to me by TMailWindow, who retains responsibility for
	//	deleting it.
	//
	fFile = file;

	//
	//	Read the header
	//
	headerLen = header_len(fFile);
	char *header = (char *)malloc(headerLen + 1);
	fFile->Seek(0, SEEK_SET);
	fFile->Read(header, headerLen);
	header[headerLen] = '\0';

	//
	//	Set the date on this message
	//
	const char *dateHeader = strstr(header, "Date: ");
	if (dateHeader != NULL) {
		dateHeader += strlen("Date: ");

		int32 i = 0;
		while ((dateHeader[i] >= 32) || (dateHeader[i] == '\t'))
			i++;
		i = (i > 128) ? 128 : i;				

		memcpy(dateStr, dateHeader, i);
		dateStr[i] = '\0';
	}

	free(header);
	sprintf(theString, "%s  %s", kDateLabel, dateStr);
	fDate->SetText(theString);


	//	
	//	Set contents of header fields
	//
	if ((fIncoming) && (!fResending)) {
		if ( fBcc != NULL )
			fBcc->SetEnabled(false);

		if ( fCc != NULL )
			fCc->SetEnabled(false);

		fSubject->SetEnabled(false);
		fTo->SetEnabled(false);
	}

	//
	//	Set Subject
	//
	char *string = NULL;
	attr_info info;

	if ((fFile->GetAttrInfo(B_MAIL_ATTR_SUBJECT, &info) == B_NO_ERROR) &&
	 (info.size > 0 )) 
	{
		string = (char*) malloc(info.size);
		fFile->ReadAttr(B_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, string, 
		  info.size);
		fSubject->SetText(string);
		free(string);
	}
	else
		fSubject->SetText("");


	//
	//	Set From/To Field
	//
	if ((fFile->GetAttrInfo(B_MAIL_ATTR_FROM, &info) == B_NO_ERROR) &&
	  (info.size > 0)) 
	{
		string = (char*) malloc(info.size);
		fFile->ReadAttr(B_MAIL_ATTR_FROM, B_STRING_TYPE, 0, string, info.size);
		fTo->SetText(string);
		free(string);
	}
	else
		fTo->SetText("");

	return B_OK;
}

//====================================================================

TTextControl::TTextControl(BRect rect, char *label, BMessage *msg, 
	bool incoming, bool resending, int32 resizingMode)
	:BComboBox(rect, "happy", label, msg, resizingMode)
	//:BTextControl(rect, "happy", label, "", msg, resizingMode)
{
	strcpy(fLabel, label);
	fCommand = msg->what;
	fIncoming = incoming;
	fResending = resending;
}

//--------------------------------------------------------------------

void TTextControl::AttachedToWindow()
{
	BFont font = *be_plain_font;
	BTextView *text;

	SetHighColor(0, 0, 0);
	// BTextControl::AttachedToWindow();
	BComboBox::AttachedToWindow();
	font.SetSize(FONT_SIZE);
	SetFont(&font);

	SetDivider(StringWidth(fLabel) + 6);
	text = (BTextView *)ChildAt(0);
	text->SetFont(&font);
}

//--------------------------------------------------------------------

void TTextControl::MessageReceived(BMessage *msg)
{
	bool		enclosure = false;
	char		type[B_FILE_NAME_LENGTH];
	char		separator[4] = ", ";
	int32		index = 0;
	int32		len;
	entry_ref	ref;
	BFile		file;
	BMessage	message(REFS_RECEIVED);
	BNodeInfo	*node;
	BTextView	*text_view;
	attr_info	info;

	switch (msg->what) {
		case B_SIMPLE_DATA:
			if ((!fIncoming) || (fResending)) {
				while (msg->FindRef("refs", index++, &ref) == B_NO_ERROR) {
					file.SetTo(&ref, O_RDONLY);
					if (file.InitCheck() == B_NO_ERROR) {
						node = new BNodeInfo(&file);
						node->GetType(type);
						delete node;

/****/
						if ((fCommand != SUBJECT_FIELD) && (!strcmp(type, "application/x-person"))) {
							if (file.GetAttrInfo("META:email", &info) == B_NO_ERROR) {
								BString email;
								file.ReadAttr("META:email", B_STRING_TYPE, 0, email.LockBuffer(info.size), info.size);
								email.UnlockBuffer();
							
							/* we got something... */	
							if (email.Length() > 0) {
								BString name;
								/* see if we can get a username as well */
								if(file.GetAttrInfo("META:name", &info) == B_NO_ERROR) {
									file.ReadAttr("META:name", B_STRING_TYPE, 0, name.LockBuffer(info.size), info.size);
									name.UnlockBuffer();
								}
									
								BString	address;
								/* if we have no Name, just use the email address */
								if (name.Length() == 0) {
									address = email;
								/* otherwise, pretty-format it */
								} else {
									address.SetTo("") << "\"" << name << "\" <" << email << ">";
								}
								
								if (int end = TextView()->TextLength()) {
									TextView()->Select(end, end);
									TextView()->Insert(", ");
								}	
								TextView()->Insert(address.String());
							}
						}
					} else {
						enclosure = true;
						message.AddRef("refs", &ref);
					}
				}
			}
			if (enclosure)
				Window()->PostMessage(&message, Window());
		}
		break;

		case M_SELECT:
			text_view = (BTextView *)ChildAt(0);
			text_view->Select(0, text_view->TextLength());
			break;

		default:
			// BTextControl::MessageReceived(msg);
			BComboBox::MessageReceived(msg);
	}
}

// ***
// QPopupMenu
// ***

QPopupMenu::QPopupMenu( const char *title )
	: QueryMenu( title, true ),
	fGroups( 0 )
{
	
}

void QPopupMenu::EntryCreated( const entry_ref &ref, ino_t node )
{
	BNode			file;
	
	if( file.SetTo( &ref ) == B_OK )
	{
		BMessage 		*msg;
		BMenuItem 		*item;
		attr_info 		info;
		BMenu			*groupMenu = NULL;
		BMenuItem		*superItem;
		int32 			items = CountItems();
		
		if( !items )
			AddSeparatorItem();
		
		// Does the file have a group attribute
		if( ((file.GetAttrInfo( "META:group", &info ) == B_NO_ERROR))&&(info.size > 1) )
		{
			char 			*str;
			BMenu			*sub;
			
			str = (char*)malloc(info.size+1);
			str[info.size] = 0;
			file.ReadAttr( "META:group", B_STRING_TYPE, 0, str, info.size );
			
			// Look for submenu with label == group name
			int32 i;
			for( i=0; i<items; i++ )
			{
				if( (sub=SubmenuAt(i)) )
				{
					superItem = sub->Superitem();
					if( strcmp( superItem->Label(), str ) == 0 )
					{
						groupMenu = sub;
						i++;
						break;
					}
				}
			}
			
			// If no submenu, create one
			if( !groupMenu )
			{
				// Find where it should go (alphabetical)
				int32 mindex = 0;
				for( ; mindex<fGroups; mindex++ )
				{
					if( strcmp( ItemAt(mindex)->Label(), str )>0 )
						break;
				}
				
				groupMenu = new BMenu( str );
				groupMenu ->SetFont(be_plain_font);
				AddItem( groupMenu, mindex );
				superItem = groupMenu->Superitem();
				superItem->SetMessage( new BMessage( B_SIMPLE_DATA ) );
				if( fTargetHandler )
					superItem->SetTarget( fTargetHandler );
				fGroups++;
			}
			free( str );
		}
		
		msg = new BMessage( B_SIMPLE_DATA );
		msg->AddRef( "refs", &ref );
		msg->AddInt64( "node", node );
		
		BString	name;
		FetchAttribute("META:name", file, name);
		BString email;
		FetchAttribute("META:email", file, email);
		BString label;
		
		// if we have no Name, just use the email address 
		if (name.Length() == 0)
			label = email;
		else // otherwise, pretty-format it 
			label.SetTo("") << name << " (" << email << ")";
			
		item = new BMenuItem( label.String(), msg );
		if( fTargetHandler )
			item->SetTarget( fTargetHandler );
		
		// If no group, just add it; else add it to group menu
		if( !groupMenu )
			AddItem( item );
		else
		{
			groupMenu->AddItem( item );
			// Add ref to group superitem
			BMessage *smsg;
			smsg = superItem->Message();
			smsg->AddRef( "refs", &ref );
		}
	}
}

void QPopupMenu::EntryRemoved( ino_t node )
{
	
}

