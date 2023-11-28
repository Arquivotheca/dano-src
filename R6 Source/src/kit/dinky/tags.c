/* :ts=8 bk=0
 *
 * tags.c:	Routines to manipulate TagArgs.
 *
 * Leo L. Schwab					1999.03.03
 */
#include <kernel/OS.h>

#include <dinky/tags.h>


status_t
BTagProcessor (
void			*object,
struct BTagArg		*args,
Btagproc_callback	tag_func,
void			*data
)
{
	status_t	err;
	uint32		t;

	err = B_OK;
	while (t = args->ta_Tag) {
		switch (t) {
		case B_TAG_JUMP:
			/*  Jump to new arg.  */
			args = (BTagArg *) args->ta_Arg;
			continue;
		case B_TAG_GOSUB:
			/*  Process sub-list of args; return on B_TAG_END.  */
			err = BTagProcessor (object,
					     (BTagArg *) args->ta_Arg,
					     tag_func,
					     data);
			break;
		default:
			/*  Hand off custom tag to client.  */
			err = (tag_func) (object, data, t, args->ta_Arg);
			break;
		}
		if (err < 0)
			break;
		args++;
	}
	return (err);
}
