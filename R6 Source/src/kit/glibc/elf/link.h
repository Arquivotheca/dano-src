/* Data structure for communication from the run-time dynamic linker for
   loaded ELF shared objects.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef	_LINK_H
#define	_LINK_H	1

#include <features.h>
#include <elf.h>
#include <dlfcn.h>

/* We use this macro to refer to ELF types independent of the native wordsize.
   `ElfW(TYPE)' is used in place of `Elf32_TYPE' or `Elf64_TYPE'.  */
#define ElfW(type)	_ElfW (Elf, __ELF_NATIVE_CLASS, type)
#define _ElfW(e,w,t)	_ElfW_1 (e, w, _##t)
#define _ElfW_1(e,w,t)	e##w##t

#include <bits/elfclass.h>		/* Defines __ELF_NATIVE_CLASS.  */

/* Rendezvous structure used by the run-time dynamic linker to communicate
   details of shared object loading to the debugger.  If the executable's
   dynamic section has a DT_DEBUG element, the run-time linker sets that
   element's value to the address where this structure can be found.  */

struct r_debug
  {
    int r_version;		/* Version number for this protocol.  */

    struct link_map *r_map;	/* Head of the chain of loaded objects.  */

    /* This is the address of a function internal to the run-time linker,
       that will always be called when the linker begins to map in a
       library or unmap it, and again when the mapping change is complete.
       The debugger can set a breakpoint at this address if it wants to
       notice shared object mapping changes.  */
    ElfW(Addr) r_brk;
    enum
      {
	/* This state value describes the mapping change taking place when
	   the `r_brk' address is called.  */
	RT_CONSISTENT,		/* Mapping change is complete.  */
	RT_ADD,			/* Beginning to add a new object.  */
	RT_DELETE		/* Beginning to remove an object mapping.  */
      } r_state;

    ElfW(Addr) r_ldbase;	/* Base address the linker is loaded at.  */
  };

/* This is the instance of that structure used by the dynamic linker.  */
extern struct r_debug _r_debug;

/* This symbol refers to the "dynamic structure" in the `.dynamic' section
   of whatever module refers to `_DYNAMIC'.  So, to find its own
   `struct r_debug', a program could do:
     for (dyn = _DYNAMIC; dyn->d_tag != DT_NULL)
       if (dyn->d_tag == DT_DEBUG) r_debug = (struct r_debug) dyn->d_un.d_ptr;
   */
extern ElfW(Dyn) _DYNAMIC[];


/* Some internal data structures of the dynamic linker used in the
   linker map.  We only provide forward declarations.  */
struct libname_list;
struct r_found_version;
struct r_search_path_elem;


/* Structure describing a loaded shared object.  The `l_next' and `l_prev'
   members form a chain of all the shared objects loaded at startup.

   These data structures exist in space used by the run-time dynamic linker;
   modifying them may have disastrous results.

   This data structure might change in future, if necessary.  User-level
   programs must avoid defining objects of this type.  */

struct link_map
  {
    /* These first few members are part of the protocol with the debugger.
       This is the same format used in SVR4.  */

    ElfW(Addr) l_addr;		/* Base address shared object is loaded at.  */
    char *l_name;		/* Absolute file name object was found in.  */
    ElfW(Dyn) *l_ld;		/* Dynamic section of the shared object.  */
    struct link_map *l_next, *l_prev; /* Chain of loaded objects.  */

    /* All following members are internal to the dynamic linker.
       They may change without notice.  */

    struct libname_list *l_libname;
    /* Indexed pointers to dynamic section.
       [0,DT_NUM) are indexed by the processor-independent tags.
       [DT_NUM,DT_NUM+DT_PROCNUM) are indexed by the tag minus DT_LOPROC.
       [DT_NUM+DT_PROCNUM,DT_NUM+DT_PROCNUM+DT_EXTRANUM) are indexed
       by DT_EXTRATAGIDX(tagvalue) and
       [DT_NUM+DT_PROCNUM+DT_VERSIONTAGNUM,
        DT_NUM+DT_PROCNUM+DT_VERSIONTAGNUM+DT_EXTRANUM)
       are indexed by DT_EXTRATAGIDX(tagvalue) (see <elf.h>).  */

    ElfW(Dyn) *l_info[DT_NUM + DT_PROCNUM + DT_VERSIONTAGNUM + DT_EXTRANUM];
    const ElfW(Phdr) *l_phdr;	/* Pointer to program header table in core.  */
    ElfW(Addr) l_entry;		/* Entry point location.  */
    ElfW(Half) l_phnum;		/* Number of program header entries.  */

    /* Array of DT_NEEDED dependencies and their dependencies, in
       dependency order for symbol lookup.  This is null before the
       dependencies have been loaded.  */
    struct link_map **l_searchlist;
    unsigned int l_nsearchlist;

    /* We keep another list in which we keep duplicates.  This is
       needed in _dl_lookup_symbol_skip to implemented RTLD_NEXT.  */
    struct link_map **l_dupsearchlist;
    unsigned int l_ndupsearchlist;

    /* Dependent object that first caused this object to be loaded.  */
    struct link_map *l_loader;

    /* Symbol hash table.  */
    ElfW(Symndx) l_nbuckets;
    const ElfW(Symndx) *l_buckets, *l_chain;

    unsigned int l_opencount;	/* Reference count for dlopen/dlclose.  */
    enum			/* Where this object came from.  */
      {
	lt_executable,		/* The main executable program.  */
	lt_library,		/* Library needed by main executable.  */
	lt_loaded		/* Extra run-time loaded shared object.  */
      } l_type:2;
    unsigned int l_relocated:1;	/* Nonzero if object's relocations done.  */
    unsigned int l_init_called:1; /* Nonzero if DT_INIT function called.  */
    unsigned int l_init_running:1; /* Nonzero while DT_INIT function runs.  */
    unsigned int l_global:1;	/* Nonzero if object in _dl_global_scope.  */
    unsigned int l_reserved:2;	/* Reserved for internal use.  */

    /* Array with version names.  */
    unsigned int l_nversions;
    struct r_found_version *l_versions;

    /* Collected information about own RPATH directories.  */
    struct r_search_path_elem **l_rpath_dirs;

    /* Collected results of relocation while profiling.  */
    ElfW(Addr) *l_reloc_result;

    /* Pointer to the version information if available.  */
    ElfW(Half) *l_versyms;
  };

#endif /* link.h */
