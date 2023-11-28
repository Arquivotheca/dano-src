/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include <ByteOrder.h>
#include <fs_attr.h>
#include <Resources.h>

#include "MessageWriter.h"

MessageWriter::MessageWriter(void)
{
	_Error = B_NO_INIT;
}


MessageWriter::~MessageWriter()
{
}

status_t MessageWriter::InitCheck()
{
	return(_Error);
}

/*****************************************************************************/

const char *MemoryMessageWriter :: _VendorClass = "Be/FlatMessageWriter";

MemoryMessageWriter :: MemoryMessageWriter(const char *name, int32 mode)
{	 
	_Error = B_OK;

	_fileName = name;
	
	_File = new BMallocIO();
	if(mode == B_WRITE_ONLY)
	{
		if((_Error = _File->Write(DATA_TYPE, strlen(DATA_TYPE)+1)) >= 0)
		{
			_Error = B_OK;
			
			int32 data = DATA_VERSION;
			swap_data(B_INT32_TYPE, &data, sizeof(int32), B_SWAP_HOST_TO_LENDIAN);
			_Error = _File->Write(&data, sizeof(int32));
			if(_Error >= 0) _Error = B_OK;
			if(!_Error){
				_Error = _File->Write(_VendorClass, strlen(_VendorClass)+1);
				_Error = B_OK;
			}
		}
		if(!_Error){
			_EndOfHeader = _File->Position();
		}
	}
	else
	{
		BFile file(name, B_READ_ONLY);
		
		const int32 BLOCK_SIZE = 65536;
		char *blockBuffer = new char[BLOCK_SIZE];
		
		ssize_t read;
		
		do
		{
			read = file.Read(blockBuffer, BLOCK_SIZE);
			
			if(read < BLOCK_SIZE)
				_File->Write(blockBuffer, read);
			else
				_File->Write(blockBuffer, BLOCK_SIZE);
				
		} while(read > 0);
		
		
		_EndOfHeader = strlen(DATA_TYPE) + strlen(_VendorClass) + sizeof(int32) + 2;	
		
		delete blockBuffer;
	}	
}

MemoryMessageWriter :: ~MemoryMessageWriter()
{
	BFile file(_fileName.String(), B_CREATE_FILE | B_WRITE_ONLY);
	file.RemoveAttr("BEOS:TYPE");
	file.WriteAttr("BEOS:TYPE", B_MIME_TYPE, 0, DATA_TYPE, sizeof(DATA_TYPE));
	
	//write in one shot so the flash will not fry.
	const char *buffer = (const char *)_File->Buffer();
	file.Write(buffer, _File->BufferLength());
	
	delete _File;
}

void MemoryMessageWriter :: ResetFilePointer(void)
{
	_File->Seek(_EndOfHeader, SEEK_SET);
}

status_t MemoryMessageWriter :: Write(BMessage *msg)
{
	if(_File->BufferLength() < MAX_MEMORY_USAGE)
		_Error = msg->Flatten(_File);
	else
		_Error = B_NO_MEMORY;
	
	if(_Error)
		fprintf(stderr, "%s:%d - Failed to Write Input: (0x%lx) %s\n",__FILE__,__LINE__, _Error, strerror(_Error));

	return _Error;
}


status_t MemoryMessageWriter :: Read(BMessage *msg)
{
	msg->MakeEmpty();
	msg->what = 0;
	_Error = msg->Unflatten(_File);
	if(_Error) fprintf(stderr, "%s:%d - Failed to Read Input Normally: (0x%lx) %s\n",__FILE__,__LINE__, _Error, strerror(_Error));

	return(_Error);
}	   


bool MemoryMessageWriter :: HaveMore(void)
{
	return (_File->Position() < _File->BufferLength());
}

/*****************************************************************************/
/*
const char *FlatMessageWriter :: _VendorClass = "Be/FlatMessageWriter";

FlatMessageWriter :: FlatMessageWriter(const char *name, int32 mode)
{	
	_File = new BFile(); 
	_Error = B_OK;

	//fprintf(stderr, "File: %s\n", name);

	int32 fmode = mode;
	if(mode & B_WRITE_ONLY) fmode |= B_CREATE_FILE | B_ERASE_FILE;
	if ((_Error = _File->SetTo(name, fmode )) == B_OK) {
		if(mode & B_WRITE_ONLY){
			_File->RemoveAttr("BEOS:TYPE");
			if((_Error = _File->Write(DATA_TYPE, strlen(DATA_TYPE)+1)) >= 0){
				_Error = B_OK;
				int32 data = 0;
				data = DATA_VERSION;
				swap_data(B_INT32_TYPE, &data, sizeof(int32), B_SWAP_HOST_TO_LENDIAN);
				_Error = _File->Write(&data, sizeof(int32));
				if(_Error >= 0) _Error = B_OK;
				if(!_Error){
					_Error = _File->Write(_VendorClass, strlen(_VendorClass)+1);
					if(_Error >= 0) {
						_Error = B_OK;
						_File->WriteAttr("BEOS:TYPE", B_MIME_TYPE, 0, DATA_TYPE, sizeof(DATA_TYPE));
					}
				}
			}
			if(!_Error){
				_EndOfHeader = _File->Position();
			}
		} else {
			char *buffer = new char[T_MAX_HEADER_SIZE];
			char *cp = NULL;
			int32 data = 0;
			bool dirtymime = false;
			if(_File->ReadAttr("BEOS:TYPE", B_MIME_TYPE, 0, buffer, T_MAX_HEADER_SIZE)<=0){
				dirtymime = true;
			}
			buffer[T_MAX_HEADER_SIZE-1] = '\0';
			if(!strcmp(buffer, "application/octet-stream")){
				dirtymime = true;
			} else if(!dirtymime && strcmp(buffer, DATA_TYPE)){
				_Error = B_BAD_TYPE;
				fprintf(stderr, "Attribute says file is not our type: (Read: '%s' Expected: '%s')\n", buffer, DATA_TYPE);
			}
			if(!_Error) {
				_File->Read(buffer, T_MAX_HEADER_SIZE-1);
				buffer[T_MAX_HEADER_SIZE-1] = '\0';
				if(memcmp(buffer, DATA_TYPE, strlen(DATA_TYPE)+1)){
					_Error = B_BAD_TYPE;
					fprintf(stderr, "File is not our type.\n");
				} else {
					cp = buffer + strlen(DATA_TYPE) +1;
					data = *((int32*)cp);
					swap_data(B_INT32_TYPE, &data, sizeof(int32), B_SWAP_LENDIAN_TO_HOST);
					cp+=sizeof(int32);
					if(data > DATA_VERSION){
						_Error = T_BAD_VERSION;
						fprintf(stderr, "Data is of later version: %ld\n", data);
					} else if(data == 1) {
						if(!memcmp(cp, _VendorClass, strlen(_VendorClass)+1)){
							cp += strlen(_VendorClass)+1;
						}
					} else if(data == 0) {
						data = *((int32*)cp);
						swap_data(B_INT32_TYPE, &data, sizeof(int32), B_SWAP_LENDIAN_TO_HOST);
						cp+=sizeof(int32);
						if(data != 2) { // once refered to as T_WRITE_NORMAL_FILES
							_Error = B_UNSUPPORTED;
							fprintf(stderr, "File is stored in unsupported format: %ld, we only support 2\n", data);
						} else if(dirtymime){
							_File->RemoveAttr("BEOS:TYPE");
							_File->WriteAttr("BEOS:TYPE", B_MIME_TYPE, 0, DATA_TYPE, sizeof(DATA_TYPE));
						}
					}
				}
			}
			if(!_Error){
				_EndOfHeader = (off_t)cp - (off_t)buffer;
			}
			delete buffer;
			_File->GetSize(&_LengthOfFile);
		}
	}
}

FlatMessageWriter :: ~FlatMessageWriter()
{
	delete _File;
}

void FlatMessageWriter :: ResetFilePointer(void)
{
	_File->Seek(_EndOfHeader, SEEK_SET);
}

status_t FlatMessageWriter :: Write(BMessage *msg)
{
	_Error = msg->Flatten(_File);
	if(_Error) fprintf(stderr, "%s:%d - Failed to Write Input: (0x%lx) %s\n",__FILE__,__LINE__, _Error, strerror(_Error));

	return(_Error);
}


status_t FlatMessageWriter :: Read(BMessage *msg)
{
	msg->MakeEmpty();
	msg->what = 0;
	_Error = msg->Unflatten(_File);
	if(_Error) fprintf(stderr, "%s:%d - Failed to Read Input Normally: (0x%lx) %s\n",__FILE__,__LINE__, _Error, strerror(_Error));

	return(_Error);
}	   


bool FlatMessageWriter :: HaveMore(void)
{
	return (_File->Position() < _LengthOfFile);
}


const char *AttributeMessageWriter :: _VendorClass = "Be/AttributeMessageWriter";

const char *AttributeMessageWriter :: _AttrBase = "InputMessage";

AttributeMessageWriter :: AttributeMessageWriter(const char *name, int32 mode)
{	
	_File = new BFile(); 
	_Error = B_OK;
	_Item = 0;

	//fprintf(stderr, "File: %s\n", name);

	int32 fmode = mode;
	if(mode & B_WRITE_ONLY) fmode |= B_CREATE_FILE | B_ERASE_FILE;
	if ((_Error = _File->SetTo(name, fmode )) == B_OK) {
		if(mode & B_WRITE_ONLY){
			_File->RemoveAttr("BEOS:TYPE");
			if((_Error = _File->Write(DATA_TYPE, strlen(DATA_TYPE)+1)) >= 0){
				_Error = B_OK;
				int32 data = 0;
				data = DATA_VERSION;
				swap_data(B_INT32_TYPE, &data, sizeof(int32), B_SWAP_HOST_TO_LENDIAN);
				_Error = _File->Write(&data, sizeof(int32));
				if(_Error >= 0) _Error = B_OK;
				if(!_Error){
					_Error = _File->Write(_VendorClass, strlen(_VendorClass)+1);
					if(_Error >= 0) {
						_Error = B_OK;
						_File->WriteAttr("BEOS:TYPE", B_MIME_TYPE, 0, DATA_TYPE, sizeof(DATA_TYPE));
					}
				}
			}
		} else {
			char *buffer = new char[T_MAX_HEADER_SIZE];
			int32 data = 0;
			bool dirtymime = false;
			if(_File->ReadAttr("BEOS:TYPE", B_MIME_TYPE, 0, buffer, T_MAX_HEADER_SIZE)<=0){
				dirtymime = true;
			}
			buffer[T_MAX_HEADER_SIZE-1] = '\0';
			if(!strcmp(buffer, "application/octet-stream")){
				dirtymime = true;
			} else if(!dirtymime && strcmp(buffer, DATA_TYPE)){
				_Error = B_BAD_TYPE;
				fprintf(stderr, "Attribute says file is not our type: (Read: '%s' Expected: '%s')\n", buffer, DATA_TYPE);
			}
			if(!_Error) {
				_File->Read(buffer, T_MAX_HEADER_SIZE-1);
				buffer[T_MAX_HEADER_SIZE-1] = '\0';
				if(memcmp(buffer, DATA_TYPE, strlen(DATA_TYPE)+1)){
					_Error = B_BAD_TYPE;
					fprintf(stderr, "File is not our type.\n");
				} else {
					const char *cp = buffer + strlen(DATA_TYPE) +1;
					data = *((int32*)cp);
					swap_data(B_INT32_TYPE, &data, sizeof(int32), B_SWAP_LENDIAN_TO_HOST);
					cp+=sizeof(int32);
					if(data > DATA_VERSION){
						_Error = T_BAD_VERSION;
						fprintf(stderr, "Data is of later version: %ld\n", data);
					} else if(data == 1) {
						if(!memcmp(cp, _VendorClass, strlen(_VendorClass)+1)){
							cp += strlen(_VendorClass)+1;
						}
					} else if(data == 0) {
						data = *((int32*)cp);
						swap_data(B_INT32_TYPE, &data, sizeof(int32), B_SWAP_LENDIAN_TO_HOST);
						cp+=sizeof(int32);
						if(data != 1) { // Once refered to as: T_WRITE_ATTRIBUTE_FILES
							_Error = B_UNSUPPORTED;
							fprintf(stderr, "File is stored in unsupported format: %ld, we only support 2\n", data);
						} else if(dirtymime){
							_File->RemoveAttr("BEOS:TYPE");
							_File->WriteAttr("BEOS:TYPE", B_MIME_TYPE, 0, DATA_TYPE, sizeof(DATA_TYPE));
						}
					}
				}
			}
			delete buffer;
		}
	}
}

AttributeMessageWriter :: ~AttributeMessageWriter()
{
	delete _File;
	_File = NULL;
}

void AttributeMessageWriter :: ResetFilePointer(void)
{
	_Item = 0;
}

status_t AttributeMessageWriter :: Write(BMessage *msg)
{
	char namebuf[B_ATTR_NAME_LENGTH+1];
	size_t length = 0;
	char *data = NULL;

	sprintf(namebuf, "%s/%08lx",_AttrBase,_Item);
	length = msg->FlattenedSize();
	data = (char*)malloc(length);
	if(data!=NULL){
		msg->Flatten(data,length);
		_Error = _File->WriteAttr(namebuf, B_MESSAGE_TYPE, 0, data, length);
		if(_Error > 0) _Error = B_OK;
		if(!_Error) _Item++;
	}

	return(_Error);
}


status_t AttributeMessageWriter :: Read(BMessage *msg)
{
	char namebuf[B_ATTR_NAME_LENGTH+1];
	char *data = NULL;
	attr_info info;
	msg->MakeEmpty();
	msg->what = 0;

	sprintf(namebuf, "%s/%08lx",_AttrBase,_Item);
	_Error = _File->GetAttrInfo(namebuf, &info);
	if(!_Error) data = (char*)malloc(info.size);
	else data = NULL;
	if(data!=NULL){
		_Error = _File->ReadAttr(namebuf, B_MESSAGE_TYPE, 0, data, info.size);
		if(_Error>=0) {
			_Error = B_OK;
			msg->Unflatten(data);
			_Item++;
		}
	}

	return(_Error);
}	   


bool AttributeMessageWriter :: HaveMore(void)
{
	char namebuf[B_ATTR_NAME_LENGTH+1];
	attr_info info;
	bool result = false;

	sprintf(namebuf, "%s/%08lx",_AttrBase,_Item);
	result = (_File->GetAttrInfo(namebuf, &info) == B_OK);

	return result;
}

*/