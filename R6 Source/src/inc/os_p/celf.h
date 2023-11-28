#ifndef CELF_H
#define CELF_H

#include <SupportDefs.h>

#define CELF_ID 'LEC\x7f'

typedef struct compressed_elf_header {
	uint32  id;
	char   *elf_entry;
	uint16  elf_type;
	uint8	elf_shstrndx;
	uint8   pheader_num;
	uint8 	sheader_num;
	uint8	plt_index;
	uint8	got_index;
	uint8	dynstr_index;
//	uint32  pheader_disk_size;
//	uint32  sheader_disk_size;
} compressed_elf_header;

//typedef struct compressed_data_header {
//	uint32  number_of_sections;
//	uint32  compressed_size;
//} compressed_data_header;

#endif
