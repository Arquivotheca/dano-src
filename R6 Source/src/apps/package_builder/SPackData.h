
// SPackData.h			mkk@starcode.com

#include "PackData.h"

#ifndef _SPACKDATA_H
#define _SPACKDATA_H


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
	const bool		*cancelVar;
	BMessenger		progMessenger;
};

#endif
