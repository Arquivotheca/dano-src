/*	comptab.c	-- files for resource compiler parse tree	*/
/*	Copyright ) 1997 Metrowerks Corporation. All rights reserved.	*/
/*	$Id$	*/

/*
Globals declared in this file are:
static struct enabled_stack *g_enabled_stack
static name_symbol *g_symbol_table
static struct cresource *g_res_list
static int g_ok
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "comptab.h"
#include "yy_funcs.h"
#include "mwbres-parser.h"
//#include <File.h>
//#include <Resources.h>
#include <private/storage/write_res.h>
#include <sys/stat.h>
//#include <ByteOrder.h>
#if !defined(_BYTE_ORDER_H)
#if defined(__POWERPC__)
#define B_BENDIAN_TO_HOST_INT32(x) (x)
#else
#define B_BENDIAN_TO_HOST_INT32(x) _swap_int32(x)
static int _swap_int32(unsigned int i) {
	return ((i>>24)&0xff)|((i>>8)&0xff00)|((i<<8)&0xff0000)|((i<<24)&0xff000000);
}
#endif
#endif
#include <fcntl.h>
#include <unistd.h>

//debug switches used by deepa
//#define ddbug 1
//#define SetToddbug 1
//#define pad_ddbug 1
//#define mdi_ddbug 1
//#define ddbug_en 1
//#define dbg_resdata_non_z_1
//#define dbg_resdata_non_z_2

//g_ok is a global variable that's incremented by 1 when yyerror is called and 
//we want to continue i.e, it's used to prevent a legitimate action 
//(return of NULL from do_import and make_data_items) from causing a panic 
//(in add_to_resource.). If g_ok > 0, it's legitimate.

//g_ok is basically a 'token' or 'permission' to continue normally.

//g_ok is decremented each time such a 'permission' is used up...So if g_ok < 0
//something went wrong somewhere and we are 
//guaranteed that, for example, (!newdata) will invoke a panic, in
//add_to_resource
static int g_ok = 0;

//defined in mwbres.l and hold the saved yyin, whenever asked to, by record_yyin,
//that's called in yy_include_file, that's called in do_include
extern FILE *g_saved_yyin;
extern FILE *yyin;

/*
merge flag
*/
int mergeflag;


#if DEBUG_MEM
/*	
GUARD_VALUE is used by the malloc() wrapper functions	
If you read outside your block, it will be detected in some cases.
0xdddddddd cannot be accessed. Odd address fundas. 
*/
    
#define GUARD_VALUE 0xdddddddd



struct malloc_guard {
	const char		*expression;
	const char		*file;
	int				line;
	int				guard;
};

typedef struct malloc_guard malloc_guard;

static char *min_malloc = (char *)0xffffffffUL;
static char *max_malloc = (char *)0;
static const char *min_literal = (char *)0xffffffffUL;
static const char *max_literal = (char *)0x0;

static int
ptr_is_ok(
	const void	*ptr) {
	
	if ((char *)ptr >= min_literal && (char *)ptr <= max_literal)
		return 1;
	if ((char *)ptr > max_malloc)
		return 0;
	if ((char *)ptr < min_malloc)
		return 0;
	return 1;	/*	probably OK pointer value	*/
}

static void *
blk_alloc(
	int			size,
	const char	*size_expr,
	const char	*file,
	int			line)
{	
	void *ret = malloc(size+sizeof(malloc_guard));
	malloc_guard guard = { size_expr, file, line, GUARD_VALUE };
	*(malloc_guard *)ret = guard;
	ret = &((malloc_guard *)ret)[1];
	if ((char *)ret < min_malloc)
		min_malloc = (char *)ret;
	if ((char *)ret > max_malloc)
		max_malloc = (char *)ret;
	if (size_expr < min_literal) min_literal = size_expr;
	if (size_expr > max_literal) max_literal = size_expr;
	if (file < min_literal) min_literal = file;
	if (file > max_literal) max_literal = file;
	return ret;
}

static void *
blk_calloc(
	int			size,
	const char	*size_expr,
	const char	*file,
	int			line)
{	
	void *ret = blk_alloc(size, size_expr, file, line);
	if (ret != NULL) {
		memset(ret, 0, size);
	}
	return ret;
}

static void
blk_free(
	void		*block,
	const char	*block_expr,
	const char	*file,
	int			line)
{ 
	if (!block)
		return;	/*	ok to free NULL	*/
	malloc_guard *ptr = (malloc_guard *)block;
	if (!ptr_is_ok(ptr)) {
		panic("bad pointer passed to free()\n%s %d free(%s)", file, line, block_expr);
	}
	ptr--;
	if (ptr->guard != GUARD_VALUE)	{
		const char *pfile = ptr_is_ok(ptr->file) ? ptr->file : "<BAD>";
		const char *pexpr = ptr_is_ok(ptr->expression) ? ptr->expression : "<BAD>";

		// dump the block
		fprintf(stderr, "%08x:  ");
		for (int ix=0; ix<sizeof(malloc_guard); ix++) {
			fprintf(stderr, "%02x", ((unsigned char *)ptr)[ix]);
			if ((ix & 3) == 3) fprintf(stderr, " ");
		}
		fprintf(stderr, "  ");
		for (int ix=0; ix<sizeof(malloc_guard); ix++) {
			unsigned char ch = ((unsigned char *)ptr)[ix];
			if (ch < 32 || ch > 127) ch = '.';
			fprintf(stderr, "%c", ch);
			if ((ix & 3) == 3) fprintf(stderr, " ");
		}
		fprintf(stderr, "\n");

			panic("memory guard overwritten\nallocated %s %d: malloc(%s)\nfreed %s %d:free(%s)",
			pfile, ptr->line, 
			pexpr, 
		    file, line, block_expr);
	}
	free(ptr);
}

static char *
blk_strdup(
	const char *	str,
	const char *	size_expr,
	const char *	file,
	int				line) {
	
	int len = 0;
	if (!str)
		return NULL;
	len = strlen(str) + 1;		// bds
	char *ret = (char *)blk_alloc(len, size_expr, file, line);
	if (ret)
		strcpy(ret, str);
	return ret;
}

#define malloc(x)	blk_alloc(x, #x, __FILE__, __LINE__)
#define calloc(x,y)	blk_calloc((x)*(y), #x "," #y, __FILE__, __LINE__)
#define free(x)		blk_free(x, #x, __FILE__, __LINE__)
#define strdup(x)	blk_strdup(x, #x, __FILE__, __LINE__)

#endif /* DEBUG_MEM */

struct enabled_stack
{
	struct enabled_stack	*next;
	int						root_enabled;
	int						leaf_enabled;
	int						seen_else;
	
};


// g_enabled_stack is a global stack. If root_enabled is non-zero then, the answer to

// "Before this ifdef was seen, was the code enabled?" is YES. If root_enabled is 0, then
// leaf_enabled has to be 0 as well. If leaf_enabled is 1, then the answer to "Is the code
// from this point on, enabled?" is YES.

// g_enabled_stack always holds info pertaining to present scope.
// g_enabled_stack->next is about the immediate outer scope
// g_enabled_stack ==  NULL means not   inside an ifdef.
static struct enabled_stack *g_enabled_stack = NULL;

static int
is_enabled()
{
	return !g_enabled_stack || g_enabled_stack->leaf_enabled;
}

//Will enable the code in the ifdef, until an endif is seen
//or an else is seen, whichever happens first.
static int
en_ifdef(
	int		if_value)
{
	struct enabled_stack *newstack = (struct enabled_stack *)malloc(sizeof(struct enabled_stack));
	
	/*Just before en_ifdef was called, what was is_enabled() status?*/
	newstack->root_enabled = is_enabled();
	
	newstack->leaf_enabled = if_value && newstack->root_enabled;
	newstack->seen_else = 0;
	newstack->next = g_enabled_stack;
	g_enabled_stack = newstack;
	return 1;
}

static int
en_else()
{
	#ifdef ddbug_en
	printf("Inside en_else\n");
	#endif
	
	if (!g_enabled_stack || g_enabled_stack->seen_else)
		return 0;
	if (g_enabled_stack->root_enabled)
	{
		g_enabled_stack->leaf_enabled = !g_enabled_stack->leaf_enabled;
	}
	g_enabled_stack->seen_else = 1;
	return 1;
}

static int
en_endif() 				//If new ifdef seen, en_ifdef will be called...	
{

	#ifdef ddbug_en
	printf("Inside en_endif\n");
	#endif
	
	struct enabled_stack *top = g_enabled_stack;
	if (!g_enabled_stack)
		return 0;
	
	g_enabled_stack = top->next;//rm the top of the stack, as an endif has been seen.
						//We are going to the scope above.
	free(top);
	return 1;
}

void
panic(
	const char *	format,
	...)
{
	va_list li;
	fprintf(stderr, "### PANIC ###\nFile %s; Line %d # ",
		yy_current_file_name(), yy_current_file_line());
	va_start(li, format);
	vfprintf(stderr, format, li);
	va_end(li);
	fprintf(stderr, "\n");
	fflush(stderr);
#if DEBUG_MEM
	abort();
#endif
	exit(1);
}

static int
is_value_type(
	int		type)
{
	return (type == kcstring) || (type == kcnumeric) || (type == kcname);
}


/*	
Right now we only do a linked list. At some point, we'll make it
a hash table.%%%
*/

static name_symbol *g_symbol_table = NULL;

name_symbol *
add_name_int(
	struct cname	*name,
	long			value,
	int				prio)
{
	#ifdef ddbug
	printf("Inside add_name_int\n");
	#endif
	
	name_symbol *sym = (name_symbol *)malloc(sizeof(name_symbol));
	if (!is_enabled())
	{
		panic("add_name_int when not enabled");
	}
	if (!name || !name->string)
	{
		panic("no name in add_name_int");
	}
	if (!sym)
	{
		panic("out of memory in add_name_int");
	}
	sym->type = name_is_integer;
	sym->name = strdup(name->string);
	if (!sym->name)
	{
		panic("out of memory in add_name_int");
	}
	sym->value.intval = value;
	sym->prio = prio;
	sym->next = g_symbol_table;
	g_symbol_table = sym;
	return sym;
}

name_symbol *
add_name_float(
	struct cname	*name,
	double			value,
	int				prio)
{
	name_symbol *sym = (name_symbol *)malloc(sizeof(name_symbol));
	if (!is_enabled())
	{
		panic("add_name_float when not enabled");
	}
	if (!name || !name->string)
	{
		panic("no name in add_name_float");
	}
	if (!sym)
	{
		panic("out of memory in add_name_float");
	}
	sym->type = name_is_float;
	sym->name = strdup(name->string);
	if (!sym->name)
	{
		panic("out of memory in add_name_float");
	}
	sym->value.floatval = value;
	sym->prio = prio;
	sym->next = g_symbol_table;
	g_symbol_table = sym;
	return sym;
}

name_symbol *
add_name_string(
	struct cname	*name,
	const char		*value,
	int				prio)
{
	name_symbol *sym = (name_symbol *)malloc(sizeof(name_symbol));
	if (!is_enabled())
	{
		panic("add_name_string when not enabled");
	}
	if (!name || !name->string)
	{
		panic("no name in add_name_string");
	}
	if (!sym)
	{
		panic("out of memory in add_name_string");
	}
	sym->type = name_is_string;
	sym->name = strdup(name->string);
	if (!sym->name)
	{
		panic("out of memory in add_name_string");
	}
	sym->value.stringval = strdup(value);
	sym->prio = prio;
	sym->next = g_symbol_table;
	g_symbol_table = sym;
	return sym;
}

int
remove_name(
	struct cname	*name,
	int				prio)
{
	struct name_symbol **nsptr = &g_symbol_table;
	int found = 0;
	if (!is_enabled())
	{
		panic("remove_name when not enabled");
	}
	if (!name || name->type != kcname || !name->string)
	{
		panic("not a cname in remove_name");
	}
	while (*nsptr)
	{
		if (!strcmp((*nsptr)->name, name->string) && ((*nsptr)->prio == prio))
		{
			name_symbol *ptr = *nsptr;
			*nsptr = (*nsptr)->next;
			free(ptr->name);
			if (ptr->type == name_is_string)
			{
				free(ptr->value.stringval);
			}
			free(ptr);
			found = 1;
			break;
		}
		nsptr = &(*nsptr)->next;
	}
	return found;
}

//	find lowest-numbered item, which means highest priority	
struct name_symbol *
find_name(
	struct cname	*name,
	int required)
{
	struct name_symbol *nsptr = g_symbol_table;
	struct name_symbol *found = NULL;
	if (!name || name->type != kcname || !name->string)
	{
		panic("not a cname in find_name");
	}
	while (nsptr)
	{
		if (!strcmp(nsptr->name, name->string))
		{
			if (!found || ( found && (nsptr->prio < found->prio)))
			{
				found = nsptr;
			}
		}
		nsptr = nsptr->next;
	}
	if (!found && required) {
		char msg[300];
		sprintf(msg, "unknown identifier '%s'", name->string);
		yyerror(msg);
	}
		
	return found;
}


//find the value of a given name, if it's present in symbol table
//already. If absent in symbol table, return NULL.
YYSTYPE
get_name_value(
	struct cname	*name)
{
	struct name_symbol *found;
	
	if (!name || (name && (name->type != kcname)) || (!name && !name->string))/*%%%D name && */
		{
			panic ("not a cname in get_name_value");
		}//if
		
	found = find_name(name, 1);
		
		if (!found) {
			return NULL;
			}
	
	struct cnumeric *ret = NULL;
	struct cstring *rets = NULL;

	switch (found->type)
	{
		case name_is_integer: 
		#ifdef ddbug
		printf("Inside case name_is_integer in get_name_value\n");
		#endif
					ret = &alloc_node()->cnumeric;
					ret->type = kcnumeric;
					ret->isinteger = 1;
					ret->value.intval = found->value.intval;
					break;
		case name_is_float:	
					ret = &alloc_node()->cnumeric;
					ret->type = kcnumeric;
					ret->isinteger = 0;
					ret->value.dblval =  found->value.floatval;
					break;
		case name_is_string:
		#ifdef ddbug
		printf("Inside case name_is_string in get_name_value\n");
		#endif
				    rets = &alloc_node()->cstring;
					rets->type = kcstring;
					rets->string = strdup(found->value.stringval);
					break;
		case name_is_nothing:
					panic (" name has no value is get_name_value");
	}

	YYSTYPE retyys = (YYSTYPE)ret;
	YYSTYPE retSyys = (YYSTYPE)rets;
		
	( ret ) ? (retyys->token = kcnumeric) : (retSyys->token = kcstring);
	return ( ret ) ?  retyys :  retSyys;
					
}

YYSTYPE
alloc_node()
{
	return (YYSTYPE) calloc(sizeof(union parsetype), 1);
}

void
free_item(
	void *	item)
{
	#ifdef ddbug
	printf("Inside start free_item\n");
	#endif
	
	if (!item)
		return;
	
	YYSTYPE yys = (YYSTYPE)item;
	
	#ifdef ddbug
	printf("TOKEN NOW IS %d \n", yys->token);
	#endif
	
	switch (yys->token)
	{
	case knothing:
		break;
	case kcstring:
		free(((struct cstring *)item)->string);
		break;
	case kcnumeric:
		#ifdef ddbug
		printf("Inside free_item's kcnumeric to break\n");
		#endif
		
		break;
	case kcname:
	
		#ifdef ddbug
		printf("Inside free_item's kcname\n");
		#endif
		
		free(((struct cname *)item)->string);
		break;
	case kcinclude:
	
		#ifdef ddbug
		printf("Inside free_item's kcinclude\n");
		#endif
		
		free(((struct cinclude *)item)->filename);
		/*	the file is closed by the flex scanner...	*/
		
		#ifdef ddbug
		printf("Just leaving free_item's kcinclude\n");
		#endif
		
		break;
	case kcdefine:
		
		#ifdef ddbug
		printf("Inside free_item's kcdefine\n");			  
		#endif
		
		free_item(((struct cdefine *)item)->name);
		
		#ifdef ddbug
		printf("Back from free_item1 i.e name and inside kcdfine\n");
		#endif
		
		free_item(((struct cdefine *)item)->value);
		
		#ifdef ddbug
		printf("Back from free_item2 i.e value and inside kcdefine to break\n");
		#endif
		
		break;
	case kcundef:
		free_item(((struct cundef *)item)->name);
		break;
	case kcifdef:
		free_item(((struct cifdef *)item)->name);
		break;
	case kcelse:
		break;
	case kcendif:
		break;
	case kcenum:
		{	
			#ifdef ddbug
			printf("Just inside case kcenum of free_item\n");
			#endif
			
			struct cenum *ptr = &yys->cenum;
			while (ptr)
			{
				free_item(ptr->name);
				free_item(ptr->value);
				ptr = ptr->next;
			}
		}
		break;
	case kcresource:
		free_item(((struct cresource *)item)->resname);
		
		#ifdef ddbug
		printf("Back from free_item of resname is kcresource\n");
		#endif
		
		free_item(((struct cresource *)item)->resdata);
		
		#ifdef ddbug
		printf("Back from free_item of resdata is case kcresource\n");
		#endif
		
		break;
	case kcdata:
		
		#ifdef ddbug
		printf("INSIDE kcdata free_item\n");
		#endif
		
		free(((struct cdata *)item)->data);
		
		#ifdef ddbug
		printf("Back from free of data in case kcdata\n");
		#endif
		
		break;
	default:
		panic("bad struct type %d in free_item", yys->token);
		break;
	}
  if (item)
  	free(item);
}

YYSTYPE
make_include(
	struct cstring *filename)
{
	#ifdef ddbug
	printf("Inside make_include\n");
	#endif
	
	if (!filename)
		return NULL;
	if (!is_enabled())
		return NULL;
	if (filename->type != kcstring)
	{
		free_item(filename);
		panic("not a cstring in make_include");
		return NULL;
	}
	char *name = filename->string;
	FILE *f = fopen(name, "r");
	if (!f)
	{
		free_item(filename);
		yyerror("file not found");
		return NULL;
	}
	struct cinclude *ret= &alloc_node()->cinclude;	/*	will not dereference pointer	*/
	if (!ret)
	{
		free_item(filename);
		panic("out of memory in make_include");
		return NULL;
	}
	ret->type = kcinclude;
	ret->filename = strdup(filename->string);
	ret->file = f;
	free_item(filename);
	
	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcinclude;
	return retyys;
}


YYSTYPE
do_include(
	YYSTYPE include)
{
	#ifdef ddbug
	printf("Inside do_include\n");
	#endif
	
	struct cinclude *item = (struct cinclude *)include;
	if (!include)
		return NULL;
	if (!is_enabled())
		return NULL;
	if (item->type != kcinclude)
	{
		free_item(include);
		panic("not a cinclude in do_include");
		return NULL;
	}
	if (item->file)
	{
		#ifdef ddbug
		printf("item->file is %p\n", item->file);
		#endif
		
		yy_include_file(item->file, item->filename);
		item->file = NULL;
	}
	else
	{
		fprintf(stderr, "File '%s'; Line %d # warning: file '%s' not found\n", 
			yy_current_file_name(), yy_current_file_line(), 
			item->filename);
	}
	free_item(include);
	return NULL;
}


YYSTYPE
do_define(
	YYSTYPE define)
{
	struct cdefine *item = (struct cdefine *)define;
	if (!define)
		return NULL;
	if (!is_enabled())
		return NULL;
	if (item->type != kcdefine)
	{
		free_item(define);
		panic("not a cdefine in do_define");
		return NULL;
	}
	if (!item->name || item->name->type != kcname)
	{
		panic("not a cname in do_define");
	}
	if (!item->value)	/*	default 0	*/
	{
		(void)add_name_int(item->name, 0, prio_define);
	}
	else
	{
		YYSTYPE val = (YYSTYPE)((item) ? item->value : NULL); // %%%
		if (val && val->token == kcname)
		{
			val = get_name_value(&val->cname);
		}
		if (!val)
		{
			yyerror("undefined identifier");	/*	here, there is NO default of 0	*/
		}
		else switch (item->value->token)
		{
		case kcnumeric:
			if (item->value->cnumeric.isinteger)
			{
				(void *)add_name_int(item->name,
					item->value->cnumeric.value.intval,
					prio_define);
			}
			else
			{
				(void *)add_name_float(item->name,
					item->value->cnumeric.value.dblval,
					prio_define);
			}
			break;
		case kcstring:
			#ifdef ddbug
			printf("Inside case kcstring in do_define, abto enter add_name_string\n");
			#endif
			(void *)add_name_string(item->name, item->value->cstring.string, prio_define);
			break;
		default:
			panic("unknown value type %d in do_define", item->value->token);
			break;
		}
	}
	
	#ifdef ddbug
	printf("Inside do_define...about to call free_item\n");
	#endif
	
	free_item(item);
	
	#ifdef ddbug
	printf("Have returned from free_item called in do_define\n");
	#endif
	
	return NULL;
}



YYSTYPE
do_undef(
	YYSTYPE		undef)
{
	struct cundef *item = (struct cundef *)undef;
	if (!undef)
		return NULL;
	if (!is_enabled())
		return NULL;
	if (item->type != kcundef)
	{
		free_item(undef);
		panic("not a cundef in do_undef");
		return NULL;
	}
	if (!item->name || item->name->type != kcname)
	{
		panic("not a cname in do_undef");
	}
	if (!remove_name(item->name, prio_define))
	{
		yyerror("name not found");
		return NULL;
	}
	
	#ifdef ddbug
	printf("Inside do_undef\n");
	#endif
	
	free_item(item);
	return NULL;
}


YYSTYPE
do_ifdef(
	YYSTYPE		ifdef)
{
	#ifdef ddbug
	printf("Inside do_ifdef\n");
	#endif
	
	struct cifdef *item = (struct cifdef *)ifdef;
	struct name_symbol *sym = NULL;
	if (!ifdef)
		return NULL;
	if (item->type != kcifdef)
	{
		free_item(ifdef);
		panic("not a cifdef in do_ifdef");
		return NULL;
	}
	if (!item->name || item->name->type != kcname)
	{
		panic("not a cname in do_ifdef");
	}
	sym = find_name(item->name, 0);
	int val = (sym != NULL);
	if (item->reverse) {
		val = !val;
	}
	en_ifdef(val);
	free_item(ifdef);
	return NULL;
}


YYSTYPE
do_else(
	YYSTYPE		ifelse)
{
	struct celse *item = (struct celse *)ifelse;
	if (!ifelse)
		return NULL;
	if (!en_else())
	{
		yyerror("unmatched #else");
	}
	free_item(ifelse);
	return NULL;
}


YYSTYPE
do_endif(
	YYSTYPE		endif)
{
	struct cendif *item = (struct cendif *)endif;
	if (!endif)
		return NULL;
	if (!en_endif())
	{
		yyerror("unmatched #endif");
	}
	free_item(endif);
	return NULL;
}


YYSTYPE
do_enum(
	YYSTYPE		anenum)
{
	#ifdef ddbug
	printf("Just inside do_enum\n");
	#endif
	
	struct cenum *item = (struct cenum *)anenum;
	YYSTYPE val = NULL;
	long value = 0;
	if (!anenum)
		return NULL;
	if (!is_enabled())
		return NULL;
	while (item)
	{
		#ifdef ddbug
		printf("Just inside while of make_enum\n");
		#endif
		
		if (item->type != kcenum)
		{
			panic("not a cenum in do_enum");
		}
		if (!item->name || item->name->type != kcname)
		{
			panic("not a cname in do_enum");	/*	error should have been detected before this	*/
		}
		val = item->value;
		if (val && (val->token != kcnumeric || !val->cnumeric.isinteger))
		{
			panic("value is not integer in do_enum");	/*	error should have been detected before this	*/
		}
		if (val)
		{
			value = val->cnumeric.value.intval;
		}
		add_name_int(item->name, value, prio_enum);
		value++;
		item = item->next;
	}
	
	free_item(anenum);
	
	#ifdef ddbug
	printf("Just exiting make_enum\n");
	#endif
	
	return NULL;
}


YYSTYPE
make_enum(
	struct cname	*name,
	YYSTYPE			val)
{
	//val can be NULL if enum color { red }

	#ifdef ddbug
	printf("Just inside make_enum\n");
	#endif
	
	if (!name || name->type != kcname)
	{
		panic("no name in make_enum");
	}
	if (val && val->token != kcnumeric)
	{
		yyerror("enum value must be integral");
		val = NULL;
	}
	struct cenum *ret = &alloc_node()->cenum;	/*	will not dereference pointer	*/
	if (!ret)
	{
		panic("out of memory in make_enum");
	}
	ret->type = kcenum;
	ret->name = name;
	ret->value = val;
	ret->next = NULL;
	
	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcenum;

	#ifdef ddbug
	printf("Finished make_enum\n");
	#endif
	
	return retyys;
	
}


YYSTYPE
add_to_enum(
	struct cenum	*first,
	struct cenum	*second)
{
	#ifdef ddbug
	printf("Just inside add_to_enum\n");
	#endif
	
	if (!first || !second || first->type != kcenum || second->type != kcenum)
	{
		panic("bad input to add_to_enum");
	}
	if (first->next)
	{
		panic("first enum item already has chain! in add_to_enum");
	}
	first->next = second;
	return (YYSTYPE)first;
}


YYSTYPE
make_string(
	const char		*data,
	const char		*delim)
{
	/* The string data points to looks like <the string> or "the_string".
	   The string delim points to looks like <> or "" */
	
	#ifdef ddbug
	printf("INSIDE !!!!!!!!!!!!!!!!!make_string and the data is '%s'\n", data);
	#endif
	
	int len = 0;
	struct cstring *ret = NULL;
	if (!data || !delim)
	{
		panic("bad data in make_string");
	}
	len = strlen(data);
	ret = &alloc_node()->cstring;	/*	will not actually dereference pointer	*/
	ret->type = kcstring;
	
	/* If len < 2, the 1st argument to make_string is obviously malformed! */
	if (len < 2 || (data[0] != delim[0] || data[len-1] != delim[1]))
	{
		fprintf(stderr, "File '%s'; Line %d # warning: string does not conform to %s delimiters\n", 
			yy_current_file_name(), yy_current_file_line(), delim);
		ret->string = strdup(data);
	}
	else
	{
		ret->string = (char *)malloc(len-1);
		/* Copy only the abcd.h part of "abcd.h" or <abcd.h>, as the case may be */
		memcpy(ret->string, data+1, len-2);
		ret->string[len-2] = 0;
	}

	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcstring;
	return retyys;
	
}


YYSTYPE
make_numeric(
	const char		*data,
	int				isinteger)
{	/* The number pointed to by data is present in ASCII form. If isinteger is 1
	the number it represents is an integer, else double (not yet supported here) */

	char * tptr;
	
	#ifdef ddbug
	printf("INSIDE !!!!!!!!!!!!!!!!!make_numeric and the data is '%s'\n",data);
	#endif
	
	int len = 0;
	struct cnumeric *ret = NULL;
	char temp[32];
	if (!data)
	{
		panic("bad data in make_numeric");
	}
	ret = &alloc_node()->cnumeric;	/*	will not actually dereference pointer	*/
	if (!ret)
	{
		panic("out of memory in make_numeric");
	}
	ret->type = kcnumeric;
	ret->isinteger = isinteger;
	if (!isinteger)
	{
		panic("double not yet supported in make_numeric");
	}
	if (data[0] == '0') {
		if ((data[1] == 'x') || (data[1] == 'X')) {
			unsigned long ul = strtoul(data+2, &tptr, 16);
			ret->value.intval = *(long *)&ul;
		}
		else {
			unsigned long ul = strtoul(data, &tptr, 8);
			ret->value.intval = *(long *)&ul;
		}
	}
	else {
		ret->value.intval = strtol(data, &tptr, 10);
	}
	
	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcnumeric;

	#ifdef ddbug
	printf("Inside and at end of make_numeric\n");
	#endif
	
	return retyys;

}


YYSTYPE
make_numeric_char(
	const char		*data)
{
		
	#ifdef ddbug
	printf("INSIDE !!!!!!!!!!!!!!!!!make_numeric_char and data is '%s'\n", data);
	#endif
	
	int len = 0;
	struct cnumeric *ret = NULL;
	if (!data)
	{
		panic("bad data in make_numeric_char");
	}
	len = strlen(data);
	/*
	len < 3 causes error as CHARACTER is forced to be of min
	length 1, and the single quotes add 2 more to len
	*/
	if (len < 3 || (data[0] != '\'' || data[len-1] != '\'') || len > 6)
	{
		yyerror("bad character format");
		return NULL;
	}
	ret = &alloc_node()->cnumeric;	/*	will not actually dereference pointer	*/
	ret->type = kcnumeric;
	ret->isinteger = true;
	ret->value.intval = 0;

	// Copy the bytes big endian and right justified
	memcpy(((char *)(&ret->value.intval)+len-6), data+1, len-2);
	// swap on little endian
	ret->value.intval = B_BENDIAN_TO_HOST_INT32(ret->value.intval);

	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcnumeric;
	return retyys;
	
}

YYSTYPE 
make_define(
	struct cname *name, YYSTYPE value)
{

	if (!name || name->type != kcname)
	{
		panic("no name in make_define");
	}
	struct cdefine *ret = &alloc_node()->cdefine;	/*	will not dereference pointer	*/
	if (!ret)
	{
		panic("out of memory in make_define");
	}
	ret->type = kcdefine;
	ret->name = name;
	ret->value = value;
	
	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcdefine;

	#ifdef ddbug
	printf("Inside make_define\n");
	#endif
	
	return retyys;
	
}

YYSTYPE
make_name(
	const char *string)
{
	if (!string )
	{
		panic("no string passed into make_name");
	}	
	
	#ifdef ddbug
	printf("Inside make name and string is '%s'\n", string);
	#endif
	
	struct cname *ret = &alloc_node()->cname;	/*	will not dereference pointer	*/
	if (!ret)
	{
		panic("out of memory in make_name");
	}

	ret->type = kcname;
	/*ret->string = (char *)malloc(strlen(string) + 1);
	strcpy(ret->string, string);
	*/
	ret->string = strdup(string);
	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcname;

	#ifdef ddbug
	printf("Inside and finished make_name\n");
	#endif
	
	return retyys;

}

YYSTYPE
make_else ( )
{
	#ifdef ddbug
	printf("Inside make_else\n");
	#endif
	
	/* make sure that there is an entry in the enable stack */
	if (g_enabled_stack == NULL)
	{
		yyerror("encountered else without an ifdef");
		return NULL;
	}

	struct celse *ret = NULL;
	ret = &alloc_node()->celse;	/*	will not dereference pointer	*/
	if (!ret)
	{
		panic("out of memory in make_else");
	}
	ret->type = kcelse;
	
	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcelse;
	return retyys;
}


YYSTYPE
make_endif ()
{
	/* make sure that there is an entry in the enable stack */
	if (g_enabled_stack == NULL)
	{
		yyerror("encountered endif without an ifdef");
		return NULL;
	}

	struct cendif *ret = NULL;
	ret = &alloc_node()->cendif;	/*	will not dereference pointer	*/
	if (!ret)
	{
		panic("out of memory in make_endif");
	}
	ret->type = kcendif;
	
	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcendif;
	return retyys;
}

YYSTYPE
make_undef (
	struct cname *name)
{
	if (!name || name->type != kcname)
	{
		panic("no name in make_undef");
	}
	struct cundef *ret = &alloc_node()->cundef;	/*	will not dereference pointer*/

	if (!ret)
	{
		panic("out of memory in make_undef");
	}
	ret->type = kcundef;
	ret->name = name;
	
	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcundef;
	return retyys;
	
}

YYSTYPE
make_ifdef (
	struct cname *name,
	int reverse)
{
	if (!name || name->type != kcname)
	{
		panic("no name in make_ifdef");
	}
	struct cifdef *ret = &alloc_node()->cifdef;	/*	will not dereference pointer*/

	if (!ret)
	{
		panic("out of memory in make_ifdef");
	}
	ret->type = kcifdef;
	ret->name = name;
	ret->reverse = reverse;

	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcifdef;
	return retyys;
	
}

/* 
g_res_list is the head of the linked list of cresource elements.
*/

static struct cresource *g_res_list = NULL;

YYSTYPE
make_resource (
	YYSTYPE type, YYSTYPE id, YYSTYPE name)
{//character_value, integer_value, string_value
	struct cnumeric *ltype = (struct cnumeric *) type;
	struct cnumeric *lid = (struct cnumeric *) id;
	struct cstring *lname = (struct cstring *) name;

	/* make sure that the type is a cnumeric and that isinteger is true 
	for it since it either returns make_numeric_char (CHARACTER) or 
	it returns get_name_value which must be an integer for this field in 
	the resource 
	*/
	if (!ltype || (ltype->type != kcnumeric) || (!ltype->isinteger))
		panic("no type in make_resource");

	/* make sure that the id is a cnumeric and it is an integer 
   since either it returns make_numeric (INTEGER) or it returns 
   get_name_value
   which must be an integer for this field in the resource 
	*/	
	if (!lid || (lid->type != kcnumeric) || (!lid->isinteger))
		panic("no id in make_resource");

	/* make sure that the id is a cstring
   since either it returns make_string (QUOTESTRING) or it returns
   get_name_value which must be a cstring for this field in the resource 
	*/	
	if(!lname) printf(" lnam was null!!\n");
	if (!lname || (lname->type != kcstring))
		panic("no name in make_resource");

	struct cresource *ret = &alloc_node()->cresource; /* will not dereference pointer */

	if (!ret)
	{
		panic("out of memory in make_resource");
	}

	ret->type = kcresource;
	ret->restype = ltype->value.intval;
	ret->resid = lid->value.intval;
	ret->resname = lname;
	ret->resdata = NULL; 

	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcresource;
	return retyys;
}


// do_pad  is called to setup a cdata as a pad
YYSTYPE
do_pad(
	YYSTYPE pad_item )
{
	//pad_item will create YYSTYPE of the token kcpadinfo 
	struct cnumeric *pad = (struct cnumeric *)pad_item; 

	struct cdata *ret = &alloc_node()->cdata;
	ret->type = kcdata;
	ret->ispad = 1;
	ret->next = NULL;
	ret->datasize = pad->value.intval;
	ret->data = NULL;

	YYSTYPE retyys = (YYSTYPE)ret;
	retyys->token = kcdata;
	return retyys;
}//do_pad

// pad_merge is calld to pad a cdata with a given padsize
void
pad_merge(
	struct cdata* item,
	int padsize )
{
	
	size_t olddatasz = item->datasize;
	
#ifdef pad_ddbug
printf("olddatasz is: %d and item  is %p\n", olddatasz, item);
#endif
	
	if (padsize < 2) {
		yyerror("pad() value must be at least 2");
		return;
	}
		
#if 0
	/*decide new datasize */
	if (olddatasz > padsize)
		{
			
#ifdef pad_ddbug
printf("olddatasz and padsize are: %d and %d\n", olddatasz, padsize);
#endif
			if ((olddatasz%padsize) == 0)//already divisible
				item->datasize = olddatasz; //retain old datasize
			else //not 'already divisible'
				{	
					item->datasize = padsize;
					size_t temporary1 = olddatasz;
					if (olddatasz%padsize != 0) {//if
						while (temporary1%padsize != 0) {//while
							item->datasize = item->datasize + padsize;
							temporary1 = item->datasize;
						}//while
					}//if
				}//else
		}//if
	else //olddatasz <= padsize
		{
			item->datasize = padsize; //new datasize is now divisible with a remaindr of 0, by padsize. 
		}
#else
	item->datasize = item->datasize + (padsize - (item->datasize % padsize))%padsize;
#endif
		
	if (item->datasize >= item->physsize) {
		item->physsize = item->datasize + BLKFACTOR;
		void *new_data = (char *)malloc(item->physsize);
		if (!new_data) panic("out of memory in pad_merge");
		if (item->data) {
			memcpy((char *)new_data, (char *)item->data, olddatasz);//retain actual data
			free(item->data);
		}
		item->data = new_data;
	}
//add correct amount of nulls to the end of the old data.
	size_t i;
	char *temp = (char *)item->data; 
	for(i = olddatasz; i < item->datasize; i++)
		temp[i] = 0;
 
 #ifdef pad_ddbug
 printf("************************padsize is %d\n", padsize);
 printf("old datasize is %d\n", olddatasz);
 printf("************************item->datasize is %d\n", item->datasize);
 #endif
 
}//pad_merge


// cdata_merge is called to merge a second data item into the first ( second one is not a pad )
void
data_merge(
	struct cdata *item1,
	struct cdata *item2)
{

#ifdef mdi_ddbug
printf("Inside data_merge *#*#*#*#*#\n\n");
#endif
	
#ifdef mdi_ddbug
printf("Inside if (item2) in real_mdi:item1 and item2 are %p and %p*#*#*#*#*#*#*#\n\n", item1, item2);
#endif
						
	int newsize = item1->datasize + item2->datasize;
	if ( newsize > item1->physsize)
	{
		/* grow block */
		int newphys = newsize + BLKFACTOR;
	
		void *newblk = (char *)malloc( (size_t)newphys );
		if (!newblk)
			panic("out of memory in data_merge");
		if (item1->data)
		{
			memcpy(newblk,item1->data,item1->datasize); // copy the items from 
							// item1_data to newblk
			free(item1->data);
		}
		item1->data = newblk;
		item1->physsize = newphys;
	}//if (newsize ...

		/* append data from item2 to item1 */
	memcpy(((char *)(item1->data))+item1->datasize, item2->data, item2->datasize);
	
	item1->datasize = newsize;
	free_item(item2);
	
	
#ifdef ddbug
	printf("About to return from data_merge \n");
#endif
}



//Can be called on any data_items and is called atleast once.
YYSTYPE
make_data_block(
	 YYSTYPE made_data_items
)
{
#ifdef mdi_ddbug
printf("Inside make_data_block\n");
#endif
	

	struct cdata * items = (struct cdata *) made_data_items;

	if (!items) {
		yyerror("No data in resource");
		return NULL;
	}
	//items NULL inside make_data_block

	struct cdata * newcdata = &alloc_node()->cdata;
	newcdata->type = kcdata;
	newcdata->datasize = 0;
	newcdata->physsize = 0;
	newcdata->data = NULL;
	newcdata->next = NULL;
	newcdata->ispad = 0;

	struct cdata *temp = items;

	while (true)
	{
//		fprintf(stderr, "make_data_block(%x)\n", temp);
		if (!temp) break;
		struct cdata * n_temp = temp->next;
		if (temp->ispad) {
			pad_merge(newcdata,temp->datasize);
		}
		else {
			data_merge(newcdata,temp);
		}
		temp = n_temp;
	}
//	fprintf(stderr, "make_data_items -- done\n");

	YYSTYPE retyys = (YYSTYPE)newcdata;
	retyys->token = kcdata;

	return retyys;
}//fun

void
ok_test()
{
	if (g_ok)
		{
			g_ok--;			//used up a g_ok privilege, hence dcr it.
			g_ok++; 		//give a token to g_ok, so add_to_resource will use it
			return;	//don't do make_data_items anymore. This line is executed,
							//for example, when import was unable to find the resource asked for.
		}					
		else//!g_ok
			panic("Bad data_item to make_data_items");
 	return;
}


// { pad(64), a,b,c} is an legal data_block even tho' pad is the leftmost in this data_block.
YYSTYPE
make_data_items(
	 YYSTYPE data_item, YYSTYPE data_items)
{
	
	#ifdef mdi_ddbug
	printf("Inside make_data_items\n");
	#endif
	YYSTYPE retPyys;

	if (!data_item) {
		ok_test();
		return NULL;
	}


	struct cdata *item1 = (struct cdata *) data_item;
	struct cdata *item2 = (struct cdata *) data_items;

//	fprintf(stderr, "make_data_items(%x, %x)\n", item1, item2);

	item1->next = item2;

	retPyys = (YYSTYPE) item1;
	if (retPyys->token != kcdata) {
		fprintf(stderr, "token kind is %d\n", retPyys->token);
		panic("trying to concatenate non-data in make_data_items()");
	}
//	retPyys->token = kcdata;

	return retPyys;
}//fun


int 
numericsize(
	struct cnumeric * num)
{
	if (!num)
		panic("num not valid in numericsize");

	#ifdef ddbug
	printf("size of long is %d",sizeof(long));	
	#endif
	
	return ( num->isinteger ? sizeof(long) : sizeof(double));
}

static void *
numeric_ptr(
	struct cnumeric * num)
{
	if (num->isinteger) {
		return &num->value.intval;
	}
	return &num->value.dblval;
}

YYSTYPE
make_cdata(
	YYSTYPE cwhatever)
	/*cwhatever can have token == kcnumeric or kcstring*/
{	
	#ifdef ddbug
	printf("_______________Inside make_cdata\n");
	#endif
	
	struct cdata *ret = &alloc_node()->cdata;	/*	will not dereference pointer	*/
	YYSTYPE retyys;
	
	if(!cwhatever)
	{
		panic("Argument to make_cdata was NULL!!");
	}
		
	
	ret->type = kcdata;

	switch (cwhatever->token)
	{
	case kcnumeric:{ /* make_numeric/ make_numeric_char/ get_name_value */
		
		struct cnumeric *tnum = (struct cnumeric *)cwhatever;

		ret->datasize = numericsize(tnum); /* sizeof(long) or sizeof(double) */
		ret->physsize = ret->datasize + 64; /* size of allocated block */
		
		ret->data = (void *)malloc(ret->physsize);
		if ( !ret->data) {
			panic("out of memory in make_cdata (numeric)");
		}
		
		memcpy((char *)ret->data, numeric_ptr(tnum), ret->datasize);
		
		} break;
	
	case kcstring:{ /* make_string / get_name_value */

        		struct cstring * tstr = ( struct cstring * ) cwhatever;
        		int len = strlen(tstr->string);

		ret->datasize = len + 1;
        		ret->physsize = len + 1 + 64;

        		ret->data = (char *) malloc ( ret->physsize );
				if (!ret->data) {
					panic("out of memory in make_cdata (string)");
				}
        		memcpy ((char *)ret->data, tstr->string, len);
				*((char *)ret->data + len) = 0;
 
        		} break;               

	default: 
		panic("Illegal data to make_cdata");
	}
	
	#ifdef ddbug
	printf("__________________Inside and at end of make_cdata\n");
	#endif
	
	ret->next = NULL;
	ret->ispad = 0;
			
	retyys = (YYSTYPE)ret;
	retyys->token = kcdata;	
	return retyys;
}



YYSTYPE
add_to_resource(
	struct cresource *res, struct cdata *new_data)
{

	#ifdef ddbug
	printf("g_ok is ggggggggggggggggggggggggg %d\n", g_ok);
	#endif
	
	if ( !res || res->type != kcresource )
	{
		panic("bad input to add_to_resource");
	}
	
	if (!new_data)
	{
		if (!g_ok)//g_ok is null iff panic isto be called
				  //i.e if g_ok has not been given a token...
			panic("bad second argument to add_to_resource");
	}
	
	if (new_data)
		if ( new_data->type != kcdata)
		{
			panic("bad type in 2nd argument to add_to_resource");
		}
	
	#ifdef ddbug
	printf("res is ****************%p\n", res);
	#endif
	
	res->resdata = new_data;
	
	#ifdef ddbug
		printf("Exiting add_to_resource\n");
	#endif

	return (YYSTYPE)res;

}

YYSTYPE
do_resource(
	YYSTYPE res)
{
	/*
	do_resource's argument was returned by add_to_resource.
	Now do_resource adds this incoming resource to the global
	linked list of cresources called g_res_list.
	*/
	if (!res)
		return NULL;
	if (!is_enabled())
		return NULL;
	struct cresource *cres = (struct cresource *)res;
	add_to_g_res_list(cres);
//	free_item(res);
	return NULL;

}

void
add_to_g_res_list(
	struct cresource *cres)
{
	if (!cres) 
		panic("bad arg to add_to_g_res_list");
	
	/*
	Insertion is at head of g_res_list
	*/
//	fprintf(stderr, "adding block: %08x ('%c%c%c%c' %d)\n", cres,
//		cres->restype>>24, cres->restype>>16, cres->restype>>8, cres->restype,
//		cres->resid);
	cres->next = g_res_list;
	g_res_list = cres;
}

YYSTYPE
do_read(
	YYSTYPE file_name)
{
	/*
	make_string called when file_name seen in .y file.
	Hence file_name here looks like abcd.h ...no stripping req.d
	*/
	if (!file_name)
		return NULL;
	if (!is_enabled())
		return NULL;
	
	struct cdata *ret = &alloc_node()->cdata;	/*	will not dereference pointer	*/
	struct cstring *file_name_cstring = (struct cstring *)file_name;
	FILE *to_read = fopen(file_name_cstring->string,"r" ); 
	
	if (!to_read ) {
		char msg[1050];
		free(ret);
		sprintf(msg, "cannot open '%s'\n", file_name_cstring->string);
		yyerror(msg);
		return NULL;
	} 
		
	/*	
	struct stat *file_info = (struct stat *)malloc(sizeof(struct stat));
	if (!stat(file_name_cstring->string, file_info))
		panic("Could not determine size of file\n");
	*/
	fseek(to_read, 0, SEEK_END);
	long physize = ftell(to_read);
	rewind(to_read);
			
	char *file_contents = (char *)malloc( physize + BLKFACTOR);
	if (!file_contents) {
		panic("Out of memory in do_read\n");
	}
	
	fread(file_contents, physize, 1, to_read);

        ret->data = file_contents;

	ret->datasize = physize;
	ret->physsize = physize + BLKFACTOR; 

        fclose(to_read);

        ret->next = NULL;
        ret->ispad = 0;

        YYSTYPE retyys = (YYSTYPE)ret;
        retyys->token = kcdata;
        return retyys;
} 

//#define imp_dbg 1

YYSTYPE
do_import(
	 YYSTYPE filename,
	 YYSTYPE character_value,
	 YYSTYPE num_or_string)
 {	//do_import
	if (!filename)
		return NULL;
	if (!is_enabled())
		return NULL;

 struct cdata *ret = &alloc_node()->cdata;
 struct cstring *filenm = (struct cstring *)filename;
 
 struct cnumeric *char_val;
 switch (character_value->token)
 	 {
 	 	case kcnumeric:
 	 	char_val = (struct cnumeric *)character_value;
  		break;
  		
  		case kcstring: {
  		#ifdef ddbug
  		printf("\n\n\n***Inside case kcstring for character_value...in import\n\n\n");
  		#endif
  		panic("error: Identifier to 2nd argument to import is of type string"); 		
 		} break;
 	
		case kcname:{ //A kcnumeric passed as 2nd arg to import, as an identifier. 
		YYSTYPE the_identifier = get_name_value((struct cname *)character_value);
		if (!the_identifier)
			panic("Second argument to import command is an unknown identifier.");
		if (the_identifier->token != kcnumeric)
			panic("Second argument to import command is a bad identifier.");
		else
			char_val = (struct cnumeric *)the_identifier;		
		} break;
 		
 		default:
 		panic("Bad second arg to import");
 		break;
 	}
 
 char *filenam = filenm->string;
 int flags = O_RDONLY;	
// BFile output(filenam, flags);


#if 1
	int fd = open(filenam, flags);
	res_map * rsrcs = NULL;
	int endian;

	if (fd < 0) {
		yyerror("Can't find input file in import");
		return NULL;
	}
	if (position_at_map(fd, 0, &endian) < 0) {
		yyerror("Input resource file has no resource map in import");
		close(fd);
		return NULL;
	}
	if (read_resource_file(&rsrcs, fd, endian, NULL) < 0) {
		yyerror("Input resource file is bad in import");
		close(fd);
		return NULL;
	}
	close(fd);	/* we loaded the resources; don't need the file anymore */
	YYSTYPE specifier;
	if (num_or_string->token == kcname) {
		specifier = get_name_value((struct cname *)num_or_string);
	}
	else {
		specifier = num_or_string;
	}
	switch (num_or_string->token) {
	case kcnumeric: {
			int id = ((struct cnumeric *)specifier)->value.intval;
			int size = 0;
			const void * resdata = find_resource_by_id(rsrcs, char_val->value.intval, id, &size, NULL);
			if (!resdata) {
				g_ok++;
				yyerror("resource not found");
				dispose_resource_map(rsrcs);
				return NULL;
			}
			ret->data = malloc(size);
			memcpy(ret->data, resdata, size);
			ret->datasize = size;
			ret->physsize = size;
			ret->type = kcdata;
		}
		break;
	case kcstring: {
			const char * name = ((struct cstring *)specifier)->string;
			int size = 0;
			const void * resdata = find_resource_by_name(rsrcs, char_val->value.intval, name, &size, NULL);
			if (!resdata) {
				g_ok++;
				yyerror("resource not found");
				dispose_resource_map(rsrcs);
				return NULL;
			}
			ret->data = malloc(size);
			memcpy(ret->data, resdata, size);
			ret->datasize = size;
			ret->physsize = size;
			ret->type = kcdata;
		}
		break;
	default:
		yyerror("illegal resource specifier");
		dispose_resource_map(rsrcs);
		return NULL;
	}
	dispose_resource_map(rsrcs);
#else
  if (!output.InitCheck()) 
  {  //Init
	BResources Bobject;

	#ifdef ddbug
	printf("Init suceeded and About to enter SetTo...\n");
	#endif

	size_t temp_char_val = char_val->value.intval;	
	
	#ifdef imp_dbg
	printf("temp_char_val is %d\n", temp_char_val);
	#endif
	
	size_t datasz; 									
	
  	#ifdef SetToddbug
	printf("%p is returned by Bobject.SetTo(&input)\n",Bobject.SetTo(&output) );
	#endif
	
	if ( !Bobject.SetTo(&output))
		{//SetTo
		
			YYSTYPE tmp = alloc_node();
			
			if ((num_or_string->token) == kcname)
			{//if kcname
				#ifdef ddbug
				printf("Inside if kcname in do_import\n");
				#endif
				
				struct cname *tmpname = (struct cname *)num_or_string;
				
				tmp = get_name_value(tmpname);
				
				#ifdef ddbug
				printf(" exited get_name_value in go_import and tmp is %p", tmp);
				printf(" and tmp->token is is %d\n", tmp->token);
				#endif
			}
			else
			{
				#ifdef ddbug
				printf("Inside else in do_import\n");
				#endif
				
				tmp = num_or_string;
			}
			switch	(tmp->token)
				
				{//switch
				case kcnumeric:
				
				struct cnumeric *cnum = (struct cnumeric *)tmp;
				size_t temp_num = cnum->value.intval;
				
				#ifdef ddbug
				printf(" just b4 hasnresource temp_num is %d\n", temp_num);
				#endif
				
				int hasnresource = Bobject.HasResource(temp_char_val, (size_t)temp_num);
				
				if (!hasnresource)
				{
					g_ok++;//give a g_ok privilege...make_data_items will need it now!
					printf("In an import command requesting resource of type %d and id , on ", temp_char_val, (size_t)temp_num );
					yyerror(" : the resource to be imported was not found");
					return NULL;
				}
				else
				{
					ret->data = Bobject.FindResource( temp_char_val, (size_t)temp_num, &datasz);
				
					#ifdef ddbug
					printf("Find returned  %p and datasz is %d\n", ret->data, datasz);
					#endif
				
					ret->datasize = datasz;
					ret->physsize = datasz;
					ret->type = kcdata;
				}//else hasresource
				break;
				
				case kcstring:
				
				#ifdef ddbug
				printf("Inside string part of do_import\n");
				#endif
				
				struct cstring *cstr = (struct cstring *)num_or_string;  
				
				char *temp_string = cstr->string;	
				
				int hasresource = Bobject.HasResource(temp_char_val, temp_string);
				
				#ifdef ddbug
				printf("hasresource value is %d\n", hasresource);
				#endif
				
				if (!hasresource)
				{
					g_ok++;//give a g_ok privilege...make_data_items will need it now!
					printf("In an import command requesting resource of type %d and name '%s', on\n", temp_char_val, temp_string );
					yyerror(" : resource to be imported is not found");
					return NULL;
				}
				else
				{				
					ret->data = Bobject.FindResource( temp_char_val, temp_string, &datasz);
				
					ret->datasize = datasz;
					ret->physsize = datasz;
					ret->type = kcdata;
				
				#ifdef ddbug
				printf(" Find returned  %p and datasz is %d\n", ret->data, datasz);
				#endif
				
				}//else hasnresource
				break;
				
				default:
				
				#ifdef ddbug
				printf("4th argument to do_import was illegal\n");
				#endif
				
				panic("Illegal third argument to import");		
				break;
				}//switch 

		}//SetTo
		else
			panic("SetTo failed in do_import");
 }//Init
	else
		panic("InitCheck failed in do_import");
#endif
		//memory leaks take care yet%%%
    
 ret->ispad = 0;
 ret->next = NULL;

 YYSTYPE retyys = (YYSTYPE)ret;
 if (!retyys)
 	 retyys->token = kcdata;
 	 
 #ifdef ddbug
 printf("about to exit do_import\n");
 #endif
 
 return retyys;
 }//do_import		


#if BMESSAGE
YYSTYPE flatten_message(YYSTYPE message)
{
	if (message->token != kcmessage) {
		panic("non-message passed to flatten_message()!\n");
	}
	if (!message->cmessage.message) {
		panic("NULL message passed to flatten_message()!\n");
	}
	struct cdata * data = (struct cdata *)malloc(sizeof(struct cdata));
	data->type = kcdata;
	data->datasize = message->cmessage.message->FlattenedSize()+100;
	data->physsize = data->datasize;
	data->data = (char *)malloc(data->physsize);
	data->ispad = 0;
	data->next = NULL;
	if (!data->data) {
		panic("out of memory in flatten_message()\n");
	}
	status_t err = message->cmessage.message->Flatten(data->data, data->physsize);
	if (err < B_OK) {
		char msg[300];
		sprintf(msg, "flatten_message() error %x (%s)\n", err, strerror(err));
		yyerror(err);
	}
	return message;
}

YYSTYPE make_message(YYSTYPE what)
{
	if (what->token != cnumeric) {
		yyerror("message() requires integer/character constant\n");
		g_cur_message = NULL;
		return NULL;
	}
	struct cmessage * msg = (struct cmessage *)malloc(sizeof(cmessage));
	msg->type = kcmessage;
	msg->message = new BMessage(((struct cnumeric *)what)->val.intval);
	g_cur_message = msg->message;
	return msg;
}

void add_message_data(YYSTYPE type, YYSTYPE name, YYSTYPE value)
{
	if (!g_cur_message) {
		return;	/* no message there */
	}
	if (type->token != cnumeric) {
		yyerror("message data type requires integer/character constant\n");
		return;
	}
	if (name->token != cstring) {
		yyerror("message data name requires quoted string\n");
		return;
	}
	void * data = NULL;
	size_t size = 0;
	switch (value->what) {
	case kcnumeric:
		ASSERT(type->cnumeric.isinteger != 0);
		data = &value->cnumeric.val;
		size = 4;
		break;
	case kcstring:
		data = value->cstring.string;
		size = strlen(data)+1;
		break;
	case kcpoint:
		data = &value->cpoint.point;
		size = sizeof(BPoint);
		break;
	case kcstring:
		data = &value->crect.rect;
		size = sizeof(BRect);
		break;
	case kcdata:
		data = value->cdata.data;
		size = value->cdata.datasize;
		break;
	default:
		data = NULL;
		size = 0;
		break;
	}
	if (data != NULL) {
		status_t err = g_cur_message->AddData(name->cstring.string, type->cnumeric.val.intval, data, size, false);
		if (err != B_OK) {
			char msg[300];
			sprintf(msg, "error %x in BMessage::AddData() (%s)\n", err, strerror(err));
			yyerror(err);
		}
	}
	else {
		panic("NULL data in add_message_data()!\n");
	}
}
#endif



#if 0
/* dupresource  checks if the specified resource exists already in the resource file or has been previously been added in this file*/
int
dupresource(
	long restype,
	int32 resid,
BResources resources)
{

	if (mergeflag && resources.HasResource(restype,resid))
		return 1;
	
	struct cresource * tmpres;

	for( tmpres = g_res_list; tmpres != NULL; tmpres = tmpres->next)
	{
		if (tmpres->restype == restype && tmpres->resid == resid)
			return 1;
	}
	return 0;
}
#endif


/*mergeflag is dynamically set to 1, if user does a -merge from command line*/
int
emit_resources(
	 char		*filename)
{	
	int done = 0;
	if (!g_res_list){

		#ifdef ddbug
		printf("There ain't no resources. I am returning.\n");
		#endif
		
		return done;
		}
	struct cresource *tempres 	= g_res_list;
	struct cdata *tempdata = tempres ? tempres->resdata : (struct cdata *)NULL;
	int flags = O_RDWR | O_CREAT;

/*
	if (!mergeflag) 
		flags |= O_TRUNC;
*/

	#ifdef ddbug
	printf("mergeflag is %d\n",mergeflag);
	#endif
	
//	BFile output(filename, flags);

//        if (!output.InitCheck()) {  //Init
	int out_fd = open(filename, flags, 0666);
	res_map * map = NULL;
	if (out_fd >= 0) {
//		BResources resources;

		int position_result;
		int endian = 0;
		if ((position_result = position_at_map(out_fd, 1, &endian)) < 0) {
			panic("position_at_map() failed for output file (corrupt?)");
		}

		#ifdef ddbug
		printf("About to enter SetTo in emit_resources...\n");
		#endif
		
		int dont_merge;
		mergeflag ? (dont_merge = false) : (dont_merge = true);
		
//		if (mergeflag) {
//			panic("merge is not implemented\n");
//		}
		if (mergeflag && (position_result > 0)) {	/* if there are resources */
			off_t pos = lseek(out_fd, 0, SEEK_CUR);
			if (read_resource_file(&map, out_fd, endian, NULL) < 0) {
				fprintf(stderr, "warning: -merge specified but destination was not a resource file\n");
			}
			lseek(out_fd, pos, SEEK_SET);
		}
		//if dont_merge is true, SetTo will NOT merge.
//		if ( !resources.SetTo(&output,dont_merge)) {
			while(tempres){//while
		
				#ifdef ddbug_resdata_non_z_1
				printf("Inside while\n");
				printf("restype is %d and resid is %d\n", tempres->restype, (int32)tempres->resid);
				printf("*****and resname's string is '%s'\n",tempres->resname->string);

//				#ifdef dbg_resdata_non_z_2
//				printf("resdata's data is %p\n", tempres->resdata->data);
//				printf("*************resdata's datasize is %p \n",tempres->resdata->datasize); 
//				FILE *f = fopen("/boot/DUMP", "w");
//				if (f) {
//				fwrite(tempres->resdata->data, tempres->resdata->datasize, 1, f);
//				fclose(f);
//				}
//				#endif
				
				#endif
				
				status_t addresource ;
			
				//If in import we ask for a resource that
				//will not be found, then tempres->resdata will be NULL...
				//So, when such an import is present, give yyerror
				//and continue with adding future resources...i.e don't call AddResources when
				//tempres->resdata is null...
				if (tempres->resdata)
			   	 {	
					/**/ //if (!dupresource(tempres->restype, tempres->resid, resources))	
					
//					addresource = resources.AddResource((type_code)tempres->restype, (int32)tempres->resid, tempres->resdata->data, (size_t)tempres->resdata->datasize, tempres->resname->string);
					addresource = add_resource(&map, tempres->restype, tempres->resid, tempres->resdata->data, tempres->resdata->datasize, tempres->resname->string);

					/**/ //else
					/**/	//panic("Re-defining resource type %d , id %d\n", tempres->restype, tempres->resid);
			
				#ifdef ddbug
				printf("AddResource for this resource (whose ->resdata is nonzero) returned %d\n", addresource);
				#endif
			
				//NOTE::: AddResource fails if its 3rd or 4th or 5th argument is zero or ofcourse if
				//any of it's arguments are bad!
					// AddResources has a bug reported in the BeBook.  If you ask it to add
					// a resource that is a duplicate it returns a positive integer that
					// is the number of bytes written
					// some modifications here BDS
#if 0
					if (addresource > 0)
					{
						if (! mergeflag)
						printf("Duplicate resource overwritten, %4s/%d/%s\n", &tempres->restype, tempres->resid, tempres->resname->string);
					}
					else
#endif
					if (addresource < 0)
					{			
						printf("AddResource failed, %.4s/%d/%s\n", (char*) &tempres->restype, tempres->resid, tempres->resname->string);
//						panic("AddResource failed\n");// panic here leaves the rsrc file in an inconsistent state
					}
				 }
				 else //not required. 
					addresource = 0;
				
				 if (!addresource)
				 {
					#ifdef ddbug
					printf("AddResources succeeded ,or, had to be bypassed due to tempres->resdata being null\n");
					#endif
				 }//if !Add
				 tempres = tempres->next;
			 }//while
//		}
//		else
//			panic("resources.SetTo failed in emit_resource\n");

		status_t error = write_resource_file(map, out_fd, endian, NULL);
		if (error < 0) {
			fprintf(stderr, "writing resource file %s failed: %x (%s)\n", filename, error, strerror(error));
		}
	} //InitCheck
	else
		panic("InitCheck failed in emit_resources");

	#ifdef ddbug
	printf("Done emit_resources\n");
	#endif
	
	return done;
}		//emit
