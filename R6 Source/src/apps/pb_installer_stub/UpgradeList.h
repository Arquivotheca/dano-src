#ifndef _UPGRADELIST_H_
#define _UPGRADELIST_H_


#include "RList.h"
#include <Message.h>

class UpgradeItem
{
public:
					UpgradeItem(BMessage *newdata);
					~UpgradeItem();
		int64		VersionID() {
						int64 v;
						data.FindInt64("versionid",&v);
						return v;
					};
		void		PrintToStream();
	
	BMessage		data;
};

class UpgradeItemList : public RList <UpgradeItem *>
{
public:
					UpgradeItemList();
					UpgradeItemList(const BMessage *archive);
	virtual 		~UpgradeItemList();
	status_t		Archive(BMessage *archive, bool deep);
	
	status_t		SetTo(const BMessage *);
	
	void			MakeEmpty();
					
	bool	 		AddUpgrade(BMessage *add);
	bool			AddUpgrade(UpgradeItem *add, int32 atindex = -1);
private:
					UpgradeItemList(const UpgradeItemList &);

};

#endif
