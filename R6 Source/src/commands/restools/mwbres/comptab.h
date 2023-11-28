/*	comptab.h	*/

#ifndef _COMPTAB_H
#define _COMPTAB_H


// #include <SupportDefs.h>
#if !defined(_SUPPORT_DEFS_H)
typedef long int32;
typedef unsigned long uint32;
typedef long status_t;
#endif

#include "filechain.h"

#define YYSTYPE	union parsetype*

#if defined(__cplusplus)
#define EX extern //"C"
#else
#define EX extern
#endif

#define BLKFACTOR 4096

enum {
	knothing = 30000,
	kcstring,
	kcnumeric,
	kcname,
	kcinclude,
	kcdefine,
	kcundef,
	kcifdef,
	kcelse,
	kcendif,
	kcenum,
	kcresource,
	kcdata,
	kcpadinfo
#if BMESSAGE
	,kcmessage,
	kcpoint,
	kcrect
#endif
};

/* The type field of a struct cstring object will be kcstring */

struct cstring {
	int				type;
	char			*string;
};
struct cnumeric {
	int				type;
	int				isinteger;
	union {
		long			intval;
		double			dblval;
	}				value;
};
struct cname {
	int				type;
	char			*string;
};
struct cinclude {
	int				type;
	char			*filename;
	FILE			*file;
};
struct cdefine {
	int				type;
	struct cname	*name;
	YYSTYPE			value;
};
struct cundef {
	int				type;
	struct cname	*name;
};
struct cifdef {
	int				type;
	struct cname	*name;
	int				reverse;
};
struct celse {
	int				type;
};
struct cendif {
	int				type;
};
struct cenum {
	int				type;
	struct cname	*name;
	YYSTYPE			value;
	struct cenum	*next;
};
struct cresource {
	int				type;
	long			restype;
	int32			resid;//int to int32 July 11%%%
	struct cstring	*resname;
	/*
	resdata points to the cdata element of this resource
	*/
	struct cdata	*resdata; 
	/*
	g_res_list is the head of the linked list of cresource 
	elements. The 'next' field below, points to any next element
	of this linked list. Both do_resource and emit_resource may
	access this linked list.
	*/
	struct cresource *next;
};
struct cdata {
	int				type;
	size_t			datasize; 
	size_t			physsize; 
	void			*data;
	int			ispad;
	struct cdata		*next; /* pointer to mext cdata in resource */
};								//in case a pad is seen, we need to pad the 
								//the current cdata block with (appropriate # of) nulls.
								//pad_list == NULL for cdata just before
								//the } of a data_block, as
								//every cdata is part of a data_block. 
								
struct cpadinfo {
	int				type; 
	//size_t 		old_datasz;
								 //sum of all datasizes in this data_block
					   			// before a pad was seen.
	struct cdata 	*current_data_block;//contains the current cdata block to be padded
	struct cnumeric *pad_value;//contains the pad value kcnumeric
};
#if BMESSAGE
struct cmessage {
	int				type;
	BMessage *			message;
};
struct cpoint {
	int				type;
	BPoint				point;
};
struct crect {
	int				type
	BRect				rect;
};
#endif

	/* 
	token specifies which of the possible elements of the union
 	is present in a given instance of the union. For example, 
 	token == kcendif, if the instance of the union contains 
 	struct cendif. Whenever a YYSTYPE is instantiated, the
 	token must also be set appropriately. Each of the structs in the
 	union has a type field. That will be the same as token.
 	*/
union parsetype {
	int					token;
	struct cstring		cstring;
	struct cnumeric		cnumeric;
	struct cname		cname;
	struct cinclude		cinclude;
	struct cdefine		cdefine;
	struct cundef		cundef;
	struct cifdef		cifdef;
	struct celse		celse;
	struct cendif		cendif;
	struct cenum		cenum;
	struct cresource	cresource;
	struct cdata		cdata;
	struct cpadinfo		cpadinfo;
#if BMESSAGE
	struct cmessage	cmessage;
#endif
};


EX YYSTYPE do_include(YYSTYPE);
EX YYSTYPE do_define(YYSTYPE);
EX YYSTYPE do_undef(YYSTYPE);
EX YYSTYPE do_ifdef(YYSTYPE);
EX YYSTYPE do_else(YYSTYPE);
EX YYSTYPE do_endif(YYSTYPE);
EX YYSTYPE do_resource(YYSTYPE);
EX YYSTYPE do_enum(YYSTYPE);
EX YYSTYPE do_read(YYSTYPE);
EX YYSTYPE do_import(YYSTYPE, YYSTYPE, YYSTYPE);
EX YYSTYPE do_pad(YYSTYPE);
EX YYSTYPE	make_data_block(YYSTYPE made_data_items);
EX YYSTYPE do_pad_zeroes(struct cpadinfo *);
EX void pad_merge(	struct cdata* item,	int padsize );
EX void data_merge(struct cdata *item1, struct cdata *item2);
//int dupresource(long restype,	int32 resid, BResources resources);

EX YYSTYPE make_string(const char *string, const char *terminators);
EX YYSTYPE make_numeric(const char *string, int isinteger);
EX YYSTYPE make_numeric_char(const char *string);
EX YYSTYPE make_name(const char *string);
EX YYSTYPE make_include(struct cstring *filename);
EX YYSTYPE make_define(struct cname *name, YYSTYPE value);
EX YYSTYPE make_undef(struct cname *name);
EX YYSTYPE make_ifdef(struct cname *name, int reverse);
EX YYSTYPE make_else();
EX YYSTYPE make_endif();
EX YYSTYPE make_resource(YYSTYPE type, YYSTYPE id, YYSTYPE name);
EX YYSTYPE add_to_resource(struct cresource *cresource, struct cdata *data);
EX YYSTYPE add_to_data(struct cdata *data, YYSTYPE value);
EX YYSTYPE make_enum(struct cname *name, YYSTYPE value);
EX YYSTYPE add_to_enum(struct cenum *base, struct cenum *more);
EX YYSTYPE make_data_items(YYSTYPE data_item, YYSTYPE data_items);
EX YYSTYPE make_cdata(YYSTYPE cwhatever);
EX YYSTYPE real_mdi(YYSTYPE data_item, YYSTYPE data_items);
EX YYSTYPE pad_seen(YYSTYPE padd_item);
EX void ok_test();
#if BMESSAGE
EX YYSTYPE flatten_message(YYSTYPE message);
EX YYSTYPE make_message(YYSTYPE what);
EX void add_message_data(YYSTYPE type, YYSTYPE name, YYSTYPE value);
#endif

EX int is_pad( YYSTYPE data_item);
EX size_t bigger(size_t, size_t);
EX void add_to_g_res_list(struct cresource* cres);
EX char *numeric2strm (struct cnumeric * num);
EX int numericsize(struct cnumeric * num);
EX void dcr_pad_seen();
EX void incr_pad_seen();
EX int incr_g_ok();
EX int dcr_g_ok();

#if BMESSAGE
extern BMessage * g_cur_message;
#endif

enum {
	name_is_nothing,
	name_is_integer,
	name_is_float,
	name_is_string
};

	/* 
	The name_symbol struct is for an entry into the symbol table
   	g_symbol_table.	Insertion is at head of symbol table 
   	(linked list presently).
	
   	The prio field represents priority assigned to that instance of
   	that name.
   
   	type can be any of the 4 listed in the above enum. 
   	*/
struct name_symbol {
	int					type;
	char				*name;
	int					prio;
	struct name_symbol	*next;
	union {
		long				intval;
		double				floatval;
		char				*stringval;
	}					value;
};

enum {	
/*	for "prio"	*/
	prio_define,
	prio_enum
};

EX struct name_symbol *add_name_int(struct cname *name, long intval, int prio);
EX struct name_symbol *add_name_float(struct cname *name, double fltval, int prio);
EX struct name_symbol *add_name_string(struct cname *name, const char *stringval, int prio);
EX int remove_name(struct cname *name, int prio);

/*	uses #define precedence	*/
EX struct name_symbol *find_name(struct cname *name, int required);	

/*	uses #define precedence	*/
EX YYSTYPE get_name_value(struct cname *name);
EX int emit_resources( char *filename);
EX void free_item(void *item);
EX YYSTYPE alloc_node();
EX void panic(const char *fmt, ...);


#endif /* _COMPTAB_H */
