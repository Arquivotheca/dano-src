/* getsysstats - Determine various system internal values, stub version.
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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

#include <errno.h>
#include <sys/sysinfo.h>

int
__get_nprocs_conf ()
{
  /* We don't know how to determine the number.  Simply return always 1.  */
  return 1;
}
weak_alias (__get_nprocs_conf, get_nprocs_conf)

link_warning (get_nprocs_conf, "warning: get_nprocs_conf will always return 1")



int
__get_nprocs ()
{
  /* We don't know how to determine the number.  Simply return always 1.  */
  return 1;
}
weak_alias (__get_nprocs, get_nprocs)

link_warning (get_nprocs, "warning: get_nprocs will always return 1")


int
__get_phys_pages ()
{
  /* We have no general way to determine this value.  */
  __set_errno (ENOSYS);
  return -1;
}
weak_alias (__get_phys_pages, get_phys_pages)

stub_warning (get_phys_pages)


int
__get_avphys_pages ()
{
  /* We have no general way to determine this value.  */
  __set_errno (ENOSYS);
  return -1;
}
weak_alias (__get_avphys_pages, get_avphys_pages)

stub_warning (get_avphys_pages)
#include <stub-tag.h>
