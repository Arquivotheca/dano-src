//        FileName: AttributeList.cpp
//          Author: Myron W. Walker
//            Date: 9/04/98
//         Version: 1.1

// AttributeList.cpp: implementation of the AttributeList class.
//
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <unistd.h>
#include <Debug.h>

#include "AttributeList.h"
#include "mergesort.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AttributeList::AttributeList()
{
	fHead = NULL;
	fSize = 0;
}

AttributeList::AttributeList(AttributeList *attrlst)
{
	fHead = NULL;
	fSize = 0;
}

AttributeList::AttributeList(char *path)
{
	fHead = NULL;
	fSize = 0;
	
	build(path);
}

AttributeList::~AttributeList()
{
	clear();
}

status_t AttributeList::add(st_attrlistnode *ds)
{
	status_t rtn = B_OK;
	
	if(ds->next == NULL)
	{
		ds->next = fHead;
		fHead = ds;
		fSize++;
	}
	
	return B_OK;
}

status_t AttributeList::add(const char *name, uint32 type, off_t size, const char *data)
{
	status_t rtn = B_OK;
	
	//Create Attribute Node
	st_attrlistnode *nn = new st_attrlistnode;
	int namelen = strlen(name) + 1;
	nn->name = new char[namelen];
	memcpy(nn->name, name, namelen);
	nn->type = type;
	nn->size = size;
	nn->data = new char[size];
	memcpy(nn->data, data, size);
	nn->next = NULL;
		
	//Add Node to List
	nn->next = fHead;
	fHead = nn;
	fSize++;

	return B_OK;
}

status_t AttributeList::build(char *path)
{
	status_t rtn = B_OK;
	
	DIR *Tfile;
	dirent_t *ent;
	attr_info info;
	int fd;
	char *buffer;

	Tfile = fs_open_attr_dir(path);
	
	if (Tfile)
	{ 
		//Get Tfile's Attributes
		ent = fs_read_attr_dir(Tfile);
		while (ent)
		{ 
			int tlen = strlen(ent->d_name) + 1;
			char *tn = new char[tlen];
			memcpy(tn, ent->d_name, tlen);
						
			fd = open(path, O_RDONLY); 
			if(fd > 0)
			{
				if(fs_stat_attr(fd, tn, &info) == 0)
				{ 
					buffer = (char *) malloc((size_t) info.size); 
					if (buffer) 
					{
						fs_read_attr(fd, tn, info.type, 0, buffer, info.size); 
						
						st_attrlistnode *nn = new st_attrlistnode;
						int namelen = strlen(tn) + 1;
						nn->name = new char[namelen];
						memcpy(nn->name, tn, namelen);
						nn->type = info.type;
						nn->size = info.size;
						nn->data = buffer;
						nn->next = NULL;
						
						//Add Node to List
						nn->next = fHead;
						fHead = nn;
						fSize++;
					}
					else rtn = B_ERROR;
				}
				else rtn = B_ERROR;
				close(fd);
			}
			else rtn = B_ERROR;
			
			delete tn;
			fs_close_attr_dir(Tfile);
			ent = fs_read_attr_dir(Tfile);
		}
	}
	else rtn = B_ERROR;
		
	return rtn;
}
		
void AttributeList::clear()
{
	fTempNode.next = fHead;
	fPrevious = &fTempNode;
	fCurrent = fHead;

	while(fCurrent != NULL && fSize > 0)
	{
		fPrevious = fCurrent;
		fCurrent = fPrevious->next;
		delete fPrevious->name;
		delete fPrevious->data;
		delete fPrevious;
		fSize--;
	}
}

AttributeList* AttributeList::compare(AttributeList *olist)
{
	AttributeList *rtnlist= NULL;
	
	if(!fSorted) sortlist();
	if(!olist->fSorted) olist->sortlist();
	
	if(!fHead)
	{
		rtnlist = new AttributeList(olist);
	}
	else if (!olist->fHead)
	{
		rtnlist = new AttributeList(this);
	}
	else
	{
		st_attrlistnode *st_this = fHead;
		st_attrlistnode *st_that = olist->fHead;
		
		while(st_this || st_that)
		{
			int cmpresult;
			
			if(!st_this) cmpresult = 1;
			else if (!st_that) cmpresult = (-1);
			else cmpresult = strcmp(st_this->name, st_that->name);
			
			if(cmpresult < 0)
			{
				if(rtnlist == NULL) rtnlist = new AttributeList();
				
				rtnlist->add(st_this->name, st_this->type, st_this->size, st_this->data);
				st_this = st_this->next;
			}
			else if (cmpresult > 0)
			{
				if(rtnlist == NULL) rtnlist = new AttributeList();
				
				rtnlist->add(st_that->name, st_that->type, st_that->size, st_that->data);
				st_that = st_that->next;
			}		
			else
			{
				bool theymatch = false;				
				//They have equal names so we have to compare size, type, and data;
				if(st_this->type == st_that->type &&
				   st_this->size == st_that->size)
				{
					theymatch = true;
					
					for(int iA = 0; iA < st_this->size; iA++)
					{
						if(st_this->data[iA] != st_that->data[iA])
							theymatch = false;
					}	
				}
				
				if(!theymatch)
				{
					if(rtnlist == NULL) rtnlist = new AttributeList();
				
					rtnlist->add(st_this->name, st_this->type, st_this->size, st_this->data);
					rtnlist->add(st_that->name, st_that->type, st_that->size, st_that->data);
				}
				
				st_this = st_this->next;
				st_that = st_that->next;
			}
		}
	}

	return rtnlist;
}
	

status_t AttributeList::remove(char *name)
{
	status_t retvalue = B_OK;
	
	st_attrlistnode * temp;

	fTempNode.next = fHead;
	fPrevious = &fTempNode;
	fCurrent = fHead;

	if (search(name))
	{
		temp = fCurrent;
		fPrevious->next = fCurrent->next;
		delete temp->name;
		delete temp->data;
		delete temp;
	}
	else
		retvalue = B_ERROR;
	
	fHead = fTempNode.next;

	return retvalue;
}

bool AttributeList::search(const char *name)
{
	bool found = false;
	
	fTempNode.next = fHead;
	fPrevious = &fTempNode;
	fCurrent = fHead;

	while((fCurrent != NULL) && (found == false))
	{
		fPrevious = fCurrent;
		fCurrent = fPrevious->next;
		
		if(strcmp(fCurrent->name, name) == 0);
		{
			fTarget = fCurrent;
			found = true;
		}
	}

	return found;
}

int AttributeList::sortlist()
{
	return(MergeSort(fHead, fSize));
}

bool operator > (const st_attrlistnode& l_val, const st_attrlistnode& r_val)
{
	if(strcmp(l_val.name, r_val.name) > 0)
		return true;
	return false;
};

bool operator < (const st_attrlistnode& l_val, const st_attrlistnode& r_val)
{
	if(strcmp(l_val.name, r_val.name) < 0)
		return true;
	return false;
};

