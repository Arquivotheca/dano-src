// ---------------------------------------------------------------------------
/*
	profile.cpp
	
	Copyright (c) 1998-2001 Be Inc. All Rights Reserved.
	
	Author:	derived from profile.c (probably Dominic)
			John R. Dance
			6 December 2000

	Use the profiling interface through the debugger nub to start and profile
	a team.
	
	When images are loaded, iterate through and get all symbols.  These symbols
	are passed to the debugger nub and act as buckets for the pc counting.
	
	When the thread stops, we are notified, and we can then get the counts for
	each bucket.  Use the symbol table to map the pc back to the function name.
	
	The main problem (that existed in profile.c) is that profiling needs to be
	restarted when an add-on is loaded so that the new functions can be added
	to the symbol list passed to the debugger nub.  However, just a stop/start
	alone throws away all the date from that time frame.  If an app loads
	a lot of add-ons, this can totally destroy the accuracy of the profiling.
	
	The solution is to keep track of current state of the numbers throughout the
	thread's entire life.  Rather than copying the entire symbol table for each
	thread, I only keep track of the top "count * 2" functions, and then reduce
	that to count for the final display.
	
	These top counts are kept in the ThreadInfo, which is then used for
	the final tally as the thread exits.
	
*/
// ---------------------------------------------------------------------------

#include <SupportDefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <algorithm>
#include <numeric>

#include <OS.h>
#include <debugger.h>
#include <Unmangle.h>
#include "profile.h"

#ifdef __INTEL__

#if _SUPPORTS_COMPRESSED_ELF
#include "uncrush.h"

/* we need a dprintf() function here because libuncrush.a references it */
#include <KernelExport.h>
extern "C" void dprintf(const char* format, ...)
{
}
#endif

#include <fcntl.h>
#include <unistd.h>
#include <elf.h>

#endif

extern char **environ;

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

port_id gPort;			/* this is our port for reading messages on */
port_id gNubPort;		/* and this one is for sending messages to the nub */
port_id gReplyPort;		/* and this one is for getting replies on */


perfmon_event_selector	perfmon_event = {
	0,					/* event */
	0,					/* event_qualifier */
	PERFMON_USER_MODE,	/* privilege */
	-100*1000,			/* init_val */ 
	0					/* HW-specific bits */
};	

int	perfmon_counter = 0;

// ---------------------------------------------------------------------------

	SymbolTable gSymbolTable;
	ThreadList gActiveThreads;

	int32 gPrintCount_arg = 10;
	int32 gMinimumHits_arg = 1;

// ---------------------------------------------------------------------------
// class Bucket - helps
// ---------------------------------------------------------------------------

bool
HitCountGreater(const Bucket& left, const Bucket& right)
{
	return left.fHitCount > right.fHitCount;
}

// ---------------------------------------------------------------------------

int32
AddHitCount(int32 partialSum, const Bucket& aBucket)
{
	return partialSum += aBucket.fHitCount;
}

// ---------------------------------------------------------------------------
// class ImageList - member functions
// ---------------------------------------------------------------------------

bool
ImageList::IsLoaded(image_id id) const
{
	ImageVector::const_iterator item = find(fImages.begin(), fImages.end(), id);
	return (item != fImages.end()) ? true : false;
}

// ---------------------------------------------------------------------------

bool
ImageList::AddImage(image_id id)
{
	fImages.push_back(id);
	return true;
}

// ---------------------------------------------------------------------------

bool
ImageList::RemoveImage(image_id id)
{
	ImageVector::iterator item = find(fImages.begin(), fImages.end(), id);
	if (item != fImages.end()) {
		fImages.erase(item);
		return true;
	}
	else {
		return false;
	}
}

// ---------------------------------------------------------------------------
// class ThreadInfo - member functions
// ---------------------------------------------------------------------------

ThreadInfo::ThreadInfo(thread_id id)
{
	thread_info info;
	if (get_thread_info(id, &info) == B_OK) {
		strncpy(fName, info.name, B_OS_NAME_LENGTH);
		fName[B_OS_NAME_LENGTH] = '\0';
	}
	else {
		sprintf(fName, "thid %ld", id);
	}
}

// ---------------------------------------------------------------------------

void
ThreadInfo::AddToTopCounts(const Bucket& aBucket)
{
	// Insert Bucket into our top count list.  If the same bucket is already
	// in the list, then add the hit count into the existing hit count.
	
	typedef pair<set<Bucket>::iterator, bool> InsertResult;

	// (If the bucket hit count is 0, then no use adding it)
	if (aBucket.fHitCount > 0) {
		InsertResult result = fTopCount.insert(aBucket);
		if (result.second == false) {
			(*(result.first)).AddToCount(aBucket);
		}
	}
}

// ---------------------------------------------------------------------------
// class ThreadList - member functions
// ---------------------------------------------------------------------------

ThreadList::ThreadList()
{
}

// ---------------------------------------------------------------------------

ThreadList::~ThreadList()
{
	// delete all the ThreadInfo
	for (ThreadMap::iterator item = fThreads.begin(); item != fThreads.end(); item++) {
		delete (*item).second;
	}
	fThreads.clear();
}

// ---------------------------------------------------------------------------

bool
ThreadList::IsRunning(thread_id id) const
{
	ThreadMap::const_iterator aThread = fThreads.find(id);
	return (aThread != fThreads.end()) ? true : false;
}

// ---------------------------------------------------------------------------

void
ThreadList::ForEach(ThreadFunction func)
{
	for_each(fThreads.begin(), fThreads.end(), func);
}

// ---------------------------------------------------------------------------

ThreadInfo*
ThreadList::GetThreadInfo(thread_id id)
{
	ThreadMap::const_iterator aThread = fThreads.find(id);
	return (*aThread).second;
}

// ---------------------------------------------------------------------------

const char* kUnknownThread = "Unknown Thread";

const char*
ThreadList::GetThreadName(thread_id id) const
{
	ThreadMap::const_iterator aThread = fThreads.find(id);
	if (aThread != fThreads.end()) {
		return (*aThread).second->ThreadName();
	}
	else {
		return kUnknownThread;
	}
}

// ---------------------------------------------------------------------------

bool
ThreadList::AddThread(thread_id id)
{
	ThreadInfo* aThread = new ThreadInfo(id);
	
	fThreads[id] = aThread;

	return true;
}

// ---------------------------------------------------------------------------

bool
ThreadList::RemoveThread(thread_id id)
{
	ThreadMap::iterator item = fThreads.find(id);
	if (item != fThreads.end()) {
		delete (*item).second;
		fThreads.erase(item);
		return true;
	}
	else {
		return false;
	}
}

// ---------------------------------------------------------------------------
// class SymbolTable - member functions
// ---------------------------------------------------------------------------

SymbolTable::SymbolTable()
{
}

// ---------------------------------------------------------------------------

SymbolTable::~SymbolTable()
{
	// delete all the symbol names
	for (SymbolMap::iterator item = fMap.begin(); item != fMap.end(); item++) {
		delete (*item).second;
	}
	fMap.clear();
}

// ---------------------------------------------------------------------------

void 
SymbolTable::AddSymbol(unsigned long location, const char* name)
{
	char* symbolName = new char[strlen(name)+1];
	strcpy(symbolName, name);
	fMap[location] = symbolName;
}

// ---------------------------------------------------------------------------

const char* kUnknownSymbol = "Unknown Function";

const char* 
SymbolTable::GetSymbolName(unsigned long addr)
{
	SymbolMap::iterator item = fMap.find(addr);
	if (item != fMap.end()) {
		return (*item).second;
	}
	else {
		return kUnknownSymbol;
	}
}

// ---------------------------------------------------------------------------

void
SymbolTable::FillBuckets(Bucket* buckets)
{
	// Because the buckets are actually part of the debugger message,
	// it has already been allocated with the correct size
	// Just iterate the symbol table and fill in the buckets

	for (SymbolMap::iterator item = fMap.begin(); item != fMap.end(); item++) {
		buckets->fAddress = (*item).first;
		buckets->fHitCount = 0;
		buckets++;
	}
}

// ---------------------------------------------------------------------------

#ifdef __ELF__

template <class T>
class auto_array_delete
{
public:
	explicit auto_array_delete(T* p) { mObj = p; }
	~auto_array_delete() { delete [] mObj; }

private:
	auto_array_delete(const auto_array_delete<T>&);
	auto_array_delete<T>& operator=(const auto_array_delete<T>&);
	T* mObj;
};

class ELFLoader
{
public:
	ELFLoader(const char* path);
	~ELFLoader();

	void LoadELFSection(const char* sectionName, uint8*& outSectionData, size_t& outSectionSize);

private:
	int mFile;
	Elf32_Ehdr mElfHeader;
	Elf32_Shdr* mSectionHeaders;
	uint8* mSectionStrings;
#if _SUPPORTS_COMPRESSED_ELF
	bool mIsCompressed;
	void* mCookie;
#endif
};


ELFLoader::ELFLoader(const char *path)
{
	mFile = open(path, O_RDONLY);
	if (mFile >= 0)
	{
#if _SUPPORTS_COMPRESSED_ELF
		status_t err = open_celf_file(mFile, &mCookie, NULL, 0);
		mIsCompressed = (err == B_OK);
		if (mIsCompressed)	// skip to the uncompressed ELF code if it's a standard ELF binary
		{
			// read the ELF header
			get_celf_elf_header(mCookie, &mElfHeader);

			// read the section headers
			mSectionHeaders = new Elf32_Shdr[mElfHeader.e_shnum];
			get_celf_section_headers(mCookie, mSectionHeaders);

			// read the section header string table
			Elf32_Shdr *shstr = mSectionHeaders + mElfHeader.e_shstrndx;
			mSectionStrings = new uint8[shstr->sh_size];
			get_celf_section(mCookie, mElfHeader.e_shstrndx, mSectionStrings, shstr->sh_size);
			return;		// explicitly return before reaching the uncompressed ELF code
		}
#endif

		// read the ELF header
		lseek(mFile, 0, SEEK_SET);
		read(mFile, &mElfHeader, sizeof(mElfHeader));

		// read the section headers
		mSectionHeaders = new Elf32_Shdr[mElfHeader.e_shnum];
		lseek(mFile, mElfHeader.e_shoff, SEEK_SET);
		read(mFile, mSectionHeaders, sizeof(Elf32_Shdr) * mElfHeader.e_shnum);

		// read the section header string table
		Elf32_Shdr *shstr = mSectionHeaders + mElfHeader.e_shstrndx;
		mSectionStrings = new uint8[shstr->sh_size];
		lseek(mFile, shstr->sh_offset, SEEK_SET);
		read(mFile, mSectionStrings, shstr->sh_size);
	}
}


ELFLoader::~ELFLoader()
{
	delete [] mSectionStrings;
	delete [] mSectionHeaders;

#if _SUPPORTS_COMPRESSED_ELF
	if (mIsCompressed)
		close_celf_file(mCookie);
#endif
	close(mFile);
}

void ELFLoader::LoadELFSection(const char* sectionName, uint8*& outSectionData, size_t& outSectionSize)
{
	if (mFile >= 0)
	{
		for (int i = 0; i < mElfHeader.e_shnum; i++)
		{
			if (strcmp(sectionName, (char*) mSectionStrings + mSectionHeaders[i].sh_name) == 0)
			{
				size_t size = mSectionHeaders[i].sh_size;
				outSectionData = new uint8[size];
				outSectionSize = size;
#if _SUPPORTS_COMPRESSED_ELF
				if (mIsCompressed)
					get_celf_section(mCookie, i, outSectionData, size);
				else
#endif
				{
					lseek(mFile, mSectionHeaders[i].sh_offset, SEEK_SET);
					read(mFile, outSectionData, size);
				}
				return;
			}
		}
	}

	// failure case -- return NULL data & 0 size
	outSectionData = NULL;
	outSectionSize = 0;
}

status_t 
SymbolTable::LoadSymbolsFromELFFile(const char *path, ulong codeaddr, ulong /*dataaddr*/)
{
	uint8 *symtabcp;
	uint8* strtab;
	size_t size;
	ELFLoader loader(path);
	loader.LoadELFSection(".strtab", strtab, size);
	if (!strtab) return B_OK;	// with no string table, we can't usefully look up symbols anyway

	auto_array_delete<uint8> strtabDeletor(strtab);
	loader.LoadELFSection(".symtab", symtabcp, size);
	if (symtabcp)
	{
		auto_array_delete<uint8> symtabDeletor(symtabcp);
		Elf32_Sym* symtabp = (Elf32_Sym*) symtabcp;
		size /= sizeof(Elf32_Sym);
	
		// Some functions are STT_NOTYPE (but are GLOBAL) so
		// keep STT_NOTYPE && GLOBAL (throw away LOCAL)
		// Notice that we are not keeping data objects (type OBJECT) 
		for (uint32 i = 0; i < size; i++)
		{
			if (symtabp->st_shndx != STN_UNDEF &&
				((ELF32_ST_TYPE(symtabp->st_info) == STT_FUNC && ELF32_ST_BIND(symtabp->st_info) <= STB_WEAK) ||
				 (ELF32_ST_TYPE(symtabp->st_info) == STT_NOTYPE && ELF32_ST_BIND(symtabp->st_info) == STB_GLOBAL)))
			{
				this->AddSymbol(codeaddr + (ulong) symtabp->st_value, (char*) strtab + symtabp->st_name);
			}
			symtabp++;
		}
	}

	return B_OK;
}

#endif

// ---------------------------------------------------------------------------
// LoadSymbolsFromFile are essentially just the c versions
// ---------------------------------------------------------------------------

#if __POWERPC__

static char *text_buffer;
static char *text_ptr;
static long text_size;

char *
mygets(char *s, int n)
{
	int i = 0;
	while ((text_ptr < text_buffer + text_size) && (i < n-1)) {
		s[i++] = *text_ptr++;
		if (text_ptr[-1] == '\n')
			break;
	}
	s[i] = '\0';
	return (i == 0 ? NULL : s);
}


status_t
SymbolTable::LoadSymbolsFromFile(const char* path, ulong codeaddr, ulong dataaddr)
{
	int			c, converted=-1;
	char        *cur_file = "<NO FILE>";
	char		str[512], fred[512], name[512], id[64], file[512], section[64];
	char        buff[512];
	unsigned long addr, base;
	int save_num_syms = 0;
	

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return B_ERROR;
	}
	
	text_size = lseek(fd, 0, 2);
	lseek(fd, 0, 0);
	text_buffer = (char *) malloc(text_size);
	text_ptr = text_buffer;
	if (text_buffer == NULL) {
		return B_ERROR;
	}
	
	size_t res = read(fd, text_buffer, text_size);
	if (res != text_size)
		goto error;
	close(fd);
		
	while (mygets(buff, sizeof(buff)) != NULL) {
		sscanf(buff, "%s", name);
	
		if (!strcmp(name, "Code"))
			break;
	}
	
	while (mygets(buff, sizeof(buff)) != NULL) {
		converted = sscanf(buff, "%s %s %s %s %s %s\n",
						   str, id, name, fred, fred, file);
	
		if (!strcmp(str, "Data"))
			break;
	
		addr = codeaddr + strtol(str, 0, 16);
	
		if (id[0] == 'P' && id[1] == 'R' && id[2] == '\0' && name[0] != '@')
			this->AddSymbol(addr, name);
	}
	
	/* it's only data symbols from here on out so skip 'em... */
	
	if (converted < 0) {   /* hmm, we must not have found any symbols... */
		goto error;
	}
	
	free(text_buffer);
	return B_OK;
	
error:
	printf("error loading symbols from: %s\n", path);
	free(text_buffer);
	return B_ERROR;
}

#endif

// ---------------------------------------------------------------------------

bool
SymbolTable::LoadImageSymbols(image_id id, const char* name)
{
	bool newlyLoaded = false;
	
	if (fLoadedImages.IsLoaded(id) == false) {
		status_t status;
		image_info info;
		get_image_info(id, &info);

#ifdef __ELF__
		printf("Loading symbols from: %s\n", name);
		status = this->LoadSymbolsFromELFFile(name, (ulong)info.text, (ulong)info.data);
#else
		char path[MAXPATHLEN];
		strcpy(path, name);
		strcat(path, ".xMAP");
		printf("Loading symbols from: %s\n", path);
		status = this->LoadSymbolsFromPEFFile(path, (ulong)info.text, (ulong)info.data);
#endif

		if (status == B_OK) {
			fLoadedImages.AddImage(id);
			newlyLoaded = true;
		}
		else {
			fprintf(stderr, "Error loading symbols for: %s 0x%lx %s\n", name, status, strerror(status));
		}
	}
	
	return newlyLoaded;
}

// ---------------------------------------------------------------------------

bool
SymbolTable::LoadTeamSymbols(team_id teamID)
{
	bool somethingLoaded = false;

	image_info info;
	image_id cookie = 0;
	while (get_next_image_info (teamID, &cookie, &info) == B_OK) {
		somethingLoaded |= this->LoadImageSymbols(info.id, info.name);
	}

	return somethingLoaded;
}

// ---------------------------------------------------------------------------
// BucketPrinter - little helper class for printing one line of report
// ---------------------------------------------------------------------------

class BucketPrinter {
public:
	BucketPrinter(int32 totalHits) 		{ fTotalHits = totalHits; }
	void operator()(const Bucket& aBucket);
private:
	int32		fTotalHits;
};

void
BucketPrinter::operator()(const Bucket& aBucket)
{
	char buffer[2046];
	const char* symbolName = gSymbolTable.GetSymbolName(aBucket.fAddress);
	if (demangle(symbolName, buffer, sizeof(buffer))) {
		symbolName = buffer;
	}
	
	printf("%7ld   %5.2f   0x%.8lx - %s\n", 
				aBucket.fHitCount,
				100.0*(double)aBucket.fHitCount / (double)fTotalHits,
				aBucket.fAddress, symbolName);
}

// ---------------------------------------------------------------------------

void
DumpProfileStatistics(thread_id threadID, int32 count, int32 minimum_hits)
{
	// Dump the profile statistics collected for this thread in descending order of hits
	// If the count != 0, then only count entries are dumped (or until we get to 0 hits)
	
	// Get our top hits counts recorded throughout the thread execution
	// Since the top count is ordered by address, we group them now by hit count
	ThreadInfo* threadInfo = gActiveThreads.GetThreadInfo(threadID);
	vector<Bucket> hits(threadInfo->fTopCount.begin(), threadInfo->fTopCount.end());
	sort(hits.begin(), hits.end(), HitCountGreater);

	// Add up the hits and make sure they are greater than our interest threshold
	int32 totalHits = 0;
	totalHits = accumulate(hits.begin(), hits.end(), totalHits, AddHitCount);

	if (totalHits < minimum_hits) {
		return;
	}
		
	// Now print out the top count items...	
	printf("Thread %ld - %s\n", threadID, threadInfo->ThreadName());
	printf("Total # of program counter samples for thread %ld\n", totalHits);
	printf("  #Hits    %%Pct   Name\n");

	BucketPrinter printer(totalHits);
	for_each(&hits[0], &hits[count], printer);	
}

// ---------------------------------------------------------------------------

status_t
StartProfiling(thread_id threadID)
{
	// The message to the nub contains the profiling header and then a set of buckets
	// that will contain the counter for that bucket.  We have to allocate the message
	// dynamically since it depends on the number of buckets.
	
	int32 size = sizeof(nub_start_profiler_msg) + gSymbolTable.Count() * sizeof(Bucket);
	nub_start_profiler_msg* msg = (nub_start_profiler_msg *) malloc(size);
	if (msg == NULL) {
		fprintf(stderr, "can't allocate debugger msg for %ld syms (%ld)\n", gSymbolTable.Count(), size);
		return B_ERROR;
	}

	msg->thid = threadID;
	msg->reply_port = gReplyPort;
	msg->perfmon_event = perfmon_event;
	msg->perfmon_counter = perfmon_counter;
	msg->num = gSymbolTable.Count();
	gSymbolTable.FillBuckets((Bucket*) msg->slots);

	status_t status = B_OK;
	
	int32 what;
	if (write_port(gNubPort, B_START_PROFILER, msg, size) == B_OK) {
		if (read_port(gReplyPort, &what, NULL, 0) >= 0) {
			status = B_OK;
		}
	}
	
	return status;
}

// ---------------------------------------------------------------------------

int
StopProfiling(thread_id threadID, int32 neededCount)
{
	// Read the hitcounts back from the profiler, and save the top counts
	// So that we save of the top hits for accurate reporting after
	// potentially restarting and continuing with these buckets,
	// we save the neededCount * 4 (with a max of what is available)
	
	nub_stop_profiler_msg msg;
	msg.thid = threadID;
	msg.reply_port = gReplyPort;
	msg.perfmon_counter = perfmon_counter;
	status_t status = write_port(gNubPort, B_STOP_PROFILER, &msg, sizeof msg);
	if (status != B_OK) {
		fprintf(stderr, "write port to nub_port %ld failed! 0x%lx %s\n", gNubPort, status, strerror(status));
		return status;
	}

	ssize_t size = port_buffer_size(gReplyPort);
	if (size < 0) {
		fprintf(stderr, "could not find out reply port size (%ld) 0x%lx %s\n", gReplyPort, size ,strerror(size));
		return size;
	}

	nub_stop_profiler_reply* reply = (nub_stop_profiler_reply *) malloc(size);
	if (reply == NULL) {
		fprintf(stderr, "could not allocate %ld bytes for reply\n", size);
		return B_NO_MEMORY;
	}

	int32 what;
	status = read_port(gReplyPort, &what, reply, size);
	if (status < 0) {
		fprintf(stderr, "read port(%ld) failed 0x%lx (%s)\n", gReplyPort, status, strerror(status));
		free(reply);
		return status;
	}

	if (what != B_OK) {
		fprintf(stderr, "what == 0x%lx (%s)\n", what, strerror(what));
		free(reply);
		return what;
	}
	
	// Add the top counts into our saved set
	// First, sort the results by hit order (highest to lowest)
	// (rember that STL end points to one beyond the last valid entry, and we need to use num*2
	// because the slot is only declared as a long)
	Bucket* firstBucket = (Bucket*) &reply->slots[0];
	Bucket* beyondLastBucket = (Bucket*) &reply->slots[reply->num*2];
	sort(firstBucket, beyondLastBucket, HitCountGreater);

	// Now insert the top hit buckets into our saved set (save 4x what is needed at end)
	ThreadInfo* threadInfo = gActiveThreads.GetThreadInfo(threadID);
	neededCount *= 4;
	int32 saveCount = neededCount < reply->num ? neededCount : reply->num;
	Bucket* aBucket = firstBucket;
	for (int i = 0; i < saveCount; i++) {
		threadInfo->AddToTopCounts(*aBucket);
		aBucket++;
	}

	free(reply);
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------

void
RestartProfiling(const ThreadPair& pair)
{
	StopProfiling(pair.first, gPrintCount_arg);
	StartProfiling(pair.first);
}

// ---------------------------------------------------------------------------

void
dump_usage(char *prog)
{
	fprintf(stderr, "\nUsage:  %s [-m minhits] [-c count] [-b] [-e event [-p period] [-pmc pmcounter] ]  program [program arguments]\n", prog);
	fprintf(stderr, "\tLaunch the specified program and display processor\n");
	fprintf(stderr, "\tconsumption statistics for each thread when it exits.\n");
	fprintf(stderr, "\n\tStatistics are based on random sampling of the program\n");
	fprintf(stderr, "\tcounter.  The -m option allows you to specify a minimum\n");
	fprintf(stderr, "\tnumber of samples; threads without that many samples\n");
	fprintf(stderr, "\tare considered statistically insignificant and are not\n");
	fprintf(stderr, "\tdisplayed. The -c option specifies how many levels deep to\n");
	fprintf(stderr, "\tprint. The -b option allows you to take into account time\n");
	fprintf(stderr, "\tspent in blocking calls.\n");
}


// ---------------------------------------------------------------------------

#ifdef __ELF__
#define IMAGE_CREATED B_ELF_IMAGE_CREATED
#define IMAGE_DELETED B_ELF_IMAGE_DELETED
#else
#define IMAGE_CREATED B_PEF_IMAGE_CREATED
#define IMAGE_DELETED B_PEF_IMAGE_DELETED
#endif

// ---------------------------------------------------------------------------

int
main(int argc, char **argv)
{
	int image_argument = 0;
	
	for(int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			dump_usage(argv[0]);
			return 0;
		}
		else if (strcmp(argv[i], "-c") == 0) {
			if (argc > i + 1) {
				gPrintCount_arg = atoi(argv[++i]);
			}
		}
		else if (strcmp(argv[i], "-m") == 0) {
			if (argc > i + 1) {
				gMinimumHits_arg = atoi(argv[++i]);
			}
		}
		else if (strcmp(argv[i], "-e") == 0) {
			if (argc > i + 1) {
				perfmon_event.event = strtol(argv[++i], NULL, 0);
			}
		}
		else if (strcmp(argv[i], "-p") == 0) {
			if (argc > i + 1) {
				perfmon_event.init_val = -strtol(argv[++i], NULL, 0);
			}
		}
		else if (strcmp(argv[i], "-pmc") == 0) {
			if (argc > i + 1)
				perfmon_counter = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-b") == 0) {
			perfmon_event.flags = 1;
		}
		else {
			image_argument = i;
			break;
		}
	}

	if (image_argument == 0) {
		dump_usage(argv[0]);
		return 1;
	}

	thread_id threadID = load_image(argc-image_argument, (const char**) &argv[image_argument], (const char**) environ);
	if (threadID < 0) {
		fprintf(stderr, "Could not load %s: %s\n", argv[image_argument], strerror(threadID));
		exit(5);
	}
	
	thread_info threadInfo;
	try {
		if (get_thread_info(threadID, &threadInfo) < 0) {
			throw "Cannot get thread information";
		}
	
		if (gSymbolTable.LoadTeamSymbols(threadInfo.team) == false) {
			throw "No symbol information";
		}

		// set ourselves up as the team debugger so we can do profiling magic
		gPort = create_port(5, "profile-dbport");
		gReplyPort = create_port(5, "profile-reply_port");
		if (gPort < 0 || gReplyPort < 0) {
			throw "Internal error - cannot create ports for profiler";
		}
	
		gNubPort = install_team_debugger(threadInfo.team, gPort);
		if (gNubPort < 0) {
			throw "Internal error - cannot install ourselves as the team debugger (needed for profiling)";
		}
	
		gActiveThreads.AddThread(threadID);
		StartProfiling(threadID);
	
		resume_thread(threadID);
	}
	catch (const char* error) {
		// print the error, kill the thread we started, and quit
		fprintf(stderr, "%s - %s [%ld]\n", error, argv[image_argument], threadID);
		kill(threadID, SIGKILL);
		exit(10);
	}
	
	int32 keep_going = 1;
	int32 what;
	to_debugger_msg  msg;
	while (keep_going) {
		read_port(gPort, &what, &msg, sizeof (msg));

		switch (what) {
			case B_THREAD_STOPPED:
			case B_TEAM_DELETED:
				keep_going = 0;
				break;

			case IMAGE_CREATED: 
			{
				nub_acknowlege_image_created_msg m;
				m.token = msg.pef_image_created.reply_token;	
				write_port(gNubPort, B_ACKNOWLEGE_IMAGE_CREATED, &m, sizeof(m));
				
				// Got an add-on, load up the new symbols
				// After doing that, we need to restart all the profiling so that
				// the profiler knows about the symbols that (potentially) will
				// be called.
				// We need to load the entire team again here because the add-on can
				// load in new shared libraries that we want to pick up.
				// (We only load the new stuff...)
				
				if (gSymbolTable.LoadTeamSymbols(msg.pef_image_created.team) == true) {
					gActiveThreads.ForEach(&RestartProfiling);
				}
				break;
			}

			case B_THREAD_CREATED:
				// bump keep_going, and start profiling in this new thread
				keep_going++;
				gActiveThreads.AddThread(msg.thread_created.thread);
				StartProfiling(msg.thread_created.thread);
				break;
	
			case B_GET_PROFILING_INFO:
				// called when a thread goes away, report what we found for that thread
				StopProfiling(msg.get_profile_info.thread, gPrintCount_arg);
				DumpProfileStatistics(msg.get_profile_info.thread, gPrintCount_arg, gMinimumHits_arg);
				gActiveThreads.RemoveThread(msg.get_profile_info.thread);
				keep_going--;
				break;
			
			default:
				break;
		}
	}

	return 0;
}
