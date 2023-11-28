/*
 * cardmgr.h 1.30 1999/07/20 16:02:24
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

#define MAX_SOCKS	8
#define MAX_BINDINGS	4
#define MAX_MODULES	4

typedef struct adjust_list_t {
    adjust_t		adj;
    struct adjust_list_t *next;
} adjust_list_t;

typedef struct func_ident_t {
    u_char		funcid;
} func_ident_t;

typedef struct manfid_ident_t {
    u_short		manf;
    u_short		card;
} manfid_ident_t;

typedef struct vers_ident_t {
    int			ns;
    char		*pi[4];
} vers_ident_t;

typedef struct tuple_ident_t {
    cisdata_t		code;
    long		ofs;
    char		*info;
} tuple_ident_t;

typedef struct device_info_t {
    dev_info_t		dev_info;
    int			needs_mtd;
    int			modules;
    char		*module[MAX_MODULES];
    char		*opts[MAX_MODULES];
    char		*class;
    struct device_info_t *next;
} device_info_t;

typedef struct card_info_t {
    char		*name;
    enum {
	VERS_1_IDENT=1, MANFID_IDENT, TUPLE_IDENT, FUNC_IDENT,
	BLANK_IDENT, CARDBUS_IDENT
    } ident_type;
    union {
	vers_ident_t	vers;
	manfid_ident_t	manfid;
	tuple_ident_t	tuple;
	func_ident_t	func;
    } id;
    int			bindings;
    device_info_t	*device[MAX_BINDINGS];
    int			dev_fn[MAX_BINDINGS];
    char		*cis_file;
    struct card_info_t	*next;
} card_info_t;

typedef struct mtd_ident_t {
    char		*name;
    enum {
	JEDEC_MTD=1, DTYPE_MTD, DEFAULT_MTD
    } mtd_type;
    int			dtype, jedec_mfr, jedec_info;
    char		*module, *opts;
    struct mtd_ident_t	*next;
} mtd_ident_t;
    
extern adjust_list_t	*root_adjust;
extern device_info_t	*root_device;
extern card_info_t	*blank_card;
extern card_info_t	*root_card, *root_func;
extern mtd_ident_t	*root_mtd, *default_mtd;

int parse_configfile(char *fn);
