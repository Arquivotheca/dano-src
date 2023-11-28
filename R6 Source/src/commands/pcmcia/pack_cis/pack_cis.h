/*
 * pack_cis.h 1.3 1999/07/20 16:04:46
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License. 
 *
 * The initial developer of the original code is David A. Hinds
 * <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
 * are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
 */
typedef struct tuple_info_t {
    u_char		type;
    cisparse_t		*parse;
    struct tuple_info_t	*next;
} tuple_info_t;

extern tuple_info_t *cis_root, *mfc[8];
extern int nf;

void parse_cis(FILE *f);
