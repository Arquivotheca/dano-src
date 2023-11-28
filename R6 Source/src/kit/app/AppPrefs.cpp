// a settings file class.
// Copyright (c) 1999-2000, Be Incoporated. All Rights Reserved.

#include <AppPrefs.h>

#include <string.h>
#include <stdlib.h>
#include <Path.h>
#include <File.h>
#include <Directory.h>
#include <fs_attr.h>
#include <stdio.h>
#include <errno.h>
#include <Roster.h>
#include <Application.h>

#define M_SIGNATURE	"BPRF00\n"

struct _PrivateAppPrefs {
	BPath path;
	status_t check;
};

#define _p _p_AppPrefs

BAppPrefs::BAppPrefs(char const*const lname,char const*const bname,const directory_which d) {
	_CtorInit(lname,bname,d);
}

BAppPrefs::BAppPrefs(const BMessage&m,char const*const lname,char const*const bname,const directory_which d):BMessage(m) {
	_CtorInit(lname,bname,d);
}

void BAppPrefs::_CtorInit(char const*const lname,char const*const bname,const directory_which d) {
	_p=new _PrivateAppPrefs;
	_p->check=SetTo(lname,bname,d);
}

BAppPrefs::BAppPrefs(const BAppPrefs&source):BMessage(source) {
	_p->check=source._p->check;
	_p->path=source._p->path;
}

BAppPrefs&BAppPrefs::operator=(const BAppPrefs&source) {
	this->BMessage::operator=(source);
	_p->check=source._p->check;
	_p->path=source._p->path;
	return *this;
}

BAppPrefs::~BAppPrefs() {
	delete _p;
}

status_t BAppPrefs::InitCheck() const {
	return _p->check;
}

status_t BAppPrefs::SetTo(char const*const lname,char const*const bname,const directory_which d) {
	status_t check;

	if ((check=find_directory(d,&_p->path))!=B_OK) return check;

	char _sig[B_MIME_TYPE_LENGTH]="";
	char*sig=_sig;
	app_info ai;
	int plen=strlen("application/x-vnd.");

	if ((bname==NULL) || (!strncmp(bname,"application/x-vnd.",plen))) { // no base name, or signature-basename, try to figure it out from the signature
		if (bname==NULL) { // no base name
			if ((check=be_app->GetAppInfo(&ai))!=B_OK)
				return check;
			strcpy(sig,ai.signature);
		} else { // signature-basename
			strncpy(sig,bname,B_MIME_TYPE_LENGTH);
			sig[B_MIME_TYPE_LENGTH-1]='\0';
		}
		if (strncmp(sig,"application/x-vnd.",plen)) {
			plen=strlen("application/");
			if (strncmp(sig,"application/",plen)) {
				check=B_BAD_VALUE;
				return check; // the signature is really broken. bail out.
			}
		}
		sig+=plen;
	}

	if (*sig!='\0') {
		bool founddot=false;
		char*sep;
		while ((sep=strchr(sig,'.'))!=NULL) { // replace each '.' by a '/' in the signature to build a relative path
			*sep='/';
			founddot=true;
		}
		if (!founddot&&((sep=strchr(sig,'-'))!=NULL)) { // no '.' was found. replace the first '-' by a '/', if there's a '-'
			*sep='/';
		}
		if ((check=_p->path.Append(sig))!=B_OK) { _p->path.Unset();return check; }
	} else {
		if ((check=_p->path.Append(bname))!=B_OK) { _p->path.Unset();return check; }
	}

	if (lname==NULL) {
		if ((check=_p->path.Append("Settings"))!=B_OK) { _p->path.Unset();return check; }
	} else {
		if ((check=_p->path.Append(lname))!=B_OK) { _p->path.Unset();return check; }
	}
	return check;
}

const char*BAppPrefs::Path() const {
	return _p->path.Path();
}

status_t BAppPrefs::Load(const uint32) {
	status_t ret;
	BFile file(_p->path.Path(),B_READ_ONLY);
	ret=file.InitCheck();
	if (ret!=B_OK) {
		return ret;
	}
	ret=file.Lock();
	if (ret!=B_OK) {
		return ret;
	}

	// Read the file signature.
	char *signature[8];
	if (file.Read(signature, 8) != 8)
	{
		file.Unlock();
		return B_IO_ERROR;
	}
	
	// TODO: We should verify that the signature is good

	ret=Unflatten(&file);
	if (ret!=B_OK) {
		file.Unlock();
		MakeEmpty();
		return ret;
	}
	ret=file.RewindAttrs();
	if (ret!=B_OK) {
		file.Unlock();
		MakeEmpty();
		return ret;
	}
	char attr_name[B_ATTR_NAME_LENGTH];
	while ((ret=file.GetNextAttrName(attr_name))!=B_ENTRY_NOT_FOUND) { // walk all the attributes of the settings file
		if (ret!=B_OK) {
			file.Unlock();
			return ret;
		}
			// found an attribute
		attr_info ai;
		ret=file.GetAttrInfo(attr_name,&ai);
		if (ret!=B_OK) {
			file.Unlock();
			return ret;
		}
		if (_SupportsType(ai.type)) {
			char*partial_name=strdup(attr_name);
			if (partial_name==NULL) {
				file.Unlock();
				return B_NO_MEMORY;
			}
			ret=_ExtractAttribute(this,&file,attr_name,partial_name,&ai);
			free(partial_name);
			if (ret!=B_OK) {
				file.Unlock();
				return ret;
			}
		}		
	}
	file.Unlock();
	return B_OK;
}

status_t BAppPrefs::_ExtractAttribute(BMessage*const m,BFile const*const f,char const*const full_name,char const*const partial_name,attr_info const*const ai) {
	status_t ret;
	char*end=strchr(partial_name,':');
	if (end==NULL) { // found a leaf
		if (!m->HasData(partial_name,ai->type)) { // the name does not exist in the message - ignore it
			return B_OK;
		}
		void* buffer=malloc(ai->size);
		if (buffer==NULL) { // cannot allocate space to hold the data
			return B_NO_MEMORY;
		}
		if (f->ReadAttr(full_name,ai->type,0,buffer,ai->size)!=ai->size) { // cannot read the data
			free(buffer);
			return B_IO_ERROR;
		}
		ret=m->ReplaceData(partial_name,ai->type,buffer,ai->size);
		if (ret!=B_OK) { // cannot replace the data
			free(buffer);
			return ret;
		}
		free(buffer);
		return B_OK;
	}
	if (end[1]!=':') { // found an un-numbered sub-message
		*(end++)='\0'; // zero-terminate the name, point to the rest of the sub-string
		if (!m->HasMessage(partial_name)) { // archived message does not contain that entry. go away.
			return B_OK;
		}
		BMessage subm;
		ret=m->FindMessage(partial_name,&subm); // extract the sub-message
		if (ret!=B_OK) {
			return ret;
		}
		ret=_ExtractAttribute(&subm,f,full_name,end,ai); // keep processing
		if (ret!=B_OK) {
			return ret;
		}
		ret=m->ReplaceMessage(partial_name,&subm); // replace the sub-message
		if (ret!=B_OK) {
			return ret;
		}
		return B_OK;
	} else { // found a numbered entry
		char* endptr;
		errno=0;
		*end='\0'; // zero-terminate the name
		int32 r=strtol(end+2,&endptr,10); // get the entry number
		if (errno!=0) {
			return B_OK;
		}
		if (r>=1000000000) { // sanity-check.
			return B_OK;
		}
		if (*endptr==':') { // this is a numbered message
			if (!m->HasMessage(partial_name,r)) { // archived message does not contain that entry, go away
				return B_OK;
			}
			BMessage subm;
			ret=m->FindMessage(partial_name,r,&subm); // extract the sub-message
			if (ret!=B_OK) {
				return ret;
			}
			ret=_ExtractAttribute(&subm,f,full_name,endptr+1,ai); // recurse
			if (ret!=B_OK) {
				return ret;
			}
			ret=m->ReplaceMessage(partial_name,r,&subm); // replace the sub-message
			if (ret!=B_OK) {
				return ret;
			}
			return B_OK;
		} else if (*endptr=='\0') { // this is a numbered leaf
			if (!m->HasData(partial_name,ai->type,r)) { // archived message does not contain this leaf
				return B_OK;
			}
			void* buffer=malloc(ai->size);
			if (buffer==NULL) {
				return B_NO_MEMORY;
			}
			if (f->ReadAttr(full_name,ai->type,0,buffer,ai->size)!=ai->size) { // extract the attribute data
				free(buffer);
				return B_IO_ERROR;
			}
			ret=m->ReplaceData(partial_name,ai->type,r,buffer,ai->size); // and replace it in the message
			if (ret!=B_OK) {
				free(buffer);
				return ret;
			}
			free(buffer);
			return B_OK;
		}
	}
	return B_OK;
}

status_t BAppPrefs::Save(const uint32) const {
	status_t ret;
	BFile file(_p->path.Path(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	ret=file.InitCheck();
	if (ret==B_BAD_VALUE) { // try to create the parent directory if creating the file fails the first time
		BPath parent;
		ret=_p->path.GetParent(&parent);
		if (ret!=B_OK) {
			return ret;
		}
		ret=create_directory(parent.Path(),0777);
		if (ret!=B_OK) {
			return ret;
		}
		ret=file.SetTo(_p->path.Path(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	}
	if (ret!=B_OK) {
		return ret;
	}
	ret=file.Lock(); // lock the file to do atomic attribute transactions on it.
	if (ret!=B_OK) {
		return ret;
	}

	// Add a signature to the setting file
	const char *kSignature = M_SIGNATURE;
	if (file.Write(kSignature, 8) != 8)
	{
		file.Unlock();
		return B_IO_ERROR;
	}

	ret=Flatten(&file);
	if (ret!=B_OK) {
		file.Unlock();
		return ret;
	}
	ret=_StoreAttributes(this,&file);
	if (ret!=B_OK) {
		file.Unlock();
		return ret;
	}
	file.Unlock();
	return B_OK;
}

status_t BAppPrefs::_StoreAttributes(BMessage const*const m,BFile*const f,char const*const basename) {
	const char* namefound;
	type_code typefound;
	int32 countfound;
	status_t ret;
	for (int32 i=0;i<m->CountNames(B_ANY_TYPE);i++) { // walk the entries in the message
		ret=m->GetInfo(B_ANY_TYPE,i,&namefound,&typefound,&countfound);
		if (ret!=B_OK) {
			return ret;
		}
		if (strchr(namefound,':')!=NULL) { // do not process anything that contains a colon (considered a magic char)
			break;
		}
		if (typefound==B_MESSAGE_TYPE) {
			if (countfound==1) { // single sub-message
				char* lname=(char*)malloc(strlen(basename)+strlen(namefound)+2); // allocate space for the base name
				if (lname==NULL) {
					return B_NO_MEMORY;
				}
				sprintf(lname,"%s%s:",basename,namefound); // create the base name for the sub-message
				BMessage subm;
				ret=m->FindMessage(namefound,&subm);
				if (ret!=B_OK) {
					free(lname);
					return ret;
				}
				ret=_StoreAttributes(&subm,f,lname); // and process the sub-message with the base name
				if (ret!=B_OK) {
					free(lname);
					return ret;
				}
				free(lname);
			} else if (countfound<1000000000) { // (useless in 32-bit) sanity check
				char* lname=(char*)malloc(strlen(basename)+strlen(namefound)+11); // allocate space for the base name
				if (lname==NULL) {
					return B_NO_MEMORY;
				}
				sprintf(lname,"%ld",countfound-1); // find the length of the biggest number for that field
				char format[12];
				sprintf(format,"%%s%%s::%%0%ldld:",strlen(lname)); // create the sprintf format
				for (int32 j=0;j<countfound;j++) {
					sprintf(lname,format,basename,namefound,j); // create the base name for the sub-message
					BMessage subm;
					ret=m->FindMessage(namefound,j,&subm);
					if (ret!=B_OK) {
						free(lname);
						return ret;
					}
					ret=_StoreAttributes(&subm,f,lname); // process the sub-message with the base name
					if (ret!=B_OK) {
						free(lname);
						return ret;
					}
				}
				free(lname);
			}
		} else if (_SupportsType(typefound)) {
			if (countfound==1) {
				char* lname=(char*)malloc(strlen(basename)+strlen(namefound)+1);
				if (lname==NULL) {
					return B_NO_MEMORY;
				}
				sprintf(lname,"%s%s",basename,namefound);
				const void* datafound;
				ssize_t sizefound;
				ret=m->FindData(namefound,typefound,&datafound,&sizefound);
				if (ret!=B_OK) {
					free(lname);
					return ret;
				}
				sizefound=f->WriteAttr(lname,typefound,0,datafound,sizefound);
				if (sizefound<0) {
					free(lname);
					return sizefound;
				}
				free(lname);
			} else if (countfound<1000000000) {
				char* lname=(char*)malloc(strlen(basename)+strlen(namefound)+10);
				if (lname==NULL) {
					return B_NO_MEMORY;
				}
				sprintf(lname,"%ld",countfound-1);
				char format[12];
				sprintf(format,"%%s%%s::%%0%ldld",strlen(lname));
				for (int32 j=0;j<countfound;j++) {
					sprintf(lname,format,basename,namefound,j);
					const void* datafound;
					ssize_t sizefound;
					ret=m->FindData(namefound,typefound,j,&datafound,&sizefound);
					if (ret!=B_OK) {
						free(lname);
						return ret;
					}
					sizefound=f->WriteAttr(lname,typefound,0,datafound,sizefound);
					if (sizefound<0) {
						free(lname);
						return sizefound;
					}
				}
				free(lname);
			}
		}
	}
	return B_OK;
}

bool BAppPrefs::_SupportsType(const type_code type) {
	switch(type) {
		case B_CHAR_TYPE :
		case B_STRING_TYPE :
		case B_BOOL_TYPE :
		case B_INT8_TYPE :
		case B_INT16_TYPE :
		case B_INT32_TYPE :
		case B_INT64_TYPE :
		case B_UINT8_TYPE :
		case B_UINT16_TYPE :
		case B_UINT32_TYPE :
		case B_UINT64_TYPE :
		case B_FLOAT_TYPE :
		case B_DOUBLE_TYPE :
		case B_OFF_T_TYPE :
		case B_SIZE_T_TYPE :
		case B_SSIZE_T_TYPE :
		case B_POINT_TYPE :
		case B_RECT_TYPE :
		case B_RGB_COLOR_TYPE :
		case B_TIME_TYPE :
		case B_MIME_TYPE : {
			return true;
		}
		default : {
			return false;
		}
	}
}

status_t BAppPrefs::Perform(const perform_code,void*) {
	return B_BAD_VALUE;
}

void BAppPrefs::_ReservedAppPrefs1() {}
void BAppPrefs::_ReservedAppPrefs2() {}
void BAppPrefs::_ReservedAppPrefs3() {}
void BAppPrefs::_ReservedAppPrefs4() {}
void BAppPrefs::_ReservedAppPrefs5() {}
void BAppPrefs::_ReservedAppPrefs6() {}
void BAppPrefs::_ReservedAppPrefs7() {}
void BAppPrefs::_ReservedAppPrefs8() {}
void BAppPrefs::_ReservedAppPrefs9() {}
void BAppPrefs::_ReservedAppPrefs10() {}
