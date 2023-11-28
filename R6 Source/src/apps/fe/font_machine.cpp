#include "main.h"
#include "twindow.h"
#include "font_machine.h"
#include <Alert.h>
#include <stdio.h>
#include <string.h>
#include <Debug.h>
#include <malloc.h>
#include <Screen.h>
#include <Window.h>

typedef struct {
	uint32       total_length;
	uint16       family_length;
	uint16       style_length;
	float        rotation;
	float        shear;
	uint32       hmask;
	uint16       size;
	uint8        bpp;
	uint8        version;
	uint32       _reserved_[2];
} fc_tuned_file_header; 
 
//----------------------------------------------------
		
FontMachine::FontMachine() {
	fdirty = false;
	buffer = 0L;
	list_code = 0L;
	list_char = 0L;
	count_char = 0;
}

//----------------------------------------------------

FontMachine::~FontMachine() {
	if (fdirty) {
		if ((new BAlert("", "Save changes?", "Cancel", "Save"))->Go()) {
			_sPrintf("Save last font...\n");
			SaveLastFont();
		}
	}
}

//----------------------------------------------------

fc_char *FontMachine::GetChar(uint32 char_num) {
	int          char_offset, index;
	fc_hset_item *fitem;
	
	index = ((((char_num&0xffff)<<3)^((char_num&0xffff)>>2))+(char_num>>16))&hmask;
	while (TRUE) {
		fitem = ((fc_hset_item*)(buffer+offset))+index;
		char_offset = B_BENDIAN_TO_HOST_INT32(fitem->offset);
		if (char_offset <= 0)
			return 0L;

		if (((B_BENDIAN_TO_HOST_INT16(fitem->code[0])) == (char_num&0xffff)) &&
			((B_BENDIAN_TO_HOST_INT16(fitem->code[1])) == (char_num>>16)))
			return (fc_char*)(buffer+char_offset);
		index = (index+1)&hmask;
	}
}

//----------------------------------------------------

void FontMachine::RestoreBuffer(uint32 char_num) {
	int       dx, dy, size;
	fc_char   *ch;

	ch = GetChar(list_code[char_num]);
	free(list_char[char_num]);
	dx = B_BENDIAN_TO_HOST_INT16(ch->bbox.right)-B_BENDIAN_TO_HOST_INT16(ch->bbox.left)+1;
	dy = B_BENDIAN_TO_HOST_INT16(ch->bbox.bottom)-B_BENDIAN_TO_HOST_INT16(ch->bbox.top)+1;
	size = ((dx+1)>>1)*dy;
	if (size < 0) size = 0;
	list_char[char_num] = (fc_char*)malloc(size+CHAR_HEADER_SIZE);
	memcpy(list_char[char_num], ch, size+CHAR_HEADER_SIZE);	
	SwapCharFromBig(list_char[char_num]);
}

//----------------------------------------------------

fc_char *FontMachine::GetCharBits(uint32 char_num) {
	if (char_num < count_char)
		return list_char[char_num];
	else
		return 0L;
}

//----------------------------------------------------

void FontMachine::SetCharBits(uint32 char_num, fc_char *ch) {
	int       dx, dy, size;

	if (char_num >= count_char)
		return;
	free(list_char[char_num]);
	dx = ch->bbox.right-ch->bbox.left+1;
	dy = ch->bbox.bottom-ch->bbox.top+1;
	size = ((dx+1)>>1)*dy;
	if (size < 0) size = 0;
	list_char[char_num] = (fc_char*)malloc(size+CHAR_HEADER_SIZE);
	memcpy(list_char[char_num], ch, size+CHAR_HEADER_SIZE);	
}

//----------------------------------------------------

int	FontMachine::LoadNewFont(char *path) {
	int                  i, family_length, style_length, char_offset, index;
	int 	             dx, dy, size, cur_length;
	char                 *names;
	char                 *cur_buf;
	FILE                 *fp;
	fc_char              *ch;
	fc_hset_item         *fitem;
	fc_tuned_file_header *tfh;
	fc_tuned_file_header t_header;

	/* close and save the previous opened file */
	if (fdirty)
	SaveLastFont();
	/* open the new file */
	memcpy(pathname, path, PATH_MAX);
	printf("opening %s\n", pathname);
	fp = fopen(pathname, "rb");
	if (fp == NULL)
		return -1;
	fseek(fp, 4, SEEK_SET);
	fread(&t_header, sizeof(fc_tuned_file_header), 1, fp);
	length = B_BENDIAN_TO_HOST_INT32(t_header.total_length);
	buffer = (char*)malloc(length);
	fseek(fp, 0, SEEK_SET);
	cur_buf = buffer;
	cur_length = length;
	while (cur_length > 128*1024) {
		fread(cur_buf, 1, 128*1024, fp);
		cur_buf += 128*1024;
		cur_length -= 128*1024;
	}
	fread(cur_buf, 1, cur_length, fp);
	/* extract information from the file header */
	tfh = (fc_tuned_file_header*)(buffer+4);
	family_length = B_BENDIAN_TO_HOST_INT16(tfh->family_length);
	style_length = B_BENDIAN_TO_HOST_INT16(tfh->style_length);
	if ((family_length < 0) || (family_length > 63) ||
		(style_length < 0) || (style_length > 63))
		goto exit_false;
	/* read parameters of the header */
 	hmask = B_BENDIAN_TO_HOST_INT32(tfh->hmask);
	shear = B_BENDIAN_TO_HOST_FLOAT(tfh->shear);
	rotation = B_BENDIAN_TO_HOST_FLOAT(tfh->rotation);
	if (((hmask&(hmask+1)) != 0) || (hmask < 3))
		goto exit_false;
	size = B_BENDIAN_TO_HOST_INT16(tfh->size);
	if ((size <= 1) || (size > 10000))
		goto exit_false;
	if (tfh->version != 0)
		goto exit_false;
	/* read family and style names */
	length = family_length+style_length+2;
	names = (char*)buffer+(4+sizeof(fc_tuned_file_header));
	/* the font is okay. Register it */
	offset = 4+sizeof(fc_tuned_file_header)+length;
	/* extract a copy of each character */
	count_char = 0;
	for (index=0; index<=hmask; index++) {
		fitem = ((fc_hset_item*)(buffer+offset))+index;
		if (B_BENDIAN_TO_HOST_INT32(fitem->offset) > 0)
			count_char++;
	}
	list_code = (uint32*)malloc(sizeof(uint32)*count_char);
	list_char = (fc_char**)malloc(sizeof(fc_char*)*count_char);
	count_char = 0;
	for (index=0; index<0x10000; index++) {
		ch = GetChar(index);
		if (ch != 0L) {
			dx = (int16)B_BENDIAN_TO_HOST_INT16(ch->bbox.right)-(int16)B_BENDIAN_TO_HOST_INT16(ch->bbox.left)+1;
			dy = (int16)B_BENDIAN_TO_HOST_INT16(ch->bbox.bottom)-(int16)B_BENDIAN_TO_HOST_INT16(ch->bbox.top)+1;
			size = ((dx+1)>>1)*dy;
			if (size < 0) size = 0;
			list_char[count_char] = (fc_char*)malloc(size+CHAR_HEADER_SIZE);
			memcpy(list_char[count_char], ch, size+CHAR_HEADER_SIZE);
			SwapCharFromBig(list_char[count_char]);
			list_code[count_char] = index;
			count_char++;
		}
	}

	for (i=0; i<32; i++) {
		((TApplication*)be_app)->samp_view->add_test_char(i, (i == 31));
		if (i >= count_char) break;
	}

	fclose(fp);
	return TRUE;

	/* bad file */
 exit_false:
 	count_char = 0;
	fclose(fp);
	return FALSE;
}

//------------------------------------------------------------------

void FontMachine::Apply() {
	BFont		font;
	BWindow		*w;

	Save();
	_font_control_(&((TApplication*)be_app)->aWindow->SelectFont->font, FC_FLUSH_FONT_SET, 0L);
	_font_control_(&font, FC_RESCAN_ALL_FILES, 0L);

	w = new TWindow(BScreen(((TApplication*)be_app)->aWindow).Frame());
	w->Show();
	w->Hide();
	w->Close();
}

//------------------------------------------------------------------

void FontMachine::Save() {
	int          i, dx, dy, size, cur_length;
	int          index, key, code, char_buf;
	char         *new_buffer;
	char         *cur_buf;
	FILE         *fp;
	BFont        font;
	fc_char      *ch;
	fc_hset_item *set;

	if ((list_char == 0L) || (count_char == 0))
		return;

	/* calculate the new total size of the character buffer */
	length = offset + (hmask+1)*sizeof(fc_hset_item);
	for (i=0; i<count_char; i++) {
		ch = list_char[i];
		dx = ch->bbox.right-ch->bbox.left+1;
		dy = ch->bbox.bottom-ch->bbox.top+1;
		size = ((dx+1)>>1)*dy;
		if (size < 0) size = 0;
		length += CHAR_HEADER_SIZE+size;
	}

	/* free old buffer, and allocate a new one (just the good size) */
	new_buffer = (char*)malloc(length);
	memcpy(new_buffer, buffer, offset);
	((fc_tuned_file_header*)(new_buffer+4))->total_length = B_HOST_TO_BENDIAN_INT32(length);
	free(buffer);
	buffer = new_buffer;
	/* register the character, and the character hset_item in the hash table */
	set = (fc_hset_item*)(buffer+offset);
	char_buf = offset+(hmask+1)*sizeof(fc_hset_item);
	for (i=0; i<=hmask; i++)
		set[i].offset = B_HOST_TO_BENDIAN_INT32(-1);

	for (i=0; i<count_char; i++) {
		ch = list_char[i];
		dx = ch->bbox.right-ch->bbox.left+1;
		dy = ch->bbox.bottom-ch->bbox.top+1;
		size = ((dx+1)>>1)*dy;
		if (size < 0) size = 0;
		size += CHAR_HEADER_SIZE;

		code = list_code[i];
		key = (code<<3)^(code>>2);
		index = key & hmask;
		while ((int32)B_BENDIAN_TO_HOST_INT32(set[index].offset) >= 0)
			index = (index+1)&hmask;
		SwapCharToBig(ch);
		memcpy(buffer+char_buf, ch, size);
		SwapCharFromBig(ch);
		set[index].offset = B_HOST_TO_BENDIAN_INT32(char_buf);
		set[index].code[0] = B_HOST_TO_BENDIAN_INT16(code); 
		set[index].code[1] = B_HOST_TO_BENDIAN_INT16(0); 
		char_buf += size;
	}

	/* save the new tuned font file */
	for (i=0; i<4; i++) {
		_font_control_(&font, FC_CLOSE_ALL_FONT_FILES, 0L);
		fp = fopen(pathname, "wb");
		if (fp != 0L) break;
	}
	if (fp == 0L)
		return;
	fseek(fp, 0, SEEK_SET);
	cur_buf = buffer;
	cur_length = length;
	while (cur_length > 128*1024) {
		fwrite(cur_buf, 1, 128*1024, fp);
		cur_buf += 128*1024;
		cur_length -= 128*1024;
	}
	fwrite(cur_buf, 1, cur_length, fp);
	/* flush the font set corresponding to that file
	   (to allow a proper update of the font cache. */
	_font_control_(&((TApplication*)be_app)->aWindow->SelectFont->font, FC_FLUSH_FONT_SET, 0L);
	fclose(fp);
}

void FontMachine::SaveLastFont() {
	int     i;

	Save();
	if (buffer != 0L) {
		free(buffer);
		buffer = 0L;
	}
	if (list_char != 0L) {
		for (i=0; i<count_char; i++)
			free(list_char[i]);
		free(list_char);
		free(list_code);
		list_code = 0L;
		list_char = 0L;
	}
}


void
FontMachine::SwapCharFromBig(
	fc_char	*realCh)
{
	realCh->edge.left = B_BENDIAN_TO_HOST_FLOAT(realCh->edge.left);
	realCh->edge.right = B_BENDIAN_TO_HOST_FLOAT(realCh->edge.right);
	realCh->bbox.left = B_BENDIAN_TO_HOST_INT16(realCh->bbox.left);
	realCh->bbox.top = B_BENDIAN_TO_HOST_INT16(realCh->bbox.top);
	realCh->bbox.right = B_BENDIAN_TO_HOST_INT16(realCh->bbox.right);
	realCh->bbox.bottom = B_BENDIAN_TO_HOST_INT16(realCh->bbox.bottom);
	realCh->escape.x_escape = B_BENDIAN_TO_HOST_FLOAT(realCh->escape.x_escape);
	realCh->escape.y_escape = B_BENDIAN_TO_HOST_FLOAT(realCh->escape.y_escape);
}


void
FontMachine::SwapCharToBig(
	fc_char	*realCh)
{
	realCh->edge.left = B_HOST_TO_BENDIAN_FLOAT(realCh->edge.left);
	realCh->edge.right = B_HOST_TO_BENDIAN_FLOAT(realCh->edge.right);
	realCh->bbox.left = B_HOST_TO_BENDIAN_INT16(realCh->bbox.left);
	realCh->bbox.top = B_HOST_TO_BENDIAN_INT16(realCh->bbox.top);
	realCh->bbox.right = B_HOST_TO_BENDIAN_INT16(realCh->bbox.right);
	realCh->bbox.bottom = B_HOST_TO_BENDIAN_INT16(realCh->bbox.bottom);
	realCh->escape.x_escape = B_HOST_TO_BENDIAN_FLOAT(realCh->escape.x_escape);
	realCh->escape.y_escape = B_HOST_TO_BENDIAN_FLOAT(realCh->escape.y_escape);
}














