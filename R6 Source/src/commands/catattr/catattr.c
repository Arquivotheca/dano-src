#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <fs_attr.h>
#include <AppDefs.h>
#include <TypeConstants.h>
#include <errno.h>


static void
dump(const unsigned char *buffer, long length)
{
	const kBytesPerLine = 16;
	long remaining = 0;
	long offset;
	for (offset = 0; ; offset += kBytesPerLine, buffer += kBytesPerLine) {
		long remain = length;
		int index;

		printf( "0x%06x: ", offset);

		for (index = 0; index < kBytesPerLine; index++) {

			if (remain-- > 0)
				printf("%02x%c", buffer[index], remain > 0 ? ',' : ' ');
			else
				printf("   ");
		}

		remain = length;
		printf(" \'");
		for (index = 0; index < kBytesPerLine; index++) {

			if (remain-- > 0)
				printf("%c", buffer[index] > ' ' ? buffer[index] : '.');
			else
				printf(" ");
		}
		printf("\'\n");

		length -= kBytesPerLine;
		if (length <= 0)
			break;

	}
	fflush(stdout);
}

int
main(int argc, char **argv)
{
	int fd = -1, arg_index, err = 0;
	int type = B_STRING_TYPE;
	char *attr_name, *fname, *attr_val;
	uint32  ival;
	uint64  llval;
	float   fval;
	double  dval;
	bool    bval;
	attr_info ai;
	
	if (argc == 1) {
		goto args_error;
	}

	arg_index = 1;

	attr_name = argv[arg_index++];
	if (arg_index >= argc)
		goto args_error;

	for(; arg_index < argc; arg_index++) {
		fname = argv[arg_index];
		if (arg_index >= argc)
			goto args_error;
	
		if (fd >= 0)
			close(fd);

		fd = open(fname, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "%s: can't open file %s to cat attribute %s\n",
					argv[0], fname, attr_name);
			continue;
		}

		if (fs_stat_attr(fd, attr_name, &ai) < 0) {
			fprintf(stderr, "%s: file %s attribute %s: %s\n", argv[0],
					fname, attr_name, strerror(errno));
			continue;
		}

		printf("%s : ", fname);

		if (ai.type == B_INT32_TYPE) {
			err = fs_read_attr(fd, attr_name, ai.type, 0, &ival, sizeof(int32));
			printf("int32 : %d\n", ival);
		} else if (ai.type == B_INT64_TYPE) {
			err = fs_read_attr(fd, attr_name, ai.type, 0, &llval, sizeof(int64));
			printf("int64 : %Ld\n", llval);
		} else if (ai.type == B_BOOL_TYPE) {
			err = fs_read_attr(fd, attr_name, ai.type, 0, &bval, sizeof(bool));
			printf("bool : %u\n", bval);
		} else if (ai.type == B_FLOAT_TYPE) {
			err = fs_read_attr(fd, attr_name, ai.type, 0, &fval, sizeof(float));
			printf("float : %f\n", fval);
		} else if (ai.type == B_DOUBLE_TYPE) {
			err = fs_read_attr(fd, attr_name, ai.type, 0, &dval, sizeof(double));
			printf("double : %f\n", dval);
		} else if (ai.type == B_STRING_TYPE || ai.type == B_ASCII_TYPE || 
                   ai.type == 0x4d494d53) {
			attr_val = (char *)malloc(ai.size + 1);
			err = fs_read_attr(fd, attr_name, ai.type, 0, attr_val, ai.size);

			attr_val[ai.size] = '\0';           /* ensure null termination */
			printf("string : %s\n", attr_val);
			free(attr_val);
		} else {
			printf("raw_data :\n");
			attr_val = (char *)malloc(ai.size);
			err = fs_read_attr(fd, attr_name, ai.type, 0, attr_val, ai.size);
			dump((const unsigned char *)attr_val, ai.size);
			free(attr_val);
		}

		if (err < 0) {
			fprintf(stderr, "%s: file %s : error reading attribute %s: %s\n",
					argv[0], fname, attr_name, strerror(errno));
			return 1;
		}
	}

	return 0;

 args_error:
	fprintf(stderr, "usage: %s attr_name file1 [file2...]\n", argv[0]);
	return 1;
}
