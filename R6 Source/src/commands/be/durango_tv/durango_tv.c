#include <Drivers.h>
#include <OS.h>
#include <Accelerant.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DURANGO_PRIVATE_DATA_MAGIC	0x0202 /* a private driver rev, of sorts */

enum {
	DURANGO_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	DURANGO_DEVICE_NAME,
	DURANGO_WRITE_CH7005,
	DURANGO_READ_CH7005,
	DURANGO_LOCK
};

typedef struct {
	uint32	magic;
	uint8	count;
	uint8	index;
	uint8	*data;
} durango_ch7005_io;

static const uint8 ch7005_reg_masks[0x3E] = {
	0x00, // 0x00 800x600 NTSC 7/10
	0x00, // 0x01
	0xFF, // 0x02 NOT USED
	0x00, // 0x03 Video Bandwidth
	0x90, // 0x04 input data format
	0xFF, // 0x05 NOT USED
	0x20, // 0x06 Clock mode
	0x00, // 0x07 Start Active Video
	0xF8, // 0x08 Position overflow
	0x00, // 0x09 Black level (7F is neutral)
	0x00, // 0x0A Horiz. pos.
	0x00, // 0x0B Verti. pos.
	0xFF, // 0x0C NOT USED
	0xF0, // 0x0D sync polarity
	0xE0, // 0x0E Power managment (output select) 
	0xFF, // 0x0F NOT USED
	0x00, // 0x10 Connection detect
	0xF8, // 0x11 Contrast enhancement (0x03 is neutral)
	0xFF, // 0x12 NOT USED
	0xE0, // 0x13 PLL Overflow
	0x00, // 0x14 PLL M
	0x00, // 0x15 PLL N
	0xFF, // 0x16 NOT USED
	0xC0, // 0x17 Buffered clock output (14 MHz)
	0xF0, // 0x18 SubCarrier values (not used in slave mode, but...)
	0xF0, // 0x19 for mode 24, there are two values:
	0xF0, // 0x1A normal dot crawl: 428,554,851
	0xE0, // 0x1B no dot crawl    : 438,556,645
	0xE0, // 0x1C (also, buffered clock out enable)
	0xF0, // 0x1D
	0xF0, // 0x1E
	0xF0, // 0x1F
	0xC0, // 0x20 PLL control
	0xE0, // 0x21 CIV control (automatic)
	0xFF, // 0x22
	0xFF, // 0x23
	0xFF, // 0x24
	0xFF, // 0x25
	0xFF, // 0x26
	0xFF, // 0x27
	0xFF, // 0x28
	0xFF, // 0x29
	0xFF, // 0x2A
	0xFF, // 0x2B
	0xFF, // 0x2C
	0xFF, // 0x2D
	0xFF, // 0x2E
	0xFF, // 0x2F
	0xFF, // 0x30
	0xFF, // 0x31
	0xFF, // 0x32
	0xFF, // 0x33
	0xFF, // 0x34
	0xFF, // 0x35
	0xFF, // 0x36
	0xFF, // 0x37
	0xFF, // 0x38
	0xFF, // 0x39
	0xFF, // 0x3A
	0xFF, // 0x3B
	0xFF, // 0x3C
	0xF8  // 0x3D
};

void
read_regs(int force, int fd, FILE *f)
{
	char line_buf[128]; // the line buffer
	char *s;			// for striping off comments
	int index;			// the register to read
	durango_ch7005_io dci;	// ioctl() control block
	uint8 data;			// storage for the register read

	/* prep the ioctl() data block */
	dci.magic = DURANGO_PRIVATE_DATA_MAGIC;
	dci.data = &data;
	/* should we force a read of "unused" registers? */
	dci.count = force ? 0x81 : 1;

	/* read a line from the input stream */
	while (fgets(line_buf, sizeof(line_buf), f))
	{
		/* remove any comments */
		s = strchr(line_buf, '#');
		if (s) *s = '\0';
		/* parse one number from the string */
		if (sscanf(line_buf, "%i", &index) == 1)
		{
			/* a valid index? */
			if ((index >= 0) && (index <= 0x3e))
			{
				status_t result;
				dci.index = (uint8)index;
				/* call the driver */
				result = ioctl(fd, DURANGO_READ_CH7005, &dci, sizeof(dci));
				if (result == B_OK)
				{
					fprintf(stdout, "read index %3d (0x%.2x) : %3d (0x%.2x)\n", index, index, data, data);
				}
				else
				{
					fprintf(stderr, "read_regs: failed reading index %3d (0x%.2x) with reason %s\n",
						index, index, strerror(errno));
				}
			}
			else
			{
				fprintf(stderr, "read_regs: index %3d out of range\n", index);
			}
		}
	}
}

void
write_regs(int force, int fd, FILE *f)
{
	char line_buf[128]; // the line buffer
	char *s;			// for striping off comments
	int index;			// the register to write
	int val;			// the value to write
	durango_ch7005_io dci;	// ioctl() control block
	uint8 data;			// storage for the register read

	/* prep the ioctl() data block */
	dci.magic = DURANGO_PRIVATE_DATA_MAGIC;
	dci.data = &data;

	/* read a line from the input stream */
	while (fgets(line_buf, sizeof(line_buf), f))
	{
		/* remove any comments */
		s = strchr(line_buf, '#');
		if (s) *s = '\0';
		/* parse two numbers from the string */
		if (sscanf(line_buf, "%i %i", &index, &val) == 2)
		{
			/* a valid index? */
			if ((index >= 0) && (index <= 0x3e))
			{
				status_t result;
				dci.index = (uint8)index;
				/* should we force a read of "unused" registers? */
				dci.count = force ? 0x81 : 1;
				/* store the data to write */
				data = (uint8)val;
				/* call the driver */
				result = ioctl(fd, DURANGO_WRITE_CH7005, &dci, sizeof(dci));
				if (result != B_OK)
				{
					fprintf(stderr, "write_regs: failed writing index %3d (0x%.2x) with reason %s\n",
						index, index, strerror(errno));
				}
			}
			else
			{
				fprintf(stderr, "write_regs: index %3d out of range\n", index);
			}
		}
	}
}

void
read_mod_write(int force, int fd, FILE *f)
{
	char line_buf[128]; // the line buffer
	char *s;			// for striping off comments
	int index;			// the register to read
	int and_mask;			// the value's logical AND mask
	int or_mask;			// the value's logical OR mask
	durango_ch7005_io dci;	// ioctl() control block
	uint8 data;			// storage for the register read

	/* prep the ioctl() data block */
	dci.magic = DURANGO_PRIVATE_DATA_MAGIC;
	dci.data = &data;
	/* should we force a write of "unused" registers? */
	dci.count = force ? 0x81 : 1;

	/* read a line from the input stream */
	while (fgets(line_buf, sizeof(line_buf), f))
	{
		/* remove any comments */
		s = strchr(line_buf, '#');
		if (s) *s = '\0';
		/* parse three numbers from the string */
		if (sscanf(line_buf, "%i %i %i", &index, &and_mask, &or_mask) == 3)
		{
			/* a valid index? */
			if ((index >= 0) && (index <= 0x3e))
			{
				status_t result;
				dci.index = (uint8)index;
				/* read the data */
				result = ioctl(fd, DURANGO_READ_CH7005, &dci, sizeof(dci));
				if (result == B_OK)
				{
					/* save the read value */
					int orig_data = (int)data;
					/* store the data to write */
					data &= (uint8)and_mask;
					data |= (uint8)or_mask;
					/* call the driver */
					result = ioctl(fd, DURANGO_WRITE_CH7005, &dci, sizeof(dci));
					if (result == B_OK)
					{
						fprintf(stdout, "read_mod_write: index %3d (0x%.2x) read %3d (0x%.2x) wrote %3d (0x%.2x)\n",
							index, index, orig_data, orig_data, data, data);
					}
					else
					{
						fprintf(stderr, "read_mod_write: failed writing index %3d (0x%.2x) with reason %s\n",
							index, index, strerror(errno));
					}
				}
				else
				{
					fprintf(stderr, "read_mod_write: failed read of index %3d (0x%.2x) with reason %s\n",
						index, index, strerror(errno));							
				}
			}
			else
			{
				fprintf(stderr, "write_regs: index %3d out of range\n", index);
			}
		}
	}
}

int
main(int argc, char **argv)
{
	FILE *input = stdin;
	void (*process)(int, int, FILE *) = 0;
	int force = 0;
	int fd;
	
	/* open the device first */
	fd = open("/dev/graphics/durango", B_READ_WRITE);
	if (fd >= 0)
	{
		int i;
		/* process args */
		for (i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "force") == 0)
			{
				force = 1;
			}
			else if (strcmp(argv[i], "read") == 0)
			{
				process = read_regs;
			}
			else if (strcmp(argv[i], "write") == 0)
			{
				process = write_regs;
			}
			else if (strcmp(argv[i], "rmw") == 0)
			{
				process = read_mod_write;
			}
			else if (strcmp(argv[i], "-") == 0)
			{
				input = stdin;
			}
			else
			{
				/* a source file */
				if (input && (input != stdin))
				{
					fclose(input);
					input = 0;
				}
				input = fopen(argv[i], "r");
				if (!input)
				{
					fprintf(stderr, "Failed to open %s with reason %s\n",
						argv[i], strerror(errno));
				}
			}
		}
		if (process && input)
		{
			(*process)(force, fd, input);
		}
		else
		{
			fprintf(stderr, "Missing command or input file.\n"
				"Usage: %s [force] command path/to/input/file\n"
				"  where 'force' ensures the driver reads and/or writes the registers\n"
				"    even if its internal table marks the register as 'UNUSED'\n"
				"  where command is one of the following\n"
				"    read - read specified registers.  Input file contains lines with\n"
				"           a single value per line indicating the index of the CH7005\n"
				"           register to read.\n\n"
				"   write - write specified registers.  Input file contains lines with\n"
				"           two values per line indicating the index of the CH7005 register\n"
				"           to write and the value to write to the register.\n\n"
				"     rmw - read-modify-write specified registers.  Input file contains\n"
				"           lines with three values: index, logical-AND mask, and logical-OR\n"
				"           mask.  See the source code for details.\n",
				argv[0]);
		}
	}
	else
	{
		fprintf(stderr, "Couldn't open /dev/graphics/durango with reason %s\n",
			strerror(errno));
	}
	return 0;
}