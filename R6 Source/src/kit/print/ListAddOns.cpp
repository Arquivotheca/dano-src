#include <Application.h>
#include <Roster.h>
#include <Directory.h>
#include <Path.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ListAddOns.h"

char* BPrivate::ListAddOns(char* p) {
	char* names=NULL;
	const char* apath;
	apath=getenv("ADDON_PATH");
	if (apath==NULL) {
		return NULL;
	}
	char* mapath=new char[strlen(apath)+1];
	strcpy(mapath,apath);
	char* tok=mapath;
	char* ttok;
	while ((tok=strtok_r(tok,":",&ttok))!=NULL) {
		char* parsed=parse_add_on_path(tok);
		if (parsed)
		{
			char* extend;
			if ((p==NULL)||(*p=='\0')) {
				extend=new char[strlen(parsed)+1];
				strcpy(extend,parsed);
			} else {
				extend=new char[strlen(parsed)+strlen(p)+2];
				strcpy(extend,parsed);
				if (*p!='\0') {
					strcat(extend,"/");
					strcat(extend,p);
				}
			}
			add_names(extend,p,&names);
			delete extend;
			delete parsed;
			if (tok!=mapath) {
				tok[-1]=':';
			}
		}
		tok=NULL;
	}
	delete[] mapath;
	return names;
}


char* BPrivate::parse_add_on_path(char* path) {
	if (!strncmp(path,"%A",2)) {
		if (be_app == NULL)	// We don't have a BApplication
			return NULL;
		
		app_info ai;
		if (be_app->GetAppInfo(&ai) != B_OK) // The roster is not responding ?!
			return NULL;

		BEntry ent(&ai.ref);
		BPath pth(&ent);
		
		if (pth.InitCheck() != B_OK)	// something gone wrong
			return NULL;
		
		char* ret = new char[strlen(path+2)+strlen(pth.Path())+1];
		strcpy(ret, pth.Path());
		*strrchr(ret,'/')='\0';
		strcat(ret,path+2);
		return ret;		
	} else {
		char* ret=new char[strlen(path)+1];
		strcpy(ret,path);
		return ret;
	}
}

void BPrivate::add_names(const char* src,const char* prepend,char** dst) {
	BDirectory dir(src);
	if (dir.InitCheck()!=B_OK) {
		return;
	}
	BEntry entry;
	while (dir.GetNextEntry(&entry)==B_OK) {
		BPath path(&entry);
		if ((path.InitCheck()==B_OK)&&(path.Leaf()!=NULL)) {
			char* fname;
			if ((prepend==NULL)||(*prepend=='\0')) {
				fname=new char[strlen(path.Leaf())+1];
				strcpy(fname,path.Leaf());
			} else {
				fname=new char[strlen(prepend)+strlen(path.Leaf())+2];
				strcpy(fname,prepend);
				strcat(fname,"/");
				strcat(fname,path.Leaf());
			}
			add_unique_name(fname,dst);
			delete fname;
		}
	}
}

void BPrivate::add_unique_name(const char* name,char** dst) {
	if (*dst==NULL) {
		*dst=new char[strlen(name)+1];
		strcpy(*dst,name);
	} else {
		bool dup=false;
		char* tok=*dst;
		char* ttok;
		while((tok=strtok_r(tok,":",&ttok))!=NULL) {
			if (!strcmp(name,tok)) {
				dup=true;
			}
			if (tok!=*dst) {
				tok[-1]=':';
			}
			tok=NULL;
		}
		if (!dup) {
			char* ret=new char[strlen(*dst)+strlen(name)+1+1];
			strcpy(ret,*dst);
			strcat(ret,":");
			strcat(ret,name);
			delete *dst;
			*dst=ret;
		}
	}
}


