// DestinationList.h
#include "FListView.h"

#ifndef _DESTINATIONLIST_H
#define _DESTINATIONLIST_H

// regular indexes for other destinations
enum {
	D_INSTALL_FOLDER = -1,
	D_PARENT_FOLDER = -2,
	D_NO_DEST = -1000,
	D_FIND_OFFSET = 65535
};


class DestItem : public ListItem
{
public:
	DestItem(const char *p, const char *name = NULL, int32 code = D_NO_DEST);
	virtual ~DestItem();
	char		*path;
	char		*findName;
	int32		findCode;
};


class FindItem : public DestItem
{
public:
	FindItem(const char *_name, off_t _size, const char *_app_sig);
	FindItem(const char *_name);
	
	virtual ~FindItem();
//	ulong type;
//	ulong creator;
	off_t			size;
	
	//bool			queryFailed;
	//entry_ref		foundItem;
	
	//void			AddPredicate(const char *);
	//const char		*PredicateAt(int32 index)	const;	
	//int32			CountPredicates()	const;
	
	void			SetSignature(const char *);
	const char		*Signature() const;	
private:
	//RList<char *>	*fPredList;
	char		*appSig;
};



class DestList : public RList<DestItem *>
{
public:
	~DestList() {
		for (long i = CountItems()-1; i >= 0; i--) {
			delete ItemAt(i);
		}
	}

	BLocker		lock;
};



enum {
	M_SET_TYPE =	'STyp',
	M_SET_CREATOR =	'SCre',
	M_SET_SIZE =	'SSiz'
};
#endif
