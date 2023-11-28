#include <NodeInfo.h>
#include <Debug.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fs_attr.h>
#include <fs_info.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <OS.h>
#include <Drivers.h>
#include <Mime.h>
#include <File.h>
#include <Path.h>
#include <Directory.h>
#include <Application.h>
#include <Resources.h>
#include <Bitmap.h>

#include <roster_private.h>
#include <private/storage/mime_private.h>

BNodeInfo::BNodeInfo(void)
{
	fNode = NULL;
	fCStatus = B_NO_INIT;
}

/* ---------------------------------------------------------------- */

BNodeInfo::BNodeInfo(BNode *node)
{
	SetTo(node);
}

/* ---------------------------------------------------------------- */

BNodeInfo::~BNodeInfo()
{
}

/* ---------------------------------------------------------------- */

status_t BNodeInfo::SetTo(BNode *node)
{
	if (!node || node->InitCheck() != B_OK) {
		fNode = NULL;
		return (fCStatus=B_BAD_VALUE);
	}

	fNode = node;
	return (fCStatus=B_OK);
}

/* ---------------------------------------------------------------- */

status_t BNodeInfo::InitCheck() const
{
	return fCStatus;
}

/* ---------------------------------------------------------------- */

status_t BNodeInfo::GetType(char *type) const
{
	status_t	err;
	if (!fNode)
		return B_NO_INIT;

	err = get_attr(fNode, B_MIME_TYPE_ATTR, B_MIME_STRING_TYPE,
		type, B_MIME_TYPE_LENGTH);
	return err;
}

/* ---------------------------------------------------------------- */

status_t BNodeInfo::SetType(const char *type)
{
	if (!fNode)
		return B_NO_INIT;

	size_t length = type ? strlen(type) + 1 : 0;
	return set_attr(fNode, B_MIME_TYPE_ATTR, B_MIME_STRING_TYPE, type, length);
}

/* ---------------------------------------------------------------- */


namespace BPrivate {

_EXPORT bool CheckNodeIconHintPrivate(const BNode *node, bool miniOnly)
{
	// private speedup call used by Tracker; keep in sync with
	// GetIcon
	// node considered to have an icon only if has both the large and the mini
	// icon
	
	icon_size which = B_MINI_ICON;
	uint32 type = MINI_ICON_TYPE;
	const char *sizeSpecifier = "M:";

	for (;;) {
		char rsrc_name[256];
		strcpy(rsrc_name, MIME_PREFIX);
		strcat(rsrc_name, sizeSpecifier);	
		strcat(rsrc_name, STD_ICON_SUFFIX);

		if (!has_icon_attr(node, rsrc_name, type))
			return false;
		
		if (miniOnly || which != B_MINI_ICON)
			break;

		which = B_LARGE_ICON;
		type = LARGE_ICON_TYPE;
		sizeSpecifier = "L:";
	}
	return true;
}

}

/* ---------------------------------------------------------------- */

status_t BNodeInfo::GetIcon(BBitmap *icon, icon_size which) const
{
	if (!fNode)
		return B_NO_INIT;

	long		size;
	status_t	err;
	char		rsrc_name[256];
	char		icon_buffer[1024];
	uint32		type;

	strcpy(rsrc_name, MIME_PREFIX);

	if (which == B_LARGE_ICON) {
		size = LARGE_ICON_SIZE;
		type = LARGE_ICON_TYPE;
		strcat(rsrc_name, "L:");
	} else {
		size = MINI_ICON_SIZE;
		type = MINI_ICON_TYPE;
		strcat(rsrc_name, "M:");
	}

	BRect rect(0, 0, size - 1, size - 1);

	if (icon->Bounds() != rect)
		return B_BAD_VALUE;

	strcat(rsrc_name, STD_ICON_SUFFIX);

	err = get_icon_attr(fNode, rsrc_name, type, icon_buffer);
	if (!err)
		icon->SetBits(icon_buffer, icon->BitsLength(), 0, B_COLOR_8_BIT);

	return err;
}

/* ---------------------------------------------------------------- */

status_t BNodeInfo::SetIcon(const BBitmap *icon, icon_size which)
{
	if (!fNode)
		return B_NO_INIT;

	long		size;
	uint32		type;
	char		rsrc_name[256];

	strcpy(rsrc_name, MIME_PREFIX);
	if (which == B_LARGE_ICON) {
		size = LARGE_ICON_SIZE;
		type = LARGE_ICON_TYPE;
		strcat(rsrc_name, "L:");
	} else {
		size = MINI_ICON_SIZE;
		type = MINI_ICON_TYPE;
		strcat(rsrc_name, "M:");
	}

	BRect rect(0, 0, size - 1, size - 1);

	if (icon && (icon->Bounds() != rect))
		return B_BAD_VALUE;

	strcat(rsrc_name, STD_ICON_SUFFIX);
	return set_icon_attr(fNode, rsrc_name, type, 
		icon ? icon->Bits() : NULL, 
		icon ? icon->BitsLength() : 0);
}

/* ---------------------------------------------------------------- */

status_t BNodeInfo::GetPreferredApp(char *sig, app_verb) const
{
	if (!fNode)
		return B_NO_INIT;

	return get_attr(fNode, B_PREFERRED_APP_ATTR, B_MIME_STRING_TYPE,
		sig, B_MIME_TYPE_LENGTH);
}

/* ---------------------------------------------------------------- */

status_t BNodeInfo::SetPreferredApp(const char *sig, app_verb)
{
	if (!fNode)
		return B_NO_INIT;

	int len = sig ? (strlen(sig)+1) : 0;
	if (len > B_MIME_TYPE_LENGTH)
		return B_BAD_VALUE;
	return set_attr(fNode, B_PREFERRED_APP_ATTR, B_MIME_STRING_TYPE,
		sig, len);
}

/* ---------------------------------------------------------------- */

status_t BNodeInfo::GetAppHint(entry_ref *ref) const
{
	if (!fNode)
		return B_NO_INIT;

	/*
	 See if this node has a hardwired application.
	*/
	status_t	err;
	size_t		len;

	if (!ref)
		return B_BAD_VALUE;

	if (ref->name)
		free(ref->name);
	ref->name = NULL;
	ref->device = -1;
	ref->directory = -1;

	if ((err = get_attr_size(fNode, B_PREFERRED_PATH_ATTR, B_MIME_STRING_TYPE,
		&len)) != B_OK) {
			return err;
	}

	char	*p = (char *) malloc(len);
	if (!p)
		return B_NO_MEMORY;

	if ((err = get_attr(fNode, B_PREFERRED_PATH_ATTR, B_MIME_STRING_TYPE,
		p, len)) != B_OK) {
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

status_t BNodeInfo::SetAppHint(const entry_ref *ref)
{
	if (!fNode)
		return B_NO_INIT;

	if (ref && (ref->name == NULL))
		return B_BAD_VALUE;

	status_t	err;
	BPath		path;
	int32		len = 0;
	const char	*p = NULL;

	if (ref) {
		BEntry	entry(ref);

		if ((err = entry.InitCheck()) != B_OK)
			return err;
		if ((err = entry.GetPath(&path)) != B_OK)
			return err;
		p = path.Path();
		len = strlen(p) + 1;
	}

	err = set_attr(fNode, B_PREFERRED_PATH_ATTR, B_MIME_STRING_TYPE, p, len);

	return err;
}

/* ---------------------------------------------------------------- */


_EXPORT status_t
GetTrackerIconPrivate(BBitmap *bits, BNode *node, 
	const char *mimeTypeSignature, icon_size size)
{	
	// this call needs to be in synch with the logic in NuIconCache
	// in Tracker as it is duplicated for perfromance vs. versatility

	BNodeInfo nodeInfo(node);
	
	status_t err = nodeInfo.GetIcon(bits, size);
	if (err == B_OK) 
		// file knew which icon to use, we are done
		return err;

	char preferredApp[B_MIME_TYPE_LENGTH];
	err = nodeInfo.GetPreferredApp(preferredApp);
	if (err == B_OK && preferredApp[0]) {
		BMimeType preferredAppType(preferredApp);
		err = preferredAppType.GetIconForType(mimeTypeSignature, bits, size);
		if (err == B_OK) 
			return err;
	}

	BMimeType mimeType(mimeTypeSignature);

	err = mimeType.GetIcon(bits, size);
	if (err == B_OK) 
		// the system knew what icon to use for the type, we are done
		return err;
	
	err = mimeType.GetPreferredApp(preferredApp);
	if (err != B_OK) 
		// no preferred App for document, give up
		return err;

	BMimeType preferredAppType(preferredApp);
	return preferredAppType.GetIconForType(mimeTypeSignature, bits, size);
}

/* ---------------------------------------------------------------- */

extern "C" int	_kstatfs_(dev_t dev, long *pos, int fd, const char *path,
	struct fs_info *fs);

#define	B_FILE_MIMETYPE		"application/octet-stream"
#define	B_DIR_MIMETYPE		"application/x-vnd.Be-directory"
#define	B_VOLUME_MIMETYPE	"application/x-vnd.Be-volume"
#define	B_LINK_MIMETYPE		"application/x-vnd.Be-symlink"
#define	B_ROOT_MIMETYPE		"application/x-vnd.Be-root"

status_t BNodeInfo::GetTrackerIcon(BBitmap *icon, icon_size size) const
{
	status_t result = InitCheck();
	if (result != B_OK)
		return result;

	char signature[B_MIME_TYPE_LENGTH];
	result = GetType(signature);

	if (result == B_OK) 
		result = GetTrackerIconPrivate(icon, fNode, signature, size);
	
	if (result == B_OK) 
		return result;

	// node has no type or no icon defined for node, return
	// default icons
	struct stat statbuf;
	result = fNode->GetStat(&statbuf);
	if (result != B_OK)
		// can't stat the node, bail
		return result;

	if (S_ISDIR(statbuf.st_mode)) {
		fs_info info;
		result = _kstatfs_(statbuf.st_dev, NULL, -1, NULL, &info);
		if (result != B_OK)
			return result;

		if (info.root == statbuf.st_ino) {
			// volume icon, try getting it
			result = get_device_icon(info.device_name, icon->Bits(), size);
			if (result != B_OK) {
				// return the default volume icon
				BMimeType metamime(B_VOLUME_MIMETYPE);
				result = metamime.GetIcon(icon, size);
			}
		} else {
			// directory
			// return default directory icon
			BMimeType metamime(B_DIR_MIMETYPE);
			result = metamime.GetIcon(icon, size);
		}
	} else if (S_ISREG(statbuf.st_mode)) {
		if (statbuf.st_mode & S_IXUSR) {
			// executable file
			// return default application icon
			BMimeType metamime(B_APP_MIME_TYPE);
			result = metamime.GetIcon(icon, size);
		} else {
			// unknown plain file
			// return default document icon
			BMimeType metamime(B_FILE_MIMETYPE);
			result = metamime.GetIcon(icon, size);
		}
	} else if (S_ISLNK(statbuf.st_mode)) {
		// symlink - should descend but can't, because we only have the node
		// instead return a default symlink icon
		BMimeType metamime(B_LINK_MIMETYPE);
		result = metamime.GetIcon(icon, size);
	} else
		result = B_ERROR;

	return result;
}

/* ---------------------------------------------------------------- */

status_t BNodeInfo::GetTrackerIcon(const entry_ref *ref, BBitmap *icon, 
	icon_size size)
{
	BNode node(ref);

	status_t result = node.InitCheck();
	if (result != B_OK)
		return result;

	BNodeInfo info(&node);

	return info.GetTrackerIcon(icon, size);
}

/* ---------------------------------------------------------------- */

void BNodeInfo::_ReservedNodeInfo1() {}
void BNodeInfo::_ReservedNodeInfo2() {}
void BNodeInfo::_ReservedNodeInfo3() {}

/* ---------------------------------------------------------------- */

BNodeInfo &BNodeInfo::operator=(const BNodeInfo &) { return *this; }
BNodeInfo::BNodeInfo(const BNodeInfo &) {}

/*---------- Deprecated --------- */

#if __GNUC__ || __MWERKS__
extern "C" {

	_EXPORT status_t
	#if __GNUC__
	GetTrackerIcon__9BNodeInfoP9entry_refP7BBitmap9icon_size
	#elif __MWERKS__
	GetTrackerIcon__9BNodeInfoFP9entry_refP7BBitmap9icon_size
	#endif
	(entry_ref *ref, BBitmap *icon, icon_size size)
	{
		return BNodeInfo::GetTrackerIcon(ref, icon, size);
	}

}
#endif

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
