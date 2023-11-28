//========================================================================
//	MPrefsContainer.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MPrefsContainer.h"
#include "MBlockFile.h"
#include "MPrefsStruct.h"
#include "MBuildersKeeper.h"
#include "IDEMessages.h"

#include <ByteOrder.h>
#include <Debug.h>

struct genericPrefHeader
{
	void		SwapHostToBig();
	void		SwapBigToHost();

	uint32		type;
	int32		datasize;
};

// ---------------------------------------------------------------------------
//		~MPrefsContainer
// ---------------------------------------------------------------------------
//	Constructor

MPrefsContainer::MPrefsContainer()
{
	fMessage = new BMessage;
}

// ---------------------------------------------------------------------------
//		~MPrefsContainer
// ---------------------------------------------------------------------------
//	Constructor

MPrefsContainer::~MPrefsContainer()
{
	delete fMessage;
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Get the preferences from the message.

void
MPrefsContainer::SetData(
	BMessage&	inMessage)
{
	CopyData(inMessage, *fMessage);
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind specified in the
//	BMessage.

void
MPrefsContainer::GetData(
	BMessage&	inMessage) const
{
	const char *	name;
	type_code		type;
	int32			count;
	int32			i = 0;
	status_t		err;

	while (B_NO_ERROR == inMessage.GetInfo(B_ANY_TYPE, i++, &name, &type, &count))
	{
		void*		fromData;
		ssize_t		fromSize;

		if (B_NO_ERROR == fMessage->FindData(name, type, (const void**) &fromData, &fromSize))
		{
			if (fromData)
			{
				err = inMessage.ReplaceData(name, type, fromData, fromSize);
				if (err != B_NO_ERROR)
				{
					err = inMessage.RemoveData(name);
					err = inMessage.AddData(name, type, fromData, fromSize);
				}
			}	
		}	
	}	
}

// ---------------------------------------------------------------------------
//		WriteToFile
// ---------------------------------------------------------------------------
//	should use flatten.

void
MPrefsContainer::WriteToFile(
	MBlockFile& inFile) const
{
	genericPrefHeader		header;

	// Write out all the generic preferences blocks
	inFile.StartBlock(kGenericPrefsBlockType);	// Start prefs block

	const char *name;
	int32		count;
	int32		i = 0;

	while (B_NO_ERROR == fMessage->GetInfo(B_ANY_TYPE, i++, &name, &header.type, &count))
	{
		ASSERT(count == 1);
		const void* fromData;

		if (B_NO_ERROR == fMessage->FindData(name, header.type, &fromData, &header.datasize))
		{
			if (fromData)
			{
				int32	datasize = header.datasize;
				int32	namelen = strlen(name) + 1;

				header.SwapHostToBig();
				inFile.StartBlock(kGenericBlockType);		// Start generic block
				inFile.PutBytes(sizeof(genericPrefHeader), &header);
				inFile.PutBytes(namelen, name);
				inFile.PutBytes(datasize, fromData);
				inFile.EndBlock();							// End the generic block
			}
		}
	}

	inFile.EndBlock();							// End the preferences block
}

// ---------------------------------------------------------------------------
//		ReadFromFile
// ---------------------------------------------------------------------------

void
MPrefsContainer::ReadFromFile(
	MBlockFile& inFile)
{
	BlockType			type;
	size_t				blockSize;
	int32				bufferLen = 1024;
	char*				buffer = new char[bufferLen];

	//	Scan file for data to fill in
	while (B_NO_ERROR == inFile.ScanBlock(type)) 
	{
		blockSize = inFile.GetCurBlockSize();

		switch (type) 
		{
			case kGenericBlockType:
			{
				genericPrefHeader		header;
				char					name[B_FILE_NAME_LENGTH+1];

				inFile.GetBytes(sizeof(genericPrefHeader), &header);
				header.SwapBigToHost();
				int32		len = min(blockSize - sizeof(genericPrefHeader) - header.datasize, (size_t) B_FILE_NAME_LENGTH);
				inFile.GetBytes(len, name);
				name[B_FILE_NAME_LENGTH] = 0;	// just in case
				if (header.datasize > bufferLen)
				{
					delete [] buffer;
					bufferLen = header.datasize;
					buffer = new char[bufferLen];
				}
				inFile.GetBytes(header.datasize, buffer);
				fMessage->AddData(name, header.type, buffer, header.datasize);
				break;
			}
			
			default:
				ASSERT(!"found invalid block in generic blocks");
				break;
		}

		inFile.DoneBlock(type);	//	done with whatever block we got; move to next
	}
	
	delete [] buffer;
	
	MBuildersKeeper::ValidateGenericData(*fMessage);
}

// ---------------------------------------------------------------------------
//		CopyData
// ---------------------------------------------------------------------------
//	Merge all of the fields from the From message into the to message.
//	It's best if fields added to messages aren't arrays (i.e., you don't
//	add multiple items with the same name and type).

void
MPrefsContainer::CopyData(
	BMessage&	inFrom,
	BMessage&	inTo)
{
	const char *name;
	uint32		type;
	int32		count;
	int32		i = 0;

	while (B_NO_ERROR == inFrom.GetInfo(B_ANY_TYPE, i++, &name, &type, &count))
	{
		for (int32 j = 0; j < count; j++)
		{
			status_t	err = B_OK;
			const void*	toData = nil;
			const void*	fromData = nil;
			ssize_t		toSize;
			ssize_t		fromSize;

			(void) inTo.FindData(name, type, j, &toData, &toSize);
			(void) inFrom.FindData(name, type, j, &fromData, &fromSize);

			if (toData != nil)
			{
				err = inTo.ReplaceData(name, type, fromData, fromSize);
				if (err != B_OK)
				{
					err = inTo.RemoveData(name, j);
					err = inTo.AddData(name, type, fromData, fromSize, false);
				}
			}	
			else
				inTo.AddData(name, type, fromData, fromSize, false);
		}
	}
}

// ---------------------------------------------------------------------------
//		CopyTo
// ---------------------------------------------------------------------------

void
MPrefsContainer::CopyTo(
	BMessage&	inTo) const
{
	CopyData(*fMessage, inTo);
}

// ---------------------------------------------------------------------------
//		RemoveData
// ---------------------------------------------------------------------------
//	This is a hack to clean up problems with data that got into the generic
//	data by mistake.

void
MPrefsContainer::RemoveData(
	const char*		inName)
{
	fMessage->RemoveName(inName);
}

// ---------------------------------------------------------------------------
//		FillMessage
// ---------------------------------------------------------------------------
//	Copy all of the data entries for the specified tool from the built-in 
//	message to the passed-in message.  Each builder/prefs panel stores
//	its prefs structs in the message under a specific type.

void
MPrefsContainer::FillMessage(
	BMessage&		inMessage,
	uint32			inType) const
{
	const void* data;
	ssize_t		size;
	const char *name;
	type_code	type;
	int32		count;
	int			i = 0;

	while (B_NO_ERROR == fMessage->GetInfo(B_ANY_TYPE, i++, &name, &type, &count))
	{
		if (inType == type)
		{
			for (int j = 0; j < count; j++)
			{
				if (B_NO_ERROR == fMessage->FindData(name, type, j, &data, &size))
				{
					ASSERT(data);
					if (data)
						inMessage.AddData(name, type, data, size, false);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		FillMessage
// ---------------------------------------------------------------------------
//	Copy all of the data entries for the specified tool from the built-in 
//	message to the passed-in message.  Each builder/prefs panel stores
//	its prefs structs in the message under a specific type.

void
MPrefsContainer::FillMessage(
	BMessage&		inFrom,
	BMessage&		inTo,
	uint32			inType)
{
	const void* data;
	ssize_t		size;
	const char *name;
	type_code	type;
	int32		count;
	int32		i = 0;

	while (B_NO_ERROR == inFrom.GetInfo(B_ANY_TYPE, i++, &name, &type, &count))
	{
		if (inType == type && B_NO_ERROR == inFrom.FindData(name, type, &data, &size))
		{
			ASSERT(data);
			if (data)
				inTo.AddData(name, type, data, size, false);
		}
	}
}

// ---------------------------------------------------------------------------
//		Flatten
// ---------------------------------------------------------------------------
//	Flatten the data in the message into a single block.  I don't use
//	the BMessage call of the same name because I don't want to be caught
//	out when they change the format.  The block returned should be deleted
//	with 'delete [] outData'.

void
MPrefsContainer::Flatten(
	char*&			outData,
	size_t&			outSize) const
{
	genericPrefHeader	header;
	genericPrefHeader	swapheader;
	const char *		name;
	int32				count;
	int32				i = 0;
	int32				blockSize = 0;

	// How many blocks and how much data?
	while (B_NO_ERROR == fMessage->GetInfo(B_ANY_TYPE, i++, &name, &header.type, &count))
	{
		ASSERT(count == 1);
		const void* fromData;

		if (B_NO_ERROR == fMessage->FindData(name, header.type, &fromData, &header.datasize) &&
			fromData != nil)
		{
			int32	namelen = strlen(name) + 1;
		
			blockSize += sizeof(genericPrefHeader) + namelen + header.datasize;
			int32	align = blockSize % sizeof(int32);		// align to longword
			if (align != 0)
				blockSize += sizeof(int32) - align;
		}
	}

	blockSize += sizeof(genericPrefHeader) + 1;				// For the sentinel

	char*			data = new char[blockSize];
	outData = data;
	outSize = blockSize;
	i = 0;
	blockSize = 0;
	const char			zeros[3] = { 0, 0, 0 };

	// Build the block
	while (B_NO_ERROR == fMessage->GetInfo(B_ANY_TYPE, i++, &name, &header.type, &count))
	{
		const void* fromData;

		if (B_NO_ERROR == fMessage->FindData(name, header.type, &fromData, &header.datasize) &&
			fromData != nil)
		{
			swapheader = header;
			swapheader.SwapHostToBig();
			memcpy(data, &swapheader, sizeof(genericPrefHeader));
			data += sizeof(genericPrefHeader);
			int32	namelen = strlen(name) + 1;
			memcpy(data, name, namelen);
			data += namelen;
			memcpy(data, fromData, header.datasize);
			data += header.datasize;
			blockSize += sizeof(genericPrefHeader) + namelen + header.datasize;

			int32	align = blockSize % sizeof(int32);		// align to longword
			if (align != 0)
			{
				align = sizeof(int32) - align;
				memcpy(data, zeros, align);
				data += align;
				blockSize += align;
			}
		}
	}
	
	// Append the sentinal
	header.type = (uint32) -1;
	header.datasize = -1;
	memcpy(data, &header, sizeof(genericPrefHeader));
	data += sizeof(genericPrefHeader);
	memcpy(data, zeros, 1);
}

// ---------------------------------------------------------------------------
//		UnFlatten
// ---------------------------------------------------------------------------
//	UnFlatten the data in the block.  The contents of the fMessage are replaced
//	with the contents of the block.

void
MPrefsContainer::UnFlatten(
	const char*			inData)
{
	genericPrefHeader*	header;
	const char*			name;
	const char*			data = inData;

	fMessage->MakeEmpty();

	while (true)
	{
		header = (genericPrefHeader*) data;
		if (header->type == (uint32) -1 && header->datasize == -1)	// Sentinel?
			break;
		
		int32		datasize = B_BENDIAN_TO_HOST_INT32(header->datasize);
		data += sizeof(genericPrefHeader);
		name = data;
		data += strlen(name) + 1;
		fMessage->AddData(name, header->type, data, datasize);
		data += datasize;

		int32	blockSize = data - inData;
		int32	align = blockSize % sizeof(int32);		// align to longword
		if (align != 0)
			data += sizeof(int32) - align;;
	}

}

// ---------------------------------------------------------------------------
//		ValidateGenericData
// ---------------------------------------------------------------------------

bool
MPrefsContainer::ValidateGenericData()
{
	return MBuildersKeeper::ValidateGenericData(*fMessage);
}

// ---------------------------------------------------------------------------
//		• SwapBigToHost
// ---------------------------------------------------------------------------

void
genericPrefHeader::SwapBigToHost()
{
	if (B_HOST_IS_LENDIAN)
	{
		datasize = B_BENDIAN_TO_HOST_INT32(datasize);
	}
}

// ---------------------------------------------------------------------------
//		• SwapHostToBig
// ---------------------------------------------------------------------------

void
genericPrefHeader::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN)
	{
		datasize = B_HOST_TO_BENDIAN_INT32(datasize);
	}
}
