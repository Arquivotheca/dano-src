//========================================================================
//	MBlockFile.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MBLOCKFILE_H
#define _MBLOCKFILE_H

#include <stack.h>
#include <vector.h>
#include <Entry.h>


//	Errors from reading functions
enum {
	NO_MORE_BLOCKS = 1,		//	From ScanBlock
	NO_MORE_DATA = 2,		//	From GetXxx
	INCONSISTENT_MODE = 3,	//	Read when writing or vv
	NO_SUCH_BLOCK = 4,		//	From EndBlock and DoneBlock
	DISK_FULL = 5			//	From PutXxx and everything calling RawWrite
};

typedef uint32 BlockType;


class MBlockFile : public BEntry
{
protected:

		struct BlockHeader {					//	On-disk info on blocks
			BlockType				type;
			size_t					size;		//	bigendian
		};
		struct BlockHeaderInfo {				//	In-memory info on blocks
			BlockHeader				header;
			off_t					pos;
			size_t					blocksize;
		};

// I can't figure out what this is for.  Doesn't seem to be used
// (commented out by John Dance)
// null_template
// struct iterator_trait<const MBlockFile::BlockHeaderInfo*>
// {
// 	typedef ptrdiff_t						distance_type;
//	typedef const MBlockFile::BlockHeaderInfo	value_type;
//	typedef random_access_iterator_tag		iterator_category;
// };

typedef allocator<BlockHeaderInfo> headerallocator;
typedef stack<BlockHeaderInfo, vector<BlockHeaderInfo, headerallocator > > HeaderStack;

//	vector is faster than list and dequeu

		//	I/O bottlenecks

virtual	status_t				RawRead(
									size_t bytes,
									void * data);
virtual	status_t				RawWrite(
									size_t bytes,
									const void * data);
virtual	status_t				Position(
									off_t pos);

virtual	off_t					GetPos() { return fPos; }

		//	Wrappers for functionality

virtual	status_t				DoEndBlock(
									BlockHeaderInfo& info);
virtual	status_t				DoDoneBlock(
									BlockHeaderInfo& info);
		void					Flush();

		off_t					fPos;			//	Since there's no BFile::GetPosition()
		bool					fIsReading;		//	TRUE between ResetRead() and Close()
		bool					fIsWriting;		//	TRUE between ResetWrite() and Close()
		BFile*					fFile;
		void*					fBuffer;
		off_t					fBufferStart;
		off_t					fBufferEnd;
		off_t					fBufferCurrEnd;
		HeaderStack				fblockStack;

public:

		//	Constructor stuff

								MBlockFile();
								MBlockFile(
									const entry_ref& id);
								~MBlockFile();

virtual	status_t				Open();
virtual	status_t				Close();
		status_t				Reset();

		BFile*					File();		// only valid if the file is open

		//	Writing stuff

virtual	status_t				ResetWrite(
									size_t atPosition = 0,
									BlockType useType = 0);
virtual	status_t				StartBlock(
									BlockType type);
virtual	status_t				PutBytes(
									size_t size,
									const void * data);
virtual	status_t				PutInt32(
									int32 l);
virtual	status_t				PutString(
									const char *	inString);
virtual	status_t				EndBlock(
									BlockType type = 0);

		//	Reading stuff

virtual	status_t				ResetRead(
									int32 atPosition = 0);
virtual	status_t				ScanBlock(
									BlockType & type);
virtual	size_t					GetCurBlockSize();
virtual	BlockType				GetCurBlockType();
virtual	int32					GetBlockLeft();
virtual	status_t				GetBytes(
									size_t size,
									void * data);
virtual	status_t				GetInt32(
									int32 & l);
virtual	status_t				GetString(
									char *	outString,
									size_t	inBufferSize);
virtual	status_t				DoneBlock(
									BlockType type = 0);

};

#endif
