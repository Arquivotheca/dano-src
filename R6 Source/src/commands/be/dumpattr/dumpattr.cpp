#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <fs_attr.h>
#include <ByteOrder.h>
#include <Node.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <Directory.h>
#include <Point.h>

char *prefix = NULL;

enum tok 
{
	tEOF, tNUMBER, tSTRING, tOPENBRACE, tCLOSEBRACE, 
	tFILE, tATTR
};

tok curtok;
uint32 curval; /* length of curtok == tDATA */
char *curstr; /* only valid if curtok == tDATA */
char *in;
int curline;
char *curfile;


int check = 0;
int tracker_only = 1;
int create_if_needed = 0;

void err(char *msg)
{
	fprintf(stderr,"%s:%d: %s\n",curfile,curline,msg);
}

tok NextTok()
{
	char *p;
	int l;
	uint32 n;
	
	while(isspace(*in)) {
		if(*in == '\n') curline++;
		in++;
	}
	
	switch(*in){
	case 0:
		return curtok = tEOF;
	case '"':
		l = 0;
		in++;
		curstr = in;
		p = (char *) in;
		while(*in){
			switch(*in){
			case '\\':
				in++;
				/* escaped octal value ? */
				if(isdigit(*in)){
					n = *in - '0';
					in++;
					if(isdigit(*in)) {
						n = (n * 8) + (*in - '0');
						in++;
						if(isdigit(*in)){
							n = (n * 8) + (*in - '0');
							in++;
						}
					}
					*((unsigned char *)p) = n;
					p++;
					l++;
					break;
				}
				/* otherwise a simple escape */
				if(*in){
					*p = *in;
					in++;
					p++;
					l++;
				}
				break;
			case '"':
				*in = 0;
				in++;
				curval = l;
				return curtok = tSTRING;
			default:
				*p = *in;
				p++;
				in++;
				l++;
			}
		}
		curval = l;
		return curtok = tSTRING;
		
	case '{':
		in++;
		return curtok = tOPENBRACE;

	case '}':
		in++;
		return curtok = tCLOSEBRACE;

	default:
		p = in;
		while(*in && !isspace(*in)){
			in++;
		}
		if(*in) {
			*in = 0;
			in++;
		}
		if(!strcmp(p,"file")) return curtok = tFILE;
		if(!strcmp(p,"attr")) return curtok = tATTR;
		curval = strtoul(p,NULL,16);
		return curtok = tNUMBER;
	}	
}


static void 
HexStringDump(const void *buf, long length, const char *hdr, const int kLineWidth)
{
	unsigned char *buffer = (unsigned char *)buf;
	
	for (int32 index = 0; index < length; ) {
		printf("%s",hdr);
		for (int32 count = 4 + strlen(hdr); count < kLineWidth; ) {
			printf(" %02x",buffer[index]);
			count += 3;
			if (++index >= length)
				break;
		}
		if(index < length) {
			printf("\n");
		}
	}
}

static void
EscapedStringDump(const void *buf, long length, const char *hdr, const int kLineWidth)
{
	unsigned char *buffer = (unsigned char *)buf;
	
	for (int32 index = 0; index < length; ) {
		printf("%s \"",hdr);
		for (int32 count = 6 + strlen(hdr); count < kLineWidth; ) {
			if (buffer[index] >= ' ' && buffer[index] <= 127) {
				switch(buffer[index]){
				case '"':
					printf("\\\"");
					count += 2;
					break;
				case '\\':
					printf("\\\\");
					count += 2;
					break;
				default:
					printf("%c", buffer[index]);
					count++;
				}
			} else {
				unsigned char ch = buffer[index];
				printf("\\%c%c%c", '0' + (ch / 64), '0' + ((ch / 8) & 7),
					'0' + (ch & 7));
				count += 4;
			}
			if (++index >= length)
				break;
		}
		if(index < length) {
			printf("\"\n");
		} else {
			printf("\"");
		}
		
	}
}

unsigned char abuf[8192];


/* ------- from tracker -------- */
class PoseInfo {
public:
	static void EndianSwap(void *castToThis);
	void PrintToStream();
	
	bool invisible;
	ino_t inited_dir;
	BPoint location;	
};
#define	ATTR_POSE_INFO_be				"_trk/pinfo"
#define	ATTR_POSE_INFO_le				"_trk/pinfo_le"
/* ----------------------------- */

void ApplyAttr(BNode &node, BEntry &entry, char *name, uint32 type, int len, void *data)
{
	if(!strcmp(ATTR_POSE_INFO_be,name)){
		/* fixup inode info first */
		entry_ref ref;
		PoseInfo *pi = (PoseInfo *) data;
		if(len != sizeof(PoseInfo)) {
			fprintf(stderr,"EEEK!\n");
			exit(0);
		}
		
		entry.GetRef(&ref);
		pi->inited_dir = B_HOST_TO_BENDIAN_INT64(ref.directory);
#if 1
		printf("be %s: (%f,%f)\n",ref.name,
		B_BENDIAN_TO_HOST_FLOAT(pi->location.x),B_BENDIAN_TO_HOST_FLOAT(pi->location.y));
#endif
	}
	if(!strcmp(ATTR_POSE_INFO_le,name)){
		/* fixup inode info first */
		entry_ref ref;
		PoseInfo *pi = (PoseInfo *) data;
		if(len != sizeof(PoseInfo)) {
			fprintf(stderr,"EEEK!\n");
			exit(0);
		}
		
		entry.GetRef(&ref);
		pi->inited_dir = B_HOST_TO_LENDIAN_INT64(ref.directory);
#if 1
		printf("le %s: (%f,%f)\n",ref.name,
		B_LENDIAN_TO_HOST_FLOAT(pi->location.x),B_LENDIAN_TO_HOST_FLOAT(pi->location.y));
#endif
	}
	
	if(node.WriteAttr(name, type, 0, data, len) != len) {
		fprintf(stderr,"error: cannot write attr \"%s\"\n",name);
	}
}

char realname[8192];

void Parse(char *file,char *buf)
{
	char *aname;
	int alen;
	uint32 atype;
	BNode node;
	BEntry entry;
	int i, skip;
	
	in = buf;
	curline = 1;
	curfile = file;
	
	for(;;){
		NextTok();
		if(curtok == tEOF) return;
		if(curtok != tFILE){
			err("Expected FILE identifier");	
			return;
		}
		if(NextTok() != tSTRING) {
			err("Missing <name> after FILE");
			return;
		}
		if(check) {
			EscapedStringDump(curstr, curval, "file", 1000000);
			printf(" {\n");
		}	
		
		if(prefix){
			sprintf(realname,"%s/%s",prefix,curstr);
		} else {
			strcpy(realname,curstr);
		}
		
		node.SetTo((char*)realname);
		status_t error = node.InitCheck();
		if (error != B_OK && create_if_needed) {
			fprintf(stderr,"don't have %s yet, creating new\n",realname);
			BFile tmp(realname, O_CREAT);
			node = tmp;
			error = node.InitCheck();
		}
		if (error != B_OK){
			skip = 1;
			fprintf(stderr,"warning: target \"%s\" does not exist, %s\n",
				realname, strerror(error));
		} else {
			entry.SetTo((char *)realname);
			if(entry.InitCheck() != B_OK){
				fprintf(stderr,"error: could not get entry of \"%s\"\n",realname);
				skip = 1;
			} else {
				skip = 0;
			}
		}
		
		
		if(NextTok() != tOPENBRACE) {
			err("Missing '{' after FILE <name>");
			return;
		}
		while(NextTok() == tATTR){
			if(NextTok() != tSTRING){
				err("Missing name for ATTR");
				return;
			}
			aname = curstr;
			if(NextTok() != tNUMBER){
				err("Missing type for ATTR");
				return;
			}
			atype = curval;
			if(NextTok() != tNUMBER){
				err("Missing size for ATTR");
				return;
			}
			alen = curval;
			i = 0;
			if(NextTok() != tOPENBRACE){
				err("Missing opening '{' for ATTR");
				return;
			}
			while(i < alen){
				if(NextTok() != tNUMBER){
					err("Short ATTR data block");
					return;
				}
				abuf[i] = curval;
				i++;
			}	
			if(NextTok() != tCLOSEBRACE){
				err("Missing closing '}' for ATTR");
				return;
			}	
			if(!skip) ApplyAttr(node, entry, aname, atype, alen, abuf);
			if(check) {
				EscapedStringDump(aname, strlen(aname), "  attr", 1000000);
				printf(" %08x %x {\n", atype, alen);
				HexStringDump(abuf, alen, "   ", 79);
				printf("\n  }\n");		
			}
		}
		if(curtok != tCLOSEBRACE){
			err("Missing closing '}' for FILE");
			return;
		}
		if(check) printf ("}\n");
	}
}

status_t Restore(char *path)
{
	int sz;
	char *buf;
	struct stat info;
	BEntry entry(path, true);
	BFile file(path,B_READ_ONLY);
	status_t error;
	
	if((error = file.InitCheck()) != B_OK) {
		fprintf(stderr,"error %s opening %s\n", strerror(error), path);
		return error;
	}
	if((error = entry.InitCheck()) != B_OK) {
		fprintf(stderr,"error %s opening %s\n", strerror(error), path);
		return error;
	}

	if((error = entry.GetStat(&info)) != B_OK){
		fprintf(stderr,"error %s stating %s\n", strerror(error), path);
		return error;
	}

	buf = (char *) malloc(info.st_size) + 1;
	file.Read((void*)buf,(size_t)info.st_size);
	buf[info.st_size] = 0;
	Parse(path,buf);
	
	return B_OK;
}


static void
DumpOneAttr(BNode *node, const char *attrName)
{
	attr_info info;

	if(tracker_only && strncmp(attrName,"_trk",4) != 0)
		return;
	
	node->GetAttrInfo(attrName, &info);	
	EscapedStringDump(attrName, strlen(attrName), "  attr", 1000000);
	printf(" %08x %x {\n", info.type, ((int)info.size));
	char *buffer = new char [info.size+1];
	node->ReadAttr(attrName, info.type, 0, buffer, info.size);
	buffer[info.size]=0;
	HexStringDump(buffer, info.size, "   ", 79);
	
	delete [] buffer;
	printf("\n  }\n");
}

static status_t
DumpOneFile(const char *path)
{
	BNode file(path);
	status_t error = file.InitCheck();
	if (error != B_OK) {
		fprintf(stderr,"error %s opening %s\n", strerror(error), path);
		return error;
	}
	if(prefix && !strncmp(prefix,path,strlen(prefix))){
		path += strlen(prefix);
	}
	
	EscapedStringDump(path, strlen(path), "file", 1000000);
	printf(" {\n");
	file.RewindAttrs();

	char attrName[256];
	while (file.GetNextAttrName(attrName) == B_OK)
		DumpOneAttr(&file, attrName);
	
	printf ("}\n", path);

	return B_OK;
}

void walkpath(const char *path)
{
	BDirectory dir(path);
	if(dir.InitCheck() == B_OK){
		BEntry entry;
		DumpOneFile(path);
		while(dir.GetNextEntry(&entry,false) >= 0) {
			BPath name;
			entry.GetPath(&name);
			//fprintf(stderr,"[%s]\n",name.Path());
			if(entry.IsDirectory()) {
				walkpath(name.Path());
			} else {
				DumpOneFile(name.Path());
			}
		}
	}
}
          
int
main(int argc, char **argv)
{
	int dump = 1;
	int recur = 0;
	
	for (++argv; *argv; argv++) {
		if(*argv[0] == '-'){
			if(!strcmp(*argv,"-p")){
				argv++;
				if(*argv) {
					prefix=*argv;
				} else {
					argv--;
				}				
				continue;
			}
			if(!strcmp(*argv,"-a")){
				tracker_only = 0;
				continue;
			}	
			if(!strcmp(*argv,"-f")){
				dump = 0;
				/* restoring */
				continue;
			}
			if(!strcmp(*argv,"-t")){
				create_if_needed = true;
				/* create if needed */
				continue;
			}
			if(!strcmp(*argv,"-r")){
				recur = 1;
				continue;
			}
			continue;
		}
		if(dump){
			if(recur) walkpath(*argv);
			else DumpOneFile(*argv);
		} else {
			Restore(*argv);
		}		
	}
	return 0;
}
