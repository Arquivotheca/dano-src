#include <Application.h>
#include <InterfaceDefs.h>
#include <Resources.h>
#include <Directory.h>
#include <File.h>
#include <Entry.h>
#include <AppDefs.h>
#include <Path.h>

#include <alloca.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <byteorder.h>

#include "OBlock.h"

#define USAGE_ERROR 0xe00e0000

void
usage(
	const char *message,
	OBlock & o)
{
	o.numErrors++;
	o.lastError = USAGE_ERROR;
	fprintf(stderr, "usage: %s\n", message);
}


void
error(
	long err,
	OBlock & o,
	const char *message,
	...)
{
	va_list l;
	va_start(l, message);
	char msg[400];
	vsprintf(msg, message, l);
	va_end(l);
	fprintf(stderr, "Error 0x%08x: %s", err, msg);
	o.lastError = err;
	o.numErrors++;
}


int
intarg(
	const char *option,
	char *ptr,
	OBlock & o)
{
	char *sp = ptr;
	while (*ptr)
		if ((*ptr < '0') || (*ptr > '9'))
		{
			error(B_ERROR, o, "Option %s wanted integer, got %s\n", option, sp);
			return false;
		}
		else
			ptr++;
	return true;
}


int
beide_option(
	int argc,
	char *argv[],
	OBlock & o)
{
	port_id from, to;
	if (2 != sscanf(argv[0], "%d,%d", &from, &to))
	{
		usage("-beide port,port", o);
		return 0;
	}
	o.fromIDE = from;
	o.toIDE = to;
	return 1;
}


int
o_option(
	int argc,
	char *argv[],
	OBlock & o)
{
	int len = strlen(argv[0]);
	delete[] o.outFileName;
	o.outFileName = new char[len+1];
	strcpy(o.outFileName, argv[0]);
	return 1;
}


int
types_option(
	int argc,
	char *argv[],
	OBlock & o)
{
	OType *type = o.types;
	OType *del = NULL;

	while (type != NULL)
	{
		del = type;
		type = type->next;
		delete del;
	}

	o.types = NULL;
	ulong rtype = 0;
	char *ptr = *argv;

	while (true)
	{
		if ((*ptr == ',') || !*ptr)
		{
			if (rtype)
				o.types = new OType(rtype, o.types);
			rtype = 0;
			if (*ptr)
				ptr++;
		}
		if (!*ptr)
			break;
		if (rtype & 0xff000000)
			usage("-types requires 4-character types, separated by commas!", o);
		rtype = (rtype << 8) + *(unsigned char *)ptr;
		ptr++;
	}
	return 1;
}


struct Option {
	const char		*str;
	int				numArgs;
	int				(*func)(int, char **, OBlock &);
	const char		*help;
};
extern Option options[];

int
help_option(
	int argc,
	char *argv[],
	OBlock & o)
{
	fprintf(stderr, "iconv options:\n");
	Option *op = options;
	while (op->str)
	{
		if (op->help)
			fprintf(stderr, "%20s%-2d arguments\n",
				op->str, op->numArgs);
		op++;
	}
	return 0;
}


Option options[] = {
	{	"-beide",		1,	beide_option,		NULL						},
	{	"-types",		1,	types_option,		"Types TYPE,TYPE,..."		},
	{	"-t",			1,	types_option,		" (same)"					},
	{	"-o",			1,	o_option,			"Output to #1"				},
	{	"-help",		0,	help_option,		"This text"					},
	{	"-?",			0,	help_option,		" (same)"					},
	{	NULL,			0,	NULL,				NULL						},
};


int
has_option(
	Option * opt,
	int argc,
	char *argv[],
	int ix,
	OBlock & o)
{
	if (opt->numArgs >= argc-ix)
	{
		char str[100];
		sprintf(str, "option %s requires at least %d arguments\n",
			opt->str, opt->numArgs);
		usage(str, o);
		return 0;
	}
	return (*opt->func)(argc-ix-1, &argv[ix+1], o);
}


//	This mastodonth function does all the work.
//	Create a folder with name taken from the file to dump from.
//	Dump all the resources into separate files in that folder.
//	Write out a script file that you can use with mwbres to re-create 
//	that resource file.
void
has_file(
	const char *path,
	OBlock & o)
{
	struct stat stbuf;
	long err = stat(path, &stbuf);

	if (err)
	{
		error(err, o, "Can't get at file %s", path);
		return;
	}
	if (!o.outFileName)
	{
		o.outFileName = new char[strlen(path)+5];
		strcpy(o.outFileName, path);
		strcat(o.outFileName, ".res");
	}
	
	BFile aFile(path, O_RDONLY);
	err = aFile.InitCheck();
	if (err)
	{
		error(err, o, "Can't open file %s", path);
		return;
	}
	BResources resFile(&aFile);

	char oldpath[1025];
	getcwd(oldpath, 1024);
	mkdir(o.outFileName, 0x1ff);
	if (stat(o.outFileName, &stbuf) || !S_ISDIR(stbuf.st_mode))
	{
		error(B_ERROR, o, "Can't create directory %s", o.outFileName);
	}
	chdir(o.outFileName);

	FILE *r_file = NULL;
	{
		const char *p = strrchr(path, '/');
		if (p)
			p++;
		else
			p = path;
		char *r_name = (char *)alloca(strlen(p)+3);
		if (!r_name) {
			fprintf(stderr, "alloca(%d) failed!\n", strlen(p)+3);
			exit(1);
		}
		strcpy(r_name, p);
		if (strlen(r_name) > PATH_MAX-3)
			r_name[PATH_MAX-3] = 0;
		strcat(r_name, ".r");
		r_file = fopen(r_name, "w");
		if (!r_file) {
			error(err, o, "Can't create resource description file '%s'\n", r_name);
			return;
		}
	}

	ulong type;
	size_t size;
	long id;
	const char *name;
	for (int ix=0; resFile.GetResourceInfo(ix, &type, &id, &name, &size); ix++)
	{
		if (o.types)
		{
			OType *ty = o.types;
			while (ty)
			{
				if (ty->type == type)
					goto its_ok;
				ty = ty->next;
			}
			continue;
		}
its_ok:
		void *data = resFile.FindResource(type, id, &size);
		char outName[B_FILE_NAME_LENGTH];
		sprintf(outName, "%c%c%c%c.%d", (char)(type>>24), (char)(type>>16),
			(char)(type>>8), (char)type, id);
		if (name)
		{
			strcat(outName, ".");
			strcat(outName, name);
		}
		char *delslash = outName;
		while (*delslash)
		{
			if (*delslash == '/')
				*delslash = '_';
			delslash++;
		}
		FILE *outFile = fopen(outName, "w");
		if (!outFile)
		{
			error(errno, o, "Can't create file %s", outName);
			continue;
		}
		fwrite(data, size, 1, outFile);
		fclose(outFile);
		{
			char typeStr[30];
			type = B_HOST_TO_BENDIAN_INT32(type);		// swap here
			memcpy(&typeStr[1], &type, 4);
			typeStr[0] = '\'';
			typeStr[5] = '\'';
			typeStr[6] = 0;
			int inval = 0;
			for (int tix=1; tix<5; tix++) {
				if (typeStr[tix]<32 || typeStr[tix]>126) {
					inval = 1;
					break;
				}
			}
			if (inval) {
				sprintf(typeStr, "0x%x", type);
			}
			fprintf(r_file, "resource(%s, %ld, \"%s\") {\n\tread(\"%s\")\n}\n\n",
				typeStr, id, name, outName);
		}
	}
	fclose(r_file);
	chdir(oldpath);
}


struct m_data {
	int argc;
	char **argv;
	OBlock * ob;
};

long
the_func(
	void *data)
{
	m_data *md = (m_data *)data;
	OBlock & ob = *md->ob;;
	int argc = md->argc;
	char **argv = md->argv;

	for (int ix=1; ix<argc; ix++)
	{
		bool opt = false;
		for (Option *o = options; o->str; o++)
		{
			if (!strcmp(o->str, argv[ix]))
			{
				ix += has_option(o, argc, argv, ix, ob);
				opt = true;
			}
		}
		if (!opt)
		{
			has_file(argv[ix], ob);
			delete[] ob.outFileName;
			ob.outFileName = NULL;
		}
	}
	be_app->PostMessage(B_QUIT_REQUESTED);

	return ob.numErrors;
}


int
main(
	int argc,
	char *argv[])
{
	OBlock ob;
	BApplication app("application/x-mw-dres");

	m_data md;
	md.argc = argc;
	md.argv = argv;
	md.ob = &ob;
	
	the_func(&md);

	app.Run();

	return ob.numErrors;
}
