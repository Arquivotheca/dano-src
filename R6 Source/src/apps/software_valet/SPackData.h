#ifndef _SPACKDATA_H_
#define _SPACKDATA_H_


// SPackData.h			mkk@starcode.com

#include "PackData.h"
#include <Messenger.h>
#include <DataIO.h>
#include <Node.h>

class SPackData : public PackData
{
public:
	SPackData(BMessenger &update, const bool *cancel);
	virtual				~SPackData();
	status_t			AddNodeEntry(BPositionIO *dst, BNode *node, node_flavor flavor);

	status_t			ExtractNode(BPositionIO *src, BNode *node);
	
	//virtual ssize_t		ReadBytes(BPositionIO *fileRep, ssize_t amount, void *buf);
	//virtual ssize_t		WriteBytes(BPositionIO *fileRep, ssize_t amount, void *buf);
	//virtual off_t			Seek(BPositionIO *fileRep, off_t amount, int mode);
	//virtual off_t			Position(BPositionIO *fileRep);
	//virtual off_t			Size(BPositionIO *fileRep);
	
	status_t			UpdateProgress(size_t);
	void				SetProgressMessenger(BMessenger &update);
private:
	BMessenger		progMessenger;
};

#endif
