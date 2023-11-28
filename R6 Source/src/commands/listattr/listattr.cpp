//
// (C) 2001 Be Incorporated, All rights reserved.
//
// Authors: (unknown), Jac Goudsmit (JG)
//
// Known bugs:
// - For long attribute names (>32 characters) the columns get screwed up. Also,
//   there is no check for unprintable characters in attribute names
// - When printing string values, the terminating \0 is not printed, but also
//   no indication is printed when the string doesn't have a terminating \0.
//   This is by design: we don't want to see the terminating \0's for all those
//   strings that do have them, and for strings that don't have them, the size
//   column will indicate it.
//
// History:
// May-2001 JG Improved output
// Jan-2001 JG Overhauled
// ???-???? ?? Initial
#include <stdio.h>
#include <string.h>
#include <fs_attr.h>
#include <AppDefs.h>
#include <Node.h>
#include <TypeConstants.h>
#include <getopt.h>
#include <ctype.h>
#include <malloc.h>
#include <stdarg.h>

#define MIME_STRING_TYPE   'MIMS'

#define NELEM(array) (sizeof(array)/sizeof(array[0]))

// global variables
static bool fOptionParseable 		= false; // option to make output "easily" parseable
static bool fOptionHex       		= false; // option to force output to hex
static bool fOptionDeveloper 		= false; // option to allow printing of non-friendly names
static bool fOptionIgnoreMimeType	= false; // option to ignore MIMS attributes
static bool fOptionClassic			= false; // option to output data in classical format

// Define a function type for all the data-representation functions.
// Data-representation functions are functions that are used to
// convert a data buffer to a specific output format. There is a
// different function for each output format (printable string,
// hex dump etc.)
// First we #define a prototype, in order to make it easy to change
// the type later on.
// Then we declare a type for all data representation functions,
// this makes it easier to declare functions and to create pointers
// to this type of function.
#define BARE_DECL_REP_FUNC(name) size_t name(uchar *data, size_t size)
#define DECL_REP_FUNC(name) static BARE_DECL_REP_FUNC(name)
typedef BARE_DECL_REP_FUNC(rep_func);

// Before we define the lookup table, we need to declare all the
// functions that are referenced by it
static rep_func rep_dump; 		// just hex-dumps the buffer
static rep_func rep_bool;		// true or false
static rep_func rep_char;		// character (dec+hex [+char if printable])
static rep_func rep_float;		// double or float, depending on size
static rep_func rep_int; 		// integer (dec+hex), depending on size
static rep_func rep_uint;		// unsigned integer (dec+hex), depending on size
static rep_func rep_string;		// string; any non-printables are escaped

// Now we define a lookup-table for all types that we know.
// Any types that are not in this table will have the type
// as well as the value hex-dumped. Any type-names and friendly names that
// are NULL are stringized.
// If any new "standard" types will be added in the future,
// simply extend the table and recompile. The code will adapt to
// the new table size. The table doesn't need to be sorted in any way.
static const struct rep_lut_struct
{
	uint32		type;			// type from TypeConstants.h
	rep_func   *representer;	// function to print value of this type
	size_t		expected_size;	// expected size, 0=any size
	char 	   *friendly_name;	// name to display for this type,
								//   don't use spaces (think of users who want
								//   to parse the output with AWK)
	char	   *type_name;		// non-friendly name, corresponding to naming
								//   in TypeConstants.h, but without the B_...
								//   and the ..._TYPE (for brevity)
	char	   *classic_name;	// name to display in classic output mode
}
rep_lut[]=
{
	{ MIME_STRING_TYPE				, rep_string	, 0				, "MIMEtype"	, NULL					, "MIME str"	}, // not in TypeConstants.h
	{ B_ANY_TYPE					, rep_dump		, 0 			, "Any"			, "ANY"					, NULL			},
	{ B_BOOL_TYPE 					, rep_bool		, sizeof(bool)	, "Boolean"		, "BOOL"				, "Bool"		},
	{ B_CHAR_TYPE 					, rep_char		, sizeof(char)	, "Character"	, "CHAR"				, NULL			},
	{ B_COLOR_8_BIT_TYPE 			, rep_dump		, 0				, "8BitBitmap"	, "COLOR_8_BIT"			, NULL			},
	{ B_DOUBLE_TYPE 				, rep_float		, sizeof(double), "Double"		, "DOUBLE"				, "Double"		},
	{ B_FLOAT_TYPE 					, rep_float		, sizeof(float)	, "Float"		, "FLOAT"				, "Float"		},
	{ B_FONT_TYPE					, rep_dump		, 0				, "Font"		, "FONT"				, NULL			},
	{ B_GRAYSCALE_8_BIT_TYPE		, rep_dump		, 0				, "8BitGraySc"	, "GRAYSCALE_8_BIT"		, NULL			},
	{ B_INT64_TYPE 					, rep_int		, sizeof(int64)	, "Int64"		, "INT64"				, "Int-64"		},
	{ B_INT32_TYPE 					, rep_int		, sizeof(int32)	, "Int32"		, "INT32"				, "Int-32"		},
	{ B_INT16_TYPE 					, rep_int		, sizeof(int16)	, "Int16"		, "INT16"				, NULL			},
	{ B_INT8_TYPE 					, rep_int		, sizeof(int8)	, "Int8"		, "INT8"				, NULL			},
	{ B_MESSAGE_TYPE				, rep_dump		, 0				, "Message"		, "MESSAGE"				, NULL			},
	{ B_MESSENGER_TYPE				, rep_dump		, 0				, "Messenger"	, "MESSENGER"			, NULL			},
	{ B_MIME_TYPE					, rep_string	, 0				, "MIMEstring"  , "MIME"				, NULL			},
	{ B_MONOCHROME_1_BIT_TYPE 		, rep_dump		, 0				, "1BitMono"	, "MONOCHROME_1_BIT"	, NULL			},
	{ B_OBJECT_TYPE 				, rep_dump		, 0				, "ObjectPtr"	, "OBJECT"				, NULL			},
	{ B_OFF_T_TYPE 					, rep_int		, sizeof(off_t) , "Offset"		, "OFF_T"				, NULL			},
	{ B_PATTERN_TYPE 				, rep_dump		, 0				, "Pattern"		, "PATTERN"				, NULL			},
	{ B_POINTER_TYPE 				, rep_dump		, 0				, "Pointer"		, "POINTER"				, NULL			},
	{ B_POINT_TYPE 					, rep_dump		, 0				, "BPoint"		, "POINT"				, NULL			},
	{ B_RAW_TYPE 					, rep_dump		, 0				, "RawData"		, "RAW"					, NULL			},
	{ B_RECT_TYPE 					, rep_dump		, 0				, "BRect"		, "RECT"				, NULL			},
	{ B_REF_TYPE 					, rep_dump		, 0				, "Entry_ref"	, "REF"					, NULL			},
	{ B_RGB_32_BIT_TYPE 			, rep_dump		, 0				, "32BtBitmap"  , "RGB_32_BIT"			, NULL			},
	{ B_RGB_COLOR_TYPE 				, rep_dump		, 0				, "RGB_color"	, "RGB_COLOR"			, NULL			},
	{ B_SIZE_T_TYPE	 				, rep_uint		, sizeof(size_t), "Size_t"		, "SIZE_T"				, NULL			},
	{ B_SSIZE_T_TYPE	 			, rep_int		, sizeof(ssize_t),"SSize_t"		, "SSIZE_T"				, NULL			},
	{ B_STRING_TYPE 				, rep_string	, 0				, "String"		, "STRING"				, "Text" 		},
	{ B_TIME_TYPE 					, rep_uint		, sizeof(time_t), "Time_t"		, "TIME"				, NULL			},
	{ B_UINT64_TYPE					, rep_uint		, sizeof(uint64), "UInt64"		, "UINT64"				, NULL			},
	{ B_UINT32_TYPE					, rep_uint		, sizeof(uint32), "UInt32"		, "UINT32"				, NULL			},
	{ B_UINT16_TYPE 				, rep_uint		, sizeof(uint16), "UInt16"		, "UINT16"				, NULL			},
	{ B_UINT8_TYPE 					, rep_uint		, sizeof(uint8) , "UInt8"		, "UINT8"				, NULL			},
	{ B_MEDIA_PARAMETER_TYPE		, rep_dump		, 0				, "MediaParam"	, "MEDIA_PARAMETER"		, NULL			},
	{ B_MEDIA_PARAMETER_WEB_TYPE	, rep_dump		, 0				, "MediaWeb"	, "MEDIA_PARAMETER_WEB"	, NULL			},
	{ B_MEDIA_PARAMETER_GROUP_TYPE	, rep_dump		, 0				, "MediaGroup"	, "MEDIA_PARAMETER_GROUP",NULL			},
	{ B_ATOM_TYPE 					, rep_dump		, 0				, "Atom"		, "ATOM"				, NULL			},
	{ B_ATOMREF_TYPE 				, rep_dump		, 0				, "AtomRef"		, "ATOMREF"				, NULL			},
	{ B_ASCII_TYPE					, rep_dump		, 0				, "ASCII"		, "ASCII"				, "Text"		}, // deprecated but still supported
};

////////////////////////////////////////////////////////////////////////////////

// Output manipulation

typedef enum
{
	SEL_TYPE,
	SEL_SIZE,
	SEL_NAME,
	SEL_VALU,
	MAX_FIELD_ENUM
} field_enum;

static struct
{
	field_enum	field_sel;
	char 		field_char;
	unsigned	field_width;
	unsigned	field_old_width;
	char	   *field_header;
	char       *field_old_header;
}
print_init_lut[MAX_FIELD_ENUM] =
{
	{ SEL_TYPE, 't', 10, 10, "Type" , "  Type"           					},
	{ SEL_SIZE, 's', 10,  9, "Size" , "   Size"          					},
	{ SEL_NAME, 'n', 34, 32, "Name" , "             Name"					}, 
	{ SEL_VALU, 'v', 65,  0, "Value", "You shouldn't see this, go away!" 	},
},
print_lut[MAX_FIELD_ENUM];

field_enum print_sequence[MAX_FIELD_ENUM];
int print_sequence_length=0;
int print_sequence_index=0;

#define CHAR_REP_SIZE 5 /* array size for longest representation for a character ("\x99") */

static void init_print_lut(void)
{
	// This function initializes the table that's used during the program
	// from the initialization table. This may seem like superfluous code to a
	// beginner ("why not simply use the table as it's defined?"), but actually
	// this is about the only safe way of initializing a table that is indexed with an enum.
	// By using this method, the only thing that we need to make sure of is that
	// MAX_..._ENUM remains correct after any modifications to the enum so that the
	// the table size is correct. The values within the enum don't matter anymore thanks to this!
	unsigned i;
	
	for (i=0; i<NELEM(print_init_lut); i++)
	{
		print_lut[print_init_lut[i].field_sel]=print_init_lut[i];
		if (fOptionClassic)
		{
			// note: we could copy field_header from field_old_header here too
			// but we don't because then we can still use the new header name for debugging
			print_lut[i].field_width=print_lut[i].field_old_width;
		}
	}
}

static void init_print_sequence(char *format)
{
	char *s;
	field_enum sel;
	
	print_sequence_length=0;

	if (fOptionClassic)
	{
		format="tsn"; // option format for classic output
	}
	else if(!format)
	{
		format="tsnv"; // default format for new output
	}
	
	for (s=format; *s; s++)
	{
		int i;
		
		// Find the field that corresponds to the character
		sel=MAX_FIELD_ENUM;
		for (i=0; i<MAX_FIELD_ENUM; i++)
		{
			if (print_lut[i].field_char==*s)
			{
				int j;
				
				// Found a valid field identifier.
				sel=(field_enum)i;

				// Check if we already had this field
				for (j=0; j<print_sequence_length; j++)
				{
					if (print_sequence[j]==sel)
					{
						sel=MAX_FIELD_ENUM;
						break;
					}
				}
				
				if (sel!=MAX_FIELD_ENUM)
				{
					// no duplicate found; store the field
					print_sequence[print_sequence_length++]=sel;
				}
				break;
			}
		}
	}
}

static void print_column_ext(bool rightjustify, char *extformat, va_list args)
{
	int index=print_sequence_index;

	unsigned width=print_lut[print_sequence[index]].field_width;
	char *outstring=(char *)malloc(width+1);
	char *format;

	if (extformat==NULL)
	{
		// there was a typo in the old version's header
		if (fOptionClassic && print_sequence[index]==SEL_NAME)
		{
			width--; // print one '-' less
		}
		
		// fill the output string with '-'s
		memset(outstring, '-', width);
		outstring[width]='\0';
	}
	else
	{
		// fill the output string with the input, cut off at the width
		vsnprintf(outstring, width+1, extformat, args);
	}

	// initialize the format string
	if (index+1<print_sequence_length)
	{
		// fill in the format
		// there was a typo in the old version's header: one space too many after size
		asprintf(&format, "%%%s%us%%%us", rightjustify ? "" : "-", width,
			(fOptionClassic && index==SEL_SIZE && !extformat) ? 3 : 2);
	}
	else
	{
		// line feed after the last column
		asprintf(&format, "%%%s%us%%s\n", rightjustify ? "": "-", width);
	}
	
	// print the column's data
	printf(format, outstring, "");
	
	// free up allocated memory
	free(format);	
	free(outstring);
}

static void print_column(char *extformat,...)
{
	va_list args;
	
	va_start(args, extformat);
	print_column_ext(false, extformat, args);
	//va_end;
}

static void print_ra_column(char *extformat,...)
{
	va_list args;
	
	va_start(args, extformat);
	print_column_ext(true, extformat, args);
	//va_end;
}

static void print_header(void)
{
	int i;

	for (i=0; i<2; i++)
	{
		for (print_sequence_index=0; print_sequence_index<print_sequence_length; print_sequence_index++)
		{
			int index=print_sequence[print_sequence_index];
			
			if (i==0)
			{
				if (fOptionClassic)
				{
					print_column("%s", print_lut[index].field_old_header);
				}
				else
				{
					if (index==SEL_SIZE)
					{
						print_ra_column("%s",print_lut[index].field_header);
					}
					else
					{
						print_column("%s", print_lut[index].field_header);
					}
				}					
			}
			else
			{
				print_column(NULL);
			}
		}
	}
}	

static char *malloc_stringize(uint32 data)
{
	// poor old listattr couldn't stringize
	if (fOptionClassic)
	{
		return NULL;
	}
	
	char *s=(char *)malloc(sizeof(uint32)+1);
	unsigned i;
	
	// This code works for little-endian as well as big-endian machines.
	for (i=0; i<sizeof(uint32); i++)
	{
		char c=(char)(data & 0xFF);
		if (!isprint(s[i]))
		{
			free(s);
			return NULL;
		}
		
		data>>=8;
		s[sizeof(uint32)-(i+1)]=c;
	}
	
	s[i]='\0';
	return s;
}

static size_t appendchar(char **t, uchar c, bool finalchar)
{
	// appends a character representation to string *t unless t is NULL
	
	char s[CHAR_REP_SIZE];
	char *p;
	static const char search[]="\\\a\b\f\n\r\t\v\"";
	static const char replace[]="\\abfnrtv\"";
	size_t spaceneeded;
	
	if (c==0)
	{
		if (!finalchar)
		{
			spaceneeded=sprintf(s, "\\0");
		}
		else
		{
			s[0]='\0';
			spaceneeded=0; // don't print terminating 0
		}
	}
	else if ((p=strchr(search,c))!=NULL)
	{
		spaceneeded=sprintf(s, "\\%c", replace[p-search]);
	}
	else if (isprint(c))
	{
		spaceneeded=sprintf(s, "%c", c);
	}
	else
	{
		spaceneeded=sprintf(s, "\\x%02X", c);
	}
	
	if ((t!=NULL) && (*t!=NULL))
	{
		strcpy(*t, s);
		(*t)+=spaceneeded;
	}
	
	return spaceneeded;
}

static const struct rep_lut_struct *get_type(uint32 type)
{
	// Look up the type in the table. Returns a dummy record if not found
	// We can't return NULL here.
	size_t lut_index;
	static rep_lut_struct default_type = { B_ANY_TYPE, rep_dump, 0, NULL, NULL, NULL };

	for (lut_index=0; lut_index<NELEM(rep_lut); lut_index++)
	{
		if (rep_lut[lut_index].type==type)
		{
			// We found the type, return pointer to the entry
			return &rep_lut[lut_index];
		}
	}

	default_type.type=type;
	
	return &default_type;
}

static void print_type(const struct rep_lut_struct *typedata)
{
	if (!fOptionHex)
	{
		char *name;
		if (fOptionDeveloper)
		{
			name=typedata->type_name;
		}
		else if (fOptionClassic)
		{
			name=typedata->classic_name;
		}
		else
		{
			name=typedata->friendly_name;
		}
			
		// if the name is not NULL, print it and return
		if (name)
		{
			if (fOptionClassic)
			{
				print_ra_column("%s",name);
			}
			else
			{
				print_column("%s",name);
			}
			return;
		}
		
		// The name is unknown
		// See if we can stringize it.
		// "stringizing" means convert the data in the int32 type to characters.
		// The MSB is printed first.
		char *s=malloc_stringize(typedata->type);
		
		if (s)
		{
			print_column("'%s'", s);
			free(s);
			return;
		}
	}
	
	// The hex option was given or we can't find another way to represent the type
	if (fOptionClassic)
	{
		print_ra_column("0x%.8lx", typedata->type);
	}
	else
	{
		print_column("0x%08lX", typedata->type);
	}
}

static void print_attr(const rep_lut_struct *typedata, size_t size, char *name, uchar *value, rep_func *representer)
{
	size_t totaldone=0;
	size_t lefttodo=size;
	bool encounteredvalue=false;
	
	while (totaldone<size)
	{
		size_t donethistime=0;
		
		for (print_sequence_index=0; print_sequence_index<print_sequence_length; print_sequence_index++)
		{
			field_enum sel=print_sequence[print_sequence_index];
			
			// When printing continuation lines, print empty columns for everything except value
			if ((totaldone==0) || (sel==SEL_VALU))
			{
				switch (sel)
				{
				case SEL_TYPE:
				{
					print_type(typedata);
					break;
				}
				case SEL_SIZE:
				{
					if (fOptionHex)
					{
						print_column("0x%08lX",size);
					}
					else
					{
						print_ra_column("%.ld",size);
					}
					break;
				}
				case SEL_NAME:
				{
					if (fOptionClassic)
					{
						print_ra_column("%s", name);
					}
					else
					{
						rep_string((uchar *)name, strlen(name)+1);
					}
					break;
				}
				case SEL_VALU:
				{
					donethistime=(*representer)(value, lefttodo);
					encounteredvalue=true;
					break;
				}
				default:
					break;	// unknown field selector??? ignore
				}
			}
			else
			{
				print_column("");
			}
		}
		
		// If the value field was not encountered, we will never be done unless
		// we break out here
		if (!encounteredvalue)
		{
			break;
		}
		else
		{
			totaldone+=donethistime;
			value+=donethistime;
			lefttodo-=donethistime;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

// hex dump a data buffer
DECL_REP_FUNC(rep_dump)
{
	size_t i;
	
	if (fOptionParseable)
	{
		// print everything on one line
		for (i=0; i<size; i++)
		{
			printf("%02X%s", data[i], (i+1<size) ? " " : "");
		}
		return size;
	}
	else
	{
		// print 16 bytes per line
		char hexline[3*16+1];
		char charline[17];
		size_t done=0;
		
		for (i=0; i<16; i++)
		{
			char sep=((i==7) ? '-' : ' '); // separator in hex data

			if (i<size)
			{
				uchar c=data[i];
				charline[i]=(isprint((char)c) ? c : '.');
				sprintf(hexline+3*i, "%02X%c", c, sep);
			}
			else
			{
				charline[i]='\0';
				sprintf(hexline+3*i, "  %c", sep);
			}
			
			done++;
		}
		charline[i]='\0';
		
		print_column("%s%s", hexline, charline);
		return done;
	}
}

DECL_REP_FUNC(rep_bool)
{
	switch (*(bool *)data)
	{
	case false:
		print_column("FALSE");
		break;
	case true:
	default:
		// todo: different strings for actual 'true' and default?
		print_column("TRUE");
		break;
	}
	return size;
}

DECL_REP_FUNC(rep_char)
{
	char s[CHAR_REP_SIZE];
	char *t=s;
	
	(void)appendchar(&t, *data, false);
	
	return size;
}

DECL_REP_FUNC(rep_float)
{
	switch (size)
	{
	case sizeof(float):
		print_column("%G", *(float *)data);
		break;
		
	case sizeof(double):
		print_column("%G", *(double *)data);
		break;
		
	default:
		return rep_dump(data, size);
	}
	
	return size;
}

DECL_REP_FUNC(rep_int)
{
	switch (size)
	{
	case sizeof(int8):
		print_column("%d 0x%02X", *(int8 *)data, *(int8 *)data);
		break;
		
	case sizeof(int16):
		print_column("%d 0x%04X", *(int16 *)data, *(int16 *)data);
		break;
		
	case sizeof(int32):
		print_column("%ld 0x%08lX", *(int32 *)data, *(int32 *)data);
		break;
		
	case sizeof(int64):
		print_column("%Ld 0x%016LX", *(int64 *)data, *(int64 *)data);
		break;
		
	default:
		return rep_dump(data, size);
	}
	
	return size;
}

DECL_REP_FUNC(rep_uint)
{
	switch (size)
	{
	case sizeof(uint8):
		print_column("%u 0x%02X", *(uint8 *)data, *(uint8 *)data);
		break;
		
	case sizeof(uint16):
		print_column("%u 0x%04X", *(uint16 *)data, *(uint16 *)data);
		break;
		
	case sizeof(uint32):
		print_column("%lu 0x%08lX", *(uint32 *)data, *(uint32 *)data);
		break;
		
	case sizeof(uint64):
		print_column("%Lu 0x%016LX", *(uint64 *)data, *(uint64 *)data);
		break;
		
	default:
		return rep_dump(data, size);
	}
	
	return size;
}

DECL_REP_FUNC(rep_string)
{
	size_t done=size;
	size_t i;
	size_t outsize=2; // start with two quotes
	size_t spaceavailable=print_lut[print_sequence[print_sequence_index]].field_width;
	
	char *outdata=NULL;
	char *p=NULL;
	
	// do a dry run first to determine how much space we need
	for (int pass=0; pass<2; pass++)
	{
		if (pass!=0)
		{
			outdata=(char *)malloc(outsize+1);
			p=outdata;
			*p++='\"';
		}
		
		for (i=0; i<done; i++)
		{
			uchar c=data[i];
			size_t spaceneeded=appendchar(&p, c, i+1==done);
			
			if (pass==0)
			{
				// on the first pass, check if the new character fits
				if (outsize+spaceneeded>spaceavailable)
				{
					// too big - the rest will have to go on the next line
					done=i;
					break; // go to pass 2
				}
				
				// new character fits; add the size
				outsize+=spaceneeded;
			}
		}
	}
	
	// After the two passes, store a quote and a 0
	*p++='\"';
	*p='\0';
	
	// print the column
	print_column("%s", outdata);
	
	return done;
}

////////////////////////////////////////////////////////////////////////////////

static void print_usage(const char* name)
{
	fprintf(stderr,
	// TODO: use constants and/or table for option characters
		"usage: %s [-hmpxdcf[format]] 'filename' ['filename' ...]\n"
		"\n"
		"       -h show this help\n"
		"       -m ignore attributes that specify MIME-type ('MIMS' type)\n"
		"       -p parseable output: no header, no multiline output (for use with e.g. awk)\n"
		"       -x use hexadecimal format for types and values\n"
		"       -d use non-friendly names for types (for developers)\n"
		"       -c use 'classic' output format (resembles the old format a little)\n"
		"       -f change the format of the output: (default=`-f tsnv')\n"
		"          t=type\n"
		"          s=size\n"
		"          n=name\n"
		"          v=value\n", name);
}

int main(int argc, char **argv)
{
	long			index;
	status_t		err;
	struct stat		st;
	BNode			node;
	char			attr_name[256];
	int				retval = 0;
	bool			no_attributes_yet;
	char		   *format=NULL;

	static const struct option long_options[] = {
		{"help",			no_argument,		0,	'h'},
		{0,0,0,0}
	};
	
	// Parse command-line arguments.
	int opt;
	int option_index = 0;
	while (((opt = getopt_long(argc, argv, "+?hmpxdcf:", long_options, &option_index)) != -1)
			|| optind==argc)
	{
		switch(opt)
		{
		case '?':
		case 'h':
			print_usage(argv[0]);
			return 0;
			break;
			
		case 'm':
			// todo: add an option to ignore any user-specifiable type?
			fOptionIgnoreMimeType = true;
			break;
			
		case 'p':
			fOptionParseable = true;
			break;
	
		case 'x':
			fOptionHex = true;
			break;
			
		case 'd':
			fOptionDeveloper = true;
			break;
			
		case 'c':
			fOptionClassic = true;
			break;
			
		case 'f':
			format=optarg;
			break;
			
		default: // unknown option (opt=='?') or no filenames given (opt==-1 and optind==argc)
			print_usage(argv[0]);
			return 1;
			break;
		}
	}

	if (fOptionClassic)
	{
		if (format)
		{
			fprintf(stderr, "%s: cannot combine -c and -f options\n", argv[0]);
			return 1;
		}
		if (fOptionHex)
		{
			fprintf(stderr, "%s: cannot combine -c and -x options\n", argv[0]);
			return 1;
		}
		if (fOptionParseable)
		{
			fprintf(stderr, "%s: cannot combine -c and -p options\n", argv[0]);
			return 1;
		}
		if (fOptionDeveloper)
		{
			fprintf(stderr, "%s: cannot combine -c and -d options\n", argv[0]);
			return 1;
		}
	}
	
	// Initialize tables
	init_print_lut();
	init_print_sequence(format);
	
	// extra args are filenames
	for (; optind < argc; optind++)
	{
		node.SetTo(argv[optind]);
		err = node.GetStat(&st);
		if (err)
		{
			fprintf(stderr, "%s: error for '%s' (%s)\n", argv[0], argv[optind], strerror(err));
			retval=1;
			continue;
		}

		no_attributes_yet=true;
		
		if (!fOptionParseable)
		{
			printf("%s %s", fOptionClassic? "file" : "File: ", argv[optind]);
		}
		
		index = 0;
		while (node.GetNextAttrName(attr_name) == B_OK)
		{
			struct attr_info	ainfo;
			node.GetAttrInfo(attr_name, &ainfo);
			
			// print info for the attribute unless it can be ignored
			if ((!fOptionIgnoreMimeType) || (ainfo.type!=MIME_STRING_TYPE))
			{
				if ((!fOptionParseable) && (no_attributes_yet))
				{
					printf("\n");
					print_header();
					no_attributes_yet = false;
				}
				
				uchar *attr_value;
				size_t num_to_read=ainfo.size;
				
				if ((attr_value=(uchar *)malloc(ainfo.size))==NULL)
				{
					// if attribute is too big to be allocated, set size to 0
					// todo: allocate a smaller buffer in this case
					num_to_read = 0;
					
					// Print error message
					printf("\n");
					fprintf(stderr, "Can't allocate enough space for value\n");
					retval=1;
				}
				else
				{
					// Read the attribute into our buffer
					if (node.ReadAttr(attr_name, ainfo.type, 0, &attr_value[0], num_to_read)
						== (ssize_t)num_to_read)
					{
						// we got what we wanted. Now determine how to print the data
						rep_func *representer;
						const rep_lut_struct *typedata=get_type(ainfo.type);
						
						if ((!fOptionHex) && (typedata) && 
							((typedata->expected_size==0) || (ainfo.size==typedata->expected_size)))
						{
							// The size matches or size doesn't matter for this type.
							// Use the designated function to print the value
							representer=typedata->representer;
						}
						else
						{
							// The type wasn't found, or it was found but the 
							// size doesn't match and does matter, or the hex option was given
							// Use the default function instead of the designated one.
							representer=rep_dump;
						}
						
						// print the attribute
						print_attr(typedata, num_to_read, attr_name, attr_value, representer);
					}
					else
					{
						// We got less than we expected; normally this shouldn't happen.
						// Print error message 
						printf("\n");
						fprintf(stderr,"Error reading attribute '%s'\n", attr_name);
						retval=1;
					}
					
					// Free the allocated memory
					free(attr_value);
				} // if [malloc]
			} // if [attribute needs printing]
		} // while [attributes]
		
		// if no attributes were printed so far and everything is still ok,
		// the cursor is still behind the file name.
		if ((no_attributes_yet) && (!fOptionParseable) && (retval==0))
		{
			printf(" - no attributes to list.\n");
		}
	} // for [file]
	
	return retval;
}
