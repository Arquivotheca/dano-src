/***************************************************************************

	File : AppFileInfo.cpp

	Written by: Peter Potrebic

	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

****************************************************************************/

#include <AppFileInfo.h>
#include <Debug.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fs_attr.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <File.h>
#include <Drivers.h>
#include <Mime.h>
#include <OS.h>
#include <Path.h>
#include <Resources.h>
#include <String.h>
#include <private/storage/mime_private.h>
#include <private/storage/walker.h>

#include <roster_private.h>
#include <Mime.h>

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* -----              BAppFileInfo class                 ---------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
BAppFileInfo::BAppFileInfo()
{
	fNode = NULL;
	fCStatus = B_NO_INIT;
	fResources = NULL;
}

BAppFileInfo::BAppFileInfo(BFile *file)
	: BNodeInfo(file)
{
	fResources = NULL;
	SetTo(file);
}

/* ---------------------------------------------------------------- */

BAppFileInfo::~BAppFileInfo()
{
	if (fResources)
		delete fResources;
}

/* ---------------------------------------------------------------- */

void BAppFileInfo::SetInfoLocation(info_location loc)
{
	fWhere = loc;
}

/* ---------------------------------------------------------------- */

bool BAppFileInfo::IsUsingAttributes() const 
{
	return ((fWhere & B_USE_ATTRIBUTES) != 0);
}


/* ---------------------------------------------------------------- */

bool BAppFileInfo::IsUsingResources() const 
{
	return ((fWhere & B_USE_RESOURCES) != 0);
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::SetTo(BFile *file) 
{
	if (fResources)
		delete fResources;
	fResources = NULL;

	status_t err = inherited::SetTo(file);

	if (err != B_NO_ERROR) {
		return err;
	}

	/* B_USE_BOTH_LOCATIONS means that set calls will write to both
	 * attr's and rsrc's When getting data we'll favor attr's.
	 */
	fWhere = B_USE_BOTH_LOCATIONS;

	fResources = new BResources(file);
#if 0
	/* only need to construct the BResources object if we might use it. */
	if ((fWhere & B_USE_RESOURCES) && file->IsWritable())
		fResources = new BResources(file);
	else
		fResources = NULL;
#endif

	return B_NO_ERROR;
}

status_t BAppFileInfo::GetType(char *type) const
{
	if (!fNode)
		return B_NO_INIT;

	return get_data(fWhere, fNode, fResources, B_MIME_TYPE_ATTR, RSRC_ID+1,
		B_MIME_STRING_TYPE, type, B_MIME_TYPE_LENGTH);
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::SetType(const char *type)
{
	if (!fResources || !fNode)
		return B_NO_INIT;

	// The File Type is stored as both a resource and an attribute

	status_t	err = B_OK;

	int len = type ? (strlen(type)+1) : 0;
	if (len > B_MIME_TYPE_LENGTH)
		return B_BAD_VALUE;

	err = set_data(fWhere, fNode, fResources, B_MIME_TYPE_ATTR, RSRC_ID+1,
		B_MIME_STRING_TYPE, type, len);
	return err;
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::GetSignature(char *sig) const
{
	if (!fNode)
		return B_NO_INIT;

	return get_data(fWhere, fNode, fResources, B_APP_SIGNATURE_ATTR, RSRC_ID,
		B_MIME_STRING_TYPE, sig, B_MIME_TYPE_LENGTH);
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::SetSignature(const char *sig)
{
	if (!fResources || !fNode)
		return B_NO_INIT;

	// The App Signature is stored as both a resource and an attribute

	status_t	err = B_OK;

	int len = sig ? (strlen(sig)+1) : 0;
	if (len > B_MIME_TYPE_LENGTH)
		return B_BAD_VALUE;

	// ignoring the Compile time flags. Always store the signature
	// as an Attribute (for queries)

	err = set_data(fWhere, fNode, fResources, B_APP_SIGNATURE_ATTR, RSRC_ID,
		B_MIME_STRING_TYPE, sig, len);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::GetAppFlags(uint32 *flags) const
{
	if (!fNode)
		return B_NO_INIT;

	return get_data(fWhere, fNode, fResources, APP_FLAGS_ATTR, RSRC_ID,
		APP_FLAGS_TYPE, (char*) flags, sizeof(uint32));
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::SetAppFlags(uint32 flags)
{
	if (!fResources || !fNode)
		return B_NO_INIT;

	status_t	err = B_OK;

	err = set_data(fWhere, fNode, fResources, APP_FLAGS_ATTR, RSRC_ID,
		APP_FLAGS_TYPE, &flags, sizeof(uint32));
	return err;
}

/* ---------------------------------------------------------------- */

bool BAppFileInfo::Supports(BMimeType *mt) const
{
	if (!fNode)
		return false;

	BMessage	types;
	if (GetSupportedTypes(&types) < B_OK)
		return false;
	
	int			i = 0;
	const char	*str;

	while (types.FindString(B_SUPPORTED_MIME_ENTRY, i++, &str) == B_OK) {
		BMimeType	t(str);
		if (t.Contains(mt))
			return true;
	}
	
	return false;
}

/* ---------------------------------------------------------------- */

bool BAppFileInfo::IsSupportedType(const char *type) const
{
	BMessage	types;
	if (GetSupportedTypes(&types) != B_OK)
		return false;
	
	BMimeType	mtype(type);
	const char	*t;
	int32		i = 0;

	while (types.FindString("types", i++, &t) == B_OK) {
		if (strcasecmp(t, "application/octet-stream") == 0)
			return true;

		BMimeType	mt(t);
		if (mt.Contains(&mtype))
			return true;
	}

	return false;
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::GetSupportedTypes(BMessage *types) const
{
	if (!fNode)
		return B_NO_INIT;

	/*
	 The message should contain 1 entry named B_SUPPORTED_MIME_ENTRY 
	 ("types"). This entry should contain an array of mime_type
	 strings. An empty message is legal. It means that the app doesn't
	 support any types.
 	*/

	return get_msg_data(fWhere, fNode, fResources, APP_SUPPORTED_TYPES_ATTR,
		B_MESSAGE_TYPE, types);
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::_SetSupportedTypes(const BMessage *types)
{
	if (!fResources || !fNode)
		return B_NO_INIT;

	/*
	 The message should contain 1 entry named B_SUPPORTED_MIME_ENTRY 
	 ("types"). This entry should contain an array of mime_type
	 strings. An empty message is legal. It means that the app doesn't
	*/

	return set_msg_data(fWhere, fNode, fResources, APP_SUPPORTED_TYPES_ATTR, B_MESSAGE_TYPE,
		types);
}

/*------------------------------------------------------------------------*/

static status_t _unique_merge_string_(BMessage *dest, const BMessage *src,
	const char *field)
{
	int32	i;
	int32	j = 0;
	const	char *dstr;
	const	char *sstr;
	bool	already;
//+	PRINT(("TRY: Adding %s to entry %s\n", sstr, field));

	while (src->FindString(field, j++, &sstr) == B_OK) {
		i = 0;
		already = false;
		while (dest->FindString(field, i++, &dstr) == B_OK) {
			if (strcasecmp(dstr, sstr) == 0)
				// found a match so return
				already = true;;
		}
		if (!already) {
//+			PRINT(("Adding %s to entry %s\n", sstr, field));
			dest->AddString(field, sstr);
		}
	}

	return B_OK;
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::SetSupportedTypes(const BMessage *types)
{
	return SetSupportedTypes(types, false);
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::SetSupTypesForAll(BMimeType *meta, const BMessage *types)
{
	status_t		err;
	err = meta->SetSupportedTypes(types);

	PRINT(("Setting supported types in meta file(%x)\n", err));
//+	if (types) {
//+		PRINT_OBJECT((*types));
//+	} else {
//+		PRINT(("	<null> types\n"));
//+	}

	char			pred[PATH_MAX];

	sprintf(pred, "%s = %s", B_APP_SIGNATURE_ATTR, meta->Type());
	TQueryWalker	query(pred);
	BEntry			entry;

	while (query.GetNextEntry(&entry) == B_OK) {
		BFile			f;
		BAppFileInfo	ainfo;
		err = f.SetTo(&entry, O_RDWR);
		if (err != B_OK) {
			PRINT(("BFile::SetTo err=%x (%s)\n", err, strerror(err)));
			continue;
		}
		ainfo.SetTo(&f);
		if (err != B_OK) {
			PRINT(("BAppFileInfo::SetTo err=%x (%s)\n", err, strerror(err)));
			continue;
		}

		BPath	p;
		entry.GetPath(&p);
		PRINT(("Setting supported types in app (%s)\n", p.Path()));
		err = ainfo._SetSupportedTypes(types);
	}

	// let's also send message to roster to MERGE in any additions...
	ASSERT(_is_valid_roster_mess_(true));
	BMessage	msg(CMD_UPDATE_SUPPORTING_APPS);
	if (types)
		msg.AddMessage("new", types);
	
	msg.AddBool("force", true);
	msg.AddString("sig", meta->Type());
	_send_to_roster_(&msg, NULL, true);

	return B_OK;
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::SetSupportedTypes(const BMessage *types, bool sync_all)
{
	BMimeType	meta;

	status_t err = GetMetaMime(&meta);
	if (err != B_OK)
		return err;

	if (sync_all) {
		/*
		 _SetSupportedTypes might get called twice on 'this' Application
		 because the query in _set_sup_types_for_sig_ might find this app
		 for as well. However, what if 'this' app lives on the volume that
		 doesn't support queries? That's why we do a manual _Set here. Just in
		 case.
		*/
		if ((err = _SetSupportedTypes(types)) == B_OK)
			err = SetSupTypesForAll(&meta, types);
	} else if ((err = _SetSupportedTypes(types)) == B_OK) {
		if (types) {
			BMessage	types_in_meta;
			err = meta.GetSupportedTypes(&types_in_meta);
			_unique_merge_string_(&types_in_meta, types,
				B_SUPPORTED_MIME_ENTRY);
			err = meta.SetSupportedTypes(&types_in_meta);
			PRINT(("Merging supported types in meta file(%x)\n", err));
//+			PRINT_OBJECT(types_in_meta);
		}

		// let's also send message to roster to MERGE in any additions...
		ASSERT(_is_valid_roster_mess_(true));
		BMessage	msg(CMD_UPDATE_SUPPORTING_APPS);
		if (types)
			msg.AddMessage("new", types);

		msg.AddString("sig", meta.Type());
		_send_to_roster_(&msg, NULL, true);
	}

	return err;
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::GetIcon(BBitmap *icon, icon_size which) const
{
	if (!fNode)
		return B_NO_INIT;
	return GetIconForType(NULL, icon, which);
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::SetIcon(const BBitmap *icon, icon_size which)
{
	if (!fResources || !fNode)
		return B_NO_INIT;

	return SetIconForType(NULL, icon, which);
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::GetVersionInfo(version_info *info, version_kind k) const
{
	if (!fNode)
		return B_NO_INIT;

	version_info	all[2];
	status_t		err;

	err =  get_data(fWhere, fNode, fResources, APP_VERSION_ATTR, RSRC_ID,
		APP_VERSION_TYPE, (char*) all, sizeof(all));

	if (err == B_OK)
		*info = all[ (k == B_APP_VERSION_KIND) ? 0 : 1 ];
	return err;
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::SetVersionInfo(const version_info *info, version_kind k)
{
	if (!fResources || !fNode)
		return B_NO_INIT;

	version_info	all[2];
	status_t		err;

	err =  get_data(fWhere, fNode, fResources, APP_VERSION_ATTR, RSRC_ID,
		APP_VERSION_TYPE, (char*) all, sizeof(all));

	all[ (k == B_APP_VERSION_KIND) ? 0 : 1 ] = *info;

	err = set_data(fWhere, fNode, fResources, APP_VERSION_ATTR, RSRC_ID,
		APP_VERSION_TYPE, all, sizeof(all));
	return err;
}

/* ---------------------------------------------------------------- */
status_t BAppFileInfo::GetIconForType(const char *mime_type, BBitmap *icon,
	icon_size which) const
{
	ASSERT(this);

	ASSERT(icon);
//+	PRINT(("type=%s\n", mime_type));

	if (!fNode)
		return B_NO_INIT;

	/*
	 Custom icons are stored in a resource named after the mime_type.
	 A NULL mime_type means the STD_ICON.
	*/
	float		icon_size;
	status_t	err = B_ENTRY_NOT_FOUND;
	BRect		rect;
	uint32		type;
	char		rsrc_name[256];
	char		icon_buffer[1024];

	if ((mime_type && !BMimeType::IsValid(mime_type)) || !icon)
		return B_BAD_VALUE;

	if (which == B_LARGE_ICON) {
		icon_size = LARGE_ICON_SIZE;
		type = LARGE_ICON_TYPE;
	} else {
		icon_size = MINI_ICON_SIZE;
		type = MINI_ICON_TYPE;
	}
	rect.Set(0, 0, icon_size - 1, icon_size - 1);

	if (icon->Bounds() != rect)
		return B_BAD_VALUE;

	// Code to deal with bug #9550
	if (mime_type) {
		// try using lower case'd string for the name
		char    dup[B_MIME_TYPE_LENGTH];
		strcpy(dup, mime_type);
		tolower_str(dup);
//+		PRINT(("lower=%s, std=%s\n", dup, mime_type));
		if (strcmp(dup, mime_type) != 0) {
			strcpy(rsrc_name, MIME_PREFIX);
			strcat(rsrc_name, (which == B_LARGE_ICON) ? "L:" : "M:");
			strcat(rsrc_name, dup);
//+			PRINT(("fWhere=%d, fNode=%x, fResources=%x\n\trsrc_name=%s, type=%d, icon_buffer=%x\n", fWhere, fNode, fResources, rsrc_name, type, icon_buffer));
			err = get_icon_data(fWhere, fNode, fResources, rsrc_name,
				type, icon_buffer);
//+			PRINT(("Try #0: \"%s\" err=%x\n", rsrc_name, err));
		}
	}									 
	// End of code to deal with bug #9550

	if (err != B_OK) {
		strcpy(rsrc_name, MIME_PREFIX);

		if (which == B_LARGE_ICON) {
			type = LARGE_ICON_TYPE;
			strcat(rsrc_name, "L:");
		} else {
			type = MINI_ICON_TYPE;
			strcat(rsrc_name, "M:");
		}

		strcat(rsrc_name, mime_type ? mime_type : STD_ICON_SUFFIX);
		
//+		PRINT(("fWhere=%d, fNode=%x, fResources=%x\n\trsrc_name=%s, type=%d, icon_buffer=%x\n", fWhere, fNode, fResources, rsrc_name, type, icon_buffer));
		err = get_icon_data(fWhere, fNode, fResources, rsrc_name,
			type, icon_buffer);
//+		PRINT(("Try #1: \"%s\" err=%x\n", rsrc_name, err));
	}

	if ((err != B_OK) && mime_type) {
		strcpy(rsrc_name, MIME_PREFIX);

		if (which == B_LARGE_ICON) {
			type = LARGE_ICON_TYPE;
			strcat(rsrc_name, "L:");
		} else {
			type = MINI_ICON_TYPE;
			strcat(rsrc_name, "M:");
		}

		// try using the conoical mime_type
		char    bogus[B_MIME_TYPE_LENGTH];
		BMimeType mt(mime_type);

		// need to force it to open to get the cononical name
		mt.GetPreferredApp(bogus);

		if (mt.Type()) {
			strcat(rsrc_name, mt.Type());
			err = get_icon_data(fWhere, fNode, fResources, rsrc_name,
				type, icon_buffer);
//+			PRINT(("AppFileInfo::GetIconForType(%s), try #3 (err=%x)\n",
//+				rsrc_name, err));
		}
	}

	if (err == B_OK)
		icon->SetBits(icon_buffer, icon->BitsLength(), 0, B_COLOR_8_BIT);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BAppFileInfo::SetIconForType(const char *mime_type,
	const BBitmap *icon, icon_size which)
{
	if (!fResources || !fNode)
		return B_NO_INIT;

	/*
	 Custom icons are stored in a resource named after the mime_type.
	 If 'icon' is NULL then we'll remove any existing icon for this type.
	*/
	float		icon_size;
	status_t	err = B_OK;
	BRect		rect;
	uint32		type;
	char		rsrc_name[256];

	if ((mime_type && !BMimeType::IsValid(mime_type)))
		return B_BAD_VALUE;

	strcpy(rsrc_name, MIME_PREFIX);

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

	bool required_id = (mime_type == NULL);

	err = set_icon_data(fWhere, fNode, fResources, rsrc_name, type,
		icon ? icon->Bits() : NULL,
		icon ? icon->BitsLength() : 0,
		required_id);
	PRINT(("SetIcon: \"%s\" err=%x\n", rsrc_name, err));
	if (err == B_OK) {
		// save the new icon in the MetaMime file for this app's signature.
		BMimeType	meta;
		if (GetMetaMime(&meta) == B_OK) {
			err = meta.SetIconForType(mime_type, icon, which);
		}
	}
	return err;
}

/* ---------------------------------------------------------------- */

status_t	BAppFileInfo::GetMetaMime(BMimeType *meta) const
{
	char		sig[B_MIME_TYPE_LENGTH];
	status_t	err = GetSignature(sig);

	if (err == B_OK)
		err = meta->SetTo(sig);

	return err;
}

/* ---------------------------------------------------------------- */

static bool match_file_and_path(entry_ref *ref, const char *path,
	bool *does_cached_exist)
{
	BEntry	entry(ref);
	struct stat	st;
	status_t	err = entry.GetStat(&st);
	if (err != B_OK) {
		*does_cached_exist = false;
		return false;
	}

	// ??? assuming that the file in cached location is actually the right
	// app. Its possible that it is any old file with the same name.
	*does_cached_exist = true;

	BPath	p;
	if (entry.GetPath(&p) != B_OK)
		return false;

	return (strcmp(p.Path(), path) == 0);
}

/* ---------------------------------------------------------------- */

extern bool _is_valid_app_sig_(const char *);

status_t	BAppFileInfo::UpdateMetaMime(const char *path, bool force,
	uint32 *changes) const
{
	// The app signature pseudo-metamime is used to cache things like
	// icons from the application, etc. Set it up here.

	status_t	err;
	BMimeType	meta;
	entry_ref	cached_ref;
	bool		has_cached_attr = false;
	bool		does_cached_exist = false;
	bool		this_is_cached = false;
	bool		newly_cached = false;

	*changes = 0;

	if (!path)
		return B_BAD_VALUE;

	err = GetMetaMime(&meta);
	if (err != B_OK) 
		return err;

	
	if (!_is_valid_app_sig_(meta.Type()))
		return B_BAD_VALUE;

	err = meta.GetAppHint(&cached_ref);
	if (err == B_OK) {
		has_cached_attr = true;
		if (match_file_and_path(&cached_ref, path, &does_cached_exist))
			this_is_cached = true;
	}


	err = meta.OpenFile(true);
	if (err!= B_OK) 
		return err;

	{
		// make sure the metamime file for this app signature has the right type
		BNodeInfo ninfo(meta.fMeta);
		ninfo.SetType(META_FILE_MIME_TYPE);
	}

	// The force flag will only force things if the currently cached app
	// doesn't exist.

	if (!has_cached_attr || (force && !does_cached_exist)) {
		// The short description for an application signature metamime
		// defaults to the 'name' of the application
		BPath p(path);
		if (p.Leaf()) {
			meta.SetShortDescription(p.Leaf());
			*changes |= B_SHORT_DESCRIPTION_CHANGED;
		}
	}

	if (!has_cached_attr || (force && !does_cached_exist)) {
		// Set the cached ref in the meta-mime file.
		newly_cached = true;
		entry_ref	actual_ref;
		err = get_ref_for_path(path, &actual_ref);
		err = meta.SetAppHint(&actual_ref);
		if (err == B_OK)
			*changes |= B_APP_HINT_CHANGED;
	}


	// The preferred app for an app is the app itself
	char	cur[B_MIME_TYPE_LENGTH];
	if (meta.GetPreferredApp(cur) != B_OK
		|| strcasecmp(cur, meta.Type()) != 0) {
		err = meta.SetPreferredApp(meta.Type());
		if (err == B_OK)
			*changes |= B_PREFERRED_APP_CHANGED;
	}
	
	BMessage supported;
	err = GetSupportedTypes(&supported);
	if (err == B_OK)
		err = meta.SetSupportedTypes(&supported);


	if (!(newly_cached || (force && !does_cached_exist))) 
		// If we're not the newly cached app or the force flag isn't on
		// then don't move over the icons.
		return B_OK;

	// The following will copy all the 'icon' attributes over to the meta mime
	// file.
	char	name[256];
	char	sbuffer[2048];
	char	*dbuffer = NULL;
	int		dsize = 0;
	char	*buffer;

	while (fNode->GetNextAttrName(name) == B_OK) {
		if (strncmp(name, "BEOS:L:", 7) == 0
			|| strncmp(name, "BEOS:M:", 7) == 0) {
			struct attr_info	ainfo;
			fNode->GetAttrInfo(name, &ainfo);
			if (ainfo.size > sizeof(sbuffer)) {
				if (ainfo.size > dsize) {
					if (dsize == 0)
						dbuffer = (char *) malloc(ainfo.size);
					else
						dbuffer = (char *) realloc(dbuffer, ainfo.size);
					dsize = ainfo.size;
				}
				buffer = dbuffer;
			} else
				buffer = sbuffer;
			err = fNode->ReadAttr(name, ainfo.type, 0, buffer, ainfo.size);
			if (err >= 0) {
				memcpy(name, "META", 4);
				err = meta.fMeta->WriteAttr(name, ainfo.type, 0, buffer,
					ainfo.size);
				if (err >= 0)
					*changes |= B_ICON_CHANGED | B_ICON_FOR_TYPE_CHANGED;
			}
		}
	}

	if (dbuffer)
		free(dbuffer);

	return B_OK;
}

/* ---------------------------------------------------------------- */

status_t	BAppFileInfo::UpdateFromRsrc()
{
	if (!fResources || !fNode)
		return B_NO_INIT;

	if (fWhere & B_USE_ATTRIBUTES)
		return RealUpdateRsrcToAttr();

	return B_OK;
}

/* ---------------------------------------------------------------- */

status_t	BAppFileInfo::RealUpdateRsrcToAttr()
{
	info_location saved_where = fWhere;
	
	char buffer[B_MIME_TYPE_LENGTH];
	fWhere = B_USE_RESOURCES;
	status_t err = GetType(buffer);
	fWhere = B_USE_ATTRIBUTES;
	if (err == B_OK)
		SetType(buffer);

	fWhere = B_USE_RESOURCES;
	err = GetSignature(buffer);
	fWhere = B_USE_ATTRIBUTES;
	if (err == B_OK)
		SetSignature(buffer);

	uint32 flags;
	fWhere = B_USE_RESOURCES;
	err = GetAppFlags(&flags);
	fWhere = B_USE_ATTRIBUTES;
	if (err == B_OK)
		SetAppFlags(flags);

	BMessage types;
	fWhere = B_USE_RESOURCES;
	err = GetSupportedTypes(&types);
	fWhere = B_USE_ATTRIBUTES;
	if (err == B_OK)
		_SetSupportedTypes(&types);

	fWhere = B_USE_RESOURCES;
	version_info	vinfo;
	err = GetVersionInfo(&vinfo, B_APP_VERSION_KIND);
	fWhere = B_USE_ATTRIBUTES;
	if (err == B_OK)
		SetVersionInfo(&vinfo, B_APP_VERSION_KIND);
	fWhere = B_USE_RESOURCES;
	err = GetVersionInfo(&vinfo, B_SYSTEM_VERSION_KIND);
	fWhere = B_USE_ATTRIBUTES;
	if (err == B_OK)
		SetVersionInfo(&vinfo, B_SYSTEM_VERSION_KIND);

	// Can't use real Bitmaps because we can't assume that be_app exists.

	char icon_buf[1024];
	err = get_icon_data(B_USE_RESOURCES, fNode, fResources, "BEOS:L:STD_ICON", LARGE_ICON_TYPE,
		icon_buf);
	if (err == B_OK)
		err = set_icon_data(B_USE_ATTRIBUTES, fNode, fResources, "BEOS:L:STD_ICON", LARGE_ICON_TYPE,
			icon_buf, 1024, true);

	err = get_icon_data(B_USE_RESOURCES, fNode, fResources, "BEOS:M:STD_ICON", MINI_ICON_TYPE,
		icon_buf);
	if (err == B_OK)
		err = set_icon_data(B_USE_ATTRIBUTES, fNode, fResources, "BEOS:M:STD_ICON", MINI_ICON_TYPE,
			icon_buf, 256, true);

	const char	*type;
	int32		i = 0;
	while (types.FindString(B_SUPPORTED_MIME_ENTRY, i++, &type) == B_OK) {
		char lowerCaseType[B_MIME_TYPE_LENGTH];
		strcpy(lowerCaseType, type);
		tolower_str(lowerCaseType);
		strcpy(buffer, "BEOS:L:");
		strcat(buffer, lowerCaseType);
		err = get_icon_data(B_USE_RESOURCES, fNode, fResources, buffer, LARGE_ICON_TYPE, icon_buf);
		if (err == B_NAME_NOT_FOUND) {
			// modern lower-case type name format failed, try old style
			// canonical form
			BString oldStyleName("BEOS:L:");
			oldStyleName << type;
			err = get_icon_data(B_USE_RESOURCES, fNode, fResources, oldStyleName.String(),
				LARGE_ICON_TYPE, icon_buf);
		}

		if (err == B_OK)
			err = set_icon_data(B_USE_ATTRIBUTES, fNode, fResources, buffer, LARGE_ICON_TYPE,
				icon_buf, 1024);

		strcpy(buffer, "BEOS:M:");
		strcat(buffer, lowerCaseType);
		err = get_icon_data(B_USE_RESOURCES, fNode, fResources, buffer, MINI_ICON_TYPE,
			icon_buf);
		if (err == B_NAME_NOT_FOUND) {
			BString oldStyleName("BEOS:M:");
			oldStyleName << type;
			err = get_icon_data(B_USE_RESOURCES, fNode, fResources, oldStyleName.String(),
				MINI_ICON_TYPE,	icon_buf);
		}
		if (err == B_OK)
			err = set_icon_data(B_USE_ATTRIBUTES, fNode, fResources, buffer, MINI_ICON_TYPE,
				icon_buf, 256);
	}

	SetPreferredApp("");
		// save a NULL preferred app string - this makes Tracker opening
		// faster because reading a short attribute performs better than not
		// finding it at all if all the attributes fit into the fast attribute area
	
	fWhere = saved_where;
	return B_OK;
}

/* ---------------------------------------------------------------- */

void BAppFileInfo::_ReservedAppFileInfo1() {}
void BAppFileInfo::_ReservedAppFileInfo2() {}
void BAppFileInfo::_ReservedAppFileInfo3() {}

/* ---------------------------------------------------------------- */

BAppFileInfo &BAppFileInfo::operator=(const BAppFileInfo &) { return *this; }
BAppFileInfo::BAppFileInfo(const BAppFileInfo &node) 
	:	BNodeInfo(node)
	{}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
