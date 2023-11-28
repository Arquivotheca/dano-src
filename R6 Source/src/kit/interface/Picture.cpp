//******************************************************************************
//
//	File:		Picture.cpp
//
//	Description:	implementation of the Picture class
//	
//	Written by:	Benoit Schillings and Peter Potrebic
//
//	Copyright 1992-96, Be Incorporated
//
//******************************************************************************/
 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Debug.h>

#ifndef _PICTURE_H
#include "Picture.h"
#endif
#ifndef PICTURE_TRANSLATION_H
#include "PictureTranslation.h"
#endif
#ifndef _RECT_H
#include "Rect.h"
#endif

// ---- private includes ---
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _MESSAGES_H
#include <messages.h>
#endif
#ifndef _TOKEN_SPACE_H
#include <token.h>
#endif
#ifndef _SESSION_H
#include <session.h>
#endif
#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
#ifndef _INTERFACE_MISC_H
#include <interface_misc.h>
#endif
// ---- end private includes ---
#ifndef _APPLICATION_H
#include <Application.h>
#endif

#include <ByteOrder.h>

/*----------------------------------------------------------------*/

bool BPicture::assert_server_copy()
{
	PRINT(("BPicture::assert_server_copy\n"));

	if (fToken!=NO_TOKEN) return true;
	if (!fExtent->data) return false;

	for (int32 i=0;i<fExtent->pictureLib.CountItems();i++)
		fExtent->pictureLib[i]->assert_server_copy();

	_BAppServerLink_	link;
	link.session->swrite_l(GR_IMPORT_PICTURE);
	link.session->swrite_l(fExtent->pictureLib.CountItems());
	for (int32 i=0;i<fExtent->pictureLib.CountItems();i++)
		link.session->swrite_l(fExtent->pictureLib[i]->fToken);
	link.session->swrite_l(fExtent->size);
	link.session->swrite(fExtent->size,(void*)fExtent->data);
	link.session->flush();

	link.session->sread(4, &fToken);
	PRINT(("BPicture::assert_server_copy done\n"));
	return (fToken != NO_TOKEN);
};

/*----------------------------------------------------------------*/

bool BPicture::assert_local_copy()
{
	PRINT(("BPicture::assert_local_copy(%d) %08x %d\n",fToken,fExtent->data,NO_TOKEN));
	int32 pictureLibSize;

	if (fExtent->data) return true;
	if (fToken==NO_TOKEN) return false;
	
	_BAppServerLink_	link;

	link.session->swrite_l(GR_GET_PICTURE_DATA);
	link.session->swrite_l(fToken);
	link.session->flush();
	link.session->sread(4,&pictureLibSize);
	while (pictureLibSize-- > 0) {
		BPicture *subpic = new BPicture();
		link.session->sread(4,&subpic->fToken);
		fExtent->pictureLib.AddItem(subpic);
	};
	link.session->sread(4,&fExtent->size);
	if (fExtent->size < 0) {
		PRINT(("fExtent->size = %d\n",fExtent->size));
		return false;
	};
	fExtent->data = malloc(fExtent->size);
	link.session->sread(fExtent->size,fExtent->data);
	
	PRINT(("BPicture::assert_local_copy done\n"));
	return true;
};

/*----------------------------------------------------------------*/

bool BPicture::assert_old_local_copy()
{
	PRINT(("BPicture::assert_old_local_copy\n"));
	if (fExtent->oldData) return true;
	if (!assert_local_copy()) return false;
	convert_new_to_old(fExtent->data,fExtent->size,&fExtent->oldData,&fExtent->oldSize);
	PRINT(("BPicture::assert_old_local_copy done\n"));
	return true;
};

/*----------------------------------------------------------------*/

BPicture::BPicture()
{	
	PRINT(("BPicture::BPicture()\n"));
	fToken = NO_TOKEN;
	fUsurped = NULL;
	fDrawing = NULL;
	fExtent = new _BPictureExtent_;
	fExtent->data = fExtent->oldData = NULL;
	fExtent->size = fExtent->oldSize = 0;
	PRINT(("BPicture::BPicture() done\n"));
}

/*----------------------------------------------------------------*/

BPicture::BPicture(const BPicture &original)
	:	BArchivable()
{
	PRINT(("BPicture::BPicture(BPicture)\n"));

	init_data();

	if (original.fToken!=NO_TOKEN) {
		PRINT(("copying on server\n"));
		_BAppServerLink_	link;
		link.session->swrite_l(GR_CLONE_PICTURE);
		link.session->swrite_l(original.fToken);
		link.session->flush();
		link.session->sread(4,&fToken);
	} else if (original.fExtent->data) {
		PRINT(("copying local\n"));
		fExtent->data = malloc(original.fExtent->size);
		fExtent->size = original.fExtent->size;
		memcpy(fExtent->data,original.fExtent->data,fExtent->size);
		for (int32 i=0;i<original.fExtent->pictureLib.CountItems();i++)
			fExtent->pictureLib.AddItem(new BPicture(*original.fExtent->pictureLib[i]));
	} else if  (original.fExtent->oldData) {
		PRINT(("copying local old\n"));
		fExtent->oldData = malloc(original.fExtent->oldSize);
		fExtent->oldSize = original.fExtent->oldSize;
		memcpy(fExtent->oldData,original.fExtent->oldData,fExtent->oldSize);
	};
	PRINT(("BPicture::BPicture(BPicture) done\n"));
}

/*----------------------------------------------------------------*/

status_t BPicture::Play(void **callBackTable, int32 tableEntries, void *userData)
{
	assert_local_copy();
	return do_playback(fExtent->data,fExtent->size,fExtent->pictureLib,callBackTable,tableEntries,userData);
};

/*----------------------------------------------------------------*/

void BPicture::import_data(const void *data, int32 size, BPicture **subs, int32 subCount)
{
	PRINT(("BPicture::import_data(0x%08x, %d)\n",data,size));

	if (!data || !size) return;

	_BAppServerLink_	link;

	link.session->swrite_l(GR_IMPORT_PICTURE);
	link.session->swrite_l(subCount);
	for (int32 i=0;i<subCount;i++)
		link.session->swrite_l(subs[i]->fToken);
	link.session->swrite_l(size);
	PRINT(("BPicture::sending data\n"));
	link.session->swrite(size, const_cast<void *>(data));
		// OK to cast away constnes, swrite should be const in the first
		// place
	PRINT(("BPicture::sent data, syncing\n"));
	link.session->flush();
	PRINT(("BPicture::synced, reading token\n"));
	link.session->sread(4, &fToken);
	PRINT(("BPicture::import_data done\n"));
}

/*----------------------------------------------------------------*/

void BPicture::import_old_data(const void *data, int32 size)
{
	PRINT(("BPicture::import_old_data %08x %d\n",data,size));

	if (!data || !size) return;

	void *newData=NULL;
	int32 newSize;
	convert_old_to_new(const_cast<void *>(data),size,&newData,&newSize);
		// OK to cast away constness, convert_old_to_new should have
		// const void * as first param
	
	if (newData) {
		import_data(newData,newSize,NULL,0);
		free(newData);
	}

	PRINT(("BPicture::import_old_data done\n"));
}

/* ---------------------------------------------------------------- */

BPicture::BPicture(BMessage *data) : BArchivable(data)
{
	BMessage			subpicArchive;
	BArray<BPicture*>	piclib;
	int32				size,version;
	int8				endianess;
	const void *		origPtr;
	const void *		ptr;

	PRINT(("BPicture::BPicture(BMessage *)\n"));

	init_data();

	if (data->FindInt32(S_VERSION,&version) != B_OK) version = 0;
	if (data->FindInt8(S_ENDIANESS,&endianess) != B_OK) endianess = B_HOST_IS_BENDIAN;
	data->FindData(S_DATA, B_RAW_TYPE, &origPtr, &size);
	ptr = origPtr;
	
	void *newData = 0;
	if ((endianess != B_HOST_IS_BENDIAN) && (version != 0)) {
		newData = malloc(size);
		memcpy(newData,origPtr,size);
		swap_data(newData,size);
		ptr = newData;
	}

	int32 index = 0;
	while (data->FindMessage("piclib",index++,&subpicArchive) == B_OK)
		piclib.AddItem(new BPicture(&subpicArchive));

	switch (version) {
		case 0:
			PRINT(("BPicture importing old format\n"));
			import_old_data(ptr,size);
			break;
		case 1:
			PRINT(("BPicture importing new format\n"));
			import_data(ptr,size,piclib.Items(),piclib.CountItems());
			break;
	}

	for (index=0;index<piclib.CountItems();index++) delete piclib[index];
	if (newData) free(newData);

	PRINT(("BPicture::BPicture(BMessage *) done\n"));
}

/* ---------------------------------------------------------------- */

status_t BPicture::Flatten(BDataIO *stream)
{
	int32 count,version=2;
	int32 endianess = B_HOST_IS_BENDIAN;
	status_t err=B_OK;
	if (!assert_local_copy()) return B_ERROR;

	stream->Write(&version, 4);
	stream->Write(&endianess, 4);
	count = fExtent->pictureLib.CountItems();
	stream->Write(&count, 4);

	PRINT(("BPicture(%d)::Flatten %d %d %d\n",fToken,version,endianess,count));

	for (int32 i=0;i<count;i++) {
		err = fExtent->pictureLib[i]->Flatten(stream);
		if (err != B_OK) return err;
	};
	stream->Write(&fExtent->size, sizeof(fExtent->size));

	PRINT(("BPicture(%d)::Flatten2 %d\n",fToken,fExtent->size));

	stream->Write(fExtent->data, fExtent->size);
	return err;
}

/* ---------------------------------------------------------------- */

status_t BPicture::Unflatten(BDataIO *stream)
{
	BArray<BPicture*> piclib;
	int32 count,size,version;
	int32 endianess;
	status_t err=B_OK;
	void *newData;

	stream->Read(&version, 4);
	stream->Read(&endianess, 4);
	stream->Read(&count, 4);

	PRINT(("BPicture::Unflatten %d %d %d\n",version,endianess,count));

	while (count--) {
		BPicture *subpic = new BPicture();
		err = subpic->Unflatten(stream);
		if (err != B_OK) goto deleteEm;
		piclib.AddItem(subpic);
	};

	stream->Read(&size, sizeof(size));
	PRINT(("BPicture::Unflatten2 %d\n",size));
	newData = malloc(size);

	if (newData) {
		stream->Read(newData, size);
		if (endianess != B_HOST_IS_BENDIAN) swap_data(newData,size);
		import_data(newData,size,piclib.Items(),piclib.CountItems());
		free(newData);
	};

	deleteEm:
	for (int32 index=0;index<piclib.CountItems();index++) delete piclib[index];
	return err;
}

/* ---------------------------------------------------------------- */

status_t BPicture::Archive(BMessage *data, bool deep) const
{
	PRINT(("BPicture::Archive\n"));

	BPicture *p = const_cast<BPicture*>(this);

	if (!p->assert_local_copy())
		return B_ERROR;

	BArchivable::Archive(data, deep);
	data->AddInt32(S_VERSION, 1);
	data->AddInt8(S_ENDIANESS, B_HOST_IS_BENDIAN);
	data->AddData(S_DATA, B_RAW_TYPE, fExtent->data, fExtent->size);
	for (int32 index=0;index<fExtent->pictureLib.CountItems();index++) {
		BMessage subpic;
		fExtent->pictureLib[index]->Archive(&subpic,deep);
		data->AddMessage("piclib",&subpic);
	};
	PRINT(("BPicture::Archive done\n"));
	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *BPicture::Instantiate(BMessage *data)
{
	PRINT(("BPicture::Instantiate\n"));

	if (!validate_instantiation(data, "BPicture"))
		return NULL;
	PRINT(("BPicture::Instantiate done\n"));
	return new BPicture(data);
}

/*----------------------------------------------------------------*/

void BPicture::init_data()
{
	fToken = NO_TOKEN;
	fUsurped = NULL;
	fDrawing = NULL;
	fExtent = new _BPictureExtent_;
	fExtent->data = fExtent->oldData = NULL;
	fExtent->size = fExtent->oldSize = 0;
};

void BPicture::set_token(int32 _token)
{
	fToken = _token;
};

void BPicture::usurp(BPicture *lameDuck)
{
	if (fToken!=NO_TOKEN) {
		_BAppServerLink_	link;
		link.session->swrite_l(GR_DELETE_PICTURE);
		link.session->swrite_l(fToken);
	};
	if (fExtent->data) free(fExtent->data);
	if (fExtent->oldData) free(fExtent->oldData);
	for (int32 i=0;i<fExtent->pictureLib.CountItems();i++)
		delete fExtent->pictureLib[i];
	delete fExtent;
	init_data();
	fUsurped = lameDuck;
};

BPicture * BPicture::step_down()
{
	BPicture *p = fUsurped;
	fUsurped = NULL;
	return p;
};

/*----------------------------------------------------------------*/

BPicture::~BPicture()
{
	PRINT(("BPicture::~BPicture\n"));
	
	if (fDrawing!=NULL) {
		debugger("Can't delete picture while drawing into it");
	}
	
	if (fToken!=NO_TOKEN) {
		_BAppServerLink_	link;
		link.session->swrite_l(GR_DELETE_PICTURE);
		link.session->swrite_l(fToken);
	};
	if (fExtent->data) free(fExtent->data);
	if (fExtent->oldData) free(fExtent->oldData);
	for (int32 i=0;i<fExtent->pictureLib.CountItems();i++)
		delete fExtent->pictureLib[i];
	delete fExtent;

	PRINT(("BPicture::~BPicture done\n"));
}

/*----------------------------------------------------------------*/
/*	Deprecated functions to access data directly.  These
	must convert the picture to/from the old format. */

BPicture::BPicture(const void *data, int32 size)
{
	PRINT(("BPicture::BPicture(%08x,%d)\n",data,size));
	init_data();
	import_old_data(data, size);
	PRINT(("BPicture::BPicture(%08x,%d) done\n",data,size));
}

int32 BPicture::DataSize() const
{
	PRINT(("BPicture::DataSize()\n"));
	const_cast<BPicture*>(this)->assert_old_local_copy();
	PRINT(("BPicture::DataSize() done\n"));
	return(fExtent->oldSize);
}

const void * BPicture::Data() const
{
	PRINT(("BPicture::Data()\n"));
	const_cast<BPicture*>(this)->assert_old_local_copy();
	PRINT(("BPicture::Data() done\n"));
	return(fExtent->oldData);
}

/*----------------------------------------------------------------*/

void BPrivate::IKAccess::SetPictureToken(BPicture* picture, int32 token)
{
	picture->fToken = token;
}

int32 BPrivate::IKAccess::PictureToken(const BPicture* picture)
{
	return picture->fToken;
}

bool BPrivate::IKAccess::UsurpPicture(BPicture* picture, BPicture* previous,
									  bool append)
{
	if (picture->fUsurped) return false;
	if (append) {
		picture->fUsurped = previous;
		picture->fToken = NO_TOKEN;
	} else {
		picture->usurp(previous);
	}
	return true;
}

BPicture* BPrivate::IKAccess::StepDownPicture(BPicture* picture)
{
	return picture->step_down();
}

/*----------------------------------------------------------------*/

BPicture &BPicture::operator=(const BPicture &) { return *this; }

/*----------------------------------------------------------------*/

status_t BPicture::Perform(perform_code d, void *arg)
{
	return BArchivable::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BPicture::_ReservedPicture1() {}
void BPicture::_ReservedPicture2() {}
void BPicture::_ReservedPicture3() {}

/* ---------------------------------------------------------------- */
