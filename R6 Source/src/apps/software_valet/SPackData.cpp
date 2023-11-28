#include <ClassInfo.h>
#include <File.h>
#include <fs_attr.h>
#include "SPackData.h"
#include "ZlCodec.h"
#include "SAttrIO.h"

#include "Util.h"
#define MODULE_DEBUG 0
#include "MyDebug.h"

#include "ErrHandler.h"


/***
Deprecated BAD DATA!
const record_header FileCatItem = {'FiCa', LIS_TYPE, 0 };
const record_header DirCatItem = {'DiCa', LIS_TYPE, 0 };
const record_header PatCatItem = {'PaCa', LIS_TYPE, 0 };
const record_header ScrCatItem = {'ScCa', LIS_TYPE, 0 };
const record_header FileDatItem = {ID_FILEDATA, LIS_TYPE, 0 };
const record_header FileMainFork = {ID_MAINFORK, BIN_TYPE, 0 };
const record_header BAttrList = {ID_ATTRLIST, LIS_TYPE, 0};
const record_header BAttrItem = {ID_ATTRITEM, LIS_TYPE, 0};
const record_header BAttrData = {ID_ATTRDATA, BIN_TYPE, 0};
***/

enum {
	ID_FILEDATA	=	'FiDa',
	ID_FOLDDATA =	'FoDa',
	ID_LINKDATA =	'LnDa',
	ID_MAINFORK	=	'FiMF',
	ID_ATTRLIST	=	'FBeA',
	ID_ATTRITEM	=	'BeAI',
	ID_ATTRNAME	=	'BeAN',
	ID_ATTRTYPE	=	'BeAT',
	ID_ATTRDATA	=	'BeAD',
	ID_ADLER32	=	'Ad32'
};

const int32 BAttrName = ID_ATTRNAME;
const int32 BAttrType = ID_ATTRTYPE;


SPackData::SPackData(BMessenger &msngr, const bool *cancel)
	:	PackData(),
		progMessenger(msngr)
{
	cancel;
	
	ZlCodec *c = new ZlCodec();
	c->SetErrorHook(doError);
	SetCurrentCodec(c);	
	SetCallbackData(this);
}

SPackData::~SPackData()
{
	delete CurrentCodec();
}

status_t	MainForkProgress(size_t in, size_t out, void *data);

status_t	MainForkProgress(size_t in, size_t out, void *data)
{
	in;
	out;
	
	SPackData *pd = (SPackData *)data;
	return pd->UpdateProgress(in);
}

status_t	ExMainForkProgress(size_t in, size_t out, void *data);

status_t	ExMainForkProgress(size_t in, size_t out, void *data)
{
	in;
	out;
	
	SPackData *pd = (SPackData *)data;
	return pd->UpdateProgress(out);
}

void		SPackData::SetProgressMessenger(BMessenger &m)
{
	progMessenger = m;
}

status_t		SPackData::UpdateProgress(size_t in)
{
	const ulong	M_UPDATE_PROGRESS = 	'Uprg';
		
	BMessage updtMsg(M_UPDATE_PROGRESS);
	updtMsg.AddInt32("bytes",in);
	
	BMessage	replyM;
	progMessenger.SendMessage(&updtMsg,&replyM);
	if (replyM.FindBool("canceled"))
		return B_CANCELED;
		
	return B_OK;
}

// works for nodes other than files
status_t	SPackData::AddNodeEntry(BPositionIO *dst, BNode *srcNode, node_flavor flavor)
{
	PRINT(("SPackData::AddNodeEntry\n"));
	ErrHandler	err;
	try {
		off_t	encoded, decoded;
		// write file headers
		
		int32 nodetype; 
		switch (flavor) {
			case B_FILE_NODE:
				nodetype = ID_FILEDATA;
				break;
			case B_SYMLINK_NODE:
				nodetype = ID_LINKDATA;
				break;
			case B_DIRECTORY_NODE:
				nodetype = ID_FOLDDATA;
				break;
			default:
				err = B_ERROR;
		}
		// we will make the clients soft on this...
		// WriteRecordHeader(dst, nodetype, 0, LIS_TYPE);
		// this is the right way to do it
		err = WriteRecordHeader(dst, nodetype, LIS_TYPE, 0);

		if (flavor == B_FILE_NODE)
		{
			BFile	*srcFile = cast_as(srcNode, BFile);
			if (srcFile) {
				PRINT(("SPackData::AddNodeEntry -- adding mainfork\n"));
	
				// update progress
				
				// check size of dst file, allow writing all of file,
				// set encode max to limit
				
				encoded = decoded = ENCODE_ALL;
				int	flags = 0;
				
				err = WriteBin(dst,srcFile,&encoded,&decoded,
									&flags,MainForkProgress,ID_MAINFORK);
				srcNode = srcFile;
			}
		}
		//if (decoded < fileSize) {
			// switch to the next segment
			// loop until encoded
		//}
		
		/***
		stream = (z_stream *)(CurrentCodec()->Info());
		if (stream) {
			err = WriteInt32(dst,stream->adler,ID_ADLER32);
		}
		***/
		
		
		// now do the attributes
		{
			err = WriteRecordHeader(dst,ID_ATTRLIST, LIS_TYPE, 0);
			//err = WriteRecordHeader(dst,&BAttrList);
			
			char attrname[B_FILE_NAME_LENGTH];
			srcNode->RewindAttrs();
			while(srcNode->GetNextAttrName(attrname) == B_NO_ERROR) {
				attr_info	ai;
				srcNode->GetAttrInfo(attrname, &ai);
				
				{
					PRINT(("SPackData::AddNodeEntry -- adding attribute %s\n",attrname));
					err = WriteRecordHeader(dst,ID_ATTRITEM, LIS_TYPE, 0);
					
					WriteString(dst,attrname,BAttrName);
					err = WriteInt32(dst,ai.type,BAttrType);
					
					SAttrIO		aIO(srcNode,attrname,&ai);
					
					encoded = decoded = ENCODE_ALL;
					int flags = 0;
					WriteBin(dst,&aIO,&encoded,&decoded,
							&flags,NULL,ID_ATTRDATA );
					WriteEndHeader(dst);
					PRINT(("SPackData::AddNodeEntry -- completed adding attribute %s\n",attrname));
				}
			}
			WriteEndHeader(dst);
		}
		err = WriteEndHeader(dst);
	}
	catch (ErrHandler::OSErr info) {
		return info.err;
	}
	
	// catch (ErrHander::ZlErr info)
	// catch (ErrHandler::CksumErr info)
	// catch (ErrHandler::fooErr info)
	return B_NO_ERROR;
}


status_t	SPackData::ExtractNode(BPositionIO *src, BNode *dstNode)
{
	ErrHandler		err;
	record_header	header;
	
	try {
		PRINT(("SPackData::ExtractNode\n"));

		err = ReadRecordHeader(src,&header);
		if (!(	header.what == ID_FILEDATA  ||
				header.what == ID_FOLDDATA	||
				header.what == ID_LINKDATA)
				// && header.type == LIS_TYPE) // loosen this because
				// package bulder < 1.52 wrote some bad file formats!!
			)
		{
			// not a valid file
			// throw exception
			err = B_ERROR;
		}
			
		bool doingFile = true;
		while(doingFile) {
			PRINT(("SPackData::ExtractNode -- file list\n"));
			err = ReadRecordHeader(src,&header);
			switch(header.what) {
				case ID_MAINFORK: {
					PRINT(("SPackData::ExtractNode -- main fork\n"));
					BFile	*dstFile = cast_as(dstNode,BFile);
					if (dstFile) {
						// main fork
						err = ReadBin(dstFile,src,FALSE,ExMainForkProgress);
					}
					else {
						err = SkipData(src,&header);
					}
					dstFile->SetSize(dstFile->Position());
					break;
				}
			/**
				case ID_ADLER32:
					int32 csum;
					// read an adler32 checksum
					err = ReadInt32(src,&csum);
					// compare the checksum
					break;
			**/
				case ID_ATTRLIST: {
					bool doingAttr = true;
					while(doingAttr) {
						record_header ahead;
						err = ReadRecordHeader(src,&ahead);
						switch(ahead.what) {
							// deal with an attribute
							case ID_ATTRITEM: {
								bool isAttr = TRUE;
								char attrname[B_FILE_NAME_LENGTH];
								type_code	atype;
								while(isAttr) {
									record_header thead;
									err = ReadRecordHeader(src,&thead);
									switch(thead.what) {
										// read attrname
										case ID_ATTRNAME:
											err = ReadString(src,attrname,B_FILE_NAME_LENGTH);
											break;
										// read attrtype
										case ID_ATTRTYPE:
											err = ReadInt32(src,(int32 *)&atype);
											break;
										case ID_ATTRDATA: {
											PRINT(("SPackData::ExtractNode -- attribute data %s\n",attrname));
											// read attrdata
											SAttrIO		aIO(dstNode,attrname,atype);
											err = ReadBin(&aIO,src,FALSE);
											break;			
										}
										case ID_ADLER32:
											// read an adler32 checksum
											int32 csum;
											// read an adler32 checksum
											err = ReadInt32(src,&csum);
											// compare the checksum
											break;
										case END_TYPE:
											isAttr = FALSE;
											break;
										default:
											err = SkipData(src,&header);
											break;
									}
								}
								break;
							}
							case END_TYPE:
								doingAttr = FALSE;
								break;
							default:
								err = SkipData(src,&header);
								break;
						}
					}
					break;
				}
				case END_TYPE:
					doingFile = false;
					break;
				default:
					err = SkipData(src,&header);
					break;
			}
		}
	}
	catch (ErrHandler::OSErr info) {
		return info.err;
	}
	// catch (ErrHander::ZlErr info)
	// catch (ErrHandler::CksumErr info)
	// catch (ErrHandler::fooErr info)
	return B_NO_ERROR;
}


/***
ssize_t		SPackData::ReadBytes(BPositionIO *fileRep, ssize_t amount, void *buf)
{
	PRINT(("SPackData::ReadBytes\n"));
	PRINT(("amount is %d\n",amount));
	
	BPositionIO *io = (BPositionIO *)(fileRep);
	return io->Read(buf, amount);
}

ssize_t		SPackData::WriteBytes(BPositionIO *fileRep, ssize_t amount, void *buf)
{
	PRINT(("SPackData::WriteBytes\n"));
	
	BPositionIO *io = (BPositionIO *)fileRep;
	return io->Write(buf, amount);
}

off_t		SPackData::Seek(BPositionIO *fileRep, off_t amount, int mode)
{
	BPositionIO *io = (BPositionIO *)fileRep;
	return io->Seek(amount, mode);
}

off_t		SPackData::Position(BPositionIO *fileRep)
{
	BPositionIO *io = (BPositionIO *)fileRep;
	return io->Position();
}

off_t		SPackData::Size(BPositionIO *fileRep)
{
	BPositionIO *io = (BPositionIO *)fileRep;
	off_t	curSize;
	off_t	pos;
	
	pos = io->Position();
	curSize = io->Seek(0,SEEK_END);
	io->Seek(pos,SEEK_SET);
	
	return curSize;
}
****/
