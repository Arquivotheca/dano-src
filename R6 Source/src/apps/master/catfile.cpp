#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <kernel/OS.h>
#include <errno.h>
#include <fcntl.h>
#include <fs_attr.h>
#include <image.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <AppDefs.h>
#include <File.h>
#include <List.h>
#include <Node.h>
#include <SymLink.h>
#include <TypeConstants.h>

#include "catfile.h"

static int
catdata(FILE *f,BNode *node,const char *fn, bool &isElf)
{
	isElf = false;
	if (node->IsDirectory()) {
 		fprintf(f,"<directory>\n");
		return 0;
	}

	if (node->IsSymLink()) {
		char to[B_PATH_NAME_LENGTH+1];
		BSymLink sym(fn);

		sym.ReadLink(to,B_PATH_NAME_LENGTH);
		fprintf(f,"<symbolic link> -> %s\n",to);
		return 0;
	}

	BFile file(fn,B_READ_ONLY);
	int len;
	bool first = true;
	unsigned char buf[65536];

	while ((len = file.Read(buf,65536)) > 0) {
		if (first) {
			if ((buf[0] == 127) &&
				(buf[1] == 'E') &&
				(buf[2] == 'L') &&
				(buf[3] == 'F'))
				isElf = true;
			first = false;
		}
		fwrite(buf,len,1,f);
	}
	fprintf(f,"\n");
	return 0;
}

static int
catattr(FILE *f,BNode *node,char *attr_name)
{
	status_t err;
	char *attr_val;
	uint32  ival;
	uint64  llval;
	float   fval;
	double  dval;
	bool    bval;
	attr_info ai;

	if (node->GetAttrInfo(attr_name,&ai) != B_OK) {
		// fprintf(stderr, "%s: file %s attribute %s: %s\n", argv[0],
		// 			fname, attr_name, strerror(errno));
		return 1;
	}

	fprintf(f,"%s : ", attr_name);

	if (ai.type == B_INT32_TYPE) {
		err = node->ReadAttr(attr_name, ai.type, 0, &ival, sizeof(int32));
		fprintf(f,"int32 : %ld\n", ival);
	} else if (ai.type == B_INT64_TYPE) {
		err = node->ReadAttr(attr_name, ai.type, 0, &llval, sizeof(int32));
		fprintf(f,"int64 : %Ld\n", llval);
	} else if (ai.type == B_BOOL_TYPE) {
		err = node->ReadAttr(attr_name, ai.type, 0, &bval, sizeof(int32));
		fprintf(f,"bool : %u\n", bval);
	} else if (ai.type == B_FLOAT_TYPE) {
		err = node->ReadAttr(attr_name, ai.type, 0, &fval, sizeof(int32));
		fprintf(f,"float : %f\n", fval);
	} else if (ai.type == B_DOUBLE_TYPE) {
		err = node->ReadAttr(attr_name, ai.type, 0, &dval, sizeof(int32));
		fprintf(f,"double : %f\n", dval);
	} else if (ai.type == B_STRING_TYPE || ai.type == B_ASCII_TYPE || 
		   	ai.type == 0x4d494d53) {
		attr_val = (char *)malloc(ai.size);
		err = node->ReadAttr(attr_name, ai.type, 0, attr_val, ai.size);
		fprintf(f,"string :\n");
		fwrite((const unsigned char *)attr_val, ai.size,1,f);
		fprintf(f,"\n");
		free(attr_val);
	} else {
		fprintf(f,"raw_data :\n");
		attr_val = (char *)malloc(ai.size);
		err = node->ReadAttr(attr_name, ai.type, 0, attr_val, ai.size);
		fwrite((const unsigned char *)attr_val, ai.size,1,f);
		fprintf(f,"\n");
		free(attr_val);
	}

	if (err < 0) {
		// fprintf(stderr, "%s: file %s : error reading attribute %s: %s\n",
		//		argv[0], fname, attr_name, strerror(errno));
		return 1;
	}
	return 0;
}

static int
CompareString(const void *_a,const void *_b)
{
	char *a = *((char **)_a);
	char *b = *((char **)_b);

	return(strcmp(a,b));
}
	   
static char *attrs_to_kill[] = {
	"_stat/name",
	"_stat/size",
	"_stat/modified",
	"_stat/created",
	"_trk/path",
	"_trk/original_path",
	"_trk/app_version",
	"_trk/system_version",
	"_trk/open_with_relation",
	"_trk/windframe",
	"_trk/windwkspc",
	"_trk/qrystr",
	"_trk/qryvol1",
	"BEOS:TYPE",
	"BEOS:APP_SIG",
	"BEOS:PREF_APP",
	"BEOS:L:STD_ICON",
	"BEOS:M:STD_ICON",
	"_trk/d_windframe",
	"_trk/d_windwkspc",
	"_trk/_windows_to_open_",
	"_trk/_clipping_file_",
	"_trk/qryinitmode",
	"_trk/qryinitstr",
	"_trk/qryinitnumattrs",
	"_trk/qryinitattrs",
	"_trk/qryinitmime",
	"_trk/qrylastchange",
	"_trk/qrymoreoptions_le",
	"_trk/qrymoreoptions",
	"_trk/queryTemplate",
	"_trk/queryTemplateName",
	"_trk/queryDynamicDate",
	"_trk/pinfo",
	"_trk/pinfo_le",
	"_trk/d_pinfo",
	"_trk/d_pinfo_le",
	"_trk/columns",
	"_trk/columns_le",
	"_trk/viewstate",
	"_trk/viewstate_le",
	"_trk/d_viewstate",
	"_trk/d_viewstate_le",
	"_trk/d_columns",
	"_trk/d_columns_le",
	"_trk/xtpinfo",
	"_trk/xtpinfo_le",
	"_trk/xt_d_pinfo",
	"_trk/xt_d_pinfo_le",
	NULL
};

static bool
strip_attr(BNode *,char *name)
{
 int i;

 for (i = 0;attrs_to_kill[i];i++) {
 	if (!strcmp(name,attrs_to_kill[i])) {
		return true;
	}
 }
 return false;
}

int
catfile(FILE *f,const char *path, bool &isElf)
{
	long			index;
	status_t		err;
	struct stat		st;
	BNode			node;
	char	attr_name[256];

	node.SetTo(path);
	err = node.GetStat(&st);

	catdata(f,&node,path,isElf);
	if (!node.IsSymLink()) {
		BList attrs;
		index = 0;
		while (node.GetNextAttrName(attr_name) == B_OK) {
			attrs.AddItem(strdup(attr_name));
		}

		attrs.SortItems(CompareString);
		for (index = 0;index < attrs.CountItems();index++) {
			char *s = (char *)attrs.ItemAt(index);
			if (strip_attr(&node,s) == false) {
				catattr(f,&node,s);
			}
			free(s);
		}
	}
	return 0;
}
