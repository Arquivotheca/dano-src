#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/file.h>
#include <OS.h>
#include <SupportDefs.h>
#include <elf.h>
#include <image.h>
#include <string.h>
#include <Unmangle.h>

#if 1
#define PRINT(x)
#else
#define PRINT(x)	printf x
#endif

struct profile_data {
	char 		  *symname;
	unsigned int  addr;
	unsigned int  count;
	bigtime_t	  time;	
	bigtime_t     total_time;
};

struct symtable {
	struct symtable *next;
	struct symtable *prev;
	char name[MAXPATHLEN];
	struct profile_data *syms;
	unsigned int  numsyms;
};

typedef struct symtable symtable;

static symtable *Images;

static status_t load_symbols(char *name, symtable *t);

static int
compare_by_addr(const void *_a, const void *_b)
{
	struct profile_data *a, *b;

	a = (struct profile_data *) _a;
	b = (struct profile_data *) _b;

	if (a->addr < b->addr)
		return -1;
	else if (a->addr > b->addr)
		return 1;
	else
		return 0;
}


static int
compare_by_time(const void *_a, const void *_b)
{
	struct profile_data *a, *b;

	a = (struct profile_data *) _a;
	b = (struct profile_data *) _b;

	if (a->time < b->time)
		return -1;
	else if (a->time > b->time)
		return 1;
	else
		return 0;
}

static int
compare_by_elapsed(const void *_a, const void *_b)
{
	struct profile_data *a, *b;

	a = (struct profile_data *) _a;
	b = (struct profile_data *) _b;

	if (a->total_time < b->total_time)
		return -1;
	else if (a->total_time > b->total_time)
		return 1;
	else
		return 0;
}

static int
compare_by_call(const void *_a, const void *_b)
{
	struct profile_data *a, *b;

	a = (struct profile_data *) _a;
	b = (struct profile_data *) _b;

	if (a->count < b->count)
		return -1;
	else if (a->count > b->count)
		return 1;
	else
		return 0;
}

static struct profile_data *
my_bsearch(unsigned int addr, struct profile_data *syms, size_t nmemb)
{
	size_t l, u, idx;
	struct profile_data *p;
	unsigned int elemaddr;

	l = 0;
	u = nmemb;
	while (l < u) {
		idx = (l + u) / 2;
		p = &syms[idx];
		if (addr < p->addr)
			u = idx;
		else if (addr > p->addr)
			l = idx + 1;
		else
			goto found;
    }

//	printf("idx = %d\n", idx);
	if (idx == 0) {
		if (addr > p->addr) 
			goto found;
	} else if (idx == nmemb-1) {
		if (addr < p->addr){
			idx = idx-1;
		}
		goto found;
	} else {
		if (p->addr > addr) {
			idx -= 1;
			goto found;
		}
		else if (p->addr < addr) {
			goto found;
		}
	}

	return NULL;

 found:
	/*
	 * Weak symbols sometimes have the same
	 * address as real ones. In this case, we
	 * want only one symbol to be reported. So
	 * walk back and find the first one in the
	 * sorted list.
	 */
	elemaddr = syms[idx].addr;
	while (idx >= 0 && elemaddr == syms[idx].addr)
		idx--;
	return &syms[idx+1];
}

static int
is_library_prefix(char* string)
{
	/* 
	 * returns the version of the library prefix
	 * 0 if not a library prefix at all
	 */

	int version = 0;
		 
	if (strncmp(string, "!START!!", 8) == 0) {
		// original version - only one time per function
		version = 1;
	}
	else if (strncmp(string, "!START!2", 8) == 0) {
		// include time and total-time per function
		version = 2;
	}

	return version;
}

static void
print_output(int version, struct profile_data* symbol, char* name)
{
	// If we are using Profile.h C++ support, we get ~Profile in the mix.
	// (if the app was built without optimizations)
	// Skip that entry if it shows up
	if (symbol->count == 1 && strcmp(name, "Profile::~Profile(void)") == 0) {
		return;
	}
	
	if (version == 2) {
		printf("\t%d\t%Ld us\t%Ld us\t- %s\n", symbol->count, symbol->time, symbol->total_time, name);
	}
	else {
		printf("\t%d\t%Ld us\t- %s\n", symbol->count, symbol->time, name);
	}
}

int
main(int argc, char *argv[])
{
	int fd;
	char magic[8];
	symtable *image;
	int i;
	int version = 1;
	int file_argument = 1;
	bool sort_by_elapsed = FALSE;
	bool sort_by_call = FALSE;
	
	if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
		fprintf(stderr, "Usage: %s [-e] profile_log \n", argv[0]);
		fprintf(stderr, "Interpret a profile log created by running a program compiled and linked\n");
		fprintf(stderr, "with profiling enabled (-p).\n");
		fprintf(stderr, "    -e\t Sort output by elapsed time\n");
		fprintf(stderr, "    -f\t Sort output by function call count\n");
		fprintf(stderr, "Output format (default sorting by time in function)...\n");
		fprintf(stderr, "    Number of times function called\n");
		fprintf(stderr, "    Time spent in function in microseconds\n");
		fprintf(stderr, "    Elapsed time spent in function (and children) in microseconds\n");
		fprintf(stderr, "    Name of function (demangled for C++)\n");
		exit(-1);
	}
	
	if (strcmp(argv[1], "-e") == 0) {
		sort_by_elapsed = TRUE;
		file_argument = 2;
	}
	else if (strcmp(argv[1], "-f") == 0) {
		sort_by_call = TRUE;
		file_argument = 2;
	}

	fd = open(argv[file_argument], O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(-1);
	}

	if (read(fd, magic, 8) < 0  || (version = is_library_prefix(magic)) == 0) {
		perror("read of magic");
		exit(-2);
	}

	while (1) {
		char pathname[MAXPATHLEN+1];
		char buf[20];
		int32 bytes;
		uint32 *addr = (uint32 *) &buf[0];
		uint32 *count = (uint32 *) &buf[4];
		pathname[MAXPATHLEN] = 0;
		if ((bytes = read(fd, pathname, MAXPATHLEN)) != MAXPATHLEN) {
			if (bytes == 0) {
				// at end of file, quit the loop
				break;
			}
			else {
				// read something, but not what we wanted to
				fprintf(stderr, "error reading library name from profile - %s\n", pathname);
				break;
			}
		}

		// pathname printed in load_symbols		
		image = (symtable *) calloc(1, sizeof(symtable));
		if (Images) {
			image->next = Images;
			Images->prev = image;
		} 
		Images = image;
		if (load_symbols(pathname, image) != B_NO_ERROR) {
printf("error loading symbols\n");
//			break;
		}

		while (read(fd, buf, 8) == 8) {
			bigtime_t in_time;
			bigtime_t total_time = 0;
			struct profile_data *sym;

			if (is_library_prefix(buf) != 0) {
				break;
			}
			if (read(fd, &in_time, sizeof(in_time)) != sizeof(in_time)) {
				perror("reading time");
				break;
			}

			if (version >= 2) {
				if (read(fd, &total_time, sizeof(total_time)) != sizeof(total_time)) {
					perror("reading total time");
					break;
				}
			}
			
			sym = my_bsearch(*addr, image->syms, image->numsyms);
			if (!sym)
				continue;
//			printf("adding %x to %x\n", *addr, sym->addr); 
			if (sym->count) {
				printf("gack! overlap\n");
			}
			sym->count = *count;
			sym->time = in_time;
			if (version >= 2) {
				sym->total_time = total_time;
			}
			
			/* printf("\t0x%x: \t %d %Ld us\n", *addr, (int) *count, in_time);*/
		}

		qsort((void *) image->syms, image->numsyms, 
			  sizeof(struct profile_data), 
			  sort_by_elapsed == TRUE ? compare_by_elapsed : 
			  							sort_by_call == TRUE ? compare_by_call : compare_by_time);

		for (i = image->numsyms-1; i >= 0; i--) {
			if (image->syms[i].count) {
				char name2[UNAME_SIZE];
				char *str = name2;
				if (!demangle(image->syms[i].symname, name2, UNAME_SIZE))
					str = image->syms[i].symname;
				print_output(version, &image->syms[i], str);
			}
		}
	}
}


static char *
basename(char *name)
{
	char *base;

	base = strrchr(name, '/');
	return base ? base + 1 : name;
}   

static status_t
load_symbols(char *name, symtable *t)
{
	FILE		*file = NULL;
	Elf32_Shdr	*sects = NULL;
	Elf32_Shdr	*strsect = NULL;
	Elf32_Shdr	*symsect = NULL;
	char		*elfnames = NULL;
	Elf32_Sym	*elfsyms = NULL;
	Elf32_Sym	*s;
	int			n_elfsyms;
	Elf32_Ehdr	h;
	int			i, j, symcount = 0;
	int			type;
	int			retval = B_ERROR;

	printf("%s: ", name);
	if (!(file = fopen(name, "rb"))) {
		char *bname = basename(name);
		printf("not found\n"); 
		printf("./%s: ", bname);
		if (!(file = fopen(basename(name), "rb"))) {
			printf("not found\n");
			goto file_err_exit;
		}
	}
	printf("\n");
	
	
	/* ---
	   read in ELF header, ensure it is an ELF file
	--- */

	if (fread(&h, 1, sizeof(h), file) < sizeof (h)) {
printf("couldnt read elf header\n");
		goto file_err_exit;
}

	/* check magic number */
	if (h.e_ident[EI_MAG0] != ELFMAG0
		&& h.e_ident[EI_MAG1] != ELFMAG1
		&& h.e_ident[EI_MAG2] != ELFMAG2
		&& h.e_ident[EI_MAG3] != ELFMAG3) {
		fprintf(stderr, "%s: bad magic\n");
		goto exit;
	}

	/* check file class */
	if (h.e_ident[EI_CLASS] != ELFCLASS32) {
		PRINT(("load_elf_symbols: bad class, got 0x%x\n", h.e_ident[EI_CLASS]));
		goto exit;
	}

	/* check data encoding */
	if (h.e_ident[EI_DATA] != ELFDATA2LSB) {
		PRINT(("load_elf_symbols: bad data, got 0x%x\n", h.e_ident[EI_DATA]));
		goto exit;
	}

	/* check format version */
	if (h.e_ident[EI_VERSION] != EV_CURRENT) {
		PRINT(("load_elf_symbols: bad version, got 0x%x\n", h.e_ident[EI_VERSION]));
		goto exit;
	}

	/* ---
	   read in section header array
	--- */

	i = h.e_shnum * h.e_shentsize;
	if (!(sects = (Elf32_Shdr *) malloc(i)))
		goto mem_err_exit;
	
	if (fseek(file, h.e_shoff, SEEK_SET))
		goto file_err_exit;
	if (fread(sects, 1, i, file) != i)
		goto file_err_exit;

	/* ---
	   locate elf symbol table section, read it in
	--- */

	for (i = 0, symsect = NULL; i < h.e_shnum && !symsect; i++)
		if (sects[i].sh_type == SHT_SYMTAB)
			symsect = sects + i;

	if (!symsect) {
		PRINT(("load_elf_symbols: no symtable section!\n"));
		goto exit;
	}

	n_elfsyms = symsect->sh_size / sizeof (Elf32_Sym);

	PRINT(("load_elf_symbols: n_elfsyms = %d\n", n_elfsyms));

	if (!(elfsyms = (Elf32_Sym *) malloc(symsect->sh_size)))
		goto mem_err_exit;
	
	if (fseek(file, symsect->sh_offset, SEEK_SET))
		goto file_err_exit;
	if (fread(elfsyms, 1, symsect->sh_size, file) != symsect->sh_size)
		goto file_err_exit;

	/* ---
	   locate elf string table section, read it in
	--- */

	strsect = sects + symsect->sh_link;	/* sym sect has link to string table */

	if (!(elfnames = (char *) malloc(strsect->sh_size)))
		goto mem_err_exit;
	
	if (fseek(file, strsect->sh_offset, SEEK_SET))
		goto file_err_exit;
	if (fread (elfnames, 1, strsect->sh_size, file) != strsect->sh_size)
		goto file_err_exit;

	/* ---
	   extract the 'useful' symbols, put into new symbol table
	--- */

	t->syms = (struct profile_data *) calloc(n_elfsyms, sizeof(struct profile_data));
	if (!t->syms)
		goto mem_err_exit;

	for (i = 0; i < n_elfsyms; i++) {
		s = elfsyms + i;
		type = ELF32_ST_TYPE(s->st_info);
		j = ELF32_ST_BIND(s->st_info);
		if (type == STT_FUNC  && (j == STB_LOCAL || j == STB_GLOBAL || j == STB_WEAK)) {
			struct profile_data *pd = &t->syms[symcount];
			pd->symname = strdup(elfnames + s->st_name);
			pd->addr = (unsigned int)((int)s->st_value);
			symcount++;
		}
	}

	t->numsyms = symcount;

	/* ---
	   set up all the various indexes
	--- */

	if (t->numsyms == 0) {
		free(t->syms);
		t->syms = NULL;
		PRINT(("load_elf_symbols: nothing in symtable!\n"));
		goto exit;
	} else if (t->numsyms != n_elfsyms) {
		t->syms = (struct profile_data *) realloc(t->syms, t->numsyms * sizeof(struct profile_data));
	}

	PRINT(("symtable has %d syms\n", t->numsyms));

	qsort((void *) t->syms, t->numsyms, sizeof(struct profile_data), compare_by_addr);

#if 0
	for (i = 0; i < t->numsyms; i++) {
		char name2[UNAME_SIZE];
		char *str = t->syms[i].symname;
#if 0
		char *str = name2;
		if (!demangle(t->syms[i].symname, name2, UNAME_SIZE))
			str = t->syms[i].symname;
#endif
		printf("%x: %s\n", t->syms[i].addr, str);
	}
#endif

//	PRINT(("finished sorting\n"));

	retval = B_NO_ERROR;
	goto exit;

mem_err_exit:
	PRINT (("load_elf_symbols: out of memory\n"));
	goto exit;

file_err_exit:
	PRINT (("load_elf_symbols: error accessing file\n"));
	
exit:
	if (file) fclose (file);
	if (sects) free(sects);
	if (elfsyms) free(elfsyms);
	if (elfnames) free(elfnames);
//	PRINT (("load_elf_symbols: retval=%d\n", retval));
	return retval;
}

