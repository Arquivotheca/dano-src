
#include <Binder.h>
#include <stdio.h>
#include <string.h>
#include <String.h>

//
// unslash and find_binder_node basically ripped off from the binder shell tool
//

void unslash(char * dst, char const * src)
{
	while (*src != '\0') {
		if ( *src!='\\' ) {
			*dst = *src;
			dst++;
		} 
		src++;
	}
	*dst = '\0';
}

int find_binder_node(const char *_path, BinderNode::property &prop, BString &name)
{
	BinderNode::property prop2;
	binder_node node = BinderNode::Root();
	status_t s = B_ERROR;
	char *lastP = NULL;
	char *p,*p2,path[2048];
	char lastPropName[1024];
	bool end=false;
	
	if (!node->IsValid()) {
		printf("Could not connect to binder!\n");
		return -1;
	}

	prop = node;

	strcpy(path,_path);
	p=p2=path;

	while (!end) {
		if (lastP) {
			unslash(lastPropName, lastP);
			prop = prop[lastPropName];
		}
		// Find the start of the next path component.
		while ((*p == '/') && (*p != 0)) p++;
		p2 = p;
		// Find the end of the path component.
		while ((*p2 != '/') && (*p2 != 0)) {
			if (*p2 == '\\') p2++; // skip the following char
			p2++;
		}
		if (!*p2 || !*(p2+1)) end = true; // Special case: trailing slash
		*p2 = 0;
		lastP = p;
		p = p2+1;
	}
	if (lastP) {
		unslash(lastPropName, lastP);
	}
	
	if (!prop->IsValid()) {
		printf("Could not traverse binder to path '%s'!\n",path);
		return -1;
	}
	
	name = lastPropName;
	
	return 0;
}



extern "C" status_t do_binderput(int argc, char **argv)
{
	char *path = argv[0];
	char *value = argv[1];
	BinderNode::property p;
	status_t s = B_ERROR;
	BString name;
	
	if(find_binder_node(path, p, name) == 0)
	{
		s = p->PutProperty(name.String(), value);
		if(s != B_NO_ERROR)
		{
			printf("Could not put property %s\n", path);
		}
	} else {
		printf("Could not find property %s\n", path);
	}

	return s;
}


extern "C" status_t do_binderget(int argc, char **argv)
{
	char *path = argv[0];
	char *var = argv[1];
	BinderNode::property p, v;
	status_t s = B_ERROR;
	BString name;
	
	if(find_binder_node(path, p, name) == 0)
	{
		s = p->GetProperty(name.String(), v);
		if(s == B_NO_ERROR)
		{
			BString env = var;
			
			env += "=";
			env += v.String();
			if(putenv(env.String()) == 0)
			{
				s = B_NO_ERROR;
			} else {
				printf("could not set %s!\n", var);
				s = B_ERROR;
			}
		} else {
			printf("Could not get property %s\n", path);
		}
	} else {
		printf("Could not find property %s\n", path);
	}
	
	return s;
}


