#if defined (__POWERPC__) && !defined (__ELF__)	/* FIXME: Should probably be some PEF or Metrowerks specific define */

#include <SupportDefs.h>
#include <image.h>

extern char __code_start__[];				/*	(defined by linker)	*/
extern char	__code_end__[];					/*	(defined by linker)	*/
extern char __data_start__[];				/*	(defined by linker)	*/
extern char __data_end__[];					/*	(defined by linker)	*/
extern char __exception_table_start__[];	/*	(defined by linker)	*/
extern char __exception_table_end__[];		/*	(defined by linker)	*/


static int	fragmentID;

static __asm char *__RTOC(void)
{
		mr		r3,RTOC
		blr
}

void
_init_routine_(image_id imid)
{
	status_t	err;
	void		(*f)(void);

	err = get_image_symbol(imid, B_INIT_BEFORE_FUNCTION_NAME, B_SYMBOL_TYPE_ANY, (void **) &f);
	if (err == B_OK)
		(*f)();

	fragmentID = __register_fragment(__code_start__, __code_end__,
									__data_start__, __data_end__,
									__exception_table_start__, __exception_table_end__,
									__RTOC());
	__sinit();

	err = get_image_symbol(imid, B_INIT_AFTER_FUNCTION_NAME, B_SYMBOL_TYPE_ANY, (void **) &f);
	if (err == B_OK)
		(*f)();
}

void
_term_routine_(image_id imid)
{
	status_t	err;
	void		(*f)(void);

	err = get_image_symbol(imid, B_TERM_BEFORE_FUNCTION_NAME, B_SYMBOL_TYPE_ANY, (void **) &f);
	if (err == B_OK)
		(*f)();

	__destroy_global_chain();
	__unregister_fragment(fragmentID);

	err = get_image_symbol(imid, B_TERM_AFTER_FUNCTION_NAME, B_SYMBOL_TYPE_ANY, (void **) &f);
	if (err == B_OK)
		(*f)();
}

#endif

#if __ELF__

#include <image.h>

void
_init_one(image_id imid)
{
	status_t	err;
	void		(*f)(void);

	err = get_image_symbol(imid, B_INIT_BEFORE_FUNCTION_NAME, B_SYMBOL_TYPE_ANY, (void **) &f);
	if (err == B_OK)
		(*f)();
}

void
_init_two(image_id imid)
{
	status_t	err;
	void		(*f)(void);

	err = get_image_symbol(imid, B_INIT_AFTER_FUNCTION_NAME, B_SYMBOL_TYPE_ANY, (void **) &f);
	if (err == B_OK)
		(*f)();
}

void
_fini_one(image_id imid)
{
	status_t	err;
	void		(*f)(void);

	err = get_image_symbol(imid, B_TERM_BEFORE_FUNCTION_NAME, B_SYMBOL_TYPE_ANY, (void **) &f);
	if (err == B_OK)
		(*f)();
}

void
_fini_two(image_id imid)
{
	status_t	err;
	void		(*f)(void);

	err = get_image_symbol(imid, B_TERM_AFTER_FUNCTION_NAME, B_SYMBOL_TYPE_ANY, (void **) &f);
	if (err == B_OK)
		(*f)();
}

#endif
