#ifndef UNCRUSH_H
#define UNCRUSH_H

#include "elf.h"
#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

status_t open_celf_file(int fd, void **cookie, void *dict, size_t dict_size);
status_t close_celf_file(void *cookie);
status_t get_celf_elf_header(void *cookie, Elf32_Ehdr *elfheader);
status_t get_celf_program_headers(void *cookie, Elf32_Phdr *pheaders);
status_t get_celf_section_headers(void *cookie, Elf32_Shdr *sheaders);

/* read all sections that belong to the given segment */
status_t get_celf_segment(void *cookie, int segnum, void *segdata);

/* read section */ 
status_t get_celf_section(void *cookie, int section_num, uint8 *sectiondata, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif

