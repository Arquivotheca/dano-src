#include <stdio.h>

#include <File.h>
#include <fs_attr.h>
#include <OS.h>
#include <String.h>
#include <Mime.h>
#include <E-mail.h>

#include "MailSMTPTools.h"
#include "MDUtils.h"

bool read_attr_string (BNode *node,const char *name,BString *string)
{
	node->RewindAttrs();
	struct attr_info info;
 	if (node->GetAttrInfo(name, &info) == B_NO_ERROR)
 	{
 		char *b_string = new char[info.size+1];
		if (node->ReadAttr(name,B_STRING_TYPE,0,b_string,info.size)==0)
		{
			return false;
		}
		else
		{
			b_string[info.size] = 0;
			*string = b_string;
		}
		if (b_string)
			delete[] b_string;
		return true;
	}
	return false;
}
