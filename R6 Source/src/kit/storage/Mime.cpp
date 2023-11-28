/***************************************************************************

	File : Mime.cpp

	$Revision: 1.48 $

	Written by: Peter Potrebic

	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

****************************************************************************/

#include <algorithm>
#include <Debug.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fs_attr.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <alloca.h>

#include <OS.h>
#include <Drivers.h>
#include <Mime.h>
#include <File.h>
#include <Directory.h>
#include <Application.h>
#include <AppFileInfo.h>
#include <FindDirectory.h>
#include <Bitmap.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>

#include <MimeSniffer.h>
#include <roster_private.h>
#include <private/storage/mime_private.h>

using namespace BPrivate;

const char		*B_SUPPORTED_MIME_ENTRY		= "types";
const char		*B_SUPPORTING_APPS_ENTRY	= "applications";
const char		*B_FILE_EXTENSIONS_ENTRY	= "extensions";
const char	 	*B_RESOURCE_MIME_TYPE		= "application/x-be-resource";
const char	 	*B_PEF_APP_MIME_TYPE		= "application/x-be-executable";
const char	 	*B_PE_APP_MIME_TYPE			= "application/x-vnd.Be-peexecutable";
const char	 	*B_ELF_APP_MIME_TYPE		= "application/x-vnd.Be-elfexecutable";
const char		*B_FILE_MIME_TYPE			= "application/octet-stream";

const char		*B_URL_HTTP 				= "application/x-vnd.Be.URL.http";
const char		*B_URL_HTTPS 				= "application/x-vnd.Be.URL.https";
const char		*B_URL_FTP					= "application/x-vnd.Be.URL.ftp";
const char		*B_URL_GOPHER 				= "application/x-vnd.Be.URL.gopher";
const char		*B_URL_MAILTO 				= "application/x-vnd.Be.URL.mailto";
const char		*B_URL_NEWS					= "application/x-vnd.Be.URL.news";
const char		*B_URL_NNTP					= "application/x-vnd.Be.URL.nntp";
const char		*B_URL_TELNET 				= "application/x-vnd.Be.URL.telnet";
const char		*B_URL_RLOGIN 				= "application/x-vnd.Be.URL.rlogin";
const char		*B_URL_TN3270 				= "application/x-vnd.Be.URL.tn3270";
const char		*B_URL_WAIS					= "application/x-vnd.Be.URL.wais";
const char		*B_URL_FILE					= "application/x-vnd.Be.URL.file";

#if __ELF__
const char	 	*B_APP_MIME_TYPE			= B_ELF_APP_MIME_TYPE;
#else
const char	 	*B_APP_MIME_TYPE			= B_PEF_APP_MIME_TYPE;
#endif

const char		*B_MIME_TYPE_ATTR			= "BEOS:TYPE";
const char		*B_APP_SIGNATURE_ATTR		= "BEOS:APP_SIG";
const char		*B_PREFERRED_APP_ATTR		= "BEOS:PREF_APP";
const char		*B_PREFERRED_PATH_ATTR		= "BEOS:PPATH";

const char		*APP_FLAGS_ATTR				= "BEOS:APP_FLAGS";
const char		*APP_VERSION_ATTR			= "BEOS:APP_VERSION";
const char		*APP_SUPPORTED_TYPES_ATTR	= "BEOS:FILE_TYPES";

const int32		RSRC_ID						= 1;
const int32		RESERVED_ICON_ID			= 101;
const char		*MIME_PREFIX				= "BEOS:";
const char		*META_PREFIX				= "META:";
const char		*STD_ICON_SUFFIX			= "STD_ICON";

const uint32	APP_FLAGS_TYPE				= 'APPF';
const uint32	APP_VERSION_TYPE			= 'APPV';
const uint32	LARGE_ICON_TYPE				= 'ICON';
const uint32	MINI_ICON_TYPE				= 'MICN';

/*
 The following  constants are used for storing information in the meta-mime
 file.
*/
const char		*META_PREFERRED_APP			= "META:PREF_APP";
const char		*META_ATTR_INFO				= "META:ATTR_INFO";
const char		*META_EXTENSION_INFO		= "META:EXTENS";
const char		*META_SHORT_DESC			= "META:S:DESC";
const char		*META_LONG_DESC				= "META:L:DESC";
//+const char		*META_CACHED_REF			= "META:REF";
const char		*META_CACHED_REF			= "META:PPATH";
const char		*META_SUPPORTED_TYPES_ATTR	= "META:FILE_TYPES";
const char		*META_TYPE_ATTR				= "META:TYPE";
const char		*META_SNIFFER_RULE_ATTR		= "META:SNIFF_RULE";
const uint32	META_PREFERRED_APP_TYPE		= 'MSIG';
const uint32	META_SHORT_DESC_TYPE		= 'MSDC';
const uint32	META_LONG_DESC_TYPE			= 'MLDC';
const uint32	META_CACHED_REF_TYPE		= 'MPTH';
//+const uint32	META_CACHED_REF_TYPE		= 'MREF';

const long		LARGE_ICON_SIZE				= 32;
const long		MINI_ICON_SIZE				= 16;

const char		*META_MIME_ROOT				= "beos_mime";

/* ---------------------------------------------------------------- */

// Support for talking directly or remotely to the roster

static bool _localDispatch = false;
static BMessenger _localDispatchTarget;
	// target for sending indirect mime change calls

static void (*_rosterSendMimeChanged)(BMessage *);
	// use a function pointer for this callback to avoid having
	// to link against the roster

void 
BMimeType::_set_local_dispatch_target_(BMessenger *mess,
	 void (*mimeChangedCallback)(BMessage *))
{
	_localDispatch = true;
	_localDispatchTarget = *mess;
	_rosterSendMimeChanged = mimeChangedCallback;
}

static status_t 
SendToRoster(BMessage *msg, BMessage *reply)
{
	if (_localDispatch) {
		if (reply)
			return _localDispatchTarget.SendMessage(msg, reply, 0, 0);
		else
			return _localDispatchTarget.SendMessage(msg, (BHandler *)NULL, 0);
	}
	
	ASSERT(_is_valid_roster_mess_(true));
	return _send_to_roster_(msg, reply, true);
}

status_t create_app_meta_mime(const char *path, int recurse, int sync,
	int force)
{
	status_t	err;
	sem_id		sem = B_BAD_SEM_ID;

//+	PRINT(("------ CREATE_APP_META_MIME ----- (%s)\n", path ? path : "ALL"));

	BMessage	msg(CMD_UPDATE_MIME_INFO);

	msg.AddBool("app_meta_mime", true);

	// recursive option from a starting path isn't supported for this function.
	if (path && recurse)
		return B_BAD_VALUE;

	if (path) {
		// need to turn this into a full path
		BEntry	e(path);
		BPath	fullp;
		if ((err = e.InitCheck()) != B_OK)
			return err;
		if ((err = e.GetPath(&fullp)) != B_OK)
			return err;
		msg.AddString("path", path);
	} else
		msg.AddBool("all", true);

	if (recurse || !path) {
		msg.AddBool("recurse", true);

		if (sync) {
			// create a semaphore. If this team is killed (e.g. ctl-C)
			// before the registrar is finished it will notice that fact
			// because this 'hangup' semphore will be delted by the system.
			sem = create_sem(0, "mime_set_hangup");
			msg.AddInt32("hangup_sem", sem);
		}
	}

	if (force)
		msg.AddBool("force", true);

	if (sync) {
		BMessage reply;
		err = SendToRoster(&msg, &reply);
	} else 
		err = SendToRoster(&msg, NULL);
	
	if (sem != B_BAD_SEM_ID)
		delete_sem(sem);

	return err;
}

/* ---------------------------------------------------------------- */

int update_mime_info(const char *path, int recurse, int sync, int force)
{
	status_t	err;
	sem_id		sem = B_BAD_SEM_ID;

//+	PRINT(("------ UPDATE ----- (%s)\n", path ? path : "ALL"));

	BMessage	msg(CMD_UPDATE_MIME_INFO);

	if (path) {
		// need to turn this into a full path
		BEntry	e(path);
		BPath	fullp;
		if ((err = e.InitCheck()) != B_OK)
			return err;
		if ((err = e.GetPath(&fullp)) != B_OK)
			return err;
		msg.AddString("path", fullp.Path());
	} else
		msg.AddBool("all", true);

	if (recurse || !path) {
		msg.AddBool("recurse", true);
		if (sync) {
			// create a semaphore. If this team is killed (e.g. ctl-C)
			// before the registrar is finished it will notice that fact
			// because this 'hangup' semphore will be delted by the system.
			sem = create_sem(0, "mime_set_hangup");
			msg.AddInt32("hangup_sem", sem);
		}
	}

	if (force)
		msg.AddInt32("force", force);

	if (sync) {
		BMessage reply;
		err = SendToRoster(&msg, &reply);
	} else
		err = SendToRoster(&msg, NULL);


	if (sem != B_BAD_SEM_ID)
		delete_sem(sem);

	return err;
}

/* ---------------------------------------------------------------- */

status_t get_device_icon(const char *device, void *icon, int32 size)
{
	int32		dev;
	status_t	result;
	device_icon	dev_icon;

	if ((dev = open(device, O_RDONLY | O_CLOEXEC)) >= 0) {
		dev_icon.icon_size = size;
		dev_icon.icon_data = icon;
		result = ioctl(dev, B_GET_ICON, &dev_icon);
		close(dev);
	}
	else
		result = dev;

	return result;
}

/* ---------------------------------------------------------------- */

char *tolower_str(char *str)
{
	char *p = str;

	while (*p) {
		*p = (char) tolower(*p);
		p++;
	}

	return str;
}

/* ---------------------------------------------------------------- */

static void build_meta_mime_path(const char *real_type, BPath *p)
{
	char	mt[B_MIME_TYPE_LENGTH];
	
	find_directory(_MIME_PARENT_DIR_, p);

	p->Append(META_MIME_ROOT);

	// remember that meta-mime files are all lower case to help ensure
	// the mime types are case insensitive.
	strcpy(mt, real_type);
	p->Append(tolower_str(mt));
}

/* ---------------------------------------------------------------- */


static status_t DoRemote(int32 cmd, const char *type, BMessage *msg = NULL);

static status_t DoRemote(int32 cmd, const char *type, BMessage *msg)
{
	status_t result;
	BMessage reply;
	
	if (!type)
		return B_BAD_VALUE;

	if (!msg)
		msg = &reply;
	msg->AddString("type",type);
	msg->what = cmd;	

	result = SendToRoster(msg, &reply);
	if (result!= B_OK) 
		return result;

	if (reply.FindInt32("result",&result) != B_OK)
		return B_ERROR;

	return result;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::StartWatching(BMessenger target)
{
	return be_roster->_StartWatching(BRoster::MIME_MESSENGER, NULL,
		CMD_MONITOR_META_MIME, target, 0xFFFFFFFF);
}

/* ---------------------------------------------------------------- */

status_t BMimeType::StopWatching(BMessenger target)
{
	return be_roster->_StopWatching(BRoster::MIME_MESSENGER, NULL,
		CMD_MONITOR_META_MIME, target);
}


BMimeType::BMimeType()
{
	InitData(NULL);
}

/* ---------------------------------------------------------------- */

BMimeType::BMimeType(const char *mime_type)
{
	InitData(mime_type);
}

/* ---------------------------------------------------------------- */

void BMimeType::InitData(const char *mime_type)
{
	fMeta = NULL;
	fType = NULL;
	SetTo(mime_type);
	fWhere = B_USE_ATTRIBUTES;
}

/* ---------------------------------------------------------------- */

BMimeType::~BMimeType()
{
	free(fType);
	fType = NULL;
	CloseFile();
}

/* ---------------------------------------------------------------- */

status_t BMimeType::InitCheck(void) const
{
  return fCStatus;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::SetType(const char *mime_type)
{
  return SetTo(mime_type);
}

/* ---------------------------------------------------------------- */

//XXXbjs huh?
status_t BMimeType::SetTo(const char *mime_type)
{
	CloseFile();

	free(fType);
	fType = NULL;

	if (!mime_type)
	  return fCStatus = B_NO_INIT;

	if (!IsValid(mime_type))
		return fCStatus=B_BAD_VALUE;

	fType = strdup(mime_type);

	return (fCStatus = (fType) ? B_OK : B_NO_MEMORY);
}

/* ---------------------------------------------------------------- */

void BMimeType::Unset(void)
{
	SetTo(NULL);
}

/* ---------------------------------------------------------------- */

bool BMimeType::Contains(const BMimeType *mt) const
{
	// if either isn't valid then the types aren't equal
	if (!IsValid() || !mt->IsValid())
		return false;

	// if types are exactly equal then return true
	if (*this == (*mt))
		return true;
	
	// If 'this' isn't a super type then we know it can't 'contain' mt.
	// If mt is a super type we can infer that it can't be contained by this.
	if (!IsSupertypeOnly() || mt->IsSupertypeOnly())
		return false;

	BMimeType	super;
	if (mt->GetSupertype(&super) != B_OK) {
		return false;
	}

	return (*this == super);
}

/* ---------------------------------------------------------------- */

bool BMimeType::operator==(const BMimeType &mt) const
{
	// if <this> isn't valid then the types aren't equal
	// or if mt is empty
	if (!IsValid() || !mt.Type())
		return false;

	return strcasecmp(fType, mt.Type()) == 0;
}

/* ---------------------------------------------------------------- */

bool BMimeType::operator==(const char *type) const
{
	// if <this> isn't valid then the types aren't equal
	// or if mt is empty
	if (!IsValid() || !type)
		return false;

	return strcasecmp(fType, type) == 0;
}

/* ---------------------------------------------------------------- */

bool BMimeType::IsSupertypeOnly() const
{
	/*
	 Is the char* (fType) in the correct format?
	 proper syntax is:
	 	type
	*/

	bool	v;

	if (!fType)
		v = false;
	else {
		int	l =  strlen(fType);
		char *p = strchr(fType, '/');
		if ((l == 0) || p)
			v = false;
		else 
			v = true;
	}

	return v;
}

/* ---------------------------------------------------------------- */

static bool valid_string(const char *type)
{
	// 	illegal chars: /\<>@,;:"()[]?= and <space>

	for (const uchar *tmp = (const uchar *)type; *tmp; tmp++) {
		if (!isprint(*tmp) || *tmp >= 0x80)
			// dont allow escape and upper ascii chars
			return false;
	}
	return (type && (strlen(type) > 0) &&
			strpbrk(type, "/\\<>@,;:\"()[]?= ") == NULL);
}

/* ---------------------------------------------------------------- */

bool BMimeType::IsValid() const
{
	return IsValid(fType);
}

/* ---------------------------------------------------------------- */

bool BMimeType::IsValid(const char *type)
{
	/*
	 Is the char* (type) in the correct format?
	 proper syntax is:
	 	type[/subtype]*

	 	illegal chars: /\<>@,;:"()[]?= and <space>
	*/

	bool	v;

	if (!type)
		v = false;
	else {
		int		len = strlen(type);
		const char	*p = strchr(type, '/');
		if ((len > B_MIME_TYPE_LENGTH) || (len == 0) || (p == type)) {
			v = false;
		} else {
			if (!p) {
				v = valid_string(type);
			} else {
				const char *sub = p+1;
#if __GNUC__	/* FIXME: We should be able to use alloca with current GNU tools */
				char *head = (char*)malloc(p-type+1);
				if (!head)
					v = false;
				else {
					strncpy(head, type, p-type);
					head[p-type] = 0;
					v = valid_string(head) && valid_string(sub);
					free (head);
				}
#else
				char *head = (char*)alloca(p-type+1);
				strncpy(head, type, p-type);
				head[p-type] = 0;
				v = valid_string(head) && valid_string(sub);
#endif
			}
		}
	}

//+	PRINT(("IsValid(%s) = %d\n", type, v));

	return v;
}

/* ---------------------------------------------------------------- */

bool BMimeType::IsInstalled() const
{
	if (!IsValid())
		return false;

	BPath	p;
	build_meta_mime_path(fType, &p);

	BEntry	entry(p.Path());
	struct stat	st;
	status_t	err = entry.GetStat(&st);
//+	SERIAL_PRINT(("IsInstalled(%s) = %x (type=%s)\n", p.Path(), err, fType));
	return (err == B_OK);
}

/* ---------------------------------------------------------------- */

const char *BMimeType::Type() const
{
	return fType;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetSupertype(BMimeType *base) const
{
	if (!IsValid() || IsSupertypeOnly()) {
		base->SetTo(NULL);
		return B_BAD_VALUE;
	}

	char *p = strchr(fType, '/');

	// temporarily replace the first '/' as init a BMimeType obj with that type
	*p = '\0';
	base->SetTo(fType);
	*p = '/';

	return B_OK;
}

/* ---------------------------------------------------------------- */

/*
 When the underlying META MIME file is opened or closed is an implementation
 detail. That's why I made the OpenFile and CloseFile methods const.
*/

status_t BMimeType::CloseFile() const
{
	if (fMeta) {
		BMimeType	*THIS;
		THIS = const_cast<BMimeType*>(this);

		delete THIS->fMeta;
		THIS->fMeta = NULL;
	}

	return B_OK;
}

/* ---------------------------------------------------------------- */
status_t BMimeType::OpenFile(bool create_file, dev_t) const
{
	bool		is_supertype;
	status_t	err;
	BEntry		entry;
	bool		new_meta = false;
	BPath		pp;

	// if the file is already open
	if (fMeta)
		return B_OK;

	if (!IsValid())
		return B_BAD_VALUE;

	build_meta_mime_path(Type(), &pp);

//+	SERIAL_PRINT(("OpenFile(%s) (type=%s)\n", pp.Path(), fType));

	is_supertype = IsSupertypeOnly();

	BMimeType	*THIS;
	THIS = const_cast<BMimeType*>(this);

	err = get_ref_for_path(pp.Path(), &(THIS->fRef));

	if (err != B_OK) {
		if (!_localDispatch) {
			BMessage msg;
			msg.AddBool("create",create_file);
			err = DoRemote(CMD_MIMETYPE_CREATE, fType, &msg);
			if (err == B_OK)
				err = get_ref_for_path(pp.Path(), &(THIS->fRef));
		} else {
			BPath path;
			find_directory(_MIME_PARENT_DIR_, &path, true);
			err = mkdir(path.Path(), 0755);
			path.Append(META_MIME_ROOT);
			err = mkdir(path.Path(), 0755);
		
			if (!is_supertype) {
				BMimeType	media;
				GetSupertype(&media);
				// make sure this supertype is installed
					media.Install();
//+				path.Append(media.Type());
//+				err = mkdir(path.Path(), 0755);
			}

			err = get_ref_for_path(pp.Path(), &(THIS->fRef));
		}
	}
	
	if (err != B_OK)
		goto done;

	// Doing this entry business so that I get a real error code if
	// opening the file fails.

	err = entry.SetTo(&(THIS->fRef));
	struct stat st;
	err = entry.GetStat(&st);

	if (err != B_OK) {
		// the file doesn't exist! (only if local)
		if (!_localDispatch){
			BMessage msg;
			msg.AddBool("create",create_file);
			err = DoRemote(CMD_MIMETYPE_CREATE, fType, &msg);
			if (err == B_OK) {
				err = get_ref_for_path(pp.Path(), &(THIS->fRef));
				if (err == B_OK) {
					THIS->fMeta = new BFile;
					err = fMeta->SetTo(&entry, O_RDONLY);
				}
			}
			if (err == B_OK) 
				goto done;
		} else {
			if (!create_file) 
				goto done;

			THIS->fMeta = new BFile;

			node_ref node;
			node.device = fRef.device;
			node.node = fRef.directory;
			BDirectory dir(&node);
			if (!is_supertype) {
				err = dir.CreateFile(fRef.name, THIS->fMeta);
			} else {
				// for supertypes we need to create a directory
				BDirectory	newdir;
				err = dir.CreateDirectory(fRef.name, &newdir);
				err = fMeta->SetTo(&entry, O_RDWR);
			}

			BNodeInfo	ninfo(fMeta);
			ninfo.SetType(META_FILE_MIME_TYPE);

			MimeChanged(B_MIME_TYPE_CREATED);

			new_meta = true;
		}
	} else {
		THIS->fMeta = new BFile;
		err = fMeta->SetTo(&entry, O_RDWR);
	}

	if (err != B_OK) {
		delete THIS->fMeta;
		THIS->fMeta = NULL;
	}

	if (err == B_OK) {
		char	mt[B_MIME_TYPE_LENGTH];

		if (new_meta || (get_data(fWhere, fMeta, NULL, META_TYPE_ATTR, 0, 
			B_STRING_TYPE, mt, B_MIME_TYPE_LENGTH) != B_OK)) {
			/*
			 We've created a new meta MIME file. (or current meta-mime
			 didn't have a META:TYPE attribute for some undefined reason)
			 Save the "real" type in the META:TYPE attribute.
			 We want every such meta-mime to have a 'short description'
			 attribute.
			*/

			if(!_localDispatch)
				return err; /* the roster handled this before */
				
			set_data(fWhere, fMeta, NULL, META_TYPE_ATTR, 0,
				B_STRING_TYPE, fType, strlen(fType) + 1);
			
			BMessage msg(CMD_NEW_MIME_TYPE);
			msg.AddString("type", fType);
			SendToRoster(&msg, NULL);

		} else {
			/*
			 Let's get the cononical mime type (i.e. with the correct
			 capitalization.
			 If the capitalization doesn't match then update
			*/
			if (strcmp(mt, fType) != 0) {
				// Can't call SetTo() because that closes the file!!!
				if (fType) {
					free(fType);
					THIS->fType = NULL;
				}
				THIS->fType = strdup(mt);
				THIS->fCStatus = (fType) ? B_OK : B_NO_MEMORY;
			}
		}
	} else {
		SERIAL_PRINT(("Error in Open = %x (%s)\n", err, strerror(err)));
	}

done:
	return err;
}


/* ---------------------------------------------------------------- */

status_t BMimeType::Delete()
{
	if (!IsValid())
		return B_BAD_VALUE;

	if (!_localDispatch) 
		return DoRemote(CMD_MIMETYPE_DELETE,fType);
	
	status_t err = OpenFile();
	if (err != B_OK)
		return err;

	bool supertypeOnly = IsSupertypeOnly();
	if (supertypeOnly) {
		// have to recursively delete all types of this supertype first
		BMessage subtypes;
		GetInstalledTypes(Type(), &subtypes);

		int32 count;
		uint32 type;
		subtypes.GetInfo(B_SUPPORTED_MIME_ENTRY, &type, &count);
		
		for (int32 index = 0; index < count; index++) {
			const char *mimeTypeName;
			subtypes.FindString(B_SUPPORTED_MIME_ENTRY, index, &mimeTypeName);
			ASSERT(mimeTypeName);
			BMimeType subtype(mimeTypeName);
			PRINT(("deleting subtype %s\n", mimeTypeName));
			subtype.Delete();
		}
	}

	MimeChanged(B_MIME_TYPE_DELETED);

	BMessage old_extens;
	bool has_old_extens = (GetFileExtensions(&old_extens) == B_OK);

	CloseFile();

	BPath	p;
	build_meta_mime_path(fType, &p);

	if (supertypeOnly) {
		char buffer[2048];
		sprintf(buffer, "rm -rf \'%s\'", p.Path());
		err = system(buffer);
			// this is apparently the best way of doing a recursive remove
	} else
		err = unlink(p.Path());

	if (err != B_OK) {
		PRINT(("error %s unlinking %s\n", strerror(err), p.Path()));
		return err;
	}

	// ??? Section below only makes sense for app_signatures, not file
	// types. No real harm is done, it just wastes a little time.
	// let's also send message to roster... To have it delete all refs
	// to this 'signature' from the SupportingApps table
	BMessage	msg(CMD_UPDATE_SUPPORTING_APPS);
	msg.AddString("sig", Type());
	msg.AddBool("force", true);			// force everything to get deleted.
	SendToRoster(&msg, NULL);

	if (has_old_extens) {
		// let's also send message to roster...
		BMessage	msg(CMD_UPDATE_FILE_EXTENSION);
		msg.AddMessage("old", &old_extens);
		msg.AddString("type", Type());

		SendToRoster(&msg, NULL);
	}

	return B_OK;
}

/* ---------------------------------------------------------------- */

void 
BMimeType::_touch_()
{
	if (!IsInstalled() && OpenFile(false) == B_OK)
		CloseFile();
}

/* ---------------------------------------------------------------- */

status_t BMimeType::Install()
{
	status_t	err = B_OK;
	
	// If it isn't already installed...
	if (!IsInstalled()) {
		err = OpenFile(true);
		if (err == B_OK)
			CloseFile();
	}
	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetIcon(BBitmap *icon, icon_size which) const
{
	return GetIconForType(NULL, icon, which);
}

/* ---------------------------------------------------------------- */

status_t BMimeType::SetIcon(const BBitmap *icon, icon_size which)
{
	return SetIconForType(NULL, icon, which);
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetPreferredApp(char *sig, app_verb) const
{
	status_t err = OpenFile();
	if (err != B_OK)
		return err;

//+	SERIAL_PRINT(("GetPreferredApp(%s) (node=%x)\n", fType, fMeta));

	err = get_data(fWhere, fMeta, NULL,META_PREFERRED_APP, RSRC_ID,
		META_PREFERRED_APP_TYPE, sig, B_MIME_TYPE_LENGTH);

//+	SERIAL_PRINT(("		PreferredApp=%s (%x)\n", sig, err));
	return err;
}

/* ---------------------------------------------------------------- */

bool _is_valid_app_sig_(const char *sig)
{
	const char *prefix = "application/";
	int32	prefix_len = 12;	// precalc strlen of 'prefix' for performance

	ASSERT(prefix_len == (int32)strlen(prefix));

	if (!sig)
		return false;

	if (strncasecmp(sig, prefix, prefix_len) != 0)
		return false;
	
	// We know that 'sig' starts with the prefix. So skip that and look
	// for other 'bad' application sigs
	const char *suffix = sig + prefix_len;

	if (strcasecmp(suffix, "x-be-executable") == 0)
		return false;
	if (strcasecmp(suffix, "x-vnd.Be-executable") == 0)
		return false;
	if (strcasecmp(suffix, "x-be-resource") == 0)
		return false;
	if (strcasecmp(suffix, "octet-stream") == 0)
		return false;
	return true;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::SetPreferredApp(const char *sig, app_verb)
{
//+	PRINT(("tid=%d, type=%s, sig=%s\n", find_thread(NULL), fType, sig));
//+	SERIAL_PRINT(("tid=%d, type=%s, sig=%s\n", find_thread(NULL), fType, sig));

	if (sig && !_is_valid_app_sig_(sig)) {
		PRINT(("SetPrefApp: BAD APP SIG (%s)\n", sig));
		return B_BAD_VALUE;
	}

	if (!_localDispatch) {
		BMessage msg;
		if (sig)
			msg.AddString("signature", sig);
		return DoRemote(CMD_MIMETYPE_SET_PREFERRED_APP, fType, &msg);
	}
	
	status_t err = OpenFile(true);
	if (err != B_OK)
		return err;

	int len = sig ? (strlen(sig) + 1) : 0;
	if (len > B_MIME_TYPE_LENGTH)
		return B_BAD_VALUE;

	if (err == B_OK)
		err = set_data(fWhere, fMeta, NULL,META_PREFERRED_APP, RSRC_ID,
			META_PREFERRED_APP_TYPE, sig, len);

	if (err == B_OK)
		MimeChanged(B_PREFERRED_APP_CHANGED);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetAttrInfo(BMessage *ainfo) const
{
	/*
 	 The format of this message is more or less arbitrary. Initially it
 	 will be the MIME type pref app and the Tracker that will be using this
 	 info. They can agree on whatever format they like. Here is a suggestion,
 	 using a fictious 'People' mime type that might want 3 fields to be
 	 associated with each Person:

 		entry_name				type		element[0]	element[1]	element[2]
		------------------		------		---------	---------	---------
		attr:name				STRING		email		homeph		workph
		attr:public_name		STRING		E-mail		Home Phone	Work Phone
		attr:type				LONG		B_STRING	B_STRING	B_STRING
		attr:public				BOOL		true		true		true
		attr:editable			BOOL		true		true		true
		attr:extra				BOOL		true		false		false
		extra:email				<whatever>
		
		The MIME prefs app and Tracker (and any other app for that matter)
		can add additional fields and interpret them as desired. The only
		constraint is that the number of elements in each entry must
		remain the same.
 	*/

	status_t err = OpenFile();
	if (err != B_OK)
		return err;

	return get_msg_data(fWhere, fMeta, NULL, META_ATTR_INFO,
		B_MESSAGE_TYPE, ainfo);
}

/* ---------------------------------------------------------------- */

status_t BMimeType::SetAttrInfo(const BMessage *ainfo)
{
	if (!_localDispatch) {
		BMessage msg(*ainfo);
		return DoRemote(CMD_MIMETYPE_SET_ATTR_INFO, fType, &msg);
	}	
	
	status_t err = OpenFile(true);
	if (err != B_OK)
		return err;

	err = set_msg_data(fWhere, fMeta, NULL, META_ATTR_INFO,
		B_MESSAGE_TYPE, ainfo);

	if (err == B_OK)
		MimeChanged(B_ATTR_INFO_CHANGED);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetFileExtensions(BMessage *extensions) const
{
	/*
	 The message should contain 1 entry named B_FILE_EXTENSIONS_ENTRY 
	 ("extensions"). This entry should contain an array of file extension
	 strings. An empty message is legal. It means that the app doesn't
	 want any files with a given extension.
 	*/

	status_t err = OpenFile();
	if (err != B_OK)
		return err;

	return get_msg_data(fWhere, fMeta, NULL, META_EXTENSION_INFO,
		B_MESSAGE_TYPE, extensions);
}

/* ---------------------------------------------------------------- */

status_t BMimeType::SetFileExtensions(const BMessage *extensions)
{
	/*
	 The message should contain 1 entry named B_FILE_EXTENSIONS_ENTRY 
	 ("extensions"). This entry should contain an array of file extension
	 strings. An empty message is legal. It means that the app doesn't
	 want any files with a given extension.
 	*/

	if (!_localDispatch) {
		BMessage msg(*extensions);
		return DoRemote(CMD_MIMETYPE_SET_FILE_EXTENSIONS, fType, &msg);
	}	
	
	status_t err = OpenFile(true);
	if (err != B_OK)
		return err;

	BMessage old;
	bool has_old = (GetFileExtensions(&old) == B_OK);

	err = set_msg_data(fWhere, fMeta, NULL, META_EXTENSION_INFO,
		B_MESSAGE_TYPE, extensions);

	if (err == B_OK) {
		// let's also send message to roster...
		BMessage msg(CMD_UPDATE_FILE_EXTENSION);
		if (has_old)
			msg.AddMessage("old", &old);
		if (extensions) 
			msg.AddMessage("new", extensions);

		msg.AddString("type", Type());
		SendToRoster(&msg, NULL);
	}
	if (err == B_OK)
		MimeChanged(B_FILE_EXTENSIONS_CHANGED);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetShortDescription(char *desc) const
{
	status_t err = OpenFile();
	if (err != B_OK)
		return err;

	err = get_data(fWhere, fMeta, NULL, META_SHORT_DESC, RSRC_ID,
		META_SHORT_DESC_TYPE, desc, B_MIME_TYPE_LENGTH);
	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::SetShortDescription(const char *desc)
{
	if (!_localDispatch) {
		BMessage msg;
		if (desc)
			msg.AddString("descr",desc);
		return DoRemote(CMD_MIMETYPE_SET_SHORT_DESCR,fType,&msg);
	}
	
	int len = desc ? (strlen(desc) + 1) : 0;
	if (len > B_MIME_TYPE_LENGTH)
		return B_BAD_VALUE;

	status_t err = OpenFile(true);
	if (err != B_OK)
		return err;

	err = set_data(fWhere, fMeta, NULL, META_SHORT_DESC, RSRC_ID,
		META_SHORT_DESC_TYPE, desc, len);

	if (err == B_OK)
		MimeChanged(B_SHORT_DESCRIPTION_CHANGED);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetLongDescription(char *desc) const
{
	*desc = '\0';

	status_t err = OpenFile();
	if (err != B_OK)
		return err;

	err = get_data(fWhere, fMeta, NULL, META_LONG_DESC, RSRC_ID, 
		META_LONG_DESC_TYPE, desc, B_MIME_TYPE_LENGTH);
	return err;
}


/* ---------------------------------------------------------------- */

status_t BMimeType::SetLongDescription(const char *desc)
{
	if (!_localDispatch) {
		BMessage msg;
		if (desc)
			msg.AddString("descr", desc);
		return DoRemote(CMD_MIMETYPE_SET_LONG_DESCR, fType, &msg);
	}
	
	int len = desc ? (strlen(desc) + 1) : 0;
	if (len > B_MIME_TYPE_LENGTH)
		return B_BAD_VALUE;

	status_t err = OpenFile(true);
	if (err != B_OK)
		return err;
	
	err = set_data(fWhere, fMeta, NULL, META_LONG_DESC, RSRC_ID,
		META_LONG_DESC_TYPE, desc, len);
	if (err == B_OK)
		MimeChanged(B_LONG_DESCRIPTION_CHANGED);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetIconForType(const char *mime_type, BBitmap *icon,
	icon_size which) const
{
	/*
	 This call should only be used for MetaMime's that are application
	 signatures. This call will determine if that application has a
	 custom icon for the MIME type sepficied by the mime_type arg.
	*/
	float		icon_size;
	status_t	err;
	BRect		rect;
	uint32		type;
	char		rsrc_name[256];
	char		icon_buffer[1024];

	if ((mime_type && !IsValid(mime_type)) || !icon)
		return B_BAD_VALUE;

	if ((err = OpenFile()) != B_OK)
		return err;

	strcpy(rsrc_name, META_PREFIX);

	if (which == B_LARGE_ICON) {
		icon_size = LARGE_ICON_SIZE;
		type = LARGE_ICON_TYPE;
		strcat(rsrc_name, "L:");
	} else {
		icon_size = MINI_ICON_SIZE;
		type = MINI_ICON_TYPE;
		strcat(rsrc_name, "M:");
	}

	rect.Set(0, 0, icon_size - 1, icon_size - 1);

	if (icon->Bounds() != rect)
		return B_BAD_VALUE;

	strcat(rsrc_name, mime_type ? mime_type : STD_ICON_SUFFIX);

	err = get_icon_data(fWhere, fMeta, NULL, rsrc_name, type,icon_buffer);
//+	PRINT(("Try #1: \"%s\" err=%x\n", rsrc_name, err));

	// Code to deal with bug #9550
	if (err == B_ENTRY_NOT_FOUND || err == B_NAME_NOT_FOUND) {
		if (mime_type) {
			// try using lower case'd string for the name
			char    dup[B_MIME_TYPE_LENGTH];
			strcpy(dup, mime_type);
			tolower_str(dup);
			if (strcmp(dup, mime_type) != 0) {
				strcpy(rsrc_name, META_PREFIX);
				strcat(rsrc_name, (which == B_LARGE_ICON) ? "L:" : "M:");
				strcat(rsrc_name, dup);
				err = get_icon_data(fWhere, fMeta, NULL, rsrc_name,
					type, icon_buffer);
//+				PRINT(("Try #2: \"%s\" err=%x\n", rsrc_name, err));
			}

			if (err != B_OK) {
				strcpy(rsrc_name, META_PREFIX);
				strcat(rsrc_name, (which == B_LARGE_ICON) ? "L:" : "M:");

				// try using the conoical mime_type
				char    bogus[B_MIME_TYPE_LENGTH];
				BMimeType mt(mime_type);

				// need to force it to open to get the cononical name
				mt.GetPreferredApp(bogus);

				if (mt.Type()) {
					strcat(rsrc_name, mt.Type());
					err = get_icon_data(fWhere, fMeta, NULL, rsrc_name,
						type, icon_buffer);
//+					PRINT(("BMimeType::GetIconForType(%s), try #3 (err=%x)\n",
//+						rsrc_name, err));
				}
			}
		}									 
	}
	// End of code to deal with bug #9550

	if (err == B_OK)
		icon->SetBits(icon_buffer, icon->BitsLength(), 0, B_COLOR_8_BIT);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::SetIconForType(const char *mime_type, const BBitmap *icon,
	icon_size which)
{
	if (!_localDispatch) {
		BMessage flat;
		BMessage msg;
		if (icon) {
			icon->Archive(&flat);
			msg.AddMessage("icon",&flat);
		}
		msg.AddInt32("which",which);
		if (mime_type)
			msg.AddString("set_type",mime_type);
		return DoRemote(CMD_MIMETYPE_SET_ICON_FOR_TYPE, fType, &msg);
	}
	
	/*
	 Save an association between 'mime_type' and the icon. This BMimeType
	 object is most likely the meta-mime file for some application. In
	 this case we're saving off some icons for mime types that the app
	 supports.
	*/
	float		icon_size;
	BRect		rect;
	uint32		type;
	char		rsrc_name[256];

	if (mime_type && !IsValid(mime_type))
		return B_BAD_VALUE;

	status_t err = OpenFile(true);
	if (err != B_OK)
		return err;

	strcpy(rsrc_name, META_PREFIX);

	if (which == B_LARGE_ICON) {
		icon_size = LARGE_ICON_SIZE;
		type = LARGE_ICON_TYPE;
		strcat(rsrc_name, "L:");
	} else {
		icon_size = MINI_ICON_SIZE;
		type = MINI_ICON_TYPE;
		strcat(rsrc_name, "M:");
	}

	rect.Set(0, 0, icon_size - 1, icon_size - 1);
	if (icon && (icon->Bounds() != rect))
		return B_BAD_VALUE;

	if (mime_type) {
		// write out data using lower case'd string for the name
		char	dup[B_MIME_TYPE_LENGTH];
		strcpy(dup, mime_type);
		tolower_str(dup);
		strcat(rsrc_name, dup);
	} else {
		strcat(rsrc_name, STD_ICON_SUFFIX);
	}

	if (err == B_OK)
		err = set_icon_data(fWhere, fMeta, NULL, rsrc_name, type,
			icon ? icon->Bits() : NULL,
			icon ? icon->BitsLength() : 0);

	if (err == B_OK)
		MimeChanged(mime_type ? B_ICON_FOR_TYPE_CHANGED : B_ICON_CHANGED,
			mime_type, which == B_LARGE_ICON);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetSupportingApps(BMessage *signatures) const
{
	status_t err = OpenFile();
	if (err != B_OK)
		return err;

	BMessage msg(CMD_GET_SUPPORTING_APPS);

	msg.AddString("type", fType);
	err = SendToRoster(&msg, signatures);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetSupportedTypes(BMessage *types)
{
	status_t err = OpenFile();
	if (err != B_OK)
		return err;

	err = get_msg_data(fWhere, fMeta, NULL, META_SUPPORTED_TYPES_ATTR,
		B_MESSAGE_TYPE, types);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::SetSupportedTypes(const BMessage *types)
{
	/*
	 An Application with given signature (this->fType) just indicated that it
	 can handle/understand the mime types (in message types). So let's
	 make sure that all those MIME types are installed, and if they
	 aren't already installed then set their preferred app to 'sig'
	*/
	if (!_localDispatch) {
		BMessage msg(*types);
		return DoRemote(CMD_MIMETYPE_SET_SUPPORTED_TYPES, fType, &msg);
	}	
	
	status_t err = OpenFile();
	if (err != B_OK)
		return err;

	err = set_msg_data(fWhere, fMeta, NULL, META_SUPPORTED_TYPES_ATTR,
		B_MESSAGE_TYPE, types);

	int32		i = 0;
	const char	*m;
	while (types->FindString(B_SUPPORTED_MIME_ENTRY, i++, &m) == B_OK) {
		BMimeType	mime(m);
//+		SERIAL_PRINT(("file type (%s) %d\n", m, mime.IsInstalled()));
		if (!mime.IsInstalled()) {
			mime.Install();
			// a newly created mime type. Set the preferred app to be
			// the app that created this.
			mime.SetPreferredApp(Type());
//+			SERIAL_PRINT(("new type (%s), setting pref to (%s)\n", m, Type()));
		}
	}
	return B_OK;
}

status_t 
BMimeType::GetSnifferRule(BString *result) const
{
#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	status_t err = OpenFile();
	if (err != B_OK)
		return err;

	return get_string_data(fWhere, fMeta, NULL, META_SNIFFER_RULE_ATTR, RSRC_ID,
		 B_STRING_TYPE, result);
#else
	return B_UNSUPPORTED;
#endif
}

status_t 
BMimeType::SetSnifferRule(const char *newRule)
{	
#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	if (newRule && !Sniffer::CheckRule(newRule))
		return B_BAD_MIME_SNIFFER_RULE;
	
	if (!_localDispatch) {
		BMessage message;
		if (newRule)
			message.AddString("rule", newRule);
		return DoRemote(CMD_MIMETYPE_SET_SNIFFER_RULE, fType, &message);
	}
	
	status_t err = OpenFile(true);
	if (err != B_OK)
		return err;

	int32 length = 0;
	if (newRule)
		length = strlen(newRule) + 1;

	err = set_data(fWhere, fMeta, NULL, META_SNIFFER_RULE_ATTR, RSRC_ID,
		B_STRING_TYPE, newRule, length);

	if (err == B_OK) {
		// roster needs to know that the sniffer rule changed
		BMessage msg(CMD_UPDATE_SNIFFER_RULE);
		if (newRule) 
			msg.AddString("rule", newRule);

		msg.AddString("type", Type());
		SendToRoster(&msg, NULL);
	}

	if (err == B_OK)
		MimeChanged(B_SNIFFER_RULE_CHANGED);

	return err;
#else
	return B_UNSUPPORTED;
#endif
}

status_t 
BMimeType::CheckSnifferRule(const char *rule, BString *parseError)
{
#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	if (!Sniffer::CheckRule(rule, parseError))
		return B_BAD_MIME_SNIFFER_RULE;
	
	return B_OK;
#else
	*parseError = "Sniffer rules not supported";
	return B_UNSUPPORTED;
#endif
}

status_t
_GuessMimeTypeCommon(const entry_ref *file, const void *buffer, int32 length,
	const char *filename, BMimeType *resultingMimeType)
{
	BMessage message(CMD_GUESS_MIME_TYPE);
	BMessage reply;
	
	if (file)
		message.AddRef("refs", file);
	else if(filename){
		message.AddString("filename", filename);
	}
	else {
		length = std::min(length, (int32)2048);
		message.AddData("data", B_RAW_TYPE, buffer, length);
	}

	status_t result = SendToRoster(&message, &reply);
	if (result != B_OK)
		return result;

	result = reply.FindInt32("error", &result);
	if (result != B_OK)
		return result;

	const char *type;
	result = reply.FindString("type", &type);
	if (result != B_OK)
		return result;
	
	resultingMimeType->SetTo(type);
	if (!resultingMimeType->IsValid())
		return B_ERROR;

	return B_OK;
}

status_t 
BMimeType::GuessMimeType(const entry_ref *file, BMimeType *resultingMimeType)
{
	if (!file || !resultingMimeType)
		return B_BAD_VALUE;
	
	return _GuessMimeTypeCommon(file, NULL, 0, NULL, resultingMimeType);
}

status_t 
BMimeType::GuessMimeType(const void *buffer, int32 length, BMimeType *result)
{
	if (!buffer || !length || !result)
		return B_BAD_VALUE;
	
	return _GuessMimeTypeCommon(NULL, buffer, length, NULL, result);
}

status_t 
BMimeType::GuessMimeType(const char *filename, BMimeType *result)
{
	if (!filename || !*filename || !result)
		return B_BAD_VALUE;
	
	return _GuessMimeTypeCommon(NULL, NULL, 0, filename, result);
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetAppHint(entry_ref *ref) const
{
	/*
	 This call should only be used for MetaMime's that are application
	 signatures. This call will determine if that application has a
	 custom icon for the MIME type sepficied by the mime_type arg.
	*/
	status_t	err;
	size_t		len;

//+	PRINT(("GetAppHint(%s) %s\n", fType, META_CACHED_REF));
	if (!ref)
		return B_BAD_VALUE;

	if (ref->name)
		free(ref->name);
	ref->name = NULL;
	ref->device = -1;
	ref->directory = -1;

	if ((err = OpenFile()) != B_OK)
		return err;

	if ((err = get_data_size(fWhere, fMeta, NULL, META_CACHED_REF, RSRC_ID,
		 META_CACHED_REF_TYPE, &len)) != B_OK) {
		return err;
	}

	char	*p = (char *) malloc(len);
	if (!p)
		return B_NO_MEMORY;

	if ((err = get_data(fWhere, fMeta, NULL, META_CACHED_REF, RSRC_ID,
		META_CACHED_REF_TYPE, p, len)) != B_OK) {
		free(p);
		return err;
	}
	BEntry	entry(p);
	free(p);

	if ((err = entry.InitCheck()) != B_OK)
		return err;
	
	err = entry.GetRef(ref);
	return err;

}


/* ---------------------------------------------------------------- */

status_t BMimeType::SetAppHint(const entry_ref *ref)
{
//+	PRINT(("SetAppHint(%s) %s\n", fType, META_CACHED_REF));
	if (ref && (ref->name == NULL))
		return B_BAD_VALUE;

	if (!_localDispatch) {
		BMessage msg;
		if (ref)
			msg.AddRef("ref",ref);
		return DoRemote(CMD_MIMETYPE_SET_APP_HINT, fType, &msg);
	}
	

	BPath		path;
	int32		len = 0;
	const char	*p = NULL;

	status_t err = OpenFile(true);
	if (err != B_OK)
		return err;
	
	if (ref) {
		BEntry	entry(ref);

		if ((err = entry.InitCheck()) != B_OK)
			return err;
		if ((err = entry.GetPath(&path)) != B_OK)
			return err;
		p = path.Path();
		len = strlen(p) + 1;
	}

	err = set_data(fWhere, fMeta, NULL, META_CACHED_REF, RSRC_ID,
		META_CACHED_REF_TYPE, p, len);

	if (err == B_OK)
		MimeChanged(B_APP_HINT_CHANGED);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetWildcardApps(BMessage *wild)
{
	BMimeType	mt("application/octet-stream");
	return mt.GetSupportingApps(wild);
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetInstalledTypes(BMessage *types)
{
	return GetInstalledTypes(NULL, types);
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetInstalledTypes(const char *super_type, BMessage *types)
{
	// static member function. Return a list of all the mime types
	// found inside the given super type

	status_t	err;
	BDirectory	dir;
	entry_ref	ref;
	char		mime_name[B_MIME_TYPE_LENGTH];
	char		*s;

	if (super_type && !IsValid(super_type))
		return B_BAD_VALUE;

//+	SERIAL_PRINT(("GetInstalledTypes: super=%s\n", super_type));

	BPath	p;
	find_directory(_MIME_PARENT_DIR_, &p);
	p.Append(META_MIME_ROOT);

	if (super_type) {
		p.Append(super_type);

		strcpy(mime_name, super_type);
		strcat(mime_name, "/");
	} else {
		mime_name[0] = '\0';
	}

	s = mime_name + strlen(mime_name);		// get pointer to end of string

	err = dir.SetTo(p.Path());
	if (err)
		return err;

	while ((err = dir.GetNextRef(&ref)) == B_OK) {
		if (*ref.name == '_')
			continue;

		BNode	node(&ref);
		ssize_t r = node.ReadAttr(META_TYPE_ATTR, B_STRING_TYPE, 0,
			mime_name, sizeof(mime_name));
		if (r < 0) {
			strcat(mime_name, ref.name);
		}

//+		SERIAL_PRINT(("Installed Type: %s\n", mime_name));
		types->AddString("types", mime_name);
		if (dir.Contains(ref.name,B_DIRECTORY_NODE)) {
			GetInstalledTypes(mime_name, types);
		}
		*s = '\0';		// truncate string back to original
	}
	return B_OK;
}

/* ---------------------------------------------------------------- */

status_t BMimeType::GetInstalledSupertypes(BMessage *types)
{
	// static member function. Return a list of all the media types
	// found inside the mime directory structure

	status_t	err;
	BDirectory	dir;
	entry_ref	ref;

	BPath	p;
	find_directory(_MIME_PARENT_DIR_, &p);
	p.Append(META_MIME_ROOT);

	err = dir.SetTo(p.Path());
//+	PRINT(("GetInstalledSuper - %s (%x)\n", p.Path(), err));
	if (err)
		return err;

	while ((err = dir.GetNextRef(&ref)) == B_OK) {
		if (dir.Contains(ref.name,B_DIRECTORY_NODE)) {
//+			PRINT(("	found media type %s\n", ref.name));
			types->AddString("super_types", ref.name);
		}
	}
	return B_OK;
}

/* ---------------------------------------------------------------- */

void BMimeType::MimeChanged(int32 what, const char *type, bool large) const
{
	BMessage message(CMD_META_MIME_CHANGED);
	message.AddInt32("be:which", what);
	message.AddString("be:type", Type());
	if (type) 
		message.AddString("be:extra_type", type);

	if (what == B_ICON_FOR_TYPE_CHANGED || what == B_ICON_CHANGED) 
		message.AddBool("be:large_icon", large);

	if (!_localDispatch)
		// we are already running as the roster app, send the
		// updates directly
		(_rosterSendMimeChanged)(&message);
	else
		SendToRoster(&message, NULL);
}

status_t get_rsrc_size(BResources *rsrc, const char *,
	int32 id, type_code type, size_t *len)
{
	status_t	err;
	const char	*n;
	
	err = rsrc->GetResourceInfo(type, id, &n, len);
	return err;
}

status_t get_attr_size(BNode *node, const char *name, type_code,
	size_t *len)
{
	attr_info	ainfo;
	status_t	err;
	
	if ((err = node->GetAttrInfo(name, &ainfo)) != B_OK)
		return err;
	
	*len = ainfo.size;
	return B_OK;
}

status_t get_data_size(int where, BNode *node, BResources *rsrc,
	const char *name, int32 id, type_code type, size_t *len)
{
	*len = 0;
	if ((where & B_USE_ATTRIBUTES) != 0) 
		return get_attr_size(node, name, type, len);

	return get_rsrc_size(rsrc, name, id, type, len);
}

status_t get_string_data(int where, BNode *node, BResources *rsrc,
	const char *name, int32 id, type_code type, BString *data)
{
	uint32 length = 0;
	status_t err = B_OK;
	if ((where & B_USE_ATTRIBUTES) != 0) 
		err = get_attr_size(node, name, type, &length);
	else
		err = get_rsrc_size(rsrc, name, id, type, &length);
	
	if (err != B_OK)
		return err;
	
	
	if ((where & B_USE_ATTRIBUTES) != 0)
		err = get_attr(node, name, type, data->LockBuffer(length), length);
	else
		err = get_rsrc(rsrc, name, id, type, data->LockBuffer(length), length);

	data->UnlockBuffer(length);
	return err;
}

status_t get_data(int where, BNode *node, BResources *rsrc, const char *name,
	int32 id, type_code type, void *data, size_t len)
{
	if ((where & B_USE_ATTRIBUTES) != 0)
		return get_attr(node, name, type, data, len);

	return get_rsrc(rsrc, name, id, type, data, len);
}

status_t set_data(int where, BNode *node, BResources *rsrc, const char *name,
	int32 id, type_code type, const void *data, size_t len)
{
	status_t	err = B_OK;
	if ((where & B_USE_ATTRIBUTES) != 0)
		err = set_attr(node, name, type, data, len);
	if (err == B_OK && ((where & B_USE_RESOURCES) != 0))
		err = set_rsrc(rsrc, name, id, type, data, len);

	return err;
}

/* ---------------------------------------------------------------- */

status_t get_icon_data(int where, BNode *node, BResources *rsrc, const char *name,
	type_code type, void *icon_buffer)
{
	if ((where & B_USE_ATTRIBUTES) != 0)
		return get_icon_attr(node, name, type, icon_buffer);

	return get_icon_rsrc(rsrc, name, type, icon_buffer);
}

/* ---------------------------------------------------------------- */

status_t set_icon_data(int where, BNode *node, BResources *rsrc,
	const char *name, type_code type, const void *bits, uint32 bits_len,
	bool required_id)
{
	status_t	err = B_OK;
	if ((where & B_USE_ATTRIBUTES) != 0)
		err = set_icon_attr(node, name, type, bits, bits_len);
	if (err == B_OK && ((where & B_USE_RESOURCES) != 0))
		err = set_icon_rsrc(rsrc, name, type, bits, bits_len, required_id);

	return err;
}

/* ---------------------------------------------------------------- */

status_t get_msg_data(int where, BNode *node, BResources *rsrc,
	const char *name, type_code type, BMessage *msg)
{
	if ((where & B_USE_ATTRIBUTES) != 0)
		return get_msg_attr(node, name, type, msg);

	return get_msg_rsrc(rsrc, name, type, msg);
}

/* ---------------------------------------------------------------- */

status_t set_msg_data(int where, BNode *node, BResources *rsrc,
	const char *name, type_code type, const BMessage *msg)
{
	status_t	err = B_OK;
	if ((where & B_USE_ATTRIBUTES) != 0)
		err = set_msg_attr(node, name, type, msg);
	if (err == B_OK && ((where & B_USE_RESOURCES) != 0))
		err = set_msg_rsrc(rsrc, name, type, msg);

	return err;
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

status_t get_attr(BNode *node, const char *attr, type_code type,
	void *data, size_t l)
{
//+	PRINT(("##Reading attr (%s)\n", attr));
	ssize_t r = node->ReadAttr(attr, type, 0, data, l);
	return (r > 0) ? B_OK : (status_t) r;
}

/* ---------------------------------------------------------------- */

status_t set_attr(BNode *node, const char *attr, type_code type,
	 const void *data, size_t l)
{
	status_t	err;
//+	PRINT(("##Writing attr (%s) (data=%x, %d)\n", attr, data, l));
	err = node->RemoveAttr(attr);
	if (data) {
//+		PRINT(("##Writing attr (%s, %.4s, %x)\n", attr, (char*) &type, type));
		ssize_t w = node->WriteAttr(attr, type, 0, data, l);
		err = (w < 0) ? (status_t) w : B_OK;
	}
	return err;
}

/* ---------------------------------------------------------------- */

bool
has_icon_attr(const BNode *node, const char *attr, type_code)
{
	// private call used by CheckNodeIconHintPrivate
	attr_info	ainfo;

	return node->GetAttrInfo(attr, &ainfo) == B_OK;
}

/* ---------------------------------------------------------------- */

status_t get_icon_attr(BNode *node, const char *attr, type_code type,
	void *icon_buffer)
{
	attr_info	ainfo;
	status_t	err;

	err = node->GetAttrInfo(attr, &ainfo);
	if (err)
		return err;

//+	PRINT(("##Reading icon_attr (%s)\n", attr));
	ssize_t r = node->ReadAttr(attr, type, 0, icon_buffer, ainfo.size);
	err = (r < 0) ? (status_t) r : B_OK;

	return err;
}

/* ---------------------------------------------------------------- */

status_t set_icon_attr(BNode *node, const char *attr, type_code type,
	const void *bits, uint32 bits_len)
{
	/*
	 If icon is NULL then remove any existing icon.
	*/
	status_t	err;

//+	PRINT(("##Writing icon_attr (%s)\n", attr));

	if (bits) {
		ssize_t w = node->WriteAttr(attr, type, 0, bits, bits_len);
		err = (w < 0) ? (status_t) w : B_OK;
	} else
		err = node->RemoveAttr(attr);

	return err;
}

/* ---------------------------------------------------------------- */

status_t get_msg_attr(BNode *node, const char *name, type_code type,
	BMessage *msg)
{
	status_t	err;
	char		*buffer;
	char		sbuffer[1024];
	char		*dbuffer = NULL;

	attr_info	ai;
	err = node->GetAttrInfo(name, &ai);
	if (err)
		return err;

	if (ai.size > sizeof(sbuffer))
		buffer = dbuffer = (char *) malloc(ai.size);
	else
		buffer = sbuffer;

	if (!buffer)
		return B_NO_MEMORY;

//+	PRINT(("##Reading msg_attr (%s)\n", name));
	err = get_attr(node, name, type, buffer, ai.size);
	if (err == B_OK) 
		err = msg->Unflatten(buffer);

	free(dbuffer);
	return err;
}

/* ---------------------------------------------------------------- */

status_t set_msg_attr(BNode *node, const char *name, type_code type,
	const BMessage *msg)
{
	status_t	err = B_OK;
	size_t		size = 0;
	char		*buffer = NULL;
	char		sbuffer[1024];
	char		*dbuffer = NULL;

	if (msg) {
		size = msg->FlattenedSize();
		if (size > sizeof(sbuffer))
			buffer = dbuffer = (char *) malloc(size);
		else
			buffer = sbuffer;

		if (!buffer)
			return B_NO_MEMORY;

		err = msg->Flatten(buffer, size);
	}

//+	PRINT(("##Writing msg_attr (%s)\n", name));
	if (err == B_OK)
		err = set_attr(node, name, type, buffer, size);

	free(dbuffer);
	return err;
}

/* ---------------------------------------------------------------- */

status_t get_rsrc(BResources *rsrc, const char *, int32 id,
	type_code type, void *data, size_t l)
{
	return rsrc->ReadResource(type, id, data, 0, l);
}

/* ---------------------------------------------------------------- */

status_t set_rsrc(BResources *rsrc, const char *name, int32 id,
	type_code type, const void *data, size_t l)
{
	status_t	err;
	err = rsrc->RemoveResource(type, id);
	if (data)
		err = rsrc->AddResource(type, id, data, l, name);
	return err;
}

/* ---------------------------------------------------------------- */

status_t get_icon_rsrc(BResources *rsrc, const char *name, type_code type,
	void *icon_buffer)
{
	size_t		size;
	status_t	err;
	int32		id;

	if (rsrc->GetResourceInfo(type, name, &id, &size)) 
		err = rsrc->ReadResource(type, id, icon_buffer, 0, size);
	else 
		err = B_NAME_NOT_FOUND;

	return err;
}

/* ---------------------------------------------------------------- */

status_t set_icon_rsrc(BResources *rsrc, const char *name, type_code type,
	const void *bits, uint32 bits_len, bool required_id)
{
	/*
	 Custom icons are stored in a resource named after the mime_type.
	 If 'icon' is NULL then we'll remove any existing icon for this type.
	*/
	size_t		size;
	status_t	err = B_OK;
	int32		id;

	if (rsrc->GetResourceInfo(type, name, &id, &size)) {
		// found existing icon
		if (bits)
			err = rsrc->WriteResource(type, id, bits, 0, bits_len);
		else
			err = rsrc->RemoveResource(type, id);
	} else if (bits) {
		// There's no existing icon so add a new resource
		if (required_id) 
			id = RESERVED_ICON_ID;
		else {
			// No custom icon for this type. Need to add a new resource. 
			// We have to find an unused 'id'
			id = 0;
			while ((id == RESERVED_ICON_ID) || rsrc->HasResource(type, id)) 
				id++;
		}

		// use 'id' for the new resource
		err = rsrc->AddResource(type, id, bits, bits_len, name);
	}
	return err;
}

/* ---------------------------------------------------------------- */

status_t get_msg_rsrc(BResources *rsrc, const char *name,
	type_code type, BMessage *msg)
{
	int32		id;
	size_t		size;
	status_t	err;
	char		*buffer;
	char		sbuffer[1024];
	char		*dbuffer = NULL;

	if (!rsrc->GetResourceInfo(type, name, &id, &size))
		return B_NAME_NOT_FOUND;

	if (size > sizeof(sbuffer))
		buffer = dbuffer = (char *) malloc(size);
	else
		buffer = sbuffer;

	if (!buffer)
		return B_NO_MEMORY;

	err = get_rsrc(rsrc, name, id, type, buffer, size);
	if (err == B_OK)
		err = msg->Unflatten(buffer);

	free(dbuffer);
	return err;
}

/* ---------------------------------------------------------------- */

status_t set_msg_rsrc(BResources *rsrc, const char *name,
	type_code type, const BMessage *msg)
{
	size_t		size = 0;
	size_t		osize;
	status_t	err = B_OK;
	char		*buffer = NULL;
	char		sbuffer[1024];
	char		*dbuffer = NULL;
	long		id;

	if (msg) {
		size = msg->FlattenedSize();
		if (size > sizeof(sbuffer))
			buffer = dbuffer = (char *) malloc(size);
		else
			buffer = sbuffer;

		if (!buffer)
			return B_NO_MEMORY;

		err = msg->Flatten(buffer, size);
		if (err)
			goto done;
	}

	if (rsrc->GetResourceInfo(type, name, &id, &osize)) {
		// found existing icon

		err = rsrc->RemoveResource(type, id);
		if (err == B_OK && buffer) {
			err = rsrc->AddResource(type, id, buffer, size, name);
		}
	} else if (buffer) {
		// There's no existing data so add new data
		// We have to find an unused 'id'
		id = 1;
		while (rsrc->HasResource(type, id))
			id++;

		// use 'id' for the new resource
		err = rsrc->AddResource(type, id, buffer, size, name);
	}

done:
	free(dbuffer);
	return err;
}

/* ---------------------------------------------------------------- */

void BMimeType::_ReservedMimeType1() {}
void BMimeType::_ReservedMimeType2() {}
void BMimeType::_ReservedMimeType3() {}

/* ---------------------------------------------------------------- */

BMimeType &BMimeType::operator=(const BMimeType &) { return *this; }
BMimeType::BMimeType(const BMimeType &) {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
