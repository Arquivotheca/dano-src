// binder.cpp
// binder interaction shelltool

#include <stdio.h>
#include <Application.h>
#include <Binder.h>

#define checkpoint printf("settings: thid=%d -- %s:%d -- %s\n",find_thread(NULL),__FILE__,__LINE__,__FUNCTION__);

#define BACK_PATH  		"|  "
#define EMPTY_PATH 		"   "
#define NEW_PATH   		"+-"
#define ENDEMPTY_PATH   "  *"
#define ENDBACK_PATH   	"| *"

// format_property(dst, prop)
//     places a human-readable version of prop in dst.
//     does not own the memory in dst (as strcat, sprintf, etc.).
void format_property(char * const dst, BinderNode::property &prop) {
	BinderNode::property::type t = prop.Type();
	
	switch (t) {
		case BinderNode::property::null:
			sprintf(dst, "null");
			break;
		case BinderNode::property::string:
			sprintf(dst, "string('%s')", prop.String().String());
			break;
		case BinderNode::property::number:
			sprintf(dst, "number(%f)", prop.Number());
			break;
		case BinderNode::property::object:
		case BinderNode::property::remote_object:
			// We defer to binder's string representation for objects.
			sprintf(dst, "%s", prop.String().String()); 
			break;
		default:
			sprintf(dst, "unknown-type('%s')", prop.String().String());
			break;
	}
}

// unslash(dst,src)
// copy src to dst, skipping slashes.  [ strlen(dst)<=strlen(src) ]
void unslash(char * dst, char const * src) {
	while (*src != '\0') {
		if ( *src!='\\' ) {
			*dst = *src;
			dst++;
		} 
		src++;
	}
	*dst = '\0';
}


void recursive_dir(BinderNode::property &prop, int32 level)
{
	if(level>1023) // Got a big terminal or what?
		return;

	// String for formatting property values.
	// Binder.cpp implies a max length of 512 for a property's string value.
	// 32 additional chars are available for the type descriptor.
	static char fmt_buf[16384 + 32];

	BString name;
 	BinderNode::property prop2;
	static int32 lastobject[1024];
	static int32 currobject[1024];
	int32 count = 0;
	// prescan for last object
	{
		bool foundobject = false;
		BinderNode::iterator i = prop->Properties();
		for(count = 0; (name = i.Next()).String() != ""; count++) {
			if(prop->GetProperty(name.String(),prop2) == B_OK)
				if(prop2.IsObject())
					foundobject = true;
			lastobject[level] = count;
		}
		// If not object mark it
		if(!foundobject)
			lastobject[level] = -1;
	}
	// Display non object child properties first
	{
		BinderNode::iterator i = prop->Properties();
		for(count = 0; (name = i.Next()).String() != "";) {
			currobject[level] = count;
			if(prop->GetProperty(name.String(),prop2) == B_OK)	{
				if(!prop2.IsObject()) {
					for(int c=0;c<=level;c++) {
						if(currobject[c] < lastobject[c])
							printf(c==level ? ENDBACK_PATH : BACK_PATH);
						else
							printf(c==level ? ENDEMPTY_PATH : EMPTY_PATH);
					}
					format_property(fmt_buf, prop2);
					printf("'%s' --> %s\n", name.String(), fmt_buf);
					count++;
				}
			}
			else {
				for(int c=0;c<=level;c++) {
					if(currobject[c] < lastobject[c])
						printf(c==level ? ENDBACK_PATH : BACK_PATH);
					else
						printf(c==level ? ENDEMPTY_PATH : EMPTY_PATH);
				}
				printf("'%s' --> unknown!\n", name.String());
				count++;
			}
		}
	}
	// Then display objects and their children
	{
		BinderNode::iterator i = prop->Properties();
		for(; (name = i.Next()).String() != "";) {
			currobject[level] = count;
			if(prop->GetProperty(name.String(),prop2) == B_OK)	{
				if(prop2.IsObject()) {
					for(int c=0;c<=level;c++) {
						if(currobject[c] < lastobject[c] || c==level)
							printf(BACK_PATH);
						else
							printf(EMPTY_PATH);	
					}	
					printf("\n");
					for(int c=0;c<level;c++) {
						if(currobject[c] < lastobject[c])
							printf(BACK_PATH);
						else
							printf(EMPTY_PATH);
					}
					printf(NEW_PATH);
					format_property(fmt_buf, prop2);
					printf("'%s' --> %s\n", name.String(), fmt_buf);
					recursive_dir(prop2,level+1);
					count++;
				}
			}
		}
	}
}

int main(int argc, char **argv)
{
	// Big buffer since some of the state saving stuff
	// for the web is really large.
	char fmt_buf[16384 + 32];
	BinderNode::property prop,prop2;

//	if (argc < 3) return 3;
//	BApplication("application/x-vnd.Be-bindertool");
	binder_node node = BinderNode::Root();
	if (!node->IsValid()) {
		printf("Could not connect to binder!\n");
		return -1;
	}

	prop = node;

	char *lastP = NULL;
	char *p,*p2,path[2048];
	char realPath[1024];
	char lastPropName[1024];
	if ((argc > 2) && (argv[2][0] != 0)) {
		strcpy(path,argv[2]);
		p=p2=path;
		realPath[0] = 0;
		bool end=false;
		while (!end) {
			if (lastP) {
				unslash(lastPropName, lastP);
				prop = prop[lastPropName];
				strcat(realPath," / ");
				strcat(realPath,lastPropName);
			}
			// Find the start of the next path component.
			while ((*p == '/') && (*p != 0)) p++;
			p2 = p;
			//propIndex = 0;
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
			strcat(realPath," / ");
			strcat(realPath,lastPropName);
		}
	}
	
	if (!prop->IsValid()) {
		printf("Could not traverse binder to path '%s'!\n",realPath);
		return -1;
	}

	if ((argc < 2) || !strcmp(argv[1],"dir")) {
		void *cookie;
		int32 len = 1024;
		BString name;
		if (lastP && lastP[0]) {
			prop = prop[lastPropName];
		}
		if (prop.IsUndefined()) {
			printf("Could not traverse binder to path '%s'!\n",realPath);
			return -1;
		} else if (!prop.IsObject()) {
			printf("Path '%s' does not end in an object!\n",realPath);
			return -1;
		}
		BinderNode::iterator i = prop->Properties();
		while ((name = i.Next()).String() != "") {
			printf("    '%s' --> ",name.String());
			if (prop->GetProperty(name.String(),prop2) == B_OK) {
				format_property(fmt_buf, prop2);
				printf("%s\n", fmt_buf);
			} else {
				printf("unknown!\n");
			}
		}
	}
	else if ((argc < 2) || !strcmp(argv[1],"rdir")) {
		if (lastP && lastP[0]) {
			prop = prop[lastPropName];
		}
		if (prop.IsUndefined()) {
			printf("Could not traverse binder to path '%s'!\n",realPath);
			return -1;
		} else if (!prop.IsObject()) {
			printf("Path '%s' does not end in an object!\n",realPath);
			return -1;
		}
		printf("'%s' --> ",argv[2]);
		printf("'%s'\n",prop.String().String());
		recursive_dir(prop,1);	
	} else if (!strcmp(argv[1],"get")) {
		BinderNode::property *theArgs=NULL;
		BinderNode::property_list args;
		int32 argCount = argc-3;
		if (argCount < 0) argCount = 0;
		if (argCount) {
			theArgs = new BinderNode::property[argCount];
			for (int32 i=0;i<argCount;i++) {
				theArgs[i] = argv[i+3];
				args.AddItem(&theArgs[i]);
			}
		}

		prop->GetProperty(lastPropName,prop2,args);
		printf("%s\n",prop2.String().String());
		delete [] theArgs;
	} else if (!strcmp(argv[1],"put") && (argc == 4 || argc == 5)) {
		put_status_t r;
		if (argc == 5) { // type specified
			if (strcmp(argv[4], "number") == 0)
				r = prop->PutProperty(lastPropName, strtod(argv[3], (char **)NULL));
			else if (strcmp(argv[4], "string") == 0)
				r = prop->PutProperty(lastPropName, argv[3]);
			else if (strcmp(argv[4], "undefined") == 0)
				r = prop->PutProperty(lastPropName, BinderNode::property::undefined);
			else {
				printf("binder: error: unknown type '%s' for value '%s'\n",
					argv[4], argv[3]);
				return -1;
			}
		} else {
			r = prop->PutProperty(lastPropName,argv[3]);
		}
		
		if (r.error) {
			printf("%s\n",strerror(r.error));
		}
	} else if (!strcmp(argv[1],"sync")) {
//		node->Sync();
	} else {
		printf(	"Usage:\n"
				"  binder [command] [node] [value] [param ...]\n"
				"Commands:\n"
				"  dir  [node]             - shows content of a node\n"
				"  rdir [node]             - shows content of a node as a treeview\n"
				"  get   node [param ...]  - get and show value of a node\n"
				"  put   node value [type] - put value into a node\n"
				"                            type is optional: {number,string,undefined}\n"
				"                            undefined will delete property, ignoring value\n"
		);
	}
	node = NULL;
	return 0;
}
