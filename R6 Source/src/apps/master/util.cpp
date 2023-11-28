
#include <Directory.h>
#include <Path.h>
#include <File.h>
#include <image.h>
#include <parsedate.h>
#include "master.h"
#include "util.h"
#include "catfile.h"
#include "versioncache.h"
extern "C" {
#include "md5.h"
}

struct die_hook {
	void (*hook)(void*);
	void *userData;
	int32 id;
	die_hook *next;
};

int32 nextDieHookID = 1;
die_hook *deathHooks = NULL;
FILE *logfile=NULL;

int32 add_die_hook(void (*hook)(void*), void *userData)
{
	die_hook **p,*h = new die_hook;
	h->hook = hook;
	h->userData = userData;
	h->id = nextDieHookID++;
	h->next = NULL;
	p = &deathHooks;
	while (*p) p = &(*p)->next;
	*p = h;
	return h->id;
}

void remove_die_hook(int32 id)
{
	die_hook *h,**p = &deathHooks;
	while (*p && ((*p)->id != id)) p = &(*p)->next;
	if (*p) {
		h = *p;
		*p = (*p)->next;
		delete h;
	}
}

void close_logfile(void *bleh = NULL)
{
	fclose(logfile);
}

char *get_time(time_t t)
{
	static char buf[256];
	struct tm *tim = gmtime(&t);
	strftime(buf,255,"%m/%d/%Y %T",tim);
	return buf;
}

void log(const char *str)
{
	fprintf(stdout,str);
	fprintf(logfile,str);
	fflush(stdout);
	fflush(logfile);
}

void log(const char *str, va_list pvar)
{
	vfprintf(stdout,str,pvar);
	vfprintf(logfile,str,pvar);
	fflush(stdout);
	fflush(logfile);
}

void output(const char *type, const char *msg, ...)
{
	char buf[1024];
	va_list pvar;
	va_start(pvar,msg);
	strcpy(buf,type);
	strcat(buf,": ");
	strcat(buf,msg);
	strcat(buf,"\n");
	log(buf,pvar);
	va_end(pvar);
}

void warning(const char *msg, ...)
{
	char buf[1024];
	va_list pvar;
	va_start(pvar,msg);
	strcpy(buf,"warning: ");
	strcat(buf,msg);
	strcat(buf,"\n");
	log(buf,pvar);
	va_end(pvar);
}

void status(const char *msg, ...)
{
	char buf[1024];
	va_list pvar;
	va_start(pvar,msg);
	strcpy(buf,"status: ");
	strcat(buf,msg);
	strcat(buf,"\n");
	log(buf,pvar);
	va_end(pvar);
}

void error_die(const char *msg, ...)
{
	char buf[1024];
	va_list pvar;
	va_start(pvar,msg);
	strcpy(buf,"error: ");
	strcat(buf,msg);
	strcat(buf,"\n");
	log(buf,pvar);
	va_end(pvar);

	die_hook *p = deathHooks;
	while (p) {
		p->hook(p->userData);
		p = p->next;
	}
	exit(-1);
}

bool isdir(const char *pathname)
{
	BDirectory dir(pathname);
	return (dir.InitCheck() == 0);
}

bool isfile(const char *pathname)
{
	BFile file(pathname,O_RDONLY);
	return (file.InitCheck() == 0);
}

void create_version_record(version_rec *r, const char *name, const char *oldName)
{
	strcpy(r->versionID,name);
	strcpy(r->derivedFrom,oldName);
	r->checkoutTime = time(NULL);
	r->finishTime = -1;
	r->installByTime = -1;
	r->checkinTime = -1;
	r->reboot = r->redial = r->restart = r->flush = r->freeze = false;
}

void read_version_record(const char *versionName, version_rec *r, bool dump)
{
	char line[1024],name[128],val[128];
	time_t now = time(NULL);

	BPath fn(CACHE_PATHNAME,versionName);
	fn.Append("deltadata/version.txt");

	r->versionID[0] = 0;
	r->derivedFrom[0] = 0;
	r->checkoutTime = -1;
	r->checkinTime = -1;
	r->finishTime = -1;
	r->installByTime = -1;
	r->reboot = r->redial = r->restart = r->flush = r->freeze = false;

	FILE *f = fopen(fn.Path(),"r");
	if (!f) error_die("failed reading version record for '%s'",versionName);

	while (fgets(line,1024,f)) {
		int count = sscanf(line,"%32s %[^\n]\n",name,val);
		if (count < 2) val[0] = 0;
		if (count < 1) name[0] = 0;
		if (dump) output("info","%s %s",name,val);
		if (!strcmp(name,"version")) {
			strcpy(r->versionID,val);
			if (strcmp(r->versionID,versionName))
				error_die("corrupt version record for '%s'",versionName);
		} else if (!strcmp(name,"derived-from"))
			strcpy(r->derivedFrom,val);
		else if (!strcmp(name,"checkout-time"))
			r->checkoutTime = parsedate(val,now);
		else if (!strcmp(name,"checkin-time"))
			r->checkinTime = parsedate(val,now);
		else if (!strcmp(name,"finish-time"))
			r->finishTime = parsedate(val,now);
		else if (!strcmp(name,"installby-time"))
			r->installByTime = parsedate(val,now);
		else if (!strcmp(name,"reboot-required"))
			r->reboot = strcmp(val,"no");
		else if (!strcmp(name,"redial-required"))
			r->redial = strcmp(val,"no");
		else if (!strcmp(name,"restart-required"))
			r->restart = strcmp(val,"no");
		else if (!strcmp(name,"flush-required"))
			r->flush = strcmp(val,"no");
		else if (!strcmp(name,"freeze-required"))
			r->freeze = strcmp(val,"no");
	}
	fclose(f);

	if (!r->versionID[0] || (r->checkoutTime == -1))
		error_die("Invalid version record for '%s'",versionName);
}

void write_version_record(version_rec *r)
{
	BPath fn(CACHE_PATHNAME,r->versionID);
	fn.Append("deltadata");
	mkdir(fn.Path(),0777);
	fn.Append("version.txt");

	FILE *f = fopen(fn.Path(),"w");
	if (!f) {
		error_die("failed writing version record for version '%s'",r->versionID);
		printf("Filename: %s\n", fn.Path());
	}
	fprintf(f,"version %s\n",r->versionID);
	if (r->derivedFrom[0]) fprintf(f,"derived-from %s\n",r->derivedFrom);
	fprintf(f,"checkout-time %s\n",get_time(r->checkoutTime));
	if (r->finishTime != -1) fprintf(f,"finish-time %s\n",get_time(r->finishTime));
	if (r->installByTime != -1) fprintf(f,"installby-time %s\n",get_time(r->installByTime));
	if (r->checkinTime != -1) fprintf(f,"checkin-time %s\n",get_time(r->checkinTime));
	fprintf(f,"reboot-required %s\n",r->reboot?"yes":"no");
	fprintf(f,"redial-required %s\n",r->redial?"yes":"no");
	fprintf(f,"restart-required %s\n",r->restart?"yes":"no");
	fprintf(f,"freeze-required %s\n",r->freeze?"yes":"no");
	fprintf(f,"flush-required %s\n",r->flush?"yes":"no");
	fclose(f);
}

thread_id do_exec(int32 argc, const char **argv, int *input, int *output, int *error=NULL)
{	
	char execStr[4096];
	int written = 0;	
//	char *execStr = (char *) malloc(65536 * sizeof(char));
	char *p = execStr;
	
	p[0] = 0;

	written += sprintf(execStr + written, "exec: ");
	for (int32 i = 0; i < argc; i++) {
		if (written > 4096) {
			printf("\n$$$$ execStr is growing too fast\n");
		} else {
			written += sprintf(execStr + written, "'%s' ", argv[i]);
		}
	}
	written += sprintf(execStr + written, "\n");

/*
	strcat(p,"exec: "); p+=6;
	for (int32 i=0;i<argc;i++) {
		strcat(p,"'"); p++;
		printf("arg(%ld): %s\n",i,argv[i]);
		strcat(p,argv[i]); p+=strlen(argv[i]);
		strcat(p,"' "); p+=2;
		if(strlen(execStr) > 65000) {
			printf("execStr is too long.");
			break;
		}
	}
	strcat(p,"\n");
*/

	log(execStr);

	char inName[256],outName[256],errName[256];
	sprintf(inName,"/pipe/input%Lx",system_time());
	sprintf(outName,"/pipe/output%Lx",system_time());
	sprintf(errName,"/pipe/error%Lx",system_time());

	int oldStdin = dup(STDIN_FILENO);
	int oldStdout = dup(STDOUT_FILENO);
	int oldStdErr = dup(STDERR_FILENO);
	if (*input < 0) *input = open(inName,O_CREAT|O_RDONLY);
	if (*output < 0) *output = open(outName,O_CREAT|O_WRONLY);
	dup2(*input,STDIN_FILENO);
	dup2(*output,STDOUT_FILENO);
	close(*input);
	close(*output);
	if (error) {
		if (*error < 0) *error = open(errName,O_CREAT|O_WRONLY);
		dup2(*error,STDERR_FILENO);
		close(*error);
	} else
		close(STDERR_FILENO);

	char *env[1024];
	int32 i,c=0;
	for (i=0;environ[i];i++) {
		if (strncmp(environ[i],"MALLOC_DEBUG",12))
			env[c++] = environ[i];
	}
	env[c++] = environ[i];

	thread_id spawned = load_image(argc,argv,(const char**)env);

	dup2(oldStdin,STDIN_FILENO);
	close(oldStdin);
	dup2(oldStdout,STDOUT_FILENO);
	close(oldStdout);
	dup2(oldStdErr,STDERR_FILENO);
	close(oldStdErr);

	*input = open(inName,O_CREAT|O_WRONLY);
	*output = open(outName,O_CREAT|O_RDONLY);
	if (error) *error = open(errName,O_CREAT|O_RDONLY);
	resume_thread(spawned);

//	free(execStr);

	return spawned;
}

status_t simple_exec(int32 argc, const char **argv)
{
	status_t err;
	char buf[65536];
	int input=-1,output=-1,error=-1;
	FILE *foutput;

	thread_id thid = do_exec(argc,argv,&input,&output);
	if (thid < 0) return thid;
/*
	foutput = fdopen(error,"r");
	while (fgets(buf,65536,foutput)) {
		for (int32 l = strlen(buf)-1;buf[l] == '\n';l--) buf[l] = 0;
		::output("execerr","%s",buf);
	}
	fclose(foutput);
*/
	foutput = fdopen(output,"r");
	while (fgets(buf,65536,foutput)) {
		for (int32 l = strlen(buf)-1;buf[l] == '\n';l--) buf[l] = 0;
		::output("execout","%s",buf);
	}
	fclose(foutput);

	close(input);
	wait_for_thread(thid,&err);
	return err;
/*
	printf("exec: ");
	for (int32 i=0;i<argc;i++) {
		printf("'%s' ",argv[i]);
	}
	printf("\n");

	int oldStdin = dup(STDIN_FILENO);
	int oldStdout = dup(STDOUT_FILENO);
	close(STDIN_FILENO);
//	close(STDOUT_FILENO);

	char *env[1024];
	int32 i,c=0;
	for (i=0;environ[i];i++) {
		if (strncmp(environ[i],"MALLOC_DEBUG",12))
			env[c++] = environ[i];
	}
	env[c++] = environ[i];

	status_t err;
	thread_id spawned = load_image(argc,argv,(const char**)env);
//	setpgid(spawned,myPG);
	resume_thread(spawned);
	wait_for_thread(spawned,&err);

	dup2(oldStdin,STDIN_FILENO);
	dup2(oldStdout,STDOUT_FILENO);
	close(oldStdin);
	close(oldStdout);

	return err;
*/
}

status_t 
filelist::lbxify(filelist &in, filelist &out)
{
	char *ptr,buf[1024];
	int32 p=0;

	while (p < in.m_fileCount) {
		strcpy(buf,in.m_files[p]->filename);
		ptr = strrchr(buf,'/');
		if (!ptr) ptr = buf;
		else ptr++;
		strcpy(ptr,"images.lbx");
		out.add(buf,NULL,0,0);
		p++;
	}

	out.remove_duplicates();
	return 0;
}

status_t 
filelist::intersection(filelist &fl1, filelist &fl2, filelist &final)
{
	bool setSorted = !final.count();

	fl1.assert_sorted();
	fl2.assert_sorted();

	int32 p1=0,p2=0,cmp;
	
	while ((p1 < fl1.m_fileCount) && (p2 < fl2.m_fileCount)) {
		cmp = strcmp(fl1.m_files[p1]->filename,fl2.m_files[p2]->filename);
		if (cmp == 0) {
			if (fl2.m_files[p2]->md5)	final.add(fl2.m_files[p2]);
			else						final.add(fl1.m_files[p1]);
			p1++; p2++;
		} else if (cmp > 0) {
			p2++;
		} else if (cmp < 0) {
			p1++;
		}
	}

	if (setSorted) final.m_flags |= flSorted;
	return 0;
}

status_t 
filelist::subtract(filelist &fl1, filelist &fl2, filelist &final)
{
	bool setSorted = !final.count();

	fl1.assert_sorted();
	fl2.assert_sorted();

	int32 p1=0,p2=0,cmp;
	
	while ((p1 < fl1.m_fileCount) && (p2 < fl2.m_fileCount)) {
		cmp = strcmp(fl1.m_files[p1]->filename,fl2.m_files[p2]->filename);
		if (cmp == 0) {
			// a duplicate!
			p1++; p2++;
		} else if (cmp > 0) {
			p2++;
		} else if (cmp < 0) {
			final.add(fl1.m_files[p1]);
			p1++;
		}
	}

	while (p1 < fl1.m_fileCount) {
		final.add(fl1.m_files[p1]);
		p1++;
	}

	if (setSorted) final.m_flags |= flSorted;
	return 0;
}

status_t 
filelist::merge(filelist &fl1, filelist &fl2, bool onlyElf, filelist &final)
{
	bool setSorted = !final.count();

	fl1.assert_sorted();
	fl2.assert_sorted();

	int32 p,p1=0,p2=0,cmp;
	
	while ((p1 < fl1.m_fileCount) && (p2 < fl2.m_fileCount)) {
		cmp = strcmp(fl1.m_files[p1]->filename,fl2.m_files[p2]->filename);
		if (cmp == 0) {
			// a duplicate!
			if (!onlyElf || fl1.m_files[p1]->isElf())
				final.add(fl1.m_files[p1]);
			p1++; p2++;
		} else if (cmp > 0) {
			if (!onlyElf || fl2.m_files[p2]->isElf())
				final.add(fl2.m_files[p2]);
			p2++;
		} else if (cmp < 0) {
			if (!onlyElf || fl1.m_files[p1]->isElf())
				final.add(fl1.m_files[p1]);
			p1++;
		}
	}

	p = p2;
	filelist *list = &fl2;

	if (p1 < fl1.m_fileCount) {
		p = p1;
		list = &fl1;
	}
	
	while (p < list->m_fileCount) {
		if (!onlyElf || list->m_files[p]->isElf())
			final.add(list->m_files[p]);
		p++;
	}

	if (setSorted) final.m_flags |= flSorted;
	return 0;
}

status_t filelist::make_diffs(filelist &fl1, filelist &fl2, filelist &added, filelist &removed, filelist &changed)
{
	bool setAddedSorted = !added.count();
	bool setRemovedSorted = !removed.count();
	bool setChangedSorted = !changed.count();

	fl1.assert_sorted();
	fl2.assert_sorted();
	int32 p,p1=0,p2=0,cmp;
	
	while ((p1 < fl1.m_fileCount) && (p2 < fl2.m_fileCount)) {
		cmp = strcmp(fl1.m_files[p1]->filename,fl2.m_files[p2]->filename);
		if (cmp == 0) {
			if (!fl1.m_files[p1]->md5 || !fl2.m_files[p2]->md5 ||
				(!strcmp(fl1.m_files[p1]->md5,fl2.m_files[p2]->md5) &&
				(fl1.m_files[p1]->size == fl2.m_files[p2]->size) &&
				(fl1.m_files[p1]->flags == fl2.m_files[p2]->flags))) {
				// file has not changed
				p1++; p2++;
			} else {
				// file has been changed
				changed.add(fl2.m_files[p2]);
				p1++; p2++;
			}
		} else if (cmp > 0) {
			// file has been added
			added.add(fl2.m_files[p2]);
			p2++;
		} else if (cmp < 0) {
			// file has been removed
			removed.add(fl1.m_files[p1]);
			p1++;
		}
	}
	
	p = p2;
	filelist *list = &fl2;
	filelist *addTo = &added;
	if (p1 < fl1.m_fileCount) {
		p = p1;
		list = &fl1;
		addTo = &removed;
	}
	
	while (p < list->m_fileCount) {
		addTo->add(list->m_files[p]);
		p++;
	}
	
	if (setAddedSorted) added.m_flags |= flSorted;
	if (setRemovedSorted) removed.m_flags |= flSorted;
	if (setChangedSorted) changed.m_flags |= flSorted;
	return 0;
}

int read_to_filelist(int fd, filelist &files, bool addExtraData)
{
	char path[1024],*s;
	const char *sum;
	struct stat sb;
	bool isElf;

	FILE *foutput = fdopen(fd,"r");
	while (fgets(path,MAXPATHLEN,foutput)) {
		for (int32 l = strlen(path)-1;path[l] == '\n';l--) path[l] = 0;
		s = path;
		if ((s[0] == '.') && (s[1] == '/')) s+=2;
		if (addExtraData) {
			lstat(s,&sb);
			sum = do_md5(s,isElf);
			files.add(s,sum,sb.st_size,(isElf?filelist::IS_BINARY:0) | (S_ISDIR(sb.st_mode)?filelist::IS_DIR:0));
		} else {
			files.add(s,NULL,0,0);
		}
	}
	fclose(foutput);

	return 0;
}

int file_to_filelist(const char *pathname, filelist &files, bool addExtraData)
{
	int fd = open(pathname,O_RDONLY);
	if (fd<0) return fd;
	return read_to_filelist(fd,files,addExtraData);
}

int exec_to_filelist(int32 argc, const char **argv, filelist &files, bool addExtraData)
{
	int input=-1,output=-1;
	status_t err;

	thread_id thid = do_exec(argc,argv,&input,&output);
	if (thid < 0) return thid;

	read_to_filelist(output,files,addExtraData);
	close(input);

	wait_for_thread(thid,&err);
	return err;
}

int add_all_files(const char *dir, filelist &files, bool extraData)
{
	const char *argv[2];

	chdir(dir);

	argv[0] = "/bin/find";
	argv[1] = ".";
	return exec_to_filelist(2,argv,files,extraData);
}

int rm(const char *fn)
{
	status_t err;
	int input = -1;
	int output = -1;
	const char *argv[5];
	argv[0] = "/bin/rm";
	argv[1] = "-rf";
	argv[2] = fn;
	thread_id thid = do_exec(3,argv,&input,&output);
	close(input);
	close(output);

	wait_for_thread(thid,&err);
	return err;
}

int copyfile(const char *from, const char *to, const char *flags)
{
	status_t err;
	int input = -1;
	int output = -1;
	const char *argv[10];
	int32 argc = 1;
	argv[0] = "/bin/copyattr";
	argv[argc++] = "-d";
	if (flags) argv[argc++] = flags;
	argv[argc++] = from;
	argv[argc++] = to;
	thread_id thid = do_exec(argc,argv,&input,&output);
	close(input);
	close(output);

	wait_for_thread(thid,&err);
	return err;
}

status_t get_version(const char *name, BPath *pathToRoot, uint32 packages)
{
	uint32 flags = cache_get_version(name,packages);
	if ((flags & packages) != packages)
		error_die("failed to checkout version '%s'",name);

	pathToRoot->SetTo("/boot/station/cache",name);
	return B_OK;
}

sem_id catThreadSem,catThreadDone;
FILE *md5ThreadStream;
thread_id md5Thid;
uint8 md5Checksum[16];
int32 logfileDieHook;

int32 catThread(void *)
{
	while (acquire_sem(catThreadSem) == B_OK) {
		md5_stream(md5ThreadStream,md5Checksum);
		fclose(md5ThreadStream);
		release_sem(catThreadDone);
	}
	return 0;
}

void start_md5_thread()
{
	logfile = fopen("/boot/station/log","wa+");
	logfileDieHook = add_die_hook(close_logfile,NULL);
	catThreadSem = create_sem(0,"catThreadSem");
	catThreadDone = create_sem(0,"catThreadSem");
	resume_thread(md5Thid = spawn_thread(catThread,"catThread",B_NORMAL_PRIORITY,NULL));
}

status_t stop_md5_thread()
{
	status_t dummy;
	remove_die_hook(logfileDieHook);
	close_logfile();
	delete_sem(catThreadSem);
	delete_sem(catThreadDone);
	wait_for_thread(md5Thid,&dummy);
	return dummy;
}

static const char bin2hex[] = {
	'0', '1', '2', '3',
	'4', '5', '6', '7',
	'8', '9', 'a', 'b',
	'c', 'd', 'e', 'f'
};

char * do_md5(const char *path, bool &isElf)
{
	static char sum[33];

	int inout[2];
	pipe(inout);
	md5ThreadStream = fdopen(inout[0],"r");
	FILE *out = fdopen(inout[1],"w");
	release_sem(catThreadSem);

	catfile(out,path,isElf);
	fclose(out);

	acquire_sem(catThreadDone);
	for (int32 i=0;i<16;i++) {
		int32 j = i*2;
		sum[j] = bin2hex[md5Checksum[i] >> 4];
		sum[j+1] = bin2hex[md5Checksum[i] & 0x0F];
	}
	sum[32] = 0;
	return sum;
}
