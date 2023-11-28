
#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <string.h>
#include <SupportDefs.h>
#include <Path.h>
#include <OS.h>

struct version_rec {
	char versionID[128];
	char derivedFrom[128];
	time_t checkoutTime;
	time_t checkinTime;
	time_t finishTime;
	time_t installByTime;
	bool reboot;
	bool redial;
	bool restart;
	bool flush;
	bool freeze;
};

class arglist
{
	char **		m_args;
	bool *		m_needFree;
	int32		m_size;
	int32		m_count;

	void free_needed()
	{
		for (int32 i=0;i<m_count;i++) {
			if (m_needFree[i]) free(m_args[i]);
		}
	}
	
	public:

	void clear() {
		free_needed();
		m_count = 0;
	}
	
	arglist() {
		m_args = NULL;
		m_needFree = NULL;
		m_size = m_count = 0;
	}
	
	~arglist() {
		clear();
		if (m_args) free(m_args);
		if (m_needFree) free(m_needFree);
	}

	operator const char ** () const {
		return const_cast<const char **>(m_args);
	}		

	char * &operator[](int32 i) const {
		return m_args[i];
	}		

	int32 count() {
		return m_count;
	}
	
	void add(const char *arg, bool needFree=false) {
		if (m_count == m_size) {
			if (!m_size) m_size = 16;
			else m_size *= 2;
			m_args = (char**)realloc(m_args,m_size*sizeof(char*));
			m_needFree = (bool*)realloc(m_needFree,m_size*sizeof(bool));
		}
		m_needFree[m_count] = needFree;
		m_args[m_count++] = const_cast<char*>(arg);
	}
	
};

class filelist
{
	public:

		enum {
			IS_BINARY = 0x01,
			IS_DIR = 0x02
		};

		struct filerec {
			char *filename;
			char *md5;
			int64 size;
			uint32 flags;
			
			bool isElf() { return flags & IS_BINARY; };
			bool isDir() { return flags & IS_DIR; };
		};

	private:

		struct filerec_chunk {
			int32 size;
			int32 used;
			filerec recs[1];
		};

		enum {
			flSorted = 0x00000001
		};

		int32				m_chunksMalloced;
		filerec_chunk **	m_fileAllocs;
		int32				m_fileCount;
		int32				m_fileListSize;
		filerec	**			m_files;
		uint32				m_flags;
		
		static int compare_filerecs(const void *e1, const void *e2) {
			const filerec *f1 = *((filerec**)e1);
			const filerec *f2 = *((filerec**)e2);
			return strcmp(f1->filename,f2->filename);
		}

	public:

		filerec &operator[](int32 i) const {
			return *m_files[i];
		}		

		void init() {
			m_chunksMalloced = 0;
			m_fileAllocs = NULL;
			m_fileCount = 0;
			m_fileListSize = 0;
			m_files = NULL;
			m_flags = 0;
		}

		void clear() {
			if (m_files) free(m_files);
			if (m_chunksMalloced) {
				for (int32 i=0;i<m_chunksMalloced;i++) {
					for (int32 j=0;j<m_fileAllocs[i]->used;j++) {
						if (m_fileAllocs[i]->recs[j].filename) free(m_fileAllocs[i]->recs[j].filename);
						if (m_fileAllocs[i]->recs[j].md5) free(m_fileAllocs[i]->recs[j].md5);
					}
					free(m_fileAllocs[i]);
				}
				free(m_fileAllocs);
			}
			init();
		}
		
		filelist() {
			init();
		}

		~filelist() {
			clear();
		}

		void assert_sorted() {
			if (!(m_flags & flSorted)) {
				qsort(m_files,m_fileCount,sizeof(filerec*),compare_filerecs);
				m_flags |= flSorted;
			}
		}

		void remove_duplicates() {
			assert_sorted();
			int32 src=1,dst=1;
			if (m_fileCount < 2) return;

			while (1) {
				while (!strcmp(m_files[src]->filename,m_files[dst-1]->filename)) {
					if (++src == m_fileCount) goto done;
				}
				m_files[dst++] = m_files[src];
				if (++src == m_fileCount) goto done;
			}
			done:
			m_fileCount = dst;
		}

		void add(filerec *fr) {
			if (m_fileCount == m_fileListSize) {
				if (!m_fileListSize) m_fileListSize = 100;
				else m_fileListSize *= 2;
				m_files = (filerec**)realloc(m_files,m_fileListSize*sizeof(filerec*));
			}
			m_files[m_fileCount++] = fr;
			m_flags &= ~flSorted;
		}

		void add(const char *filename, const char *md5, int64 size, uint32 flags) {
			if (!m_chunksMalloced || (m_fileAllocs[m_chunksMalloced-1]->used == m_fileAllocs[m_chunksMalloced-1]->size)) {
				m_chunksMalloced++;
				m_fileAllocs = (filerec_chunk**)realloc(m_fileAllocs,sizeof(filerec_chunk*)*m_chunksMalloced);
				m_fileAllocs[m_chunksMalloced-1] = (filerec_chunk*)malloc(sizeof(filerec_chunk) + sizeof(filerec)*99);
				m_fileAllocs[m_chunksMalloced-1]->size = 100;
				m_fileAllocs[m_chunksMalloced-1]->used = 0;
			}
			filerec_chunk *c = m_fileAllocs[m_chunksMalloced-1];
			c->recs[c->used].filename = strdup(filename);
			c->recs[c->used].md5 = strdup(md5);
			c->recs[c->used].size = size;
			c->recs[c->used].flags = flags;
			add(&c->recs[c->used]);
			c->used++;
		}
				
		int32 count_elf() {
			int32 count = 0;
			for (int32 i=0;i<m_fileCount;i++) {
				if (m_files[i]->isElf()) count++;
			}
			return count;
		}

		void add(filelist &fl) {
			for (int32 i=0;i<fl.m_fileCount;i++)
				add(fl.m_files[i]);
		}

		void copy(filelist &fl) {
			clear();
			for (int32 i=0;i<fl.m_fileCount;i++)
				add(fl.m_files[i]);
		}

		int32 count() {
			return m_fileCount;
		}
		
		enum ma_filetype { ALL=0, ONLY_ELF, ONLY_NON_ELF };
		void make_args(arglist &args, ma_filetype ft=ALL, BPath *commonParent=NULL) {
			for (int32 i=0;i<m_fileCount;i++) {
				if ((ft==ALL) || ((ft==ONLY_ELF) == m_files[i]->isElf())) {
					bool addit = !commonParent;
					if (!addit) {
						BPath tmp(m_files[i]->filename);
						tmp.GetParent(&tmp);
						addit = !strcmp(tmp.Path(),commonParent->Path());
					}

					if (addit) args.add(m_files[i]->filename);
				}
			}
		}

		status_t read(const char *fn) {
			char file[1024];
			FILE *f = fopen(fn,"r");
			if (!f) return B_ERROR;
			while (fscanf(f,"%1023s",file) == 1)
				add(file,NULL,0,0);
			fclose(f);
			return B_OK;
		}

		status_t write(const char *fn) {
			FILE *f = fopen(fn,"w");
			if (!f) return B_ERROR;
			assert_sorted();
			for (int32 i=0;i<m_fileCount;i++)
				fprintf(f,"%s\n",m_files[i]->filename);
			fclose(f);
			return B_OK;
		}

		void output() {
			assert_sorted();
			for (int32 i=0;i<m_fileCount;i++) {
				printf("%s:%Ld:%s %s\n",m_files[i]->md5,m_files[i]->size,m_files[i]->filename,(m_files[i]->flags & IS_BINARY)?"(elf binary)":"");
			}
		}
		
		static status_t make_diffs(filelist &fl1, filelist &fl2, filelist &added, filelist &removed, filelist &changed);
		static status_t merge(filelist &fl1, filelist &fl2, bool onlyElf, filelist &final);
		static status_t subtract(filelist &fl1, filelist &fl2, filelist &final);
		static status_t intersection(filelist &fl1, filelist &fl2, filelist &final);
		static status_t lbxify(filelist &in, filelist &out);
};

extern int32 add_die_hook( void( *hook)( void *), void *userData);
extern void remove_die_hook(int32 id);
extern char *get_time(time_t t);
extern void output(const char *type, const char *msg, ...);
extern void warning(const char *msg, ...);
extern void status(const char *msg, ...);
extern void error_die(const char *msg, ...);
extern int file_to_filelist(const char *pathname, filelist &files, bool addExtraData);
extern int exec_to_filelist(int32 argc, const char **argv, filelist &files, bool addExtraData=false);
extern int add_all_files(const char *dir, filelist &files, bool extraData=true);
extern int rm(const char *fn);
extern int copyfile(const char *from, const char *to, const char *flags=NULL);
extern status_t get_version(const char *name, BPath *pathToRoot, uint32 packages);
extern status_t simple_exec(int32 argc, const char **argv);
extern thread_id do_exec(int32 argc, const char **argv, int *input, int *output, int *error=NULL);
extern char *do_md5(const char *path, bool &isElf);
extern void start_md5_thread();
extern status_t stop_md5_thread();
extern void read_version_record( const char *versionName, version_rec *r, bool dump=false);
extern void write_version_record(version_rec *r);
extern void create_version_record(version_rec *r, const char *name, const char *oldName="");
extern bool isdir(const char *pathname);
extern bool isfile(const char *pathname);

class death_hook {
	int32 id;
	public:
	death_hook(void (*hook)(void*),void *userData) {
		id = add_die_hook(hook,userData);
	}
	~death_hook() { remove_die_hook(id); };
};

#endif
