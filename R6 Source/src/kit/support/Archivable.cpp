//*****************************************************************************
//
//	File:		Archivable.cpp
//
//	Description:	Super class to any class thaht wants to be archivable
//	
//	Written by:	Peter Potrebic
//
//	Copyright 1997, Be Incorporated
//
//****************************************************************************/

#define DEBUG 1
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <Debug.h>

#include <Archivable.h>
#include <image.h>
#include <Message.h>
#include <Roster.h>
#include <String.h>
#include <Entry.h>
#include <Path.h>
#include <AppFileInfo.h>

#include <message_strings.h>

#include <typeinfo>

// Needed some source file where this global could be defined.
const char* B_EMPTY_STRING = "";

const char* B_ADD_ON_SIGNATURE_ENTRY	= "add_on";
const char* B_LOAD_EACH_TIME_ENTRY		= "be:load_each_time";
const char* B_UNLOAD_ON_DELETE_ENTRY	= "be:unload_on_delete";
const char* B_UNIQUE_REPLICANT_ENTRY	= "be:unique_replicant";
const char* B_ADD_ON_VERSION_ENTRY		= "be:add_on_version";
const char* B_ADD_ON_PATH_ENTRY			= "be:add_on_path";
const char* B_CLASS_NAME_ENTRY			= "class";

static void build_func_name(BString* out_name, const char *class_name,
							bool alternate = false);
static image_id find_add_on(const char *path);
static bool sig_match(image_info *info, const char *sig);

/*----------------------------------------------------------------*/

/*----------------------------------------------------------------*/

BArchivable::BArchivable()
{
}

/*----------------------------------------------------------------*/

BArchivable::BArchivable(BMessage *)
{
}

/*----------------------------------------------------------------*/

static int32 read_num(const char** p)
{
	int32 num=0;
	while(**p && isdigit(**p)) {
		num = (num*10) + (**p - '0');
		(*p)++;
	}
	return num;
}

status_t BArchivable::Archive(BMessage *data, bool) const
{
#if __GNUC__
	const char *original = typeid(*this).name();
	const char *name = original;
	BString buf;
	
	if (*name == 'Q') {
		// This class is in a namespace; need to decode.
		name++;
		if (!isdigit(*name)) debugger("Invalid namespace classname found");
		int32 count = *name - '0';
		name++;
		while (count > 0 && *name) {
			int32 len = read_num(&name);
			buf.Append(name, len);
			if (count > 1) buf.Append("::");
			while (len > 0 && *name) {
				len--;
				name++;
			}
			count--;
		}
		name = buf.String();
	} else {
		read_num(&name);
	}
	
//	printf("archiving %s as %s\n", original, name);
#else
	const char *name = typeid(*this).name();
#endif

	return data->AddString(B_CLASS_NAME_ENTRY, name);
}

/*----------------------------------------------------------------*/

BArchivable *BArchivable::Instantiate(BMessage *)
{
	debugger("Can't create a plain BArchivable object");
	return NULL;
}

/*----------------------------------------------------------------*/

BArchivable::~BArchivable()
{
}

/*----------------------------------------------------------------*/

status_t BArchivable::Perform(perform_code, void *)
{
	return B_ERROR;
}

/*----------------------------------------------------------------*/

void BArchivable::_ReservedArchivable1() {}
void BArchivable::_ReservedArchivable2() {}
void BArchivable::_ReservedArchivable3() {}

/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/

bool validate_instantiation(BMessage *data, const char *class_name)
{
	// check to make sure that the BMessage and the class being instantiated
	// are a matching pair

	bool		found_it = false;
	long		i = 0;
	const char	*str;

	errno = B_OK;


	while (data->FindString(B_CLASS_NAME_ENTRY, i++, &str) == B_OK) {
		if (strcmp(str, class_name) == 0) {
			found_it = true;
			break;
		}
	}

	if (!found_it) {
		syslog(LOG_ERR, "validate_instantiation failed on class %s.", class_name);
		_sPrintf("bad instantiate of class \"%s\"\n", class_name);
		errno = B_MISMATCHED_VALUES;
	}

	return found_it;
}

/*----------------------------------------------------------------*/

#if 0
// DKH -- not currently used.
static bool	match_version(version_info *v1, version_info *v2)
{
//+	PRINT(("comparing against version [%d.%d.%d <%d> %d]\n",
//+		v2->major, v2->middle, v2->minor,
//+		v2->variety, v2->internal));
	return ((v1->major == v2->major)		&&
			(v1->middle == v2->middle)		&&
			(v1->minor == v2->minor)		&&
			(v1->variety == v2->variety)	&&
			(v1->internal == v2->internal));
}
#endif

/*----------------------------------------------------------------*/

BArchivable	*instantiate_object(BMessage *data)
{
	return instantiate_object(data, NULL);
}

/*----------------------------------------------------------------*/

BArchivable	*instantiate_object(BMessage *data, image_id *paddon_id)
{
	instantiation_func	func = NULL;
	BArchivable			*obj = NULL;
	status_t			err = B_OK;
	bool				has_sig;
	const char			*class_name;

	if (paddon_id)
		*paddon_id = B_BAD_VALUE;

	if (!data) {
		errno = B_BAD_VALUE;
		syslog(LOG_ERR, "instantiate_object failed: NULL BMessage argument");
		return NULL;
	}

	err = data->FindString(B_CLASS_NAME_ENTRY, &class_name);
	if (err) {
		syslog(LOG_ERR, "instantiate_object failed: Failed to find an entry"
			"defining the class name (%s).", strerror(err));
		return NULL;
	}


	const char	*sig = NULL;
	has_sig = (data->FindString(B_ADD_ON_SIGNATURE_ENTRY, &sig) == B_OK);

	/*
	 First look to see if there's a matching class/sig pair already loaded.
	*/
	func = find_instantiation_func(class_name, sig);

	if (!func && has_sig) {
		entry_ref		ref;
#if _SUPPORT_VERSIONED_ARCHIVES_
		version_info	*pvers = NULL;

		if (data->HasInt32(B_ADD_ON_VERSION_ENTRY)) {
			int		i = 0;
			uint32	v;
			uint32	*pv = (uint32*) &vers;
			vers.major = vers.middle = vers.minor = 0;
			vers.variety = vers.internal = 0;
			while ((i < 5) && data->FindInt32(B_ADD_ON_VERSION_ENTRY, i++,
				(int32 *) &v) == B_OK) {
					*pv++ = v;
			}
//+			PRINT(("Looking for version [%d.%d.%d <%d> %d]\n",
//+				vers.major, vers.middle, vers.minor,
//+				vers.variety, vers.internal));
			pvers = &vers;
		}
#endif

		BEntry	entry;
		err = be_roster->FindApp(sig, &ref);
		if (!err)
			err = entry.SetTo(&ref, true);
		
		if (err) {
			syslog(LOG_ERR, "instantiate_object failed: Error finding app "
				"with signature \"%s\" (%s)", sig, strerror(err));
		}

#if _SUPPORT_VERSIONED_ARCHIVES_
		if (pvers) {
			// want to look for a specific 'version' of an executable
			version_info	avers;
			bool			match = false;

			if (entry.InitCheck() == B_OK) {
				// The FindApp above worked. Check to see if this default
				// app has the matching version. If so use it, otherwise
				// look again for a matching executable.
				BFile			appfile(&entry, O_RDONLY);
				BAppFileInfo	ainfo(&appfile);
				if (!(err = ainfo.GetVersionInfo(&avers, B_APP_VERSION_KIND))) {
					match = match_version(pvers, &avers);
				}
//+				PRINT(("FindApp app match=%d\n", match));
			}
			if (!match) {
				// The FindApp app wasn't a match. So look again
				BMimeType	mtype(sig);
				err = _query_for_app_(&mtype, &ref, pvers);
				if (!err)
					entry.SetTo(&ref, true);
			}
		}
#endif

		if (!err) {
			BPath	path;
			if ((err = entry.GetPath(&path)) == B_OK) {
				bool		load_each_time;
				image_id	addon_id;
				data->FindBool(B_LOAD_EACH_TIME_ENTRY, &load_each_time);
				// determine if already loaded.
				if (load_each_time ||
					(addon_id = find_add_on(path.Path())) < 0) {
						addon_id = load_add_on(path.Path());
				}
//+				PRINT(("load_each=%d, id=%d\n", load_each_time, addon_id));
				
SERIAL_PRINT(("PATH = %s, addon_id = %d\n", path.Path(), addon_id));
				if (addon_id < 0) {
					syslog(LOG_ERR, "instantiate_object failed: failed to "
						"load the addon (%s)", strerror(addon_id));
				}

				if (addon_id > 0) {
					if (paddon_id)
						*paddon_id = addon_id;

					ASSERT(class_name);
					BString func_name;
					build_func_name(&func_name, class_name);
					if (func_name.Length() > 0) {
						err = get_image_symbol(addon_id, func_name.String(),
							B_SYMBOL_TYPE_TEXT, (void **)&func);
SERIAL_PRINT(("func_name = %s, err = %s\n", func_name.String(), strerror(err)));
						if (err) {
							// look for alternate version
							build_func_name(&func_name, class_name, true);
							if (func_name.Length() > 0) {
								err = get_image_symbol(addon_id, func_name.String(),
									B_SYMBOL_TYPE_TEXT, (void **)&func);
							}
						}
						if (func) {
//+							PRINT(("FOUND: sig=%s, class=%s, func=%x (p=%s)\n",
//+								sig, class_name, func, path.Path()));
						} else {
							syslog(LOG_ERR, "instantiate_object failed: Failed to "
								"find exported Instantiate static function for class %s.", class_name);
						}
					}
				}
			}
		}
	} else if (!func) {
		syslog(LOG_ERR, "instantiate_object failed: No signature specified in archive, "
			"looking for class \"%s\".", class_name);
		errno = B_BAD_VALUE;
		return NULL;
	}

	errno = err;

	if (errno) {
//+		PRINT(("instantiate: err=%x (%s)\n", errno, strerror(errno)));
		syslog(LOG_ERR, "instantiate_object failed: %s (%x)",
			strerror(errno), errno);
	}

	// if the call to 'func' fails it too might set errno.
	if (func)
		obj = func(data);

//+	PRINT(("INSTANTIATE: sig=%s, class=%s, func=%x\n", sig, class_name, func));

	return obj;
}

/*----------------------------------------------------------------*/

#if (__GNUC__ < 3) || __MWERKS__

static void build_func_name(BString* out_name, const char *class_name, bool alternate)
{
	*out_name = "";
	
	if (alternate) 
		return;
	
	
	// If this function name contains namespaces, decode it.
	if (strchr(class_name, ':') != 0) {
		*out_name = "Instantiate__Q?";
		int32 count = 0;
		
		const char* p = class_name;
		while (*p) {
			const char* last = p;
			while (*p && *p != ':') p++;
			if (p > last) {
				(*out_name) << int32(p-last);
				out_name->Append(last, int32(p-last));
				count++;
			}
			while (*p && *p == ':') p++;
		}
		
		(*out_name)[14] = char(count) + '0';
	
	// Otherwise, just generate a plain mangled instantiation function.
	} else {
		(*out_name) << "Instantiate__" << strlen(class_name) << class_name;
	
	}
	
#if __GNUC__

	(*out_name) << "P8BMessage";

#elif __MWERKS__

	(*out_name) << "FP8BMessage";

#else

#error bla

#endif

//	printf("mangled name %s into %s\n", class_name, out_name->String());
}

#else	// the code below is for __GNUC__ >= 3

static void build_func_name(BString* out_name, const char *class_name, bool alternate)
{
	*out_name = "";
	
	if (alternate) 
		return;
	
	*out_name="_ZN";
		
	// If this function name contains namespaces, decode it.
	if (strchr(class_name, ':') != 0) {
			
		const char* p = class_name;
		while (*p) {
			const char* last = p;
			while (*p && *p != ':') p++;
			if (p > last) {
				(*out_name) << int32(p-last);
				out_name->Append(last, int32(p-last));
			}
			while (*p && *p == ':') p++;
		}
	
	// Otherwise, just generate a plain mangled instantiation function.
	} else
	{
		(*out_name) << strlen(class_name) << class_name;	
	}
	
	(*out_name) << "EP8BMessage";
}

#endif

/*----------------------------------------------------------------*/

instantiation_func find_instantiation_func(BMessage *data)
{
	const char	*name = NULL;
	const char	*sig = NULL;
	status_t	err;

	err = data->FindString(B_CLASS_NAME_ENTRY, &name);
	if (err)
		return NULL;

	err = data->FindString(B_ADD_ON_SIGNATURE_ENTRY, &sig);

	instantiation_func func = find_instantiation_func(name, sig);

//+	if (!func)
//+		PRINT(("Found instantiate_func (%s)\n", name));

	return func;
}

/*----------------------------------------------------------------*/

instantiation_func find_instantiation_func(const char *class_name)
{
	return find_instantiation_func(class_name, NULL);
}

/*----------------------------------------------------------------*/

bool sig_match(image_info *info, const char *sig)
{
	if (!sig)
		return true;
	
	// ??? won't work if path for image is > MAXPATHLEN chars. Oh well,
	// the kernel interface was designed with this limitation.

	BFile	f(info->name, O_RDONLY);
	if (f.InitCheck() != B_OK) {
		syslog(LOG_ERR, "instantiate_object - couldn't open addon %s\n", 
			info->name);
		return false;
	}
	BAppFileInfo	ainfo(&f);
	char			asig[B_MIME_TYPE_LENGTH];

	if (ainfo.GetSignature(asig) != B_OK) {
		syslog(LOG_ERR, "instantiate_object - couldn't get mime sig for %s\n", 
			info->name);
		return false;
	}

	return (strcasecmp(sig, asig) == 0);
}

/*----------------------------------------------------------------*/

instantiation_func find_instantiation_func(const char *class_name,
	const char *sig)
{
	image_id			cookie;
	instantiation_func	function = NULL;

	if (!class_name)
		return NULL;

	// Build name for function
	BString func_name;
	build_func_name(&func_name, class_name);
	if (func_name.Length() <= 0) {
//		PRINT(("failed to build function name %s\n", class_name));
		return NULL;
	}
	BString func_name2;
	build_func_name(&func_name2, class_name, true);

	thread_info	tinfo;
	thread_id	tid = find_thread(NULL);
	team_id		team;
	image_info	info;

	get_thread_info(tid, &tinfo);
	team = tinfo.team;
	cookie = 0;

	for (long i = 0; get_next_image_info(team, &cookie, &info) == B_OK; i++) {
		status_t err = get_image_symbol(info.id, func_name.String(), B_SYMBOL_TYPE_TEXT,
			(void **)&function);
		if ((err == B_OK) && (sig_match(&info, sig)))
			break;

		function = NULL;
		if (func_name2 != "") {
			err = get_image_symbol(info.id, func_name2.String(), B_SYMBOL_TYPE_TEXT,
				(void **)&function);
			if ((err == B_OK) && (sig_match(&info, sig)))
				break;
		}
		function = NULL;
	}

//+	PRINT(("FIND: class=%s, sig=%s, func=%x\n", class_name, sig, function));
//	if (!function)
//		PRINT(("couldn't find %s %s\n", class_name, func_name));

	return function;
}

/*----------------------------------------------------------------*/
static image_id find_add_on(const char *path)
{
	image_id	id = -1;

	thread_info	tinfo;
	thread_id	tid = find_thread(NULL);
	team_id		team;
	image_info	info;
	int32		cookie = 0;

	get_thread_info(tid, &tinfo);
	team = tinfo.team;

	for (long i = 0; get_next_image_info(team, &cookie, &info) == B_OK; i++) {
		if (strcmp(path, info.name) == 0) {
			id = info.id;
			break;
		}
	}

	return id;
}

/*----------------------------------------------------------------*/
