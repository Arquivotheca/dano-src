
#include "settings.h"
#include "ValidApp.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <errno.h>

#include <Locker.h>
#include <Autolock.h>
#include <String.h>

#include <string>
#include <map>

static BLocker s_lock("be:settings locker");

static map<string, string> g_settings;

static BString g_settings_file("No settings file");

int open_settings(const char* path)
{
	fprintf(stderr, "open_settings: %s\n", path);
	int fd = open(path, O_RDONLY);
	if (fd >= 0)
	{
		g_settings_file = path;
	}
	return fd;
}

const char* get_settings_file()
{
	return g_settings_file.String();
}

status_t read_settings(const char* leaf)
{
	// given the leaf, try to open the settings file first in the
	// current directory and then in the original test directory
	
	BAutolock lock(s_lock);
	
	status_t err = B_OK;
	void * mem;
	BString local_settings = ValidApp::s_current_directory;
	local_settings += "/";
	local_settings += leaf;
	int fd = open_settings(local_settings.String());
	// if we fail with current directory - look in the original location
	if (fd < 0) 
	{
		BString boot_settings = ValidApp::s_test_directory;
		boot_settings += "/";
		boot_settings += leaf;
		fd = open_settings(boot_settings.String());
	}
	if (fd < 0) return errno;
	fprintf(stderr, "result of open = %d\n", fd);
	off_t f = lseek(fd, 0, 2);
	fprintf(stderr, "result of lseek = %lld\n", (uint64)f);
	if (f < 0) err = errno;
	if (f > 500000) err = ENOMEM;
	if (err < 0)
	{
		close(fd);
		return 0;
	}
	mem = malloc(f+1);
	lseek(fd, 0, 0);
	if (f != read(fd, mem, f))
	{
		err = (errno < 0) ? errno : B_IO_ERROR;
	}
	close(fd);
	if (err == B_OK)
	{
		((char *)mem)[f] = 0;
		//	parse the file
		char * base = (char *)mem;
		char * end;
		while (base < (char *)mem+f)
		{
			end = strchr(base, '\n');
			if (!end) end = base+strlen(base);
			*end = 0;
			if (*base == '#' || !*base)
			{
				base = end+1;
				continue;
			}
			char * eq = strchr(base, '=');
			if (!eq) {
				fprintf(stderr, "bad setting (no '='): %s\n", base);
				base = end+1;
				continue;
			}
			int i = -1;
			while ((eq+i >= base) && isspace(eq[i]))
			{
				eq[i] = 0;
				i--;
			}
			if (*base == 0)
			{
				fprintf(stderr, "bad setting (no name): %s\n", eq);
				base = end+1;
				continue;
			}
			*eq = 0;
			eq++;
			while ((eq < end) && isspace(*eq))
			{
				*eq = 0;
				eq++;
			}
			g_settings[base] = eq;
			base = end+1;
		}
	}
	
	free(mem);
	return err;
}

status_t write_settings(const char * path)
{
	BAutolock lock(s_lock);
	FILE * f = fopen(path, "w");
	if (!f) return errno;
	for (map<string, string>::iterator ptr(g_settings.begin());
		ptr != g_settings.end();
		ptr++)
	{
		fprintf(f, "%s=%s\n", (*ptr).first.c_str(), (*ptr).second.c_str());
	}
	fclose(f);
	return B_OK;
}


status_t set_setting_value(const char * name, int64 number)
{
	BAutolock lock(s_lock);
	char str[40];
	sprintf(str, "%Ld", number);
	g_settings[name] = str;
	return B_OK;
}

status_t set_setting(const char * name, const char * value)
{
	BAutolock lock(s_lock);
	g_settings[name] = value;
	return B_OK;
}

status_t remove_setting(const char * name)
{
	BAutolock lock(s_lock);
	map<string, string>::iterator ptr(g_settings.find(name));
	if (ptr == g_settings.end()) return B_BAD_INDEX;
	g_settings.erase(ptr);
	return B_OK;
}


int64 get_setting_value(const char * name, int64 def)
{
	BAutolock lock(s_lock);
	int64 llv = def;
	map<string, string>::iterator ptr(g_settings.find(name));
	if (ptr != g_settings.end())
	{
		char *out;
		llv = strtoll((*ptr).second.c_str(), &out, 10);
	}
	return llv;
}

const char * get_setting(const char * name, char * buffer, int size)
{
	BAutolock lock(s_lock);
	map<string, string>::iterator ptr(g_settings.find(name));
	if (ptr != g_settings.end())
	{
		if (!buffer)
		{
			buffer = (char *)(*ptr).second.c_str();	/* thread and subsequent call unsafe */
		}
		else
		{
			strncpy(buffer, (*ptr).second.c_str(), size);
			buffer[size-1] = 0;
		}
	}
	else 
	{
		// callers depend on checking return value for non-existence of name
		buffer = NULL;
	}
	return buffer;
}

