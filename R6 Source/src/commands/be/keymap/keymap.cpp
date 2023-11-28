/*
 * Copyright (C) 1996 by Be, Inc., All Rights Reserved
 *
 * keymap: a program for remapping your keyboard
 *
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <InterfaceDefs.h>
#include <Window.h>
#include <Application.h>
#include <shared_fonts.h>
#include <FindDirectory.h>

/*
 * Maximum size of a keymap file (so malloc won't blow up)
 * Actually, this size is slightly bigger, but malloc still won't blow up.
 */
#define MAXSIZE (sizeof(key_map) + sizeof(int32) + sizeof(key_map) * (256/4))

#define BUFSIZE 512	/* we don't expect lines to be longer than this */
#define DEADTABSIZE 32 /* size of dead key table */
#define SPACING 8
#define FORCELIT 0

/*
 * Internal key table of pascal strings we store
 * Unlike the kit version, this one stores the size of the table
 */
typedef struct internal_keytab {
	unsigned size;
	char *storage;
} internal_keytab;

/*
 * For multi-byte mappings, you can be either in hex mode (started with 0x)
 * or literal mode (started with "'"
 */
typedef enum base_mode {
	NOMODE = 0,
	HEXMODE,
	LITMODE
} base_mode_t;	
	
/*
 * Data is saved into TMPNAME, and then renamed to MAPNAME.  Hopefully,
 * the rename is atomic, so we are never in an inconsistent state
 */
char TMPNAME[PATH_MAX];
char MAPNAME[PATH_MAX];
char MAPDIR[PATH_MAX];

/*
 * A helful comment to aid people when remapping
 */
const char KEY_NUMBERING[] = 
"#\n"
"#	Raw key numbering for 101 keyboard...\n"
"#                                                                                        [sys]       [brk]\n"
"#                                                                                         0x7e        0x7f\n"
"# [esc]       [ f1] [ f2] [ f3] [ f4] [ f5] [ f6] [ f7] [ f8] [ f9] [f10] [f11] [f12]    [prn] [scr] [pau]\n"
"#  0x01        0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09  0x0a  0x0b  0x0c  0x0d     0x0e  0x0f  0x10     K E Y P A D   K E Y S\n"
"#\n"
"# [ ` ] [ 1 ] [ 2 ] [ 3 ] [ 4 ] [ 5 ] [ 6 ] [ 7 ] [ 8 ] [ 9 ] [ 0 ] [ - ] [ = ] [bck]    [ins] [hme] [pup]    [num] [ / ] [ * ] [ - ]\n"
"#  0x11  0x12  0x13  0x14  0x15  0x16  0x17  0x18  0x19  0x1a  0x1b  0x1c  0x1d  0x1e     0x1f  0x20  0x21     0x22  0x23  0x24  0x25\n"
"#\n"
"# [tab] [ q ] [ w ] [ e ] [ r ] [ t ] [ y ] [ u ] [ i ] [ o ] [ p ] [ [ ] [ ] ] [ \\ ]    [del] [end] [pdn]    [ 7 ] [ 8 ] [ 9 ] [ + ]\n"
"#  0x26  0x27  0x28  0x29  0x2a  0x2b  0x2c  0x2d  0x2e  0x2f  0x30  0x31  0x32  0x33     0x34  0x35  0x36     0x37  0x38  0x39  0x3a\n"
"#\n"
"# [cap] [ a ] [ s ] [ d ] [ f ] [ g ] [ h ] [ j ] [ k ] [ l ] [ ; ] [ ' ] [  enter  ]                         [ 4 ] [ 5 ] [ 6 ]\n"
"#  0x3b  0x3c  0x3d  0x3e  0x3f  0x40  0x41  0x42  0x43  0x44  0x45  0x46     0x47                             0x48  0x49  0x4a\n"
"#\n"
"# [shift]     [ z ] [ x ] [ c ] [ v ] [ b ] [ n ] [ m ] [ , ] [ . ] [ / ]     [shift]          [ up]          [ 1 ] [ 2 ] [ 3 ] [ent]\n"
"#   0x4b       0x4c  0x4d  0x4e  0x4f  0x50  0x51  0x52  0x53  0x54  0x55       0x56            0x57           0x58  0x59  0x5a  0x5b\n"
"#\n"
"# [ctr]             [cmd]             [  space  ]             [cmd]             [ctr]    [lft] [dwn] [rgt]    [ 0 ] [ . ]\n"
"#  0x5c              0x5d                 0x5e                 0x5f              0x60     0x61  0x62  0x63     0x64  0x65\n"
"#\n"
"#	NOTE: On a Microsoft Natural Keyboard:\n"
"#			left option  = 0x66\n"
"#			right option = 0x67\n"
"#			menu key     = 0x68\n"
"#	NOTE: On an Apple Extended Keyboard:\n"
"#			left option  = 0x66\n"
"#			right option = 0x67\n"
"#			keypad '='   = 0x6a\n"
"#			power key    = 0x6b\n"
;


void
print_tab(
		  const char *name,
		  long val
		  )
{
	printf("%s = ", name);
	
	if (val & B_NORMAL_TABLE) {
		printf("Normal ");
	}
	if (val & B_SHIFT_TABLE) {
		printf("Shift ");
	}
	if (val & B_CONTROL_TABLE) {
		printf("Control ");
	}
	if (val & B_OPTION_TABLE) {
		printf("Option ");
	}
	if (val & B_OPTION_SHIFT_TABLE) {
		printf("Option-Shift ");
	}
	if (val & B_CAPS_TABLE) {
		printf("CapsLock ");
	}
	if (val & B_CAPS_SHIFT_TABLE) {
		printf("CapsLock-Shift ");
	}
	if (val & B_OPTION_CAPS_TABLE) {
		printf("CapsLock-Option ");
	}
	if (val & B_OPTION_CAPS_SHIFT_TABLE) {
		printf("CapsLock-Option-Shift ");
	}
	printf("\n");
}

int
printchar(uchar c, bool literal, bool first, bool last)
{
	char buf[12];
	int len = 0;

	if (literal) {
		if (first) {
			printf("'");
			len++;
		}
		if (c == '\\' || c == '\'') {
			printf("\\"); /* escape */
			len++;
		}
		printf("%c", c);
		len++;
		if (last) {
			printf("'");
			len++;
		}
	} else {
		if (first) {
			printf("0x");
			len += 2;
		}
		sprintf(buf, "%02x", c);
		printf(buf);
		len += 2;
	}
	return (len);
}

void 
printkey(
		 uint32 index,
		 char *keytab
		 )
{
	uchar *p = (uchar *)&keytab[index];
	int len = *p++;
	int i;
	char buf[1024];
	int size = 0;
	bool literal;

	if (len == 0) {
		printf("''");
		size = 2;
	} else {
		literal = isprint(p[0]);
		for (i = 0; i < len - 1; i++) {
			size += printchar(*p++, literal, i == 0, false);
		}
		size += printchar(*p, literal, i == 0, true);
	}
	while (size++ < SPACING) {
		printf(" ");
	}
	printf(" ");
}

/*
 * Print a dead key mapping
 */
void
print_dead_key(
			   const char *name,
			   int32 *tab,
			   char *keytab
			   )
{
	int i;

	for (i = 0; i < DEADTABSIZE; i += 2) {
		if (tab[i]) {
			printf("%s ", name);
			printkey(tab[i], keytab);
			printf(" = ");
			printkey(tab[i + 1], keytab);
			printf("\n");
		}
	}
}

void
print_lock_settings(
					unsigned long lock_settings
					)
{
	printf("#\n");
	printf("# Lock settings\n");
	printf("# To set NumLock, do the following:\n");
	printf("#   LockSettings = NumLock\n");
	printf("#\n");
	printf("# To set everything, do the following:\n");
	printf("#   LockSettings = CapsLock NumLock ScrollLock\n");
	printf("#\n");
	printf("LockSettings = ");
	if (lock_settings & B_CAPS_LOCK) {
		printf("CapsLock ");
	}
	if (lock_settings & B_NUM_LOCK) {
		printf("NumLock ");
	}
	if (lock_settings & B_SCROLL_LOCK) {
		printf("ScrollLock");
	}
	printf("\n");
}

/*
 * Will work with any compiler
 */
static const union { long l; char b[4]; } ENDIAN_TEST = { 1 };
#undef LITTLE_ENDIAN
#define LITTLE_ENDIAN   ENDIAN_TEST.b[0]

static inline ushort SWAPSHORT(ushort x)
{
	return ((x << 8) | (x >> 8));
}

static inline ulong SWAPLONG(ulong x)
{
	return ((SWAPSHORT(x) << 16) | (SWAPSHORT(x >> 16)));
}

#define swap32(x)  (LITTLE_ENDIAN ? SWAPLONG(x) : (x))

/*
 * Swap to big-endian, if necessary
 */
void
swapmap(struct key_map *map)
{
	int i;

#define DOIT(x)  map->x = swap32(map->x)

	DOIT(version);
	DOIT(caps_key);
	DOIT(scroll_key);
	DOIT(num_key);
	DOIT(left_shift_key);
	DOIT(right_shift_key);
	DOIT(left_command_key);
	DOIT(right_command_key);
	DOIT(left_control_key);
	DOIT(right_control_key);
	DOIT(left_option_key);
	DOIT(right_option_key);
	DOIT(menu_key);
	DOIT(lock_settings);

	for (i = 0; i < 128; i++) {
		DOIT(control_map[i]);
		DOIT(option_caps_shift_map[i]);
		DOIT(option_caps_map[i]);
		DOIT(option_shift_map[i]);
		DOIT(option_map[i]);
		DOIT(caps_shift_map[i]);
		DOIT(caps_map[i]);
		DOIT(shift_map[i]);
		DOIT(normal_map[i]);
	}
	for (i = 0; i < 32; i++) {
		DOIT(acute_dead_key[i]);
		DOIT(grave_dead_key[i]);
		DOIT(circumflex_dead_key[i]);
		DOIT(dieresis_dead_key[i]);
		DOIT(tilde_dead_key[i]);
	}
	DOIT(acute_tables);
	DOIT(grave_tables);
	DOIT(circumflex_tables);
	DOIT(dieresis_tables);
	DOIT(tilde_tables);
#undef DOIT
}

/*
 * Read binary key map file
 */
int
read_from_file(
			   const char *filename,
			   struct key_map **mapp,
			   char **keytab
			   )
{
	FILE *f;
	int size;
	int len;

	f = fopen(filename, "r");
	if (f == NULL) {
		perror(filename);
		return (0);
	}
	*mapp = (struct key_map *)malloc(sizeof(**mapp));
	if ((len = fread((char *)*mapp, 1, sizeof(**mapp), f))
		!= sizeof(**mapp)) {
		goto corrupt;
	}
	swapmap(*mapp);
	if ((len = fread((char *)&size, 1, sizeof(size), f)) != sizeof(size)) {
		printf("read %d/%d bytes\n", len, sizeof(size));
		goto corrupt;
	}
	size = swap32(size);
	if (size > MAXSIZE) {
		goto corrupt;
	}
	*keytab = (char *)malloc(size);
	if ((len = fread(*keytab, 1, size, f)) != size) {
		goto corrupt;
	}
	fclose(f);
	return (1);
corrupt:
	fclose(f);
	fprintf(stderr, "file [%s] is corrupt\n", filename);
	return (0);
}



/*
 * Print out the entire key map, along with helpful comments
 */
int
dumpkeys(const char *filename)
{
	int i;
	struct key_map *map = NULL;
	char *keytab = NULL;

	if (filename == NULL) {
		get_key_map(&map, &keytab);
	} else {
		if (!read_from_file(filename, &map, &keytab)) {
			fprintf(stderr, "problem reading file %s\n", filename);
			return (1);
		}
	}
	printf("#!/bin/keymap -l\n");
	printf("%s", KEY_NUMBERING);
	printf("Version = %d\n", map->version);
	printf("CapsLock = 0x%02x\n", map->caps_key);
	printf("ScrollLock = 0x%02x\n", map->scroll_key);
	printf("NumLock = 0x%02x\n", map->num_key);
	printf("LShift = 0x%02x\n", map->left_shift_key);
	printf("RShift = 0x%02x\n", map->right_shift_key);
	printf("LCommand = 0x%02x\n", map->left_command_key);
	printf("RCommand = 0x%02x\n", map->right_command_key);
	printf("LControl = 0x%02x\n", map->left_control_key);
	printf("RControl = 0x%02x\n", map->right_control_key);
	printf("LOption = 0x%02x\n", map->left_option_key);
	printf("ROption = 0x%02x\n", map->right_option_key);
	printf("Menu = 0x%02x\n", map->menu_key);
	print_lock_settings(map->lock_settings);
	printf("# Legend:\n");
	printf("#   n = Normal\n");
	printf("#   s = Shift\n");
	printf("#   c = Control\n");
	printf("#   C = CapsLock\n");
	printf("#   o = Option\n");
	printf("# Key      %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s\n",
		   "n", "s", "c", "o", "os", "C", "Cs", "Co", "Cos");
	for (i = 0; i < 128; i++) {
		printf("Key 0x%02x = ", i);
		printkey(map->normal_map[i], keytab);
		printkey(map->shift_map[i], keytab);
		printkey(map->control_map[i], keytab);
		printkey(map->option_map[i], keytab);
		printkey(map->option_shift_map[i], keytab);
		printkey(map->caps_map[i], keytab);
		printkey(map->caps_shift_map[i], keytab);
		printkey(map->option_caps_map[i], keytab);
		printkey(map->option_caps_shift_map[i], keytab);
		printf("\n");
	}
	print_dead_key("Acute", map->acute_dead_key, keytab);
	print_tab("AcuteTab", map->acute_tables);
	print_dead_key("Grave", map->grave_dead_key, keytab);
	print_tab("GraveTab", map->grave_tables);
	print_dead_key("Circumflex", map->circumflex_dead_key, keytab);
	print_tab("CircumflexTab", map->circumflex_tables);
	print_dead_key("Diaeresis", map->dieresis_dead_key, keytab);
	print_tab("DiaeresisTab", map->dieresis_tables);
	print_dead_key("Tilde", map->tilde_dead_key, keytab);
	print_tab("TildeTab", map->tilde_tables);

	free(map);
	free(keytab);

	return (0);
}


int
isterm(char c)
{
	return (c == 0 || c == '#');
}

/*
 * Return the first character that is NOT a space character
 */
const char *
skipspace(
		  const char *str
		  )
{
	while (*str == ' ' || *str == '\t') {
		str++;
	}
	return (str);
}

/*
 * Return the first character that is either a space or terminating character
 */
const char *
skipnotspace(
			 const char *str
			 )
{
	while (!isterm(*str) && (*str != ' ' && *str != '\t')) {
		str++;
	}
	return (str);
}

/*
 * Read a variable name from the file
 * Returns 0 on end-of-file, 1 otherwise
 * If no variable name is found in the line, then
 * return 1 still, but return an empty variable name (for line
 * counting purposes).
 */
int
getvar(
	   FILE *f,
	   char *var,	/* variable name */
	   char *line,	/* rest of line after buffer */
	   char *buf	/* entire line read, for diagnostics */
	   
	   )
{
	int len;
	const char *p;
	const char *s;

	if (!fgets(buf, BUFSIZE, f)) {
		return (0);
	}
	/*
	 * Remove trailing newline, if any
	 */
	len = strlen(buf);
	while (len > 0 && buf[len - 1] == '\n') {
		len--;
	}
	buf[len] = 0;

	/*
	 * Remove leading spaces, if any
	 */
	p = skipspace(buf);
	if (isterm(*p)) {
		*var = 0;	/* return empty var */
		strcpy(line, buf);
		return (1);
	}
	if (!isalpha(*p)) {
		fprintf(stderr, "Can't parse: %s\n", buf);
		*var = 0;
		strcpy(line, buf);
		return (1);
	}
	for (s = p + 1; isalpha(*s); s++) {
	}
	strncpy(var, p, s - p);
	var[s - p] = 0;
	strcpy(line, s);
	return (1);
}

/*
 * Compare two strings for string equality, ignoring case
 */
int
strcaseeq(const char *str1, const char *str2)
{
	return (strcasecmp(str1, str2) == 0);
}

/*
 * Convert text into an integer.
 * Can be octal, hex or decimal
 */
typedef const char *const_string;

#define isodigit(x) ((x) >= '0' && (x) <= '8')

/*
 * If in literal or hex mode, convert another character
 */
int
modeconvert(const char *str, const_string *where, base_mode_t *mode)
{
	char hex[3];
	int val;
	int i;

	switch (*mode) {
	case HEXMODE:
		/* hex */
		hex[0] = str[0];
		if (isxdigit(str[1])) {
			hex[1] = str[1];
			hex[2] = 0;
			i = 2;
		} else {
			hex[1] = 0;
			i = 1;
		}
		sscanf(hex, "%x", &val);
		*where = str + i;
		break;
	case LITMODE:
		if (str[0] == '\\') {
			str++; /* unescape */
		}
		val = str[0];
		if (str[1] == '\'') {
			*where = &str[2];
			*mode = NOMODE;
		} else {
			*where = &str[1];
		}
		break;
	}
	return (val);
}

/*
 * Convert a character, and if hex or literal mode is detected, indicate that
 */
int
iconvert(const char *str, const_string *where, base_mode_t *mode = NULL)
{
	int val;
	int i;
	char hex[3];

	if (mode && *mode != NOMODE) {
		return (modeconvert(str, where, mode));
	}
	if (*str == '0') {
		if (str[1] == 'x' || str[1] == 'X') {
			/* hex */
			if (!isxdigit(str[2])) {
				*where = &str[3];
				return (-1);
			}
			hex[0] = str[2];
			if (isxdigit(str[3])) {
				hex[1] = str[3];
				hex[2] = 0;
				i = 4;
			} else {
				hex[1] = 0;
				i = 3;
			}
			sscanf(hex, "%x", &val);
			if (mode && *mode == NOMODE) {
				*mode = HEXMODE;
			}
		} else {
			/* octal */
			sscanf(&str[0], "%o", &val);
			i = 1;
			while (isodigit(str[i])) {
				i++;
			}
		}
		*where = &str[i];
	} else {
		if (*str == '\'') {
			if (str[1] == '\'') {
				*where = &str[2];
				/* empty */
				return (-1);
			}
			/* literal */
			if (str[1] == '\\') {
				str++; /* unescape */
			}
			val = str[1];
			if (str[2] == '\'') {
				*where = str + 3;
			} else {
				*where = str + 2;
				if (mode && *mode == NOMODE) {
					*mode = LITMODE;
				}
			}
		} else {
			/* decimal */
			sscanf(str, "%d", &val);
			i = 1;
			while (isdigit(str[i])) {
				i++;
			}
			*where = &str[i];
		}
	}
	return (val);
}

/*
 * Convert a series of characters
 */
int
sconvert(const char *str, const_string *where, char *outstr)
{
	int i;
	base_mode_t mode = NOMODE;
	int val;

	for (i = 0; i < 255; i++) {
		val = iconvert(str, where, &mode);
		if (val == -1) {
			/* empty string */
			outstr[0] = 0;
			return (1);
		}
		outstr[i + 1] = val;
		if (*where == str) {
			return (0);
		}
		if (**where == ':') {
			str = *where + 1;
		} else if (**where == '\'') {
			str = *where + 1;
			break;
		} else {
			str = *where;
			if (mode != LITMODE && (**where == ' ' || **where == '\t')) {
				break;
			}
		}
	}
	outstr[0] = i + 1;
	return (1);
}

ulong
parse_lock(const char *mod)
{
	ulong		res;
	
	res = 0;
	if (strstr(mod, "CapsLock") != NULL)
		res |= B_CAPS_LOCK;
	if (strstr(mod, "NumLock") != NULL)
		res |= B_NUM_LOCK;
	if (strstr(mod, "ScrollLock") != NULL)
		res |= B_SCROLL_LOCK;
	return res;
}

/*
 * Parse the lock settings
 * Returns 1 on error, 0 on success
 */
int
parse_lock_settings(
					ulong *lock,
					char *line
					)
{
	const char *p;

	p = skipspace(line);
	if (isterm(*p)) {
		return (1);
	}
	if (*p != '=') {
		fprintf(stderr, "no equal sign found\n");
		return (1);
	}
	*lock  = parse_lock(p);
	return(0);
}

void
putmap(
	   int32 *map,
	   unsigned char key,
	   ulong val
	   )
{
	map[key] = val;
}

/*
 * Add a key uniquely to the table. Return the index.
 */
int
addkey_unique(
			  char *str,
			  internal_keytab *keytab
			  )
{
	int i;
	int len;
	int spot;

	for (i = 0; i < keytab->size; i += (len + 1)) {
		len = (uchar)str[0];
		if (len == keytab->storage[i] &&
			memcmp(&str[1], &keytab->storage[i + 1], len) == 0) {
			return (i);
		}
	}
	len = (uchar)str[0];
	spot = keytab->size;
	keytab->size += len + 1;
	keytab->storage = (char *)realloc(keytab->storage, keytab->size);
	memcpy(keytab->storage + spot, str, len + 1);
	return (spot);
}

/*
 * Parse a key
 * Returns 1 on error, 0 on success
 */
int
parse_key(struct key_map *kmap, char *line, internal_keytab *keytab)
{
	const char *p;
	ulong key;
	int i;
	ulong val[9];
	
	/*
	 * Remove leading spaces, if any
	 */
	p = skipspace(line);
	if (*p == 0) {
		fprintf(stderr, "premature end of line: %s\n", line);
		return (1);
	}
	key = iconvert(p, &p);
	if (isterm(*p)) {
		return (1);
	}
	p = skipspace(p);
	if (*p != '=') {
		fprintf(stderr, "no equal sign found: %s\n", line);
		return (1);
	}
	p = skipspace(p + 1);
	if (isterm(*p)) {
		return (1);
	}
	for (i = 0; i < 9; i++) {
		char str[256];

		if (!sconvert(p, &p, str)) {
			return (1);
		}
		val[i] = addkey_unique(str, keytab);
		if (i == 8) {
			break;
		}
		if (isterm(*p)) {
			return (1);
		}
		p = skipspace(p);
		if (isterm(*p)) {
			return (1);
		}
	}
	putmap(kmap->normal_map, key, val[0]);
	putmap(kmap->shift_map, key, val[1]);
	putmap(kmap->control_map, key, val[2]);
	putmap(kmap->option_map, key, val[3]);
	putmap(kmap->option_shift_map, key, val[4]);
	putmap(kmap->caps_map, key, val[5]);
	putmap(kmap->caps_shift_map, key, val[6]);
	putmap(kmap->option_caps_map, key, val[7]);
	putmap(kmap->option_caps_shift_map, key, val[8]);
	return (0);
}

ulong
parse_modifier(const char *mod)
{
	if (strcaseeq(mod, "Normal")) {
		return (B_NORMAL_TABLE);
	}
	if (strcaseeq(mod, "Shift")) {
		return (B_SHIFT_TABLE);
	}
	if (strcaseeq(mod, "Control")) {
		return (B_CONTROL_TABLE);
	}
	if (strcaseeq(mod, "Option")) {
		return (B_OPTION_TABLE);
	}
	if (strcaseeq(mod, "Option-Shift") || strcaseeq(mod, "Shift-Option")) {
		return (B_OPTION_SHIFT_TABLE);
	}
	if (strcaseeq(mod, "CapsLock")) {
		return (B_CAPS_TABLE);
	}
	if (strcaseeq(mod, "CapsLock-Shift") || strcaseeq(mod, "Shift-CapsLock")) {
		return (B_CAPS_SHIFT_TABLE);
	}
	if (strcaseeq(mod, "CapsLock-Option") ||
		strcaseeq(mod, "Option-CapsLock")) {
		return (B_OPTION_CAPS_TABLE);
	}
	if (strcaseeq(mod, "CapsLock-Option-Shift") ||
		strcaseeq(mod, "Option-CapsLock-Shift") ||
		strcaseeq(mod, "Shift-CapsLock-Option") ||
		strcaseeq(mod, "CapsLock-Shift-Option") ||
		strcaseeq(mod, "Shift-Option-CapsLock") ||
		strcaseeq(mod, "Option-Shift-CapsLock")) {
		
		return (B_OPTION_CAPS_SHIFT_TABLE);
	}
	return (0);
}

/*
 * Parse a dead key modifier table
 * Returns 1 on error, 0 on success
 */
int
parse_tab(
		  ulong *tab,
		  const char *line
		  )
{
	const char *p;
	const char *s;
	char mod[BUFSIZE];

	p = skipspace(line);
	if (isterm(*p)) {
		return (1);
	}
	if (*p != '=') {
		return (1);
	}
	*tab = 0;
	for (;;) {
		p = skipspace(p + 1);
		if (isterm(*p)) {
			break;
		}
		s = skipnotspace(p);
		strncpy(mod, p, s - p);
		mod[s - p] = 0;
		*tab |= parse_modifier(mod);
		p = s;
	}
	return (0);
}

/*
 * Parse a dead key mapping
 * Returns 1 on error, 0 on success
 */
int
parse_dead(
		   int32 deadtab[DEADTABSIZE],
		   int deadnum,
		   char *line,
		   internal_keytab *keytab
		   )
{
	const char *p;
	ulong key;
	char str[256];

	p = skipspace(line);
	if (*p == 0) {
		fprintf(stderr, "premature end of line: %s\n", line);
		return (1);
	}
	if (!sconvert(p, &p, str)) {
		return (1);
	}
	if (isterm(*p)) {
		return (1);
	}
	p = skipspace(p);
	if (*p != '=') {
		fprintf(stderr, "no equal sign found: %s\n", line);
		return (1);
	}
		
	p = skipspace(p + 1);
	if (isterm(*p)) {
		return (1);
	}
	if (deadnum >= (DEADTABSIZE / 2)) {
		return (1);
	}
	if (deadnum == 0) {
		memset(deadtab, 0, sizeof(deadtab[0]) * DEADTABSIZE);
	}
	str[0] = 1;
	deadtab[2 * deadnum] = addkey_unique(str, keytab);
	if (!sconvert(p, &p, str)) {
		return (1);
	}
	deadtab[2 * deadnum + 1] = addkey_unique(str, keytab);
	return (0);
}

/*
 * Parse a simple variable
 * Returns 1 on error, 0 on success
 */
int
parse_var(
		  ulong *var,
		  char *line
		  )
{
	const char *p;

	p = skipspace(line);
	if (*p != '=') {
		fprintf(stderr, "equal sign not found: %s\n", line);
		return (1);
	}
	p = skipspace(p + 1);
	if (isterm(*p)) {
		return (1);
	}
	*var = iconvert(p, &p);
	return (0);
}

void
addkey(
	   int32 *indexp,
	   char *xkeytab,
	   internal_keytab *keytab
	   )
{
	*indexp = addkey_unique(&xkeytab[*indexp], keytab);
}



void
add_dead_key(
			 int32 *tab,
			 char *xkeytab,
			 internal_keytab *keytab
			 )
{
	int i;

	for (i = 0; i < DEADTABSIZE; i += 2) {
		if (tab[i]) {
			addkey(&tab[i], xkeytab, keytab);
			addkey(&tab[i + 1], xkeytab, keytab);
		}
	}
}


/*
 * Convert keytab from kit version to our version which can realloc
 * its storage and knows the size
 */
void
internalize_keytab(
				   struct key_map *map,
				   char *xkeytab,
				   internal_keytab *keytab
				   )
{
	int i;

	keytab->size = 0;
	keytab->storage = NULL;
	i = map->normal_map[0];
		   
	for (i = 0; i < 128; i++) {
		addkey(&map->normal_map[i], xkeytab, keytab);
		addkey(&map->shift_map[i], xkeytab, keytab);
		addkey(&map->control_map[i], xkeytab, keytab);
		addkey(&map->option_map[i], xkeytab, keytab);
		addkey(&map->option_shift_map[i], xkeytab, keytab);
		addkey(&map->caps_map[i], xkeytab, keytab);
		addkey(&map->caps_shift_map[i], xkeytab, keytab);
		addkey(&map->option_caps_map[i], xkeytab, keytab);
		addkey(&map->option_caps_shift_map[i], xkeytab, keytab);
	
	}
	add_dead_key(map->acute_dead_key, xkeytab, keytab);
	add_dead_key(map->grave_dead_key, xkeytab, keytab);
	add_dead_key(map->circumflex_dead_key, xkeytab, keytab);
	add_dead_key(map->dieresis_dead_key, xkeytab, keytab);
	add_dead_key(map->tilde_dead_key, xkeytab, keytab);
}


/*
 * Parse the file and update the key mapping
 * Returns number of errors found in file, so 0 means success
 * The key mapping is initialized to the current key map, not zero.
 */
int
parse(FILE *f, struct key_map **kmapp, internal_keytab *keytab)
{
	char orig[BUFSIZE];
	char line[BUFSIZE];
	char var[BUFSIZE];
	int acute = 0;
	int grave = 0;
	int circumflex = 0;
	int diaeresis = 0;
	int tilde = 0;
	int errors = 0;
	int total_errors = 0;
	int lineno = 0;
	int tabsize;
	char *xkeytab;
	struct key_map *kmap;

	get_key_map(kmapp, &xkeytab);
	kmap = *kmapp;
	internalize_keytab(kmap, xkeytab, keytab);
	if (kmap == NULL) {
		fprintf(stderr, "can't get system default key map\n");
		return(1);
	}
	while (getvar(f, var, line, orig)) {
		if (strcaseeq(var, "Version")) {
			errors += parse_var(&kmap->version, line);

		} else if (strcaseeq(var, "CapsLock")) {
			errors += parse_var(&kmap->caps_key, line);

		} else if (strcaseeq(var, "ScrollLock")) {
			errors += parse_var(&kmap->scroll_key, line);

		} else if (strcaseeq(var, "NumLock")) {
			errors += parse_var(&kmap->num_key, line);

		} else if (strcaseeq(var, "LShift")) {
			errors += parse_var(&kmap->left_shift_key, line);

		} else if (strcaseeq(var, "RShift")) {
			errors += parse_var(&kmap->right_shift_key, line);

		} else if (strcaseeq(var, "LCommand")) {
			errors += parse_var(&kmap->left_command_key, line);

		} else if (strcaseeq(var, "RCommand")) {
			errors += parse_var(&kmap->right_command_key, line);

		} else if (strcaseeq(var, "LControl")) {
			errors += parse_var(&kmap->left_control_key, line);

		} else if (strcaseeq(var, "RControl")) {
			errors += parse_var(&kmap->right_control_key, line);

		} else if (strcaseeq(var, "LOption")) {
			errors += parse_var(&kmap->left_option_key, line);

		} else if (strcaseeq(var, "ROption")) {
			errors += parse_var(&kmap->right_option_key, line);

		} else if (strcaseeq(var, "Menu")) {
			errors += parse_var(&kmap->menu_key, line);

		} else if (strcaseeq(var, "LockSettings")) {
			errors += parse_lock_settings(&kmap->lock_settings, line);

		} else if (strcaseeq(var, "Key")) {
			errors += parse_key(kmap, line, keytab);

		} else if (strcaseeq(var, "Acute")) {
			errors += parse_dead(kmap->acute_dead_key, acute++, line, keytab);

		} else if (strcaseeq(var, "AcuteTab")) {
			errors += parse_tab(&kmap->acute_tables, line);

		} else if (strcaseeq(var, "Grave")) {
			errors += parse_dead(kmap->grave_dead_key, grave++, line, keytab);

		} else if (strcaseeq(var, "GraveTab")) {
			errors += parse_tab(&kmap->grave_tables, line);

		} else if (strcaseeq(var, "Circumflex")) {
			errors += parse_dead(kmap->circumflex_dead_key, circumflex++, 
								 line, keytab);

		} else if (strcaseeq(var, "CircumflexTab")) {
			errors += parse_tab(&kmap->circumflex_tables, line);

		} else if (strcaseeq(var, "Diaeresis")) {
			errors += parse_dead(kmap->dieresis_dead_key, diaeresis++, line,
								 keytab);

		} else if (strcaseeq(var, "DiaeresisTab")) {
			errors += parse_tab(&kmap->dieresis_tables, line);

		} else if (strcaseeq(var, "Tilde")) {
			errors += parse_dead(kmap->tilde_dead_key, tilde++, line, keytab);

		} else if (strcaseeq(var, "TildeTab")) {
			errors += parse_tab(&kmap->tilde_tables, line);

		} else if (var[0] != 0) {
			fprintf(stderr, "Unknown variable: %s\n", var);
			errors++;
		}
		lineno++;
		if (errors) {
			total_errors++;
			fprintf(stderr, "error on line %d: %s\n", lineno, orig);
			errors = 0;
		}
	}
	return (total_errors);
}

/*
 * Convert to kit version which doesn't know the size.
 */
int
externalize_keytab(
				   internal_keytab *keytab,
				   char **xkeytab
				   )
{
	*xkeytab = keytab->storage;
	return (keytab->size);
}


/*
 * Save the key mapping in the proper place
 */
int
save_file(struct key_map *map, internal_keytab *keytab)
{
	FILE *f;
	char *xkeytab;
	int xkeytabsize;
	int bkeytabsize;	/* above in big-endian order */

	xkeytabsize = externalize_keytab(keytab, &xkeytab);

	(void)mkdir(MAPDIR, 0777);
	f = fopen(TMPNAME, "w");
	if (f == NULL) {
		fprintf(stderr, "unable to open temp file\n");
		return (1);
	}
	map->version = 3;
	bkeytabsize = swap32(xkeytabsize);

	/*
	 * Swap to big-endian
	 */
	swapmap(map);
	if (fwrite((const char *)map, 1, sizeof(*map), f) != sizeof(*map) ||
		fwrite((const char *)&bkeytabsize, sizeof(bkeytabsize), 1, f) != 1 ||
		fwrite(xkeytab, xkeytabsize, 1, f) != 1) {
		fclose(f);
		(void)unlink(TMPNAME);
		fprintf(stderr, "unable to write temp file\n");
		return (1);
	}
	fclose(f);
	if (rename(TMPNAME, MAPNAME) < 0) {
		fprintf(stderr, "unable to rename temp file\n");
		return (1);
	}
	return (0);
}

/*
 * Load the key map into the system
 */
int
loadkeys(const char *fname, int restore)
{
	struct key_map *map;
	int errors;
	FILE *f;
	internal_keytab keytab;
	int keytabsize;

	if (fname == NULL) {
		f = stdin;
	} else {
		f = fopen(fname, "r");
		if (f == NULL) {
			fprintf(stderr, "unable to open %s\n", fname);
			return (1);
		}
	}
	errors = parse(f, &map, &keytab);
	if (errors) {
		fprintf(stderr, "errors in file, not saved\n");
		return (errors);
	}
	errors = save_file(map, &keytab);
	if (errors) {
		return (errors);
	}
	if (restore) {
		_restore_key_map_();
	}
	printf("Key map loaded.\n");
	return (0);
}

int
restorekeys(void)
{
	(void)unlink(MAPNAME);
	_restore_key_map_();
	printf("System default key map restored.\n");
	return (0);
}

void
usage(const char *myname)
{
	fprintf(stderr, "usage: %s -d  # dump key map to standard output\n",
			myname);
	fprintf(stderr, "       %s -l  # load key map from standard input\n",
			myname);
	fprintf(stderr, "       %s -r  # restore system default key map\n",
			myname);
	exit(1);
}

/*
 * Do this stuff to insure that we can access the key map stuff
 */
void
be_a_be_app(void)
{
	BApplication *app;
	BWindow *win;
	BRect frame(0, 0, 0, 0);

	app = new BApplication("application/x-vnd.Be-cmd-KYMP");
	win = new BWindow(frame, "", B_TITLED_WINDOW, 0);
}

void
init_paths(void)
{
	TMPNAME[0] = MAPNAME[0] = MAPDIR[0] = '\0';
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY,-1,TRUE,MAPDIR,sizeof(MAPDIR)) < 0) {
		fprintf(stderr, "No user settings dir!\n");
		exit(5);
	}

	sprintf(TMPNAME, "%s/Key_map.tmp", MAPDIR);
	sprintf(MAPNAME, "%s/Key_map", MAPDIR);
}


int
main(int argc, char **argv)
{
	const char *myname;
	int load = 0;
	int restore = 1;

	be_a_be_app();
	
	init_paths();

	myname = (argc > 0) ? argv[0] : "keymap";
	for (argc--, argv++; argc > 0 && **argv == '-'; argc--, argv++) {
		if (strcmp(argv[0], "-d") == 0) {
			exit(dumpkeys(argc > 1 ? argv[1] : NULL));
		}
		if (strcmp(argv[0], "-r") == 0) {
			exit(restorekeys());
		}
		if (strcmp(argv[0], "-l") == 0) {
			load++;
		}
		if (strcmp(argv[0], "-n") == 0) {
			restore = 0;
		}
	}
	if (!load) {
		usage(myname);
	}
	exit(loadkeys(argc > 0 ? argv[0] : NULL, restore));
	return (0);
}
