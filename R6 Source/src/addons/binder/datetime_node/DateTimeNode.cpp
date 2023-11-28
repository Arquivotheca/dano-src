/*
 * DateTimeNode.cpp
 *
 *  A Binder node that provides an interface to setting the system clock,
 *	querying for timezone information, and setting the timezone.
 *
 *  Exposed properties are (in alphabetical order):
 *
 *		getTimezone()
 *			: returns a string representing the system timezone (such
 *			  as "Pacific")
 *
 *		setDate(month, day, year)
 *			: sets the system date.  Example: setDate(0, 0, 2000) == Jan 1, 2000
 *
 *		setTime(hour, minute, second)
 *			: sets the system time (the RTC). Example: setTime(14, 15, 03) == 02:15:03 PM
 *
 *		setTimezone(timezoneName)
 *			: sets the system timezone.  Pass in a string from the
 *			  timezones collection.
 *
 *		timezones
 *			: a BinderContainer holding a collection of strings, one for each timezone
 *			  that the system knows about.  The names of the collection items are
 *			  suitable for displaying to a user in a list, and the values are the full
 *			  pathname of that timezone file on disk.
 *
 *
 *	TODO:
 *	
 *		* Store timezone data in a compressed archive, and only write out the timezone
 *		  file that is actually used when a new timezone is set.
 *		* Add support for timezone groups (instead of just a flat list like it is now)
 */

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <Binder.h>
#include <FindDirectory.h>
#include <List.h>
#include <OS.h>
#include <String.h>
#include <priv_syscalls.h>

/* from TimeWindow.cpp in the Time pref panel */
static void write_rtc_setting(bool is_local)
{
	char setting_file[MAXPATHLEN+1];
	int fd;

	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, TRUE, setting_file,
		MAXPATHLEN);
	strcat(setting_file, "/" RTC_SETTINGS_FILE);

	fd = open(setting_file, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		return; // XXX make settings directory? What to do here.
	}
	
	write(fd, (is_local ? "local\n": "gmt  \n"), 6);
	close(fd);
	
	_kset_tzfilename_(NULL, 0, !is_local);
}

/* from TimeWindow.cpp in the Time pref panel */
/* returns true if RTC is set to local time, false if RTC is set to GMT */
static bool get_rtc_setting()
{
	char setting_file[MAXPATHLEN+1];
	char c = 0;
	int fd;

	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, TRUE, setting_file,
		MAXPATHLEN);
	strcat(setting_file, "/" RTC_SETTINGS_FILE);

	fd = open(setting_file, O_RDONLY);
	if (fd < 0) {
		write_rtc_setting(false);	// default to GMT
		tzset();
		return false;
	}
	
	read(fd, &c, 1);
	close(fd);
	if (c == 'g')
		return false;
	else
		return true;
}


// NOTE: these properties need to stay in alphabetical order.  If a new property
// is inserted in this list, code to handle it should be added to ReadProperty().
static const char*	properties[] = {
						"getTimezone",
						"setDate",
						"setTime",
						"setTimezone",
						"timezones"
};

static const int32	numProperties = (sizeof(properties) / sizeof(properties[0]));


class DateTimeNode : public BinderNode
{
public:
								DateTimeNode();
	virtual						~DateTimeNode();

protected:

	struct store_cookie {
		int32 index;
	};

	virtual	status_t			OpenProperties(void **cookie, void *copyCookie);
	virtual	status_t			NextProperty(void *cookie, char *nameBuf, int32 *len);
	virtual	status_t			CloseProperties(void *cookie);
	
	virtual	put_status_t		WriteProperty(const char *name, const property &prop);
	virtual	get_status_t		ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

private:

			status_t			PopulateTimezones();
			status_t			SetTime(int hours, int minutes, int seconds);
			status_t			SetDate(int month, int day, int year);
			status_t			GetTimezone(BString &outStr);
			status_t			SetTimezone(const char *tzName);

	binder_node			fTZContainer;
};


DateTimeNode::DateTimeNode()
	: BinderNode(),
	  fTZContainer(NULL)
{
	BString tmp;
	GetTimezone(tmp);	// causes the timezone to be initialized if it is undefined
	BinderContainer *container = new BinderContainer();
	container->SetOrdered(true);
	fTZContainer = container;
	PopulateTimezones();
	// make sure the system is set to GMT, not local time
	if (get_rtc_setting()) {
		// NOTE: this might be a bit annoying since it will end up setting people's desktop
		// machines to RTC == GMT mode from RTC == localtime.
		write_rtc_setting(false);
		tzset();
	}
}

DateTimeNode::~DateTimeNode() {
}


status_t DateTimeNode::OpenProperties(void **cookie, void *copyCookie)
{
	store_cookie *c = new store_cookie;
	if (copyCookie) {
		*c = *((store_cookie*)copyCookie);
	} else {
		c->index = 0;
	}
	*cookie = c;
	return B_OK;
}

status_t DateTimeNode::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	status_t err = ENOENT;
	store_cookie *c = (store_cookie*)cookie;

	if (c->index < numProperties) {	
		const char *name = properties[c->index];
		strncpy(nameBuf, name, *len);
		*len = strlen(name);
		c->index++;
		err = B_OK;
	}
	
	return err;
}

status_t DateTimeNode::CloseProperties(void *cookie)
{
	store_cookie *c = (store_cookie*)cookie;
	delete c;
	return B_OK;
}

put_status_t DateTimeNode::WriteProperty(const char *, const property &)
{
	return put_status_t(B_ERROR,true);
}

get_status_t DateTimeNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	prop.Undefine();
	
	if (!strcmp(name, "getTimezone")) {
		// no parameters
		if (args.CountItems() == 0) {
			BString tz;
			GetTimezone(tz);
			prop = property(tz);
			return get_status_t(B_OK, false);
		}
	} else
	if (!strcmp(name, "setDate")) {
		// needs 3 integer parameters
		if (args.CountItems() == 3) {
			int month	= (int)args[0].Number();
			int day		= (int)args[1].Number();
			int year	= (int)args[2].Number();

			status_t s = SetDate(month, day, year);
			prop = property((int)s);
			return get_status_t(s, false);
		}
	} else
	if (!strcmp(name, "setTime")) {
		// needs 3 integer parameters
		if (args.CountItems() == 3) {
			int hours	= (int)args[0].Number();
			int minutes	= (int)args[1].Number();
			int seconds	= (int)args[2].Number();

			status_t s = SetTime(hours, minutes, seconds);
			prop = property((int)s);
			return get_status_t(s, false);
		}
	} else
	if (!strcmp(name, "setTimezone")) {
		// needs 1 string parameter
		if (args.CountItems() == 1) {
			BString str(args[0].String());

			status_t s = SetTimezone(str.String());
			prop = property((int)s);
			return get_status_t(s, false);			
		}
	} else
	if (!strcmp(name, "timezones")) {
		// no parameters
		if (args.CountItems() == 0) {
			prop = property(fTZContainer);
			return get_status_t(B_OK, false);
		}
	}

	return ENOENT;
}

void replace_chars(char *s, char remove, char add)
{
	for (int i = 0; i < NAME_MAX  &&  s[0] != 0; i++) {
		if (s[i] == remove)
			s[i] = add;
	}
}

struct tz_entry {
	char *name;
	char *path;

	tz_entry(const char *name, const char *path) {
		this->name = strdup(name);
		this->path = strdup(path);
	}
	
	~tz_entry() {
		delete name;
		delete path;
	}

	static int sort(const void *v1, const void *v2) {
		tz_entry *e1, *e2;
		e1 = (tz_entry*)*((tz_entry**)v1);
		e2 = (tz_entry*)*((tz_entry**)v2);
		char *slash1 = strchr(e1->name, '/');
		char *slash2 = strchr(e2->name, '/');

		if ((!slash1 && !slash2) || (slash1 && slash2)) {
			return strcmp(e1->name, e2->name);
		} else if (slash1) {
			// items in directories always come after top-level items
			return 1;
		} else if (slash2) {
			return -1;
		}
		return 0;	// should never get here
	}
};

status_t
ReadTimezoneDirectory(DIR *dir, const char *dirPath, const char *dirName, BList *tzList)
{
	struct dirent  	*dirent;
	BString			thisDirPath(dirPath);
	BString			filePath;
	
	if (dirName) {
		thisDirPath << dirName << "/";
	}
	
	while ((dirent = readdir(dir)) != 0) {
		int			ret;
		struct stat	statbuf;
		
		filePath = thisDirPath;
		filePath << dirent->d_name;

		ret = stat(filePath.String(), &statbuf);

		if (ret < 0)
			continue;

		if (strcmp(dirent->d_name, ".") == 0  ||
			strcmp(dirent->d_name, "..") == 0)
		{
			continue;
		} else if (S_ISDIR(statbuf.st_mode)) {
			// must recurse into subdirectory
			DIR *subdir = opendir(filePath.String());
			if (subdir != NULL) {
				ReadTimezoneDirectory(subdir, thisDirPath.String(), dirent->d_name, tzList);
				closedir(subdir);
			}
			continue;
		}
		
		// replace underscores with spaces for the user-visible names
		replace_chars(dirent->d_name, '_', ' ');
		BString fullName;
		if (dirName) {
			fullName << dirName << "/";
		}
		fullName << dirent->d_name;
		//fprintf(stderr, "ReadTimezoneDirectory(): Adding timezone '%s' for path '%s'\n", fullName.String(), filePath.String());
		tz_entry *entry = new tz_entry(fullName.String(), filePath.String());
		tzList->AddItem((void*)entry);			
	}

	return B_OK;
}

status_t DateTimeNode::PopulateTimezones()
{
	// most of the PopulateTimezones() code is taken from TimeZone.cpp in the
	// BeOS Time preferences panel.
	BList tzList(20);
	status_t r = B_ERROR;
	char tzdirname[B_PATH_NAME_LENGTH];

	BinderContainer *container = dynamic_cast<BinderContainer*>(&(*fTZContainer));
	if (container == NULL) {
		fprintf(stderr, "DateTimeNode::PopulateTimezones(): Could not access timezone container!\n");
		return B_ERROR;
	}
	container->SetPermissions(permsRead | permsWrite | permsCreate | permsDelete);

	find_directory(B_BEOS_ETC_DIRECTORY, -1, false, tzdirname, MAXPATHLEN);
	strcat(tzdirname, "/timezones/");
	DIR *dir = opendir(tzdirname);

	if (dir == NULL) {
		fprintf(stderr, "DateTimeNode::PopulateTimezones(): opening timezone directory '%s' failed\n", tzdirname);
	} else {
		ReadTimezoneDirectory(dir, tzdirname, NULL, &tzList);
		closedir(dir);
		r = B_OK;
	}

	// sort items using the tz_entry::sort function
	tzList.SortItems(tz_entry::sort);

	int32 count = tzList.CountItems();
	for (int32 i = 0; i < count; i++) {
		tz_entry *entry = (tz_entry*)tzList.ItemAt(i);
		//container->WriteProperty(dirent->d_name, property(fullPath));
		container->AddProperty(entry->name, property(entry->path));
		delete entry;
	}

	container->SetPermissions(permsRead);

	// items in tzList have already been deleted
	tzList.MakeEmpty();
	
	return r;
}


status_t DateTimeNode::SetTime(int hours, int minutes, int seconds)
{
	fprintf(stderr, "DateTimeNode::SetTime(%d, %d, %d)\n", hours, minutes, seconds);

	time_t now;
	struct tm new_time;

	now = time(NULL);
	new_time = *localtime(&now);
	//new_time = *gmtime(&now);
	
	new_time.tm_hour = hours;
	new_time.tm_min = minutes;
	new_time.tm_sec = seconds;
	
	now = mktime(&new_time);
	stime(&now);

	return B_OK;
}

status_t DateTimeNode::SetDate(int month, int day, int year)
{
	fprintf(stderr, "DateTimeNode::SetDate(%d, %d, %d)\n", month, day, year);

	time_t now;
	struct tm new_time;

	now = time(NULL);
	new_time = *localtime(&now);
	//new_time = *gmtime(&now);
	
	new_time.tm_mon = month;
	new_time.tm_mday = day;
	new_time.tm_year = (year - 1900);
	
	now = mktime(&new_time);
	stime(&now);

	return B_OK;
}

const char *kDefaultTimezone = "Los Angeles";

status_t DateTimeNode::GetTimezone(BString &outStr)
{
	fprintf(stderr, "DateTimeNode::GetTimezone() invoked\n");
	char buf[B_PATH_NAME_LENGTH];
	if (_kget_tzfilename_(buf) == B_NO_INIT) {
		// there was an error retrieving the timezone
		fprintf(stderr, "\tAn error occurred while retrieving current timezone... setting default timezone\n");
		status_t err = SetTimezone(kDefaultTimezone);
		if (err == B_OK) {
			outStr = kDefaultTimezone;
		} else {
			fprintf(stderr, "\tFAILED TO SET DEFAULT TIMEZONE!\n");
			return B_ERROR;
		}
	} else {
		char tzdir[B_PATH_NAME_LENGTH + 4];
		find_directory(B_BEOS_ETC_DIRECTORY, -1, false, tzdir, MAXPATHLEN);
		strcat(tzdir, "/timezones/");
		char *tzName = buf + strlen(tzdir);
		
		// replace underscores with spaces for the user-visible names
		replace_chars(tzName, '_', ' ');
		outStr = tzName;
		fprintf(stderr, "\treturning '%s'\n", outStr.String());
	}

	return B_OK;
}

status_t DateTimeNode::SetTimezone(const char *tz)
{
	fprintf(stderr, "DateTimeNode::SetTimezone(%s) invoked\n", tz);
	if (tz == NULL) {
		return B_ERROR;
	}

	char tzName[B_PATH_NAME_LENGTH];
	strncpy(tzName, tz, B_PATH_NAME_LENGTH);
	
	// replace spaces with underscores to convert from user-visible name to a file path
	replace_chars(tzName, ' ', '_');

	char buf[B_PATH_NAME_LENGTH + 4];
	find_directory(B_BEOS_ETC_DIRECTORY, -1, false, buf, MAXPATHLEN);

	char tzPath[B_PATH_NAME_LENGTH];	// full path to timezone file
	sprintf(tzPath, "%s/timezones/%s", buf, tzName);

	struct stat	statbuf;
	int ret = stat(tzPath, &statbuf);
	if (ret < 0) {
		fprintf(stderr, "\tcould not stat file '%s', aborting\n", tzPath);
		return B_ERROR;
	}
	
	// 	sets the timezone environment variable
	//	any call to localtime will now return that tz's time
	//sprintf(buf, "TZ=%s", tzPath);
	//putenv(buf);
	//tzset();

	status_t err = set_timezone(tzPath);
	if (err) fprintf(stderr, "\tset_timezone(%s) returned %ld\n", tzPath, err);

	return err;
}

//-----------------------------------------------------------------------

extern "C" _EXPORT BinderNode *return_binder_node()
{
	return new DateTimeNode();
}
