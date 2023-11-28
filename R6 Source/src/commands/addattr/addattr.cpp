#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include <fs_attr.h>
#include <parsedate.h>

#include <AppDefs.h>
#include <TypeConstants.h>
#include <Message.h>
#include <DataIO.h>
#include <Debug.h>

#define MIME_STRING_TYPE   'MIMS'
#define IDENTIFIER_TYPE		B_RAW_TYPE;

struct type_map {
	const char* name;
	type_code code;
};

const type_map type_names[] = {
	{ "int8", B_INT8_TYPE },
	{ "char", B_CHAR_TYPE },
	{ "int16", B_INT16_TYPE },
	{ "int32", B_INT32_TYPE },
	{ "int", B_INT32_TYPE },
	{ "int64", B_INT64_TYPE },
	{ "llong", B_INT64_TYPE },
	{ "uint8", B_UINT8_TYPE },
	{ "uint16", B_UINT16_TYPE },
	{ "uint32", B_UINT32_TYPE },
	{ "uint64", B_UINT64_TYPE },
	{ "time", B_TIME_TYPE },
	{ "bool", B_BOOL_TYPE },
	{ "string", B_STRING_TYPE },
	{ "ascii", B_STRING_TYPE },
	{ "text", B_STRING_TYPE },
	{ "float", B_FLOAT_TYPE },
	{ "double", B_DOUBLE_TYPE },
	{ "mime", MIME_STRING_TYPE },
	{ "message", B_MESSAGE_TYPE },
	{ "raw", B_RAW_TYPE },
	{ 0, 0 }
};
	
type_code
str_to_type(char *str)
{
	if (str == NULL)
		return 0;

	const type_map* names = type_names;
	while( names && names->name ) {
		if( strcasecmp(str, names->name) == 0 ) return names->code;
		names++;
	}
	
	if (isdigit(*str)) {
		type_code val = 0;
		val = strtoul(str, NULL, 0);
		return val;
	}
	
	if (*str == '+') {
		type_code type = 0;
		str++;
		for( int i=0; i<4; i++ ) {
			type <<= 8;
			if( *str != 0 ) {
				type |= *str;
				str++;
			} else {
				type |= ' ';
			}
		}
		return type;
	}
	
	return 0;
}

status_t build_type(BMessage* into, const char* name, const char* value,
					type_code type = B_STRING_TYPE)
{
	int64 i64=0;
	uint64 ui64=0;
	double dbl=0;
	
	status_t err = B_OK;
	
	// First do any common numeric conversion that is possible.
	
	switch( type ) {
		case B_SSIZE_T_TYPE:
		case B_INT64_TYPE:
		case B_INT32_TYPE:
		case B_INT16_TYPE:
		case B_INT8_TYPE:
			i64 = strtoll(value, NULL, 0);
			break;
		
		case B_SIZE_T_TYPE:
		case B_OFF_T_TYPE:
		case B_POINTER_TYPE:
		case B_UINT64_TYPE:
		case B_UINT32_TYPE:
		case B_UINT16_TYPE:
		case B_CHAR_TYPE:
		case B_UINT8_TYPE:
			ui64 = strtoull(value, NULL, 0);
			break;
			
		case B_FLOAT_TYPE:
		case B_DOUBLE_TYPE:
			dbl = strtod(value, NULL);
			break;
	}
	
	if( err != B_OK ) return err;
	
	// Now store the actual value.
	
	switch( type ) {
		case B_POINTER_TYPE:
			err = into->AddPointer(name, (void*)ui64);
			break;

		case B_TIME_TYPE: {
			time_t v = 0;
			
			// if this is a raw, unsigned number, just use it.
			const char* s = value;
			while( s != '\0' && isspace(*s) ) s++;
			const char* n = s;
			while( s != '\0' && isdigit(*s) ) s++;
			while( s != '\0' && isspace(*s) ) s++;
			if( isdigit(*n) && *s == 0 ) {
				while( *n != '\0' && isdigit(*n) ) {
					v = v*10 + (*n++) - '0';
				}
				
			// otherwise, let parsedate() work its magic.
			} else {
				v = parsedate(value, time(NULL));
			}
			
			err = into->AddData(name, B_TIME_TYPE, &v, sizeof(v));
		} break;
			
		case B_INT64_TYPE:
			err = into->AddInt64(name, i64);
			break;
		
		case B_INT32_TYPE:
			err = into->AddInt32(name, (int32)i64);
			break;
		
		case B_INT16_TYPE:
			err = into->AddInt16(name, (int16)i64);
			break;
		
		case B_CHAR_TYPE: {
			char v = (char)i64;
			err = into->AddData(name, B_CHAR_TYPE, &v, sizeof(v));
		} break;
		
		case B_INT8_TYPE:
			err = into->AddInt8(name, (int8)i64);
			break;
		
		case B_UINT64_TYPE: {
			uint64 v = ui64;
			err = into->AddData(name, B_UINT64_TYPE, &v, sizeof(v));
		} break;
		
		case B_UINT32_TYPE: {
			uint32 v = (uint32)ui64;
			err = into->AddData(name, B_UINT32_TYPE, &v, sizeof(v));
		} break;
		
		case B_UINT16_TYPE: {
			uint16 v = (uint16)ui64;
			err = into->AddData(name, B_UINT16_TYPE, &v, sizeof(v));
		} break;
		
		case B_UINT8_TYPE: {
			uint8 v = (uint8)ui64;
			err = into->AddData(name, B_UINT8_TYPE, &v, sizeof(v));
		} break;
		
		case B_BOOL_TYPE: {
			bool v = false;
			if( *value == 't' || *value == 'T' || *value == '1' ) v = true;
			else if( *value == 'f' || *value == 'F' || *value == '0' ) v = false;
			else return B_BAD_VALUE;
			err = into->AddBool(name, v);
		} break;
		
		case B_FLOAT_TYPE:
			err = into->AddFloat(name, (float)dbl);
			break;
		
		case B_DOUBLE_TYPE:
			err = into->AddDouble(name, dbl);
			break;
		
		default:
			err = into->AddData(name, type, value, strlen(value)+1, false);
	}
	
	return err;
}

ssize_t
parse_message(BDataIO* output, int argc, char **argv)
{
	int init_count = argc;
	
	BMessage values;
	ssize_t err;
	
	while( argc > 0 ) {
		type_code type = B_STRING_TYPE;
		char* item_name = "unnamed";
	
		if (**argv == '-') {
			if (strcmp(*argv, "--end") == 0 || strcmp(*argv, "-end") == 0) {
				argv++;
				argc--;
				break;
				
			} else  if (strcmp(*argv, "-t") == 0) {
				argv++;
				argc--;
				if( argc <= 0 ) {
					fprintf(stderr, "Type name expected.\n");
					return B_BAD_VALUE;
				}
				
				type = str_to_type(*argv++);
				argc--;
				if (type == 0) {
					fprintf(stderr, "Message data type %s is not valid\n", *(argv-1));
					fprintf(stderr, "\tTry one of: string, mime, int, llong, "
							"float, double, \n\t\tbool, or a <number>\n");
					return B_BAD_VALUE;
				}
			}
		}
		
		if (argc <= 0) {
			fprintf(stderr, "Data name expected.\n");
			return B_BAD_VALUE;
		}
		item_name = *argv++;
		argc--;
		
		if (argc <= 0) {
			fprintf(stderr, "Data value expected.\n");
			return B_BAD_VALUE;
		}
		
		if( type == B_MESSAGE_TYPE ) {
			err = parse_message(output, argc, argv);
			if( err < B_OK ) return err;
			argc -= err;
			argv += err;
			
		} else {
			char* string_val = *argv++;
			argc--;
	
			err = build_type(&values, item_name, string_val, type);
			if( err < B_OK ) {
				fprintf(stderr, "Error building value '%s': %s\n",
						string_val, strerror(err));
				return err;
			}
		}
	}
	
	#if DEBUG
	PRINT(("Final Message: ")); values.PrintToStream();
	#endif
	
	err = values.Flatten(output);
	return (err >= B_OK) ? (init_count-argc) : err;
}

ssize_t
parse_values(BDataIO* output, int argc, char **argv,
			 const char** attr_name = 0, type_code* attr_type = 0)
{
	int init_count = argc;
	ssize_t err;
	
	while( argc > 0 ) {
	
		type_code type = B_STRING_TYPE;
			
		if( **argv == '-' ) {
			if (strcmp(*argv, "-t") == 0) {
				argv++;
				argc--;
				if( argc <= 0 ) {
					fprintf(stderr, "Type name expected.\n");
					return B_BAD_VALUE;
				}
				
				type = str_to_type(*argv++);
				argc--;
				if (type == 0) {
					fprintf(stderr, "Attribute type %s is not valid\n", *(argv-1));
					fprintf(stderr, "\tTry one of: string, mime, int, llong, "
							"float, double, \n\t\tbool, or a <number>\n");
					return B_BAD_VALUE;
				}
			} else {
				break;
			}
		}
		
		if( attr_type ) *attr_type = type;
		
		if( attr_name ) {
			if (argc <= 0) {
				fprintf(stderr, "Attribute name expected.\n");
				return B_BAD_VALUE;
			}
			*attr_name = *argv++;
			argc--;
		}
		
		if( type == B_MESSAGE_TYPE ) {
			err = parse_message(output, argc, argv);
			if( err < B_OK ) return err;
			argc -= err;
			argv += err;
		
		} else {
			if (argc <= 0) {
				fprintf(stderr, "Attribute value expected.\n");
				return B_BAD_VALUE;
			}
			const char* string_val = *argv++;
			argc--;
		
			BMessage values;
			
			err = build_type(&values, "data", string_val, type);
			if( err < B_OK ) {
				fprintf(stderr, "Error building value '%s': %s\n",
						string_val, strerror(err));
				return err;
			}
			
			const void* data;
			ssize_t data_size;
			err = values.FindData("data", type, &data, &data_size);
			if( err < B_OK ) {
				fprintf(stderr, "Error extracting value '%s': %s\n",
						string_val, strerror(err));
				return err;
			}
			
			err = output->Write(data, data_size);
			if( err < B_OK ) {
				fprintf(stderr, "Error concatenating value '%s': %s\n",
						string_val, strerror(err));
				return err;
			}
		}
		
		if( attr_name ) break;
	}
	
	return (init_count-argc);
}

int
main(int argc, char **argv)
{
	int fd, arg_index;
	
	type_code type = B_STRING_TYPE;
	const char *attr_name = "unnamed";
	char *fname;
	
	arg_index = 1;
	ssize_t err = B_OK;
	
	BMallocIO buffer;
	
	if (argc == 1)
		goto args_error;

	if (strcmp(argv[arg_index], "--help") == 0 ||
			strcmp(argv[arg_index], "-h") == 0 ||
			strcmp(argv[arg_index], "-?") == 0)
		goto args_help;
	
	if (strcmp(argv[arg_index], "--complex") == 0 ||
			strcmp(argv[arg_index], "-complex") == 0) {
		// This is a complex data specification.
		
		arg_index++;
		
		if( arg_index >= argc ) {
			fprintf(stderr, "Missing type for --complex option.\n");
			return 1;
		}
		
		type = str_to_type(argv[arg_index]);
		if (type == 0) {
			fprintf(stderr, "%s: attribute type %s is not valid\n", argv[0],
					argv[arg_index]);
			fprintf(stderr, "\tTry one of: string, mime, int, llong, "
					"float, double, \n\t\tbool, or a <number>\n");
			return 1;
		}
		
		arg_index++;
		err = parse_values(&buffer, argc-arg_index, argv+arg_index);
		if( err < B_OK ) return 1;
		arg_index += err;
		
		if( arg_index >= argc && (
				strcmp(argv[arg_index], "--name") != 0 ||
				strcmp(argv[arg_index], "-name") != 0) ) {
			fprintf(stderr, "Missing --name option.\n");
			return 1;
		}
		
		arg_index++;
		if( arg_index >= argc ) {
			fprintf(stderr, "Missing value for --name option.\n");
			return 1;
		}
		attr_name = argv[arg_index++];
	
	} else {
		// This is an old, simple data specification.
		
		err = parse_values(&buffer, argc-arg_index, argv+arg_index,
						   &attr_name, &type);
		if( err < B_OK ) return 1;
		arg_index += err;
	}
	
	for(; arg_index < argc; arg_index++) {
		fname = argv[arg_index];
		if (arg_index >= argc)
			goto args_error;
	
		fd = open(fname, O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "%s: can't open file %s to add attribute\n",
					argv[0], fname);
			continue;
		}

		/* force deletion of the attribute if it already exists */
		fs_remove_attr(fd, attr_name);

		/* write attribute data buffer */
		err = fs_write_attr(fd, attr_name, type, 0,
							buffer.Buffer(), buffer.BufferLength());

		close(fd);

		if (err < 0) {
			fprintf(stderr, "%s: error writing attribute %s: %s\n", argv[0],
					attr_name, strerror(errno));
			return 1;
		}
	}

	return 0;

 args_error:
 	if( err != B_OK ) {
 		fprintf(stderr, "*** Error at %s: %s\n\n", argv[arg_index], strerror(err));
 	}
 
 args_help:
	fprintf(stderr, "usage:\n"
					"\t%s --complex TYPE [-t TYPE] value [-t TYPE] value ...\n"
					"\t\t--name attr file1 [file2...]\n"
					"alternate:\n"
					"\t%s [-t TYPE] attr value file1 [file2...]\n\n",
			argv[0], argv[0]);
	fprintf(stderr, "\tTYPE is one of the folllowing:\n"
					"\t\tint8, char, int16, int32, int, int64, llong,\n"
					"\t\tuint8, uint16, uint32, uint64, time, bool, string,\n"
					"\t\tascii, text, float, double, mime, message.\n"
					"\tIf [-t TYPE] is not specified, string is assumed.\n\n"
					"\tIn addition, the TYPE immediately after --complex can\n"
					"\tby raw, for B_RAW_TYPE.\n\n"
					"\tThe message type is followed by a series of\n"
					"\tname [-t TYPE] value commands, terminated with --end.\n"
					"\tThe result will be a flattened message containing the\n"
					"\tspecified fields.\n\n"
					"\tAn example, creating a B_RAW_TYPE attribute concatenating\n"
					"\ta time_t value of 0 followed by a flattened BMessage:\n"
					"\t\taddattr --complex raw -t time 0 -t message \\\n"
					"\t\t    \"a string\" \"a value\"                   \\\n"
					"\t\t    \"an int32\" -t int32 100                \\\n"
					"\t\t\t--end --name \"attr name\" \"filename\"\n");
	return 1;
}
