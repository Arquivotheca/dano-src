/* ++++++++++
		
	bt_load_pe.c
	Copyright (C) 1995 Be Labs, Inc.  All Rights Reserved
	PE loading routines for the bootstrap loader.

	Modification History (most recent first):
+++++ */

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

#include <OS.h>

#include <boot.h>
#include <ram.h>
#include <mem.h>

#include "elf.h"

#include "vm.h"

#if _SUPPORTS_COMPRESSED_ELF
#include "uncrush.h"
#endif

#define ddprintf	dprintf

#define		ABSTRACT_KERNEL_NAME	"_KERNEL_"

#define		MAX_SEGMENTS			6
#define		MAX_SECTIONS			50
#define		MAX_IMPORTS				32

#define		MATCH_TABLE_BUF_INC		512

#define		PATCH_BEFORE			0
#define		PATCH_AFTER				1
#define		PATCH_AFTER_SPECIFIER		"%AFTER"
#define		PATCH_BEFORE_SPECIFIER		"%BEFORE"
#define		CPU_FEATURE_SPECIFIER		"%CPUFEATURE"
#define		DFLT_CPU_FEATURE_STRING		"default"
#define		NO_CPU_FEATURE_STRING		"none"
#define		CPU_ATTR_MMX				1
#define		CPU_ATTR_CMOV				2
#define		CPU_ATTR_KNI				4
#define		CPU_ATTR_3DNOW				8
#define		CPU_LIB_ATTR_NUM			4


typedef struct cont_entry cont_entry;
typedef struct match_table match_table;

struct cont_entry {
	ino_t				ino;
	char				path[256];
	struct cont_entry	*next;
	struct cont_entry	*prev;

	void				*info;

	Elf32_Ehdr			eheader;
	Elf32_Shdr			sheaders[MAX_SECTIONS];
	Elf32_Phdr			pheaders[MAX_SEGMENTS];
	
	char				*base_address;
	uint32				size;
	int32				delta;

	struct cont_entry	**searchlist;
	int					color;

	match_table			*match[2];

	Elf32_Dyn			*dynamic;
	int32				import_num;
	int32				import_names[MAX_IMPORTS];
	struct cont_entry	*imports[MAX_IMPORTS];
	int32				pltrelsz;
	char				*pltgot;
	char				*got;
	Elf32_Sword			*hash;
	char				*strtab;
	Elf32_Sym			*symtab;
	Elf32_Rela			*rela;
	int32				relasz;
	int32				relaent;
	int32				strsz;
	int32				syment;
	char				*init;
	char				*fini;
	const char			*soname;
	int32				rpath;
	bool				symbolic;
#if _SUPPORTS_COMPRESSED_ELF
	bool				compressed;
#else
	uint8			filler1;
#endif
	uint16			filler2;
	Elf32_Rel			*rel;
	int32				relsz;
	int32				relent;
	int32				pltrel;
	bool				textrel;
	char				*jmprel;
};

struct match_table {
	int				num;
	char			**in;
	char			**out;
	char			*buf;
};

static cont_entry	*first_ce;
static cont_entry	*kce, *fce;
static char			*vbase;
static char			*nxinfo;

static char		got_symbol_name[] = "_GLOBAL_OFFSET_TABLE_";
static char		match_file_suffix[] = ".patch";

static cont_entry **build_search_list(cont_entry *head);
static Elf32_Sym *	search_sym(cont_entry *ice, uint32 isymind, bool outside, cont_entry **ece);
static Elf32_Sym *	search_match(cont_entry *ce, match_table *match, const char *name, bool localok);
static Elf32_Sym *	search_obj(cont_entry *ce, const char *name, bool usematch, bool localok);
static int			process_relocation(cont_entry *ce, char *addr, Elf32_Word info, Elf32_Sword addend);
static int			relocate_cont(cont_entry *ce);
static uint32		get_cpu_mask(void);
static int32		count_mask_bits(uint32 mask);
static status_t		cpu_feature_string_to_mask(const char *name, int32 len, uint32 *maskp);
static int			make_match_table(int fd, match_table **matchp);
static int			process_relocation(cont_entry *ce, char *addr, Elf32_Word info, Elf32_Sword addend);
static int			relocate_cont(cont_entry *ce);
#if _SUPPORTS_COMPRESSED_ELF
static cont_entry *	load_cont(const char *path, const char *real_kernel_name,
					          const char *mount, char *dict, uint32 dictsize);
#else
static cont_entry *	load_cont(const char *path, const char *real_kernel_name, const char *mount);
#endif

char *		xslate_addr(cont_entry *ce, char *addr);
#define	xslate_addr(ce, addr)	((ce)->delta + (addr))
#define	write_word(p, w)		*(p) = (w);

static cont_entry **
build_search_list(cont_entry *head)
{
	cont_entry		**list, **tmp;
	cont_entry		*ace, *bce;
	int				first, last, nx;
	int				i, j;

	list = (cont_entry **) malloc(16*sizeof(void *));
	if (!list)
		return NULL;
	
	for(ace=first_ce; ace; ace=ace->next)
		ace->color = 0;

	first = 0;
	last = 1;
	list[0] = head;
	nx = last;
	while(first < last) {
		for(i=first; i<last; i++) {
			ace = list[i];
			for(j=0; j<ace->import_num; j++) {
				bce = ace->imports[j];
				if (bce->color == 1)
					continue;
				if (nx % 16 == 15) {
					tmp = (cont_entry **) realloc(list, (nx+16-1)*sizeof(void *));
					if (!tmp) {
						free(list);
						return NULL;
					}
					list = tmp;
				}
				list[nx++] = bce;
				bce->color = 1;
			}
		}
		first = last;
		last = nx;
	}
	list[nx] = NULL;
	return list;
}

static Elf32_Sym *
search_sym(cont_entry *ice, uint32 isymind, bool outside, cont_entry **ece)
{
	cont_entry		*ace;
	Elf32_Sym		*isym, *esym;
	char			*name;
	int				i;

	if (!ice->symtab) {
		dprintf("search_sym(%s): NO SYM TABLE!\n", ice->path);
		return NULL;
	}
	
	isym = &ice->symtab[isymind];

	if (isymind == STN_UNDEF) {
		*ece = ice;
		return isym;
	}

	if (!outside && (isym->st_shndx != SHN_UNDEF)) {
		*ece = ice;
		return isym;
	}

	name = ice->strtab + isym->st_name;

	for(i=0; ice->searchlist[i]; i++) {
		ace = ice->searchlist[i];
		if ((ace == ice) && outside)
			continue;
		esym = search_obj(ace, name, TRUE, (ace == ice));
		if (esym) {
			*ece = ace;
			return esym;
		}
	}
	dprintf("search_sym(%s for %s): NOT FOUND!\n", name, ice->path);
	return NULL;
}

static Elf32_Sym *
search_match(cont_entry *ce, match_table *match, const char *name, bool localok)
{
	int32			low, high, med;
	int				c;

	/* perform a binary search */

	low = 0;
	high = match->num-1;
	while (TRUE) {
		med = (low + high) / 2;
		c = strcmp(name, match->in[med]);
		if (c == 0) {
dprintf("KERNEL: search_obj: %s will be patched to %s...\n", match->in[med], match->out[med]);
			return search_obj(ce, match->out[med], FALSE, localok);
		}
		if (low >= high)
			break;
		if (c < 0)
			high = (med == 0 ? 0 : med-1);
		else
			low = (med == match->num-1 ? match->num-1 : med+1);
	}
	return NULL;
}


static Elf32_Sym *
search_obj(cont_entry *ce, const char *name, bool usematch, bool localok)
{
	const char		*p;
	uint32			h, g, symind;
	uint32			nbucket;
	Elf32_Sym		*sym;
	Elf32_Sword		*htab, *bucket, *chain;
	
	if (!ce->symtab)
		return NULL;

	if (usematch && ce->match[PATCH_BEFORE]) {
		sym = search_match(ce, ce->match[PATCH_BEFORE], name, localok);
		if (sym)
			return sym;
	}

	p = name;
	h = 0;
	while (*p) {
		h = (h << 4) + *p++;
		g = h & 0xf0000000;
		if (g)
			h ^= g >> 24;
		h &= ~g;
	}
		
	htab = ce->hash;

	nbucket = htab[0];
	bucket = &htab[2];
	chain = &htab[2+nbucket];
	symind = bucket[h % nbucket];
	do {
		sym = &ce->symtab[symind];
		if (!strcmp(ce->strtab + sym->st_name, name) &&
			(sym->st_shndx != SHN_UNDEF) &&
			(localok || (ELF32_ST_BIND(sym->st_info) != STB_LOCAL)))
			return sym;
		symind = chain[symind];
	} while (symind != STN_UNDEF);

	/* not found. look up match table */

	if (usematch && ce->match[PATCH_AFTER]) {
		sym = search_match(ce, ce->match[PATCH_AFTER], name, localok);
		if (sym)
			return sym;
	}

	return NULL;
}

static int
relocate_cont(cont_entry *ce)
{
	int				err;
	Elf32_Rel		*rel;
	Elf32_Rela		*rela;
	
	if (ce->rel)
		for(rel=ce->rel; rel<ce->rel+ce->relsz/sizeof(Elf32_Rel); rel++) {
			err = process_relocation(ce, rel->r_offset, rel->r_info, *(uint32 *)(rel->r_offset + ce->delta));
			if (err)
				return ENOEXEC;
		}

	if (ce->rela)
		for(rela=ce->rela; rela<ce->rela+ce->relasz/sizeof(Elf32_Rela); rela++) {
			err = process_relocation(ce, rela->r_offset, rela->r_info, rela->r_addend);
			if (err)
				return ENOEXEC;
		}

	if (ce->jmprel) {
		if (ce->pltrel == DT_REL)
			for(rel=(Elf32_Rel*)ce->jmprel; rel<(Elf32_Rel*)ce->jmprel+ce->pltrelsz/sizeof(Elf32_Rel); rel++) {
				err = process_relocation(ce, rel->r_offset, rel->r_info, *(uint32 *)(rel->r_offset + ce->delta));
				if (err)
					return ENOEXEC;
			}
		else
			for(rela=(Elf32_Rela*)ce->jmprel; rela<(Elf32_Rela*)ce->jmprel+ce->pltrelsz/sizeof(Elf32_Rela); rela++) {
				err = process_relocation(ce, rela->r_offset, rela->r_info, rela->r_addend);
				if (err)
					return ENOEXEC;
			}
	}

	return 0;
}

static int
process_relocation(cont_entry *ce, char *addr, Elf32_Word info, Elf32_Sword addend)
{
	cont_entry	*ece;
	Elf32_Sym	*esym;
#if 0
static char *type_name[] = {
	"R_386_NONE",
	"R_386_32",
	"R_386_PC32",
	"R_386_GOT32",
	"R_386_PLT32",
	"R_386_COPY",
	"R_386_GLOB_DAT",
	"R_386_JMP_SLOT",
	"R_386_RELATIVE",
	"R_386_GOTOFF",
	"R_386_GOTPC"
};
#endif

	switch(ELF32_R_TYPE(info)) {

	case R_386_NONE:
		break;

	/* TESTED */
	case R_386_32:
		esym = search_sym(ce, ELF32_R_SYM(info), FALSE, &ece);
		if (!esym)
			return ENOEXEC;
		write_word((uint32 *)(addr + ce->delta), (uint32)(esym->st_value + ece->delta) + addend);
		break;

	/* TESTED */
	case R_386_PC32:
		esym = search_sym(ce, ELF32_R_SYM(info), FALSE, &ece);
		if (!esym)
			return ENOEXEC;
		write_word((uint32 *)(addr + ce->delta), (esym->st_value + ece->delta) + addend - (addr + ce->delta));
		break;

	case R_386_GOT32:
dprintf("GOT32!!! (addr = %p, sym = %ld)\n", addr+ce->delta, ELF32_R_SYM(info));
return ENOEXEC;
		break;

	case R_386_PLT32:
dprintf("PLT32!!! (addr = %p, sym = %ld)\n", addr+ce->delta, ELF32_R_SYM(info));
return ENOEXEC;
		break;

	/* TESTED */
	case R_386_COPY:
		esym = search_sym(ce, ELF32_R_SYM(info), TRUE, &ece);
		if (!esym)
			return ENOEXEC;
		memcpy(addr + ce->delta, esym->st_value + ece->delta, esym->st_size);
		break;

	/* TESTED */
	case R_386_GLOB_DAT:
		esym = search_sym(ce, ELF32_R_SYM(info), FALSE, &ece);
		if (!esym)
			return ENOEXEC;
		write_word((uint32 *)(addr + ce->delta), (uint32)(esym->st_value + ece->delta));
		break;

	/* TESTED */
	case R_386_JMP_SLOT:
		esym = search_sym(ce, ELF32_R_SYM(info), FALSE, &ece);
		if (!esym)
			return ENOEXEC;
		write_word((uint32 *)(addr + ce->delta), (uint32)(esym->st_value + ece->delta));
		break;

	/* TESTED */
	case R_386_RELATIVE:
		write_word((uint32 *)(addr + ce->delta), (uint32)(ce->delta + addend));
		break;

	case R_386_GOTOFF:
dprintf("GOTOFF!!! (addr = %p, sym = %ld)\n", addr+ce->delta, ELF32_R_SYM(info));
		esym = search_sym(ce, ELF32_R_SYM(info), FALSE, &ece);
		if (!esym)
			return ENOEXEC;
		write_word((uint32 *)(addr + ce->delta), (esym->st_value + ece->delta) + addend - (ce->got + ce->delta));
		break;

	case R_386_GOTPC:
dprintf("GOTPC!!! (addr = %p, sym = %ld)\n", addr+ce->delta, ELF32_R_SYM(info));
		write_word((uint32 *)(addr + ce->delta), (uint32)(ce->got + addend - addr));
		break;

	default:
		dprintf("unknown relocation type (%d)\n", ELF32_R_TYPE(info));
		return ENOEXEC;
	}
	return 0;
}

static uint32
get_cpu_mask(void)
{
	uint32		mask;
	cpuid_info	info0, info1;

	mask = 0;
	if (get_cpuid_simple(&info0, 0) == B_OK) {

		if (!strncmp(info0.eax_0.vendorid, "AuthenticAMD", 12)) {
			if (get_cpuid_simple(&info1, 0x8000000) == B_OK) {
				if (info1.regs.eax >= (uint32) 0x80000001) {
					if (get_cpuid_simple(&info1, 0x80000001) == B_OK) {
						if (info1.regs.edx & (1 << 31))
							mask |= CPU_ATTR_3DNOW;
					}
				}
			}
		}

		if (info0.eax_0.max_eax >= 1) {
			if (get_cpuid_simple(&info1, 1) == B_OK) {
				if (info1.eax_1.features & (1 << 25))
					mask |= CPU_ATTR_KNI;
				if (info1.eax_1.features & (1 << 23))
					mask |= CPU_ATTR_MMX;
				if (info1.eax_1.features & (1 << 15))
					mask |= CPU_ATTR_CMOV;
			}
		}

	}
	
	return mask;
}

static int32
count_mask_bits(uint32 mask)
{
	int			i;
	int32		cnt;

	cnt = 0;
	for(i=0; i<32; i++) {
		if (mask & 1)
			cnt++;
		mask >>= 1;
	}
	return cnt;
}

static status_t
cpu_feature_string_to_mask(const char *name, int32 len, uint32 *maskp)
{
	uint32		mask;
	int32		i;

	if ((len == strlen(DFLT_CPU_FEATURE_STRING)) && !strncmp(name, DFLT_CPU_FEATURE_STRING, len)) {
		*maskp = 0;
		return 0;
	}
	if ((len == strlen(NO_CPU_FEATURE_STRING)) && !strncmp(name, NO_CPU_FEATURE_STRING, len)) {
		*maskp = ~0;
		return 0;
	}

	mask = 0;
	for(i=0; i<len; i++) {
		switch(name[i]) {
		case 'M':
			mask |= CPU_ATTR_MMX;
			break;
		case 'C':
			mask |= CPU_ATTR_CMOV;
			break;
		case 'K':
			mask |= CPU_ATTR_KNI;
			break;
		case '3':
			mask |= CPU_ATTR_3DNOW;
			break;
		default:
			return EINVAL;
		}
	}
	*maskp = mask;
	return 0;
}


static int
make_match_table(int fd, match_table **matchp)
{
	match_table		*match[2];
	char			*p, *q, *r, *t, *u;
	char			*buf, *end;
	char			*s[2];
	char			*mend[2];
	int				nent[2];
	int				i, ent;
	int				delta;
	void			*tmp;
	int				err;
	struct stat		st;
	int				n;
	bool			duplicate;
	bool			pass;
	int				cnt;
	int				bestcnt[2];
	uint32			cpumask, mask;
	int				cur;

	if (fstat(fd, &st)) {
		err = ENOENT;
		goto error1;
	}
	buf = (char *) malloc(st.st_size);
	if (!buf) {
		err = ENOMEM;
		goto error1;
	}
	end = buf + st.st_size;
	lseek(fd, 0, 0);
	if (read(fd, buf, st.st_size) != st.st_size) {
		err = EIO;
		goto error2;
	}

	match[0] = match[1] = NULL;

	for(i=0; i<2; i++) {

		match[i] = (match_table *) malloc(sizeof(match_table));
		if (!match[i]) {
			err = ENOMEM;
			goto error3;
		}
		memset(match[i], 0, sizeof(match_table));
		
		match[i]->buf = (char *) malloc(MATCH_TABLE_BUF_INC);
		if (match[i]->buf == NULL) {
			err = ENOMEM;
			goto error3;
		}

		mend[i] = match[i]->buf + MATCH_TABLE_BUF_INC;

		match[i]->in = (char **) malloc(64 * sizeof(char *));
		if (match[i]->in == NULL) {
			err = ENOMEM;
			goto error3;
		}
		match[i]->out = (char **) malloc(64 * sizeof(char *));
		if (match[i]->out == NULL) {
			err = ENOMEM;
			goto error3;
		}
		
		match[i]->num = 0;
		nent[i] = 64;
		s[i] = match[i]->buf;
	}

	cpumask = get_cpu_mask();
	bestcnt[0] = bestcnt[1] = 0;
	pass = FALSE;
	cur = PATCH_AFTER;

	for(p=buf; p<end; p=q) {

		/* skip any non-printable char */
		while ((p < end) && !isgraph(*p))
			p++;

		/* go to the end of the line */
		q = p;
		while ((q < end) && (*q != '\n'))
			q++;

		/* does the line start with '#'? if so, skip it */
		if (*p == '#')
			continue;

		/* find first word */
		r = p;
		while ((r<q) && isgraph(*r))
			r++;

		/* is the first word empty? if so, skip line */
		if (r == p)
			continue;
			
		/* skip all non-printable chars */
		t = r;
		while ((t<q) && !isgraph(*t))
			t++;

		/* find second word */
		u = t;
		while ((u<q) && isgraph(*u))
			u++;

		/* is first word '%AFTER'? */
		if (!strncmp(p, PATCH_AFTER_SPECIFIER, min(strlen(PATCH_AFTER_SPECIFIER), r-p))) {
			cur = PATCH_AFTER;
			pass = FALSE;
			continue;
		}

		/* is first word '%BEFORE'? */
		if (!strncmp(p, PATCH_BEFORE_SPECIFIER, min(strlen(PATCH_BEFORE_SPECIFIER), r-p))) {
			cur = PATCH_BEFORE;
			pass = FALSE;
			continue;
		}

		/* is the second word empty? if so, this is an error */
		if (u == t) {
			err = EINVAL;
			goto error3;
		}

		/* is first word '%CPUFEATURE'? */
		if (!strncmp(p, CPU_FEATURE_SPECIFIER, min(strlen(CPU_FEATURE_SPECIFIER), r-p))) {
			err = cpu_feature_string_to_mask(t, u-t, &mask);
			if (err) {
				err = EINVAL;
				goto error3;
			}

			if (mask == ~0) {
				pass = FALSE;
				continue;
			}

			cnt = count_mask_bits(mask & cpumask);
			pass = TRUE;
			if (((mask == 0) && (bestcnt[cur] == 0)) || (cnt > bestcnt[cur])) {
				pass = FALSE;
				bestcnt[cur] = cnt;
			}
			continue;
		}

		if (pass)
			continue;

		/* can we create another entry in the in/out array? */
		if (match[cur]->num >= nent[cur]) {
			nent[cur] += 64;
			tmp = realloc(match[cur]->in, nent[cur] * sizeof(char *));
			if (tmp == NULL) {
				err = ENOMEM;
				goto error3;
			}
			match[cur]->in = (char **)tmp;
			tmp = realloc(match[cur]->out, nent[cur] * sizeof(char *));
			if (tmp == NULL) {
				err = ENOMEM;
				goto error3;
			}
			match[cur]->out = (char **)tmp;
		}

		/* is there enough space to copy word into buf? */
		while (r-p+1 + u-t+1 > mend[cur]-s[cur]) {
			tmp = realloc(match[cur]->buf, mend[cur] - match[cur]->buf + MATCH_TABLE_BUF_INC);
			if (tmp == NULL) {
				err = ENOMEM;
				goto error3;
			}
			delta = (char *)tmp - match[cur]->buf;
			match[cur]->buf = (char *)tmp;
			for(i=0; i<match[cur]->num; i++) {
				match[cur]->in[i] += delta;
				match[cur]->out[i] += delta;
			}
			s[cur] += delta;
			mend[cur] += delta + MATCH_TABLE_BUF_INC;
		}
		
		memcpy(s[cur], p, r-p);
		s[cur][r-p] = '\0';

		/* sort entries */
		duplicate = FALSE;
		for(ent=0; ent<match[cur]->num; ent++) {
			n = strcmp(s[cur], match[cur]->in[ent]);
			if (n == 0)
				duplicate = TRUE;
			if (n <= 0)
				break;
		}

		/* if this is a new entry, we need to make room for it and copy the first word */

		if (!duplicate) {
			memmove(&match[cur]->in[ent+1], &match[cur]->in[ent], (match[cur]->num - ent)*sizeof(char *));
			memmove(&match[cur]->out[ent+1], &match[cur]->out[ent], (match[cur]->num - ent)*sizeof(char *));
			match[cur]->in[ent] = s[cur];
			s[cur] += r-p+1;
			match[cur]->num++;
		}

		/* copy the second word */

		memcpy(s[cur], t, u-t);
		s[cur][u-t] = '\0';
		match[cur]->out[ent] = s[cur];
		s[cur] += u-t+1;

dprintf("  %s patched to %s\n", match[cur]->in[ent], match[cur]->out[ent]);
	}

for(i=0; i<2; i++)
dprintf("%d pair(s) found\n", match[i]->num);

	for(i=0; i<2; i++)
		if (match[i]->num == 0) {
			free(match[i]->buf);
			free(match[i]->in);
			free(match[i]->out);
			free(match[i]);
			match[i] = NULL;
		}

	free(buf);
	matchp[0] = match[0];
	matchp[1] = match[1];
	return 0;
	
error3:
	for(i=0; i<2; i++)
		if (match[i]) {
			if (match[i]->buf)
				free(match[i]->buf);
			if (match[i]->in)
				free(match[i]->in);
			if (match[i]->out)
				free(match[i]->out);
		}
error2:
	free(buf);
error1:
	return err;
}


#if _SUPPORTS_COMPRESSED_ELF
static cont_entry *
load_cont(const char *path, const char *real_kernel_name, const char *mount, char *dict, uint32 dictsize)
#else
static cont_entry *
load_cont(const char *path, const char *real_kernel_name, const char *mount)
#endif
{
	cont_entry		*ce;
	int				fd;
	const char		*p;
	char			*buf;
	char			**pimports;
	char			*libname;
	char			libpath[256];
	struct stat		st;
	int				i, j;
	struct {
		const char	*abstract;
		const char	*real;
	} cont_names[2];
	Elf32_Phdr		*pheader;
	Elf32_Sym		*gotsym;
	int				sz;
#if _SUPPORTS_COMPRESSED_ELF
	void			*cookie;
	status_t		err;
	bool			compressed;
#endif
	char			*fred;
	int				mfd;


	memset (cont_names, 0, sizeof (cont_names));
	cont_names[0].abstract = "_KERNEL_";
	cont_names[0].real = real_kernel_name;

	/* open file */

	ddprintf("loading %s (%s)\n", path, mount);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		goto error1;

	/* has it been already loaded? */

	if (fstat(fd, &st))
		goto error2;
	for(ce=first_ce; ce; ce=ce->next)
		if (ce->ino == st.st_ino) {
			ddprintf("already loaded\n");
			close(fd);
			return ce;
		}
	/* no: create cont_entry */

#if _SUPPORTS_COMPRESSED_ELF
	compressed = TRUE;
	err = open_celf_file(fd, &cookie, dict, dictsize);
	if (err != B_OK)
		compressed = FALSE;
#endif

	ce = (cont_entry *) malloc(sizeof(cont_entry));
	if (!ce)
		goto error3;
	memset(ce, 0, sizeof(cont_entry));

#if _SUPPORTS_COMPRESSED_ELF
	ce->compressed = compressed;
#endif
	ce->ino = st.st_ino;
	ce->next = first_ce;
	ce->prev = NULL;
	if (first_ce)
		first_ce->prev = ce;
	first_ce = ce;
	p = strrchr(path, '/');
	if (!p)
		p = path;
	else
		p++;
	strncpy(ce->path, p, sizeof(ce->path));
	ce->path[sizeof(ce->path)-1] = '\0';

	/* read in ELF header and do basic sanity check */

#if _SUPPORTS_COMPRESSED_ELF
	if (ce->compressed) {
		err = get_celf_elf_header(cookie, &ce->eheader);
		if (err != B_OK) {
			dprintf("cannot read elf header\n");
			goto error4;
		}
	} else {
		lseek(fd, 0, SEEK_SET);
		sz = read(fd, &ce->eheader, sizeof(Elf32_Ehdr));
		if (sz != sizeof(Elf32_Ehdr)) {
			dprintf("read %d bytes, which is < %ld\n", sz, sizeof(Elf32_Ehdr));
			goto error4;
		}
	}
#else
	lseek(fd, 0, SEEK_SET);
	sz = read(fd, &ce->eheader, sizeof(Elf32_Ehdr));
	if (sz != sizeof(Elf32_Ehdr)) {
		dprintf("read %d bytes, which is < %ld\n", sz, sizeof(Elf32_Ehdr));
		goto error4;
	}
#endif
	
	if ((ce->eheader.e_ident[EI_MAG0] != ELFMAG0) &&
		(ce->eheader.e_ident[EI_MAG1] != ELFMAG1) &&
		(ce->eheader.e_ident[EI_MAG2] != ELFMAG2) &&
		(ce->eheader.e_ident[EI_MAG3] != ELFMAG3) &&
		(ce->eheader.e_ident[EI_CLASS] != ELFCLASS32) &&
		(ce->eheader.e_ident[EI_DATA] != ELFDATA2LSB) &&
		(ce->eheader.e_ident[EI_CLASS] != ELFCLASS32) &&
		(ce->eheader.e_ident[EI_VERSION] != EV_CURRENT)) {
			dprintf("bad magic\n");
			goto error4;
	}

	if ((ce->eheader.e_type != ET_EXEC) && (ce->eheader.e_type != ET_DYN)) {
		dprintf("not executable\n");
		goto error4;
	}

	if (ce->eheader.e_machine != EM_386) {
		dprintf("not a 386 binary?\n");
		goto error4;
	}

	if (ce->eheader.e_version != EV_CURRENT) {
		dprintf("bad version\n");
		goto error4;
	}

	if (ce->eheader.e_phentsize != sizeof(Elf32_Phdr)) {
		dprintf("phentsize wrong\n");
		goto error4;
	}

	if (ce->eheader.e_shentsize != sizeof(Elf32_Shdr)) {
		dprintf("shentsize wrong\n");
		goto error4;
	}

	if (ce->eheader.e_ehsize != sizeof(Elf32_Ehdr)) {
		dprintf("header size wrong\n");
		goto error4;
	}

	if (ce->eheader.e_shnum > MAX_SECTIONS) {
		dprintf("too many section headers, %d\n", ce->eheader.e_shnum);
		goto error4;
	}

	if (ce->eheader.e_phnum > MAX_SEGMENTS) {
		dprintf("too many program headers, %d\n", ce->eheader.e_phnum);
		goto error4;
	}

	/* read in section and program headers */

#if _SUPPORTS_COMPRESSED_ELF
	if (ce->compressed) {
		err = get_celf_program_headers(cookie, &ce->pheaders[0]);
		if (err != B_OK) {
			dprintf("cannot read program headers\n");
			goto error4;
		}
		err = get_celf_section_headers(cookie, &ce->sheaders[0]);
		if (err != B_OK) {
			dprintf("cannot read section headers\n");
			goto error4;
		}
	} else {
		lseek(fd, ce->eheader.e_phoff, SEEK_SET);
		sz = read(fd, &ce->pheaders[0], sizeof(Elf32_Phdr)*ce->eheader.e_phnum);
		if (sz != sizeof(Elf32_Phdr)*ce->eheader.e_phnum) {
			dprintf("read %d bytes for ph, wanted %ld\n", sz, sizeof(Elf32_Phdr)*ce->eheader.e_phnum);
			goto error4;
		}
		lseek(fd, ce->eheader.e_shoff, SEEK_SET);
		sz = read(fd, &ce->sheaders[0], sizeof(Elf32_Shdr)*ce->eheader.e_shnum);
		if (sz != sizeof(Elf32_Shdr)*ce->eheader.e_shnum) {
			dprintf("read %d bytes, for sh, wanted %ld\n", sz, sizeof(Elf32_Shdr)*ce->eheader.e_shnum);
			goto error4;
		}
	}
#else
	lseek(fd, ce->eheader.e_phoff, SEEK_SET);
	sz = read(fd, &ce->pheaders[0], sizeof(Elf32_Phdr)*ce->eheader.e_phnum);
	if (sz != sizeof(Elf32_Phdr)*ce->eheader.e_phnum) {
		dprintf("read %d bytes for ph, wanted %ld\n", sz, sizeof(Elf32_Phdr)*ce->eheader.e_phnum);
		goto error4;
	}

	lseek(fd, ce->eheader.e_shoff, SEEK_SET);
	sz = read(fd, &ce->sheaders[0], sizeof(Elf32_Shdr)*ce->eheader.e_shnum);
	if (sz != sizeof(Elf32_Shdr)*ce->eheader.e_shnum) {
		dprintf("read %d bytes, for sh, wanted %ld\n", sz, sizeof(Elf32_Shdr)*ce->eheader.e_shnum);
		goto error4;
	}
#endif

	/* compute soname from path, not from DT_SONAME (see note below) */

	p = strrchr(ce->path, '/');
	if (!p)
		p = ce->path-1;
	ce->soname = p+1;

	/* determine base address, size and delta */

	ce->base_address = (char *)-1;
	ce->size = 0;

	for(i=0; i<ce->eheader.e_phnum; i++) {

		pheader = &ce->pheaders[i];
		if (pheader->p_type != PT_LOAD)
			continue;

		if (ce->base_address > RNDPAGEDWN(pheader->p_vaddr))
			ce->base_address = RNDPAGEDWN(pheader->p_vaddr);
		if (ce->base_address + ce->size < RNDPAGEUP(pheader->p_vaddr + pheader->p_memsz))
			ce->size = RNDPAGEUP(pheader->p_vaddr + pheader->p_memsz) - ce->base_address;
	}
			
	ce->delta = vbase - ce->base_address;
	vbase += ce->size;

	/* load in the segments */

	for(i=0; i<ce->eheader.e_phnum; i++) {

		pheader = &ce->pheaders[i];
		if (pheader->p_type != PT_LOAD)
			continue;

#if _SUPPORTS_COMPRESSED_ELF
		if (ce->compressed) {
			err = get_celf_segment(cookie, i, pheader->p_vaddr + ce->delta);
			if (err != B_OK) {
				dprintf("cannot read segment\n");
				goto error4;
			}
		} else {
			lseek(fd, pheader->p_offset, SEEK_SET);
			sz = read(fd, pheader->p_vaddr + ce->delta, pheader->p_filesz);
			if (sz != pheader->p_filesz) {
				dprintf("read too little for ph %d, sz %d, wanted %ld\n", i, sz,
					pheader->p_filesz);
				goto error4;
			}
		}
#else
		lseek(fd, pheader->p_offset, SEEK_SET);
		sz = read(fd, pheader->p_vaddr + ce->delta, pheader->p_filesz);
		if (sz != pheader->p_filesz) {
			dprintf("read too little for ph %d, sz %d, wanted %ld\n", i, sz,
				pheader->p_filesz);
			goto error4;
		}
#endif

		if (pheader->p_memsz > pheader->p_filesz)
			memset(pheader->p_vaddr+ce->delta+pheader->p_filesz, 0, pheader->p_memsz-pheader->p_filesz);
	}

	/* parse dynamic section */

	for(i=0; i<ce->eheader.e_phnum; i++)
		if (ce->pheaders[i].p_type == PT_DYNAMIC)
			break;
	if (i != ce->eheader.e_phnum) {
		ce->dynamic = (Elf32_Dyn *) xslate_addr(ce, ce->pheaders[i].p_vaddr);
		for(i=0; ce->dynamic[i].d_tag != DT_NULL; i++) {
			switch(ce->dynamic[i].d_tag) {

			case DT_NEEDED:
				if (ce->import_num >= MAX_IMPORTS) {
					dprintf("too many imports, %ld\n", ce->import_num);
					goto error4;
				}
				ce->import_names[ce->import_num] = ce->dynamic[i].d_un.d_val;
				ce->import_num++;
				break;

			case DT_PLTRELSZ:
				ce->pltrelsz = ce->dynamic[i].d_un.d_val;
				break;

			case DT_PLTGOT:
				ce->pltgot = (void *) xslate_addr(ce, ce->dynamic[i].d_un.d_ptr);
				break;

			case DT_HASH:
				ce->hash = (Elf32_Sword *) xslate_addr(ce, ce->dynamic[i].d_un.d_ptr);
				break;

			case DT_STRTAB:
				ce->strtab = (char *) xslate_addr(ce, ce->dynamic[i].d_un.d_ptr);
				break;

			case DT_SYMTAB:
				ce->symtab = (Elf32_Sym *) xslate_addr(ce, ce->dynamic[i].d_un.d_ptr);
				break;

			case DT_RELA:
				ce->rela = (Elf32_Rela *) xslate_addr(ce, ce->dynamic[i].d_un.d_ptr);
				break;

			case DT_RELASZ:
				ce->relasz = ce->dynamic[i].d_un.d_val;
				break;

			case DT_RELAENT:
				ce->relaent = ce->dynamic[i].d_un.d_val;
				break;

			case DT_STRSZ:
				ce->strsz = ce->dynamic[i].d_un.d_val;
				break;

			case DT_SYMENT:
				ce->syment = ce->dynamic[i].d_un.d_val;
				break;

			case DT_INIT:
				ce->init = ce->dynamic[i].d_un.d_ptr;
				break;

			case DT_FINI:
				ce->fini = ce->dynamic[i].d_un.d_ptr;
				break;

			case DT_SONAME:
				/* DT_SONAME is ignored. gld does not seem to produce them anyway. */
#if 0
				ce->soname = ce->strtab + ce->dynamic[i].d_un.d_val;
#endif
				break;

			case DT_RPATH:
				ce->rpath = ce->dynamic[i].d_un.d_val;
				break;

			case DT_SYMBOLIC:
				ce->symbolic = TRUE;
				break;

			case DT_REL:
				ce->rel = (Elf32_Rel *) xslate_addr(ce, ce->dynamic[i].d_un.d_ptr);
				break;

			case DT_RELSZ:
				ce->relsz = ce->dynamic[i].d_un.d_val;
				break;

			case DT_RELENT:
				ce->relent = ce->dynamic[i].d_un.d_val;
				break;

			case DT_PLTREL:
				ce->pltrel = ce->dynamic[i].d_un.d_val;
				break;

			case DT_DEBUG:
				break;

			case DT_TEXTREL:
				ce->textrel = TRUE;
				break;

			case DT_JMPREL:
				ce->jmprel = (void *) xslate_addr(ce, ce->dynamic[i].d_un.d_ptr);
				break;

			default:
				dprintf("unknown relocation type\n");
				goto error4;
			}	
		}
		gotsym = search_obj(ce, got_symbol_name, FALSE, TRUE);
		if (gotsym)
			ce->got = gotsym->st_value;
	}

	/* load patch file if any */

	for(i=0; i<2; i++)
		ce->match[i] = NULL;

	fred = (char *) malloc(strlen(path) + strlen(match_file_suffix) + 32 + 1);
	if (!fred)
		goto error3;
	sprintf(fred, "%s%s", path, match_file_suffix);
	mfd = open(fred, O_RDONLY);
	if (mfd >= 0) {
dprintf("opening (generic) match file for %s\n", ce->path);
		make_match_table(mfd, &ce->match[0]);
		close(mfd);
	}
	free(fred);


	/* fill in info structure */

	ce->info = nxinfo;
	buf = (char *) ce->info;
	memcpy(buf, &ce->eheader, sizeof(Elf32_Ehdr));
	buf += sizeof(Elf32_Ehdr);
	memcpy(buf, &ce->delta, sizeof(ce->delta));
	buf += sizeof(ce->delta);
	memcpy(buf, &ce->pheaders[0], ce->eheader.e_phnum * sizeof(Elf32_Phdr));
	buf += ce->eheader.e_phnum * sizeof(Elf32_Phdr);
	memcpy(buf, &ce->sheaders[0], ce->eheader.e_shnum * sizeof(Elf32_Shdr));
	buf += ce->eheader.e_shnum * sizeof(Elf32_Shdr);
	pimports = (char **)buf;
	buf += ce->import_num * sizeof(void *);
	nxinfo = buf;

	/*
	 * load the imported containers
	 */

	for (i=0; i<ce->import_num; i++) {
		libname = ce->strtab + ce->import_names[i];

		/* get rid of the directory, if specified */
		if (strrchr(libname, '/'))
			libname = strrchr(libname, '/')+1;

		for(j=0; cont_names[j].abstract; j++)
			if (!strcmp(libname, cont_names[i].abstract))
				break;
		if (!cont_names[j].abstract) {
			dprintf("LOADER: dependency %s unknown\n", libname);
			goto error4;
		}

		sprintf(libpath, "%s/beos/system/%s", mount, cont_names[j].real);
#if _SUPPORTS_COMPRESSED_ELF
		ce->imports[i] = load_cont(libpath, real_kernel_name, mount, dict, dictsize);
#else
		ce->imports[i] = load_cont(libpath, real_kernel_name, mount);
#endif
		if (ce->imports[i] == NULL)
			goto error4;
		pimports[i] = ce->imports[i]->info;
	}

#if _SUPPORTS_COMPRESSED_ELF
	if (ce->compressed)
		close_celf_file(cookie);
#endif

	close(fd);

	return ce;
	
error4:
	if (ce->prev)
		ce->prev->next = ce->next;
	else
		first_ce = ce->next;
	if (ce->next)
		ce->next->prev = ce->prev;
	free(ce);
error3:

#if _SUPPORTS_COMPRESSED_ELF
	if (compressed)
		close_celf_file(cookie);
#endif

error2:
	close(fd);
error1:
	ddprintf("load_cont(%s) error\n", path);
dprintf("error\n");
	return NULL;
}

/* ----------
	load_elf_kernel loads a the kernel and other system components
	(platform dependent library, boot driver, boot file system).
----- */
	
bool
load_elf_kernel(const char *mount, const char *kernel_name, 
	const char *fs, char *_vbase,
	int (**entry)(), void **kinfo, void **fsinfo,
	uint32 num_drivers, struct boot_filelist *drivers, 
	uint32 num_modules, struct boot_filelist *modules, 
	char **brkval)
{
	char		buf[256];
	uint32		i;
	cont_entry	*ce, *nce, *cent;
	cont_entry	**searchlist;
#if _SUPPORTS_COMPRESSED_ELF
	char		*tmp, *dict;
	uint32		dictsize;
	int			fd;
	struct stat	st;
#endif

	nxinfo = KERNEL_CONTAINER_INFO;
	vbase = _vbase;

	first_ce = NULL;
	kce = NULL;
	fce = NULL;

	/* try opening and loading the dictionary. */

#if _SUPPORTS_COMPRESSED_ELF
	dict = NULL;
	dictsize = 0;
	sprintf(buf, "%s/beos/system/dict", mount);
	fd = open(buf, O_RDONLY);
	if (fd >= 0) {
		if (!fstat(fd, &st)) {
			tmp = (char *) malloc(st.st_size);
			if (tmp) {
				if (read(fd, tmp, st.st_size) == st.st_size) {
					dict = tmp;
					dictsize = st.st_size;
				}
			}			
		}
		close(fd);
	}
#endif

	/* try loading using the current state-of-the-art path */

	sprintf(buf, "%s/beos/system/%s", mount, kernel_name);
#if _SUPPORTS_COMPRESSED_ELF
	kce = load_cont(buf, kernel_name, mount, dict, dictsize);
#else
	kce = load_cont(buf, kernel_name, mount);
#endif
	if (!kce)
		goto error1;

	sprintf(buf, "%s/beos/system/add-ons/kernel/file_systems/%s", mount, fs);
#if _SUPPORTS_COMPRESSED_ELF
	fce = load_cont(buf, kernel_name, mount, dict, dictsize);
#else
	fce = load_cont(buf, kernel_name, mount);
#endif
	if (!fce)
		goto error1;

dprintf("loading modules\n");
	/* now load all the modules */
	for (i=0;i<num_modules;i++) {
#if _SUPPORTS_COMPRESSED_ELF
		cent = load_cont(modules[i].path, kernel_name, mount, dict, dictsize);
#else
		cent = load_cont(modules[i].path, kernel_name, mount);
#endif
		modules[i].cookie = cent ? (uint32)(cent->info) : 0;
	}

	/* now load all the drivers */
	for (i=0;i<num_drivers;i++) {
#if _SUPPORTS_COMPRESSED_ELF
		cent = load_cont(drivers[i].path, kernel_name, mount, dict, dictsize);
#else
		cent = load_cont(drivers[i].path, kernel_name, mount);
#endif
		drivers[i].cookie = cent ? (uint32)(cent->info) : 0;
	}

	searchlist = build_search_list(fce);
	if (!searchlist)
		goto error1;

	for(ce=first_ce; ce; ce=ce->next)
		ce->searchlist = searchlist;

	for(ce=first_ce; ce; ce=ce->next)
		if (relocate_cont(ce)) {
			if ((ce == kce) || (ce == fce))
				goto error2;
			/* cope with a bad driver or module */
			for (i=0;i<num_modules;i++)
				if ((uint32)ce->info == modules[i].cookie)
					modules[i].cookie = 0;
			for (i=0;i<num_drivers;i++)
				if ((uint32)ce->info == drivers[i].cookie)
					drivers[i].cookie = 0;
		}

	ddprintf("START(%p)\n", kce->eheader.e_entry + kce->delta);
	*entry = (int (*)()) (kce->eheader.e_entry + kce->delta);

	ddprintf("adding kinfo = %p, fsinfo = %p\n", kce->info, fce->info);
	*kinfo = kce->info;
	*fsinfo = fce->info;

	ddprintf("brkval = %p\n", vbase);
	*brkval = vbase;

#if _SUPPORTS_COMPRESSED_ELF
	if (dict)
		free(dict);
#endif

	/*
	don't need to fill the segs array: this was used by old
	kernels (pre dr9) which we can simply ignore on Intel
	*/

	return TRUE;

error2:
	free(searchlist);
error1:
	for(ce=first_ce; ce; ce=nce) {
		nce = ce->next;
		free(ce);
	}
	ddprintf("load_elf_kernel(): error\n");
	return FALSE;
}
