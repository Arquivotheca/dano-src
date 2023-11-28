/* Handle symbol and library versioning.
   Copyright (C) 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

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

#include <elf.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <elf/ldsodefs.h>
#include <stdio-common/_itoa.h>

#include <assert.h>


#define VERSTAG(tag)	(DT_NUM + DT_PROCNUM + DT_VERSIONTAGIDX (tag))


#define make_string(string, rest...) \
  ({									      \
    const char *all[] = { string, ## rest };				      \
    size_t len, cnt;							      \
    char *result, *cp;							      \
									      \
    len = 1;								      \
    for (cnt = 0; cnt < sizeof (all) / sizeof (all[0]); ++cnt)		      \
      len += strlen (all[cnt]);						      \
									      \
    cp = result = alloca (len);						      \
    for (cnt = 0; cnt < sizeof (all) / sizeof (all[0]); ++cnt)		      \
      cp = __stpcpy (cp, all[cnt]);					      \
									      \
    result;								      \
  })


static inline struct link_map *
find_needed (const char *name, struct link_map *map)
{
  struct link_map *tmap;
  unsigned int n;

  for (tmap = _dl_loaded; tmap != NULL; tmap = tmap->l_next)
    if (_dl_name_match_p (name, tmap))
      return tmap;

  /* The required object is not in the global scope, look to see if it is
     a dependency of the current object.  */
  for (n = 0; n < map->l_nsearchlist; n++)
    if (_dl_name_match_p (name, map->l_searchlist[n]))
      return map->l_searchlist[n];

  /* Should never happen.  */
  return NULL;
}


static int
internal_function
match_symbol (const char *name, ElfW(Word) hash, const char *string,
	      struct link_map *map, int verbose, int weak)
{
  const char *strtab = (const char *) (map->l_addr
				       + map->l_info[DT_STRTAB]->d_un.d_ptr);
  ElfW(Addr) def_offset;
  ElfW(Verdef) *def;

  /* Display information about what we are doing while debugging.  */
  if (_dl_debug_versions)
    _dl_debug_message (1, "checking for version `", string, "' in file ",
		       map->l_name[0] ? map->l_name : _dl_argv[0],
		       " required by file ", name, "\n", NULL);

  if (map->l_info[VERSTAG (DT_VERDEF)] == NULL)
    {
      /* The file has no symbol versioning.  I.e., the dependent
	 object was linked against another version of this file.  We
	 only print a message if verbose output is requested.  */
      if (verbose)
	_dl_signal_error (0, map->l_name, make_string ("\
no version information available (required by ",
						       name, ")"));
      return 0;
    }

  def_offset = map->l_info[VERSTAG (DT_VERDEF)]->d_un.d_ptr;
  assert (def_offset != 0);

  def = (ElfW(Verdef) *) ((char *) map->l_addr + def_offset);
  while (1)
    {
      /* Currently the version number of the definition entry is 1.
	 Make sure all we see is this version.  */
      if (def->vd_version  != 1)
	{
	  char buf[20];
	  buf[sizeof (buf) - 1] = '\0';
	  _dl_signal_error (0, map->l_name,
			    make_string ("unsupported version ",
					 _itoa_word (def->vd_version,
						     &buf[sizeof (buf) - 1],
						     10, 0),
					 " of Verdef record"));
	  return 1;
	}

      /* Compare the hash values.  */
      if (hash == def->vd_hash)
	{
	  ElfW(Verdaux) *aux = (ElfW(Verdaux) *) ((char *) def + def->vd_aux);

	  /* To be safe, compare the string as well.  */
	  if (strcmp (string, strtab + aux->vda_name) == 0)
	    /* Bingo!  */
	    return 0;
	}

      /* If no more definitions we failed to find what we want.  */
      if (def->vd_next == 0)
	break;

      /* Next definition.  */
      def = (ElfW(Verdef) *) ((char *) def + def->vd_next);
    }

  /* Symbol not found.  If it was a weak reference it is not fatal.  */
  if (weak)
    {
      if (verbose)
	_dl_signal_error (0, map->l_name,
			  make_string ("weak version `", string,
				       "' not found (required by ", name,
				       ")"));
      return 0;
    }

  _dl_signal_error (0, map->l_name,
		    make_string ("version `", string,
				 "' not found (required by ", name, ")"));
  return 1;
}


int
internal_function
_dl_check_map_versions (struct link_map *map, int verbose)
{
  int result = 0;
  const char *strtab;
  /* Pointer to section with needed versions.  */
  ElfW(Dyn) *dyn;
  /* Pointer to dynamic section with definitions.  */
  ElfW(Dyn) *def;
  /* We need to find out which is the highest version index used
    in a dependecy.  */
  unsigned int ndx_high = 0;

  /* If we don't have a string table, we must be ok.  */
  if (map->l_info[DT_STRTAB] == NULL)
    return 0;
  strtab = (const char *) (map->l_addr + map->l_info[DT_STRTAB]->d_un.d_ptr);

  dyn = map->l_info[VERSTAG (DT_VERNEED)];
  def = map->l_info[VERSTAG (DT_VERDEF)];

  if (dyn != NULL)
    {
      /* This file requires special versions from its dependencies.  */
      ElfW(Verneed) *ent = (ElfW(Verneed) *) (map->l_addr + dyn->d_un.d_ptr);

      /* Currently the version number of the needed entry is 1.
	 Make sure all we see is this version.  */
      if (ent->vn_version  != 1)
	{
	  char buf[20];
	  buf[sizeof (buf) - 1] = '\0';
	  _dl_signal_error (0, (*map->l_name ? map->l_name : _dl_argv[0]),
			    make_string ("unsupported version ",
					 _itoa_word (ent->vn_version,
						     &buf[sizeof (buf) - 1],
						     10, 0),
					 " of Verneed record\n"));
	  return 1;
	}

      while (1)
	{
	  ElfW(Vernaux) *aux;
	  struct link_map *needed = find_needed (strtab + ent->vn_file, map);

	  /* If NEEDED is NULL this means a dependency was not found
	     and no stub entry was created.  This should never happen.  */
	  assert (needed != NULL);

	  /* NEEDED is the map for the file we need.  Now look for the
	     dependency symbols.  */
	  aux = (ElfW(Vernaux) *) ((char *) ent + ent->vn_aux);
	  while (1)
	    {
	      /* Match the symbol.  */
	      result |= match_symbol ((*map->l_name
				       ? map->l_name : _dl_argv[0]),
				      aux->vna_hash,
				      strtab + aux->vna_name,
				      needed, verbose,
				      aux->vna_flags & VER_FLG_WEAK);

	      /* Compare the version index.  */
	      if ((unsigned int) (aux->vna_other & 0x7fff) > ndx_high)
		ndx_high = aux->vna_other & 0x7fff;

	      if (aux->vna_next == 0)
		/* No more symbols.  */
		break;

	      /* Next symbol.  */
	      aux = (ElfW(Vernaux) *) ((char *) aux + aux->vna_next);
	    }

	  if (ent->vn_next == 0)
	    /* No more dependencies.  */
	    break;

	  /* Next dependency.  */
	  ent = (ElfW(Verneed) *) ((char *) ent + ent->vn_next);
	}
    }

  /* We also must store the names of the defined versions.  Determine
     the maximum index here as well.

     XXX We could avoid the loop by just taking the number of definitions
     as an upper bound of new indeces.  */
  if (def != NULL)
    {
      ElfW(Verdef) *ent;
      ent = (ElfW(Verdef) *) (map->l_addr + def->d_un.d_ptr);
      while (1)
	{
	  if ((unsigned int) (ent->vd_ndx & 0x7fff) > ndx_high)
	    ndx_high = ent->vd_ndx & 0x7fff;

	  if (ent->vd_next == 0)
	    /* No more definitions.  */
	    break;

	  ent = (ElfW(Verdef) *) ((char *) ent + ent->vd_next);
	}
    }

  if (ndx_high > 0)
    {
      /* Now we are ready to build the array with the version names
	 which can be indexed by the version index in the VERSYM
	 section.  */
      map->l_versions = (struct r_found_version *)
	calloc (ndx_high + 1, sizeof (*map->l_versions));
      if (map->l_versions == NULL)
	{
	  _dl_signal_error (ENOMEM, (*map->l_name ? map->l_name : _dl_argv[0]),
			    "cannot allocate version reference table");
	  result = 1;
	}
      else
	{
	  /* Store the number of available symbols.  */
	  map->l_nversions = ndx_high + 1;

	  /* Compute the pointer to the version symbols.  */
	  map->l_versyms = ((void *) map->l_addr
			    + map->l_info[VERSTAG (DT_VERSYM)]->d_un.d_ptr);

	  if (dyn != NULL)
	    {
	      ElfW(Verneed) *ent;
	      ent = (ElfW(Verneed) *) (map->l_addr + dyn->d_un.d_ptr);
	      while (1)
		{
		  ElfW(Vernaux) *aux;
		  aux = (ElfW(Vernaux) *) ((char *) ent + ent->vn_aux);
		  while (1)
		    {
		      ElfW(Half) ndx = aux->vna_other & 0x7fff;
		      map->l_versions[ndx].hash = aux->vna_hash;
		      map->l_versions[ndx].hidden = aux->vna_other & 0x8000;
		      map->l_versions[ndx].name = &strtab[aux->vna_name];
		      map->l_versions[ndx].filename = &strtab[ent->vn_file];

		      if (aux->vna_next == 0)
			/* No more symbols.  */
			break;

		      /* Advance to next symbol.  */
		      aux = (ElfW(Vernaux) *) ((char *) aux + aux->vna_next);
		    }

		  if (ent->vn_next == 0)
		    /* No more dependencies.  */
		    break;

		  /* Advance to next dependency.  */
		  ent = (ElfW(Verneed) *) ((char *) ent + ent->vn_next);
		}
	    }

	  /* And insert the defined versions.  */
	  if (def != NULL)
	    {
	      ElfW(Verdef) *ent;
	      ent = (ElfW(Verdef)  *) (map->l_addr + def->d_un.d_ptr);
	      while (1)
		{
		  ElfW(Verdaux) *aux;
		  aux = (ElfW(Verdaux) *) ((char *) ent + ent->vd_aux);

		  if ((ent->vd_flags & VER_FLG_BASE) == 0)
		    {
		      /* The name of the base version should not be
			 available for matching a versioned symbol.  */
		      ElfW(Half) ndx = ent->vd_ndx & 0x7fff;
		      map->l_versions[ndx].hash = ent->vd_hash;
		      map->l_versions[ndx].name = &strtab[aux->vda_name];
		      map->l_versions[ndx].filename = NULL;
		    }

		  if (ent->vd_next == 0)
		    /* No more definitions.  */
		    break;

		  ent = (ElfW(Verdef) *) ((char *) ent + ent->vd_next);
		}
	    }
	}
    }

  return result;
}


int
internal_function
_dl_check_all_versions (struct link_map *map, int verbose)
{
  struct link_map *l;
  int result = 0;

  for (l = map; l != NULL; l = l->l_next)
    result |= l->l_opencount != 0 && _dl_check_map_versions (l, verbose);

  return result;
}
