//        FileName: AttributeList.h
//          Author: Myron W. Walker
//            Date: 9/04/98
//         Version: 1.1

// AttributeList.h: interface for the AttributeList class.
//
//////////////////////////////////////////////////////////////////////

#ifndef  AFX_attributelist_H__CB5F2C20_0235_11D2_AFE5_9304E045C32E__INCLUDED_
#define  AFX_attributelist_H__CB5F2C20_0235_11D2_AFE5_9304E045C32E__INCLUDED_

#include <fs_attr.h>

struct st_attrlistnode
{
	char *name;
	uint32 type;
	off_t size;
	char *data;
	st_attrlistnode *next;
};

bool operator > (const st_attrlistnode& l_val, const st_attrlistnode& r_val);
bool operator < (const st_attrlistnode& l_val, const st_attrlistnode& r_val);

class AttributeList  
{
	friend bool operator > (const st_attrlistnode& l_val, const st_attrlistnode& r_val);
	friend bool operator < (const st_attrlistnode& l_val, const st_attrlistnode& r_val);
	public:
		AttributeList();
		AttributeList(AttributeList *attrlst);
		AttributeList(char *path);
		virtual ~AttributeList();
	
		status_t add(st_attrlistnode *ds);
		status_t add(const char *name, uint32 type, off_t size, const char *data); 
	
		status_t build(char *path);
		
		void clear();
		long size(){ return fSize; };
		int sortlist();
		
		AttributeList* compare(AttributeList *olist); 
	
		status_t remove(char *name);
	private:
		bool fSorted;
		long fSize;
		
		st_attrlistnode * fHead;
		st_attrlistnode * fTarget;
		st_attrlistnode * fPrevious;
		st_attrlistnode * fCurrent;
		
		st_attrlistnode fTempNode;
	
		bool search(const char *name);
};

#endif //AFX_attributelist_H__CB5F2C20_0235_11D2_AFE5_9304E045C32E__INCLUDED_
