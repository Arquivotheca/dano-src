/*	TranslatorRoster.cpp	*/
/*	$Id: TranslatorRoster.cpp,v 1.1 1997/07/22 21:26:58 hplus Exp $	*/

#include <BeBuild.h>
#include <TranslationDefs.h>
#include <TranslatorRoster.h>
#include <TranslatorAddOn.h>
#include <Translator.h>

#include <OS.h>
#include <image.h>
#include <File.h>
#include <Directory.h>
#include <Volume.h>
#include <string.h>
#include <OS.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <alloca.h>
#include <stdio.h>
#include <Message.h>
#include <Mime.h>
#include <Debug.h>
#include <Locker.h>
#include <Autolock.h>
#include <FindDirectory.h>
#include <String.h>
#include <Path.h>

#include <map>


static BLocker _sImageMapLock("TranslatorImageMap");
static std::map<image_id, int> _sImageMap;
static bool _sImageDebug;
static int32 _sXLID = 1000;


static	BLocker def_lock("TranslatorRoster::Default()");
const char * B_TRANSLATOR_MIME_TYPE = B_APP_MIME_TYPE;
static int32 _sPlainCount = -1;


static const char *
get_dataDefaultTranslatorPath()
{
static char *dataDefaultTranslatorPath;
	BAutolock lock(def_lock);
	if (dataDefaultTranslatorPath == 0) {
		BString s;
		BPath p;
		directory_which whiches[] = {
			B_USER_TRANSLATORS_DIRECTORY,
			B_COMMON_TRANSLATORS_DIRECTORY,
			B_BEOS_TRANSLATORS_DIRECTORY
		};
		for (int ix=0; ix<sizeof(whiches)/sizeof(whiches[0]); ix++) {
			if (!find_directory(whiches[ix], &p, true)) {
				if (s.Length() > 0) s.Append(":");
				s.Append(p.Path());
			}
		}
		if (!find_directory(B_USER_ADDONS_DIRECTORY, &p) && (p.Path() != 0)) {
			p.Append("Datatypes");
			struct stat st;
			if (!stat(p.Path(), &st)) {
				if (s.Length() > 0) s.Append(":");
				s.Append(p.Path());
			}
		}
		dataDefaultTranslatorPath = (char *)malloc(s.Length()+1);
		strcpy(dataDefaultTranslatorPath, s.String());
		if (_sImageDebug) fprintf(stderr, "default translator path: %s\n", dataDefaultTranslatorPath);
	}
	return dataDefaultTranslatorPath;
}


struct _XLInfo
{
					_XLInfo() { memset(this, 0, sizeof(*this)); }
					~_XLInfo() { }

	_XLInfo *		next;

	char			name[NAME_MAX];
	image_id		image;
	int32			id;
	const char *	handlerName;
	const char *	handlerInfo;
	int32			handlerVersion;
	const translation_format *	inputFormats;
	const translation_format *	outputFormats;
	int32			inputFormatCount;
	int32			outputFormatCount;
	BTranslator *	translator;

	status_t		(*Identify)(
						BPositionIO *		inSource,
						const translation_format *		inFormat,
						BMessage *			ioExtension,
						translator_info *			outInfo,
						uint32				outType);
	status_t		(*Translate)(
						BPositionIO *		inSource,
						const translator_info *	inInfo,
						BMessage *			ioExtension,
						uint32				outType,
						BPositionIO *		outDestination);
	status_t		(*MakeConfiguration)(
						BMessage *			ioExtension,
						BView * *			outView,
						BRect *				outExtent);
	status_t		(*GetConfigurationMessage)(
						BMessage *			ioExtension);

};


class _StAcquire
{
	sem_id		fSem;
public:
				_StAcquire(
					sem_id sem)
				{
					acquire_sem(sem);
					fSem = sem;
				}
				~_StAcquire()
				{
					release_sem(fSem);
				}
};


struct BTranslatorRoster::PrivateData
{
	PrivateData()
		{
			debug = 0;
			sInfo = NULL;
			sSem = -1;
		}
	~PrivateData()
		{
		}

	int debug;
	_XLInfo *sInfo;
	sem_id sSem;

};



BTranslatorRoster * BTranslatorRoster::_defaultTranslators = NULL;

void
BTranslatorRoster::_roster_cleanup()
{
	if (BTranslatorRoster::_defaultTranslators != 0) delete BTranslatorRoster::_defaultTranslators;
}

class _AutoRosterDeleter
{
public:
	_AutoRosterDeleter()
	{
		atexit(BTranslatorRoster::_roster_cleanup);
	}
};

static _AutoRosterDeleter _the_deleter;


static void
increment_image_use(
	image_id image)
{
	if (image < 0) {
		if (_sImageDebug) fprintf(stderr, "translators: spurious call to increment_image_use(%d)\n", image);
		return;
	}
	BAutolock lock(_sImageMapLock);
	std::map<image_id, int>::iterator ptr(_sImageMap.find(image));
	if (ptr == _sImageMap.end()) {
		if (_sImageDebug) fprintf(stderr, "translators: new ref for image %d\n", image);
		_sImageMap[image] = 1;
	}
	else {
		(*ptr).second++;
		if (_sImageDebug) fprintf(stderr, "translators: %d refs for image %d\n", (*ptr).second, image);
	}
}

static void
decrement_image_use(
	image_id image)
{
	BAutolock lock(_sImageMapLock);
	std::map<image_id, int>::iterator ptr(_sImageMap.find(image));
	if (ptr == _sImageMap.end()) {
		if (_sImageDebug) fprintf(stderr, "ERROR: decrement_image_use() called for unused image %d\n", image);
		if (_sImageDebug) debugger("Image spuriously decremented\n");
	}
	else {
		if ((*ptr).second-- == 1) {
			if (_sImageDebug) fprintf(stderr, "translators: unload image %d at ref count 0\n", image);
			unload_add_on(image);
		}
		else {
			if (_sImageDebug) fprintf(stderr, "translators: still %d refs for image %d\n", (*ptr).second, image);
		}
	}
}


#define debug _private->debug
#define sInfo _private->sInfo
#define sSem _private->sSem


BTranslatorRoster::BTranslatorRoster()
{
	memset(_reservedTranslators, 0, sizeof(_reservedTranslators));
	_private = new PrivateData;
	if (getenv("TRANSLATION_DEBUG") != NULL) {
		debug = 1;
		_sImageDebug = true;
	}
	if (sSem <= 0)
		sSem = create_sem(1, "BTranslatorRoster Lock");
}


BTranslatorRoster::BTranslatorRoster(
	BMessage * model)
{
	memset(_reservedTranslators, 0, sizeof(_reservedTranslators));
	_private = new PrivateData;
	if (getenv("TRANSLATION_DEBUG") != NULL)
		debug = 1;
	if (sSem <= 0)
		sSem = create_sem(1, "BTranslatorRoster Lock");
	if (sSem <= 0)
		return;
	const char * path = NULL;
	for (int ix=0; !model->FindString("be:translator_path", ix, &path); ix++) {
		if (path != NULL) {
			LoadTranslator(path);
			path = NULL;
		}
	}
	BMessage msg;
	for (int ix=0; !model->FindMessage("be:archived_translator", ix, &msg); ix++) {
		if (validate_instantiation(&msg, "BTranslator")) {
			BTranslator * tr = dynamic_cast<BTranslator *>(instantiate_object(&msg));
			AddTranslator(tr);
		}
	}
}


BTranslatorRoster::~BTranslatorRoster()
{
	if (this == _defaultTranslators)
		_defaultTranslators = NULL;

	if (sSem > 0)
	{
		acquire_sem(sSem);

		_XLInfo *info = sInfo;
		sInfo = NULL;
		while (info != NULL)
		{
			_XLInfo *del = info;
			info = info->next;
			if (debug) fprintf(stderr, "Unloading translator %s\n", del->name);
			if (del->image > -1) {
				decrement_image_use(del->image);
			}
			else {
				del->translator->Release();
			}
			delete del;
		}

		delete_sem(sSem);
		sSem = 0;
	}

	delete _private;
}


status_t
BTranslatorRoster::Archive(
	BMessage *into, 
	bool deep) const
{
	/*	remember a set of translators	*/
	status_t err = B_OK;
	for (_XLInfo *info = sInfo; info != NULL; info = info->next)
	{
		image_info iminfo;
		if (info->translator != NULL) {
			BMessage msg(B_ARCHIVED_OBJECT);
			status_t acc = info->translator->Archive(&msg, deep);
			if (!acc) {
				acc = into->AddMessage("be:archived_translator", &msg);
				if (acc < err) err = acc;
			}
		}
		else if ((info->image > -1) && !get_image_info(info->image, &iminfo))
		{
			status_t acc = into->AddString("be:translator_path", iminfo.name);
			if (acc < err) err = acc;
		}
	}
	return err;
}


BArchivable *
BTranslatorRoster::Instantiate(
	BMessage *from)
{
	if (validate_instantiation(from, "BTranslatorRoster"))
		return new BTranslatorRoster(from);
	return NULL;
}



status_t
BTranslatorRoster::AddTranslators(
	const char *	path)
{
	_StAcquire lock(sSem);

	status_t loadErr = 0;
	int32 nLoaded = 0;

	if (path == NULL)
	{
		path = getenv("TRANSLATORS");
	}
	if (path == NULL)
	{
		path = get_dataDefaultTranslatorPath();
	}
	/*	else parse path syntax; load folders and files	*/
	char pathbuf[PATH_MAX];
	const char *ptr = path;
	const char *end = ptr;
	struct stat stbuf;
	while (*ptr != 0)
	{
		/*	find segments specified by colons	*/
		end = strchr(ptr, ':');
		if (end == NULL)
		{
			end = ptr+strlen(ptr);
		}
		if (end-ptr > PATH_MAX-1)
		{
			loadErr = B_BAD_VALUE;
			if (debug) fprintf(stderr, "too long path!\n");
		}
		else
		{
			/*	copy this segment of the path into a path, and load it	*/
			memcpy(pathbuf, ptr, end-ptr);
			pathbuf[end-ptr] = 0;
			if (debug) fprintf(stderr, "load path: %s\n", pathbuf);
			if (!stat(pathbuf, &stbuf))
			{
				/*	files are loaded as translators	*/
				if (S_ISREG(stbuf.st_mode))
				{
					status_t err = LoadTranslator(pathbuf);
					if (err != B_OK)
						loadErr = err;
					else
						nLoaded++;
				}
				else
				{
					/*	directories are scanned	*/
					LoadDir(pathbuf, loadErr, nLoaded);
				}
			}
			else if (debug) fprintf(stderr, "cannot stat()!\n");
		}
		ptr = end+1;
		if (*end == 0)
			break;
	}

	/*	if anything loaded, it's not too bad	*/
	if (nLoaded) {
		if (debug) fprintf(stderr, "A total of %d translators loaded\n", nLoaded);
		loadErr = B_OK;
	}

	return loadErr;
}


status_t
BTranslatorRoster::AddTranslator(
	BTranslator * translator)
{
	_StAcquire lock(sSem);

	return AddTranslatorLocked(translator, atomic_add(&_sPlainCount, -1));
}

status_t
BTranslatorRoster::AddTranslatorLocked(
	BTranslator * translator,
	image_id image)
{
	_XLInfo *info = new _XLInfo;
	strncpy(info->name, translator->TranslatorName(), NAME_MAX);
	info->name[NAME_MAX-1] = 0;
	info->image = image;
	info->id = atomic_add(&_sXLID, 1);
	if (image > 0) increment_image_use(image);

	/*	find all the symbols	*/
	info->handlerName = translator->TranslatorName();
	info->handlerInfo = translator->TranslatorInfo();
	info->handlerVersion = translator->TranslatorVersion();
	int32 count = 0;
	info->inputFormats = translator->InputFormats(&count);
	info->inputFormatCount = count;
	count = 0;
	info->outputFormats = translator->OutputFormats(&count);
	info->outputFormatCount = count;

	if (!info->handlerName || !info->handlerInfo || !info->handlerVersion) {	/*	this is not a correct add-on	*/
		delete info;
		if (debug) fprintf(stderr, "AddTranslator() returns error\n");
		return B_BAD_VALUE;
	}
	info->translator = translator->Acquire();
	if (debug) fprintf(stdout, "AddTranslator() %s\n", info->handlerName);

	/*	add to global list	*/
	info->next = sInfo;
	sInfo = info;

	return B_OK;
}



const char *
BTranslatorRoster::Version(	/*	returns version string	*/
	int32 * outCurVersion,	/*	current version spoken	*/
	int32 * outMinVersion,	/*	minimum version understood	*/
	int32 appVersion)		/*	what app thinks it's doing	*/
{
	static char vString[50];
	static char vDate[] = "" /*__DATE__ */;
	if (!vString[0])
	{
		sprintf(vString, "Translation Kit v%d.%d.%d %s\n",
			B_TRANSLATION_CURRENT_VERSION/100,
			(B_TRANSLATION_CURRENT_VERSION/10)%10,
			B_TRANSLATION_CURRENT_VERSION%10,
			vDate);
	}
	if (outCurVersion)
		*outCurVersion = B_TRANSLATION_CURRENT_VERSION;
	if (outMinVersion)
		*outMinVersion = B_TRANSLATION_MIN_VERSION;
	appVersion = appVersion;	/*	ignore app's version until we need it	*/
	return vString;
}


extern "C" {
	typedef BTranslator * (*make_translator_func)(int32 n, image_id i, uint32 f, ...);
}

status_t
BTranslatorRoster::LoadTranslator(
	const char *	 path)
{
	/*	check that this ref is not already loaded	*/
	bool found = FALSE;
	const char *name = strrchr(path, '/');
	if (name)
		name++;
	else
		name = path;
	for (_XLInfo *i = sInfo; i && !found; i=i->next)
		if (!strcmp(name, i->name))
			found = TRUE;
	if (debug) fprintf(stderr, "LoadTranslator(%s) already = %s\n", path, found ? "true" : "false");
	/*	we use name for determining whether it's loaded	*/
	/*	that is not entirely foolproof, but making SURE will be 	*/
	/*	a very slow process that I don't much care for.	*/
	if (found)
		return B_NO_ERROR;

	/*	go ahead and load it	*/
	image_id image = load_add_on(path);
	if (debug) fprintf(stderr, "load_add_on(%s) = %d\n", path, image);
	if (image < 0)
		return image;

	make_translator_func make_nth;
	status_t err = B_OK;

	/*	try for a class-based add-on	*/
	err = get_image_symbol(image, "make_nth_translator",
			B_SYMBOL_TYPE_TEXT, (void **)&make_nth);
	if ((err == B_OK) && (make_nth != 0)) {
		int32 n = 0;
		BTranslator * toAdd = 0;
		while ((toAdd = (*make_nth)(n, image, 0)) != 0) {
			err = AddTranslatorLocked(toAdd, image);
			toAdd->Release();	//	because we're holding on to it in the table
			if (err < B_OK) {
				return err;
			}
			n++;
		}
		//	take an early exit
		return B_OK;
	}

	_XLInfo *info = new _XLInfo;
	strcpy(info->name, name);
	info->image = image;
	info->id = atomic_add(&_sXLID, 1);
	increment_image_use(image);

	/*	find all the symbols	*/
	err = get_image_symbol(image, "translatorName", 
			B_SYMBOL_TYPE_DATA, (void **)&info->handlerName);
	if (err < B_OK) err = get_image_symbol(image, "handlerName", 
			B_SYMBOL_TYPE_DATA, (void **)&info->handlerName);
	if (debug && err) fprintf(stderr, "get_image_symbol(translatorName) = %x\n", err);
	if (!err && get_image_symbol(image, "translatorInfo",
			B_SYMBOL_TYPE_DATA, (void **)&info->handlerInfo) && 
			get_image_symbol(image, "handlerInfo", 
			B_SYMBOL_TYPE_DATA, (void **)&info->handlerInfo))
		info->handlerInfo = NULL;
	long * vptr = NULL;
	if (!err) {
		err = get_image_symbol(image, "translatorVersion",
				B_SYMBOL_TYPE_DATA, (void **)&vptr);
		if (err) {
			err = get_image_symbol(image, "handlerVersion", 
					B_SYMBOL_TYPE_DATA, (void **)&vptr);
		}
		if (debug && err) fprintf(stderr, "get_image_symbol(translatorVersion) = %x\n", err);
	}
	if (!err && (vptr != NULL))
		info->handlerVersion = *vptr;
	if (!err && get_image_symbol(image, "inputFormats",
			B_SYMBOL_TYPE_DATA, (void **)&info->inputFormats))
		info->inputFormats = NULL;
	else if (!err)
	{
		const translation_format * f = info->inputFormats;
		while (f->type) {
			f++;
		}
		info->inputFormatCount = f-info->inputFormats;
	}
	if (!err && get_image_symbol(image, "outputFormats",
			B_SYMBOL_TYPE_DATA, (void **)&info->outputFormats))
		info->outputFormats = NULL;
	else if (!err)
	{
		const translation_format * f = info->outputFormats;
		while (f->type) {
			f++;
		}
		info->outputFormatCount = f-info->outputFormats;
	}
	if (!err) {
		err = get_image_symbol(image, "Identify", B_SYMBOL_TYPE_TEXT,
				(void **)&info->Identify);
		if (debug && err) fprintf(stderr, "get_image_symbol(Identify) = %x\n", err);
	}
	if (!err) {
		err = get_image_symbol(image, "Translate",
				B_SYMBOL_TYPE_TEXT, (void **)&info->Translate);
		if (debug && err) fprintf(stderr, "get_image_symbol(Translate) = %x\n", err);
	}
	if (!err && get_image_symbol(image, "MakeConfig",
			B_SYMBOL_TYPE_TEXT, (void **)&info->MakeConfiguration))
		info->MakeConfiguration = NULL;
	if (!err && get_image_symbol(image, "GetConfigMessage",
			B_SYMBOL_TYPE_TEXT, (void **)&info->GetConfigurationMessage))
		info->GetConfigurationMessage = NULL;

	if (err) {	/*	this is not a correct add-on	*/
		if (debug) fprintf(stderr, "...that was not a correct add-on (%s)\n", strerror(err));
		delete info;
		return err;
	}
	else if (debug) fprintf(stderr, "%s loaded\n", info->handlerName);

	/*	add to global list	*/
	/*	note that BTranslatorRoster(BMessage *) DEPENDS on us putting the new translator first!	*/
	info->next = sInfo;
	sInfo = info;

	return B_NO_ERROR;
}


BTranslatorRoster *
BTranslatorRoster::Default()
{
	def_lock.Lock();
	if (!_defaultTranslators)
	{
		_defaultTranslators = new BTranslatorRoster;
		_defaultTranslators->AddTranslators();
	}
	def_lock.Unlock();
	return _defaultTranslators;
}


void
BTranslatorRoster::LoadDir(
	const char *	path,
	int32 &			loadErr,
	int32 &			nLoaded)
{
	loadErr = B_OK;
	DIR *	dir = opendir(path);
	if (debug) fprintf(stderr, "LoadDir(%s) opendir() = %08x\n", path, dir);
	if (!dir)
	{
		loadErr = B_FILE_NOT_FOUND;
		return;
	}
	struct dirent *dent;
	struct stat stbuf;
	char cwd[PATH_MAX] = "";
	while (NULL != (dent = readdir(dir)))
	{
		strcpy(cwd, path);
		strcat(cwd, "/");
		strcat(cwd, dent->d_name);
		status_t err = stat(cwd, &stbuf);
		if (debug) fprintf(stderr, "stat(%s) = %08x\n", cwd, err);
		if (!err && S_ISREG(stbuf.st_mode) &&
			strcmp(dent->d_name, ".") && strcmp(dent->d_name, ".."))
		{
			err = LoadTranslator(cwd);
			if (B_OK == err)
			{
				nLoaded++;
			}
			else
			{
				loadErr = err;
			}
			if (debug) fprintf(stderr, "LoadTranslator(%s) = %d  (%d/%08x)\n", cwd, err, nLoaded, loadErr);
		}
	}
	closedir(dir);
}


/*	these functions call through to the translators	*/


/*	CheckFormats is a utility function that returns TRUE if the 	*/
/*	data provided and translator info can be used to make a 	*/
/*	determination (even if that termination is negative) and 	*/
/*	FALSE if content identification has to be done.	*/
bool
BTranslatorRoster::CheckFormats(
	_XLInfo * info,
	uint32 hintType,
	const char * hintMIME,
	const translation_format * & outFormat)
{
	outFormat = NULL;

	/*	return FALSE if we can't use hints for this module	*/
	
	if (!hintType && !hintMIME)
		return FALSE;
	if (!info->inputFormats)
		return FALSE;

	/*	scan for suitable format	*/
	
	const translation_format *fmt = info->inputFormats;
	int in_cnt = info->inputFormatCount;
	int mlen = 0;
	/*	check for the length of the MIME string, since it may be just a prefix	*/
	/*	so we use strncasecmp().	*/
	if (hintMIME)
		mlen = strlen(hintMIME);
	while (in_cnt--)
	{
		if ((fmt->type == hintType) ||
				(hintMIME && mlen && !strncasecmp(fmt->MIME, hintMIME, mlen)))
		{
			outFormat = fmt;
			return TRUE;
		}
		fmt++;
	}
	/*	the module did export formats, but none mathced.	*/
	/*	we return true (uses formats) but set outFormat to NULL	*/
	return TRUE;
}


status_t
BTranslatorRoster::Identify(	/*	find out what something is	*/
	BPositionIO *		inSource,	/* not NULL */
	BMessage *			ioExtension,
	translator_info *	outInfo,	/* not NULL */
	uint32				inHintType,
	const char *		inHintMIME,
	uint32				inWantType)
{
	if (debug) fprintf(stderr, "Identify called (%d, %p, %p, %p)\n", sSem, inSource, outInfo, sInfo);
	if (sSem <= 0)
		return B_NOT_INITIALIZED;
	if (!inSource || !outInfo)
		return B_BAD_VALUE;

	_XLInfo *info = sInfo;
	_XLInfo *bestMatch = NULL;
	float bestWeight = 0.0;
	while (info)
	{
		const translation_format *format = NULL;
		translator_info tmpInfo;
		float weight = 0.0;

		status_t err = B_OK;
		off_t o_err = (*inSource).Seek(0, SEEK_SET);
		if (o_err < 0)
			err = o_err;
		if (err < B_OK)
		{
			if (debug) fprintf(stderr, "Seek() returns %s (%Ld)\n", strerror(o_err), o_err);
			return err;
		}

		if (debug) {
			fprintf(stderr, "Identify() using %s\n", info->handlerName);
		}
		if (CheckFormats(info, inHintType, inHintMIME, format))
		{
			/*	after checking the formats for hints, we still need to make sure the translator recognizes 	*/
			/*	the data and can output the desired format, so we call its' Identify() function.	*/
			if (debug) fprintf(stderr, "CheckFormats(%s) OK\n", info->name);
			if (format && (
				(info->Identify && !info->Identify(inSource, format, ioExtension, &tmpInfo, inWantType)) ||
				(info->translator && !info->translator->Identify(inSource, format, ioExtension, &tmpInfo, inWantType))))
			{
				goto add_match;
			}
		}
		else if (
			(info->Identify && !info->Identify(inSource, NULL, ioExtension, &tmpInfo, inWantType)) ||
			(info->translator && !info->translator->Identify(inSource, NULL, ioExtension, &tmpInfo, inWantType)))
		{
			if (debug) fprintf(stderr, "Identify(%s) OK\n", info->name);
	add_match:
			tmpInfo.translator = info->id;
			weight = tmpInfo.quality * tmpInfo.capability;

			if (weight > bestWeight)
			{
				bestMatch = info;
				bestWeight = weight;
				(*outInfo) = tmpInfo;
			}
		}
		else {
			if (debug) {
				fprintf(stderr, "%s refuses\n", info->name);
			}
		}
		info = info->next;
	}
	return bestMatch ? B_NO_ERROR : B_NO_TRANSLATOR;
}


	static int
	_compare_data(
		const void *a,
		const void *b)
	{
		register translator_info *ai = (translator_info *)a;
		register translator_info *bi = (translator_info *)b;
		register float f = - ai->quality*ai->capability +
				bi->quality*bi->capability;
		return (f < 0.0) ? -1 : (f > 0.0) ? 1 : 0;
	}

status_t
BTranslatorRoster::GetTranslators(	/*	find all handlers for a type	*/
	BPositionIO *		inSource,
	BMessage *			ioExtension,
	translator_info * *		outInfo,	/*	call delete[] on outInfo when done	*/
	int32 *				outNumInfo,
	uint32				inHintType,
	const char *		inHintMIME,
	uint32				inWantType)
{
	if (!inSource || !outInfo || !outNumInfo)
		return B_BAD_VALUE;

	(*outInfo) = NULL;
	(*outNumInfo) = 0;

	if (sSem <= 0)
		return B_NOT_INITIALIZED;

	int32 physCnt = 10;
	(*outInfo) = new translator_info[physCnt];
	(*outNumInfo) = 0;

	_XLInfo *info = sInfo;

	while (info)
	{
		const translation_format *format = NULL;
		translator_info tmpInfo;

		status_t err = B_OK;
		off_t o_err = (*inSource).Seek(0, SEEK_SET);
		if (o_err < 0)
			err = o_err;
		if (err < B_OK)
		{
			delete (*outInfo);
			(*outInfo) = NULL;
			return err;
		}

		if (CheckFormats(info, inHintType, inHintMIME, format))
		{
			if (format && (
				(info->Identify && !info->Identify(inSource, format, ioExtension, &tmpInfo, inWantType)) ||
				(info->translator && !info->translator->Identify(inSource, format, ioExtension, &tmpInfo, inWantType))))
			{
				goto add_match;
			}
		}
		else if (
			(info->Identify && !info->Identify(inSource, NULL, ioExtension, &tmpInfo, inWantType)) ||
			(info->translator && !info->translator->Identify(inSource, NULL, ioExtension, &tmpInfo, inWantType)))
		{
	add_match:
			/*	dynamically resize output list	*/
			
			if (physCnt <= (*outNumInfo))
			{
				physCnt += 10;
				translator_info *nOut = new translator_info[physCnt];
				for (int ix=0; ix<(*outNumInfo); ix++)
				{
					nOut[ix] = (*outInfo)[ix];
				}
				delete[] (*outInfo);
				(*outInfo) = nOut;
			}

			tmpInfo.translator = info->id;

			(*outInfo)[(*outNumInfo)++] = tmpInfo;
		}
		info = info->next;
	}
	if ((*outNumInfo) > 1)
	{
		qsort((*outInfo), (*outNumInfo), sizeof(*(*outInfo)), _compare_data);
	}
	return (*outNumInfo) ? B_NO_ERROR : B_NO_TRANSLATOR;
}


status_t
BTranslatorRoster::GetAllTranslators(
	translator_id * * outList,
	int32 * outCount)
{
	if (!outList || !outCount)
		return B_BAD_VALUE;

	(*outList) = NULL;
	(*outCount) = 0;

	if (sSem <= 0)
		return B_NOT_INITIALIZED;

	/*	count handlers	*/
	_XLInfo *info = NULL;
	for (info = sInfo; info != NULL; info = info->next)
		(*outCount)++;
	(*outList) = new translator_id[(*outCount)];
	(*outCount) = 0;
	for (info = sInfo; info != NULL; info = info->next)
		(*outList)[(*outCount)++] = info->id;

	return B_NO_ERROR;
}


status_t
BTranslatorRoster::GetTranslatorInfo(
	translator_id		forTranslator,
	const char * *		outName,
	const char * *		outInfo,
	int32 *				outVersion)
{
	if (sSem <= 0)
		return B_NOT_INITIALIZED;

	/*	find the handler we've requested	*/
	_XLInfo *info = NULL;
	for (info = sInfo; info != NULL; info = info->next)
	{
		if ((info->id) == forTranslator)
			break;
	}
	if (!info)
		return B_NO_TRANSLATOR;

	if (outName)
		*outName = info->handlerName;
	if (outInfo)
		*outInfo = info->handlerInfo;
	if (outVersion)
		*outVersion = info->handlerVersion;

	return B_NO_ERROR;
}


status_t
BTranslatorRoster::GetInputFormats(
	translator_id				forTranslator,
	const translation_format * *	outFormats,		/*	not NULL, don't write contents!	*/
	int32 *				outNumFormats)			/*	not NULL	*/
{
	if (!outFormats)
		return B_BAD_VALUE;
	if (!outNumFormats)
		return B_BAD_VALUE;

	(*outFormats) = NULL;
	(*outNumFormats) = 0;

	if (sSem <= 0)
		return B_NOT_INITIALIZED;

	/*	find the handler we've requested	*/
	_XLInfo *info = NULL;
	for (info = sInfo; info != NULL; info = info->next)
	{
		if ((info->id) == forTranslator)
			break;
	}
	if (!info)
		return B_NO_TRANSLATOR;

	(*outFormats) = info->inputFormats;
	*outNumFormats = info->inputFormatCount;

	return B_NO_ERROR;
}


status_t
BTranslatorRoster::GetOutputFormats(
	translator_id forTranslator,
	const translation_format * * outFormats,
	int32 * outNumFormats)
{
	if (!outFormats)
		return B_BAD_VALUE;
	if (!outNumFormats)
		return B_BAD_VALUE;

	(*outFormats) = NULL;
	(*outNumFormats) = 0;

	if (sSem <= 0)
		return B_NOT_INITIALIZED;

	/*	find the handler we've requested	*/
	_XLInfo *info = NULL;
	for (info = sInfo; info != NULL; info = info->next)
	{
		if ((info->id) == forTranslator)
			break;
	}
	if (!info)
		return B_NO_TRANSLATOR;

	(*outFormats) = info->outputFormats;
	(*outNumFormats) = info->outputFormatCount;

	return B_NO_ERROR;
}


status_t
BTranslatorRoster::Translate(	/*	morph data into form we want	*/
	BPositionIO *		inSpec,
	const translator_info *	inInfo,			/*	may be NULL	*/
	BMessage *			ioExtension,
	BPositionIO *		outSpec,
	uint32 				inWantOutType,
	uint32				inHintType,
	const char *		inHintMIME)
{
	if (debug) fprintf(stderr, "Translate called (%d, %p, %p)\n", sSem, inSpec, outSpec);
	if (!inSpec || !outSpec)
		return B_BAD_VALUE;

	translator_info stat_info;

	if (sSem <= 0)
		return B_NOT_INITIALIZED;

	if (!inInfo) {	/*	go look for a suitable translator?	*/

		inInfo = &stat_info;

		status_t err = Identify(inSpec, ioExtension, &stat_info, 
				inHintType, inHintMIME, inWantOutType);
		if (err)
			return err;
	}

	_XLInfo *info = NULL;
	for (info = sInfo; info != NULL; info = info->next)
	{
		if ((info->id) == inInfo->translator)
			break;
	}
	if (!info)
		return B_NO_TRANSLATOR;

	status_t err = B_OK;
	off_t o_err = (*inSpec).Seek(0, SEEK_SET);
	if (o_err < 0)
		err = o_err;
	if (err < B_OK)
	{
		return err;
	}

	return (info->Translate ? info->Translate(inSpec, inInfo, ioExtension, inWantOutType, outSpec) :
		(info->translator ? info->translator->Translate(inSpec, inInfo, ioExtension, inWantOutType, outSpec) :
		B_NO_TRANSLATOR));
}


status_t
BTranslatorRoster::Translate(	/*	Use a specific translator	*/
	translator_id inTranslator,
	BPositionIO *		inSpec,
	BMessage *			ioExtension,
	BPositionIO *		outSpec,
	uint32 				inWantOutType)
{
	if (debug) fprintf(stderr, "Translate(%d) called (%d, %p, %p)\n", sSem, inTranslator, inSpec, outSpec);
	if (!inSpec || !outSpec)
		return B_BAD_VALUE;

	translator_info stat_info;

	if (sSem <= 0)
		return B_NOT_INITIALIZED;

	_XLInfo *info = NULL;
	for (info = sInfo; info != NULL; info = info->next)
	{
		if ((info->id) == inTranslator)
			break;
	}
	if (!info)
		return B_NO_TRANSLATOR;

	status_t err = B_OK;
	off_t o_err = (*inSpec).Seek(0, SEEK_SET);
	if (o_err < 0)
		err = o_err;
	if (err < B_OK)
		return err;
	if (info->Identify)
		err = info->Identify(inSpec, NULL, ioExtension, &stat_info, inWantOutType);
	else if (info->translator)
		err = info->translator->Identify(inSpec, NULL, ioExtension, &stat_info, inWantOutType);
	else
		err = B_NO_TRANSLATOR;
	if (err)
		return err;

	o_err = (*inSpec).Seek(0, SEEK_SET);
	if (o_err < 0)
		err = o_err;
	if (err < B_OK)
		return err;

	return (info->Translate ? info->Translate(inSpec, &stat_info, ioExtension, inWantOutType, outSpec) :
		(info->translator ? info->translator->Translate(inSpec, &stat_info, ioExtension, inWantOutType, outSpec) :
		B_NO_TRANSLATOR));
}


/*	A translator may support settings for things such as preferred bit depth, etc.	*/
/*	Create a BView that contains controls and logic to "run" and change these settings.	*/

status_t
BTranslatorRoster::MakeConfigurationView(
	translator_id		forTranslator,
	BMessage *			ioExtension,
	BView * *			outView,
	BRect *				outExtent)
{
	if (!outView || !outExtent)
		return B_BAD_VALUE;

	if (sSem <= 0)
		return B_NOT_INITIALIZED;

	_XLInfo *info = NULL;
	for (info = sInfo; info != NULL; info = info->next)
	{
		if ((info->id) == forTranslator)
			break;
	}
	if (!info)
		return B_NO_TRANSLATOR;

	if (info->MakeConfiguration)
		return info->MakeConfiguration(ioExtension, outView, outExtent);
	if (info->translator)
		return info->translator->MakeConfigurationView(ioExtension, outView, outExtent);
	return B_NO_TRANSLATOR;
}


/*	For batch translation using different kinds of settings, a Datatype could 	*/
/*	support returning an ioExtension message with its parameters in it. It will 	*/
/*	then be passed to Translate in the ioExtension parameter.	*/
/*	Copy into the message, which will already be allocated.	*/

status_t
BTranslatorRoster::GetConfigurationMessage(
	translator_id				forTranslator,
	BMessage *			ioExtension)
{
	if (!ioExtension)
		return B_BAD_VALUE;

	if (sSem <= 0)
		return B_NOT_INITIALIZED;

	_XLInfo *info = NULL;
	for (info = sInfo; info != NULL; info = info->next)
	{
		if ((info->id) == forTranslator)
			break;
	}
	if (!info)
		return B_NO_TRANSLATOR;

	if (info->GetConfigurationMessage)
		return info->GetConfigurationMessage(ioExtension);
	if (info->translator)
		return info->translator->GetConfigurationMessage(ioExtension);
	return B_NO_TRANSLATOR;
}


/* You can get the entry_ref for a specific Translator using GetRefFor() */
/* If you want to display its' icon or something. */
status_t
BTranslatorRoster::GetRefFor(
	translator_id translator,
	entry_ref * out_ref)
{
	if (!out_ref) {
		return B_BAD_VALUE;
	}
	if (sSem <= 0) {
		return B_NOT_INITIALIZED;
	}

	_XLInfo * info;
	for (info = sInfo; info != NULL; info = info->next) {
		if ((info->id) == translator)
			break;
	}
	if (!info) {
		return B_NO_TRANSLATOR;
	}
	if (info->image < 0) {
		return ENOENT;
	}
	image_info imin;
	status_t err = get_image_info(info->image, &imin);
	return (err < B_OK) ? err : get_ref_for_path(imin.name, out_ref);
}


/*	Welcome to the future!	*/
void
BTranslatorRoster::ReservedTranslatorRoster1()
{
}

void
BTranslatorRoster::ReservedTranslatorRoster2()
{
}

void
BTranslatorRoster::ReservedTranslatorRoster3()
{
}

void
BTranslatorRoster::ReservedTranslatorRoster4()
{
}

void
BTranslatorRoster::ReservedTranslatorRoster5()
{
}

void
BTranslatorRoster::ReservedTranslatorRoster6()
{
}








#if 0
/*	These are uninteresting, but here to avoid link warnings	*/

translation_format::translation_format()
{
	memset(this, 0, sizeof(this));
}

translation_format::~translation_format()
{
	/*	do nothing	*/
}

translation_format::translation_format(
	const translation_format & that)
{
	memcpy(this, &that, sizeof(this));
}

translation_format &
translation_format::operator=(
	const translation_format & that)
{
	memcpy(this, &that, sizeof(this));
	return *this;
}

#endif


