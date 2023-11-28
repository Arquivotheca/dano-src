#include "SavePanel.h"

#include <MenuItem.h>
#include <MenuBar.h>
#include <Window.h>

#include <string.h>

SavePanel::SavePanel(const format_def* formats,
							BMessenger *target,
							const entry_ref *start_directory,
							uint32 node_flavors,
							bool allow_multiple_selection,
							BMessage *message, 
							BRefFilter * filter,
							bool modal,
							bool hide_when_done)
	: BFilePanel(B_SAVE_PANEL, target, start_directory, node_flavors,
				 allow_multiple_selection, message, filter, modal,
				 hide_when_done),
	  fFormatMenu(0)
{
	// Add a menu to the file panel to allow the user to select
	// the file format to save in.
	BMenuBar *mb = dynamic_cast<BMenuBar *>(Window()->FindView("MenuBar"));
	if( formats && mb ) {
		fFormatMenu = new BMenu("File Format");
		fFormatMenu->SetRadioMode(true);
		for (const format_def* f = formats; f->name; f++ ) {
			BMessage* msg = new BMessage;
			msg->AddString("format_name", f->name);
			msg->AddString("format_signature", f->signature);
			msg->AddInt32("format_id", f->id);
			fFormatMenu->AddItem(new BMenuItem(f->name, msg));
		}
		mb->AddItem(fFormatMenu);
	}
}

SavePanel::~SavePanel()
{
}

void SavePanel::SendMessage(const BMessenger* target, BMessage* msg)
{
	BMessage* format = CurrentFormat();
	if( !format || !msg ) {
		inherited::SendMessage(target, msg);
	}
	
	BMessage update(*msg);
#if B_BEOS_VERSION_DANO
	const char* fieldName;
#else
	char* fieldName;
#endif
	type_code fieldType;
	int32 fieldCount;
	for( int32 i=0;
		 format->GetInfo(B_ANY_TYPE,i,&fieldName,&fieldType,&fieldCount) == B_OK;
		 i++ ) {
		for( int32 j=0; j<fieldCount; j++ ) {
			const void* fieldData=NULL;
			ssize_t fieldSize=0;
			if( format->FindData(fieldName, fieldType, j,
								 &fieldData, &fieldSize) == B_OK ) {
				update.AddData(fieldName, fieldType, fieldData, fieldSize);
			}
		}
	}
	
	inherited::SendMessage(target, &update);
}

void SavePanel::SetFormatName(const char* name)
{
	SetFormat(name, 0, -1, -1);
}

void SavePanel::SetFormatSignature(const char* signature)
{
	SetFormat(0, signature, -1, -1);
}


void SavePanel::SetFormatID(int32 id)
{
	SetFormat(0, 0, id, -1);
}


void SavePanel::SetFormatIndex(int32 idx)
{
	SetFormat(0, 0, -1, idx);
}


	
BMessage* SavePanel::CurrentFormat() const
{
	if( !fFormatMenu ) return 0;
	
	BMenuItem* item = fFormatMenu->FindMarked();
	if( !item ) return 0;
	
	return item->Message();
}
	
void SavePanel::SetFormat(const char* name, const char* signature,
						  int32 id, int32 idx)
{
	if( !fFormatMenu ) return;
	
	BMenuItem* item = 0;
	if( name ) item = fFormatMenu->FindItem(name);
	else if( idx >= 0 ) item = fFormatMenu->ItemAt(idx);
	else {
		for( int32 i=0; i<fFormatMenu->CountItems(); i++ ) {
			BMenuItem* cur = fFormatMenu->ItemAt(i);
			BMessage* msg = cur ? cur->Message() : 0;
			if( cur && msg ) {
				const char* str;
				if( msg->FindString("format_signature", &str) == B_OK ) {
					if( signature && strcmp(str, signature) == 0 ) {
						item = cur;
						break;
					}
				}
				int32 val;
				if( msg->FindInt32("format_id", &val) == B_OK ) {
					if( id >= 0 && val == id ) {
						item = cur;
						break;
					}
				}
			}
		}
	}
	
	if( item ) item->SetMarked(true);
}
