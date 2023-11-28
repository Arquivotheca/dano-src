/* :ts=8 bk=0
 *
 * tags.h:	Definitions for TagArg structures and routines.
 *
 * Leo L. Schwab					1999·03·03
 */
#ifndef	_TAGS_H
#define	_TAGS_H

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * Structure definitions.
 */
#ifdef	IM_FEELING_CREATIVE_TODAY

/*
 * A new way of defining the TarArg structure.  I have no idea if it will
 * work.
 */
typedef struct BTagArg {
	uint32	ta_Tag;
	union {
		uint32	i;
		float32	f;
		void	*p;
	} ta_Arg;
} BTagArg;

#else	/*  Play it safe  */

typedef struct BTagArg {
	uint32	ta_Tag;
	uint32	ta_Arg;
} BTagArg;

#endif


/*****************************************************************************
 * Standard Tag values.
 * Tags 0 - B_TAG_USERSTART-1 are reserved by the system.
 */
#define	B_TAG_END	0	/*  Terminates TagArg array.		   */
#define	B_TAG_JUMP	1	/*  Jump to new TagArg array; no return    */
#define	B_TAG_GOSUB	2	/*  Process a sub-list			   */
#define	B_TAG_USERSTART	256	/*  First available tag for user programs  */


/*****************************************************************************
 * Prototypes for TagArg-processing routines.
 */
typedef status_t (*Btagproc_callback)(void *object,
				     void *data,
				     uint32 tag,
				     uint32 arg);

extern status_t BTagProcessor (void *object,
			       struct BTagArg *args,
			       Btagproc_callback tag_func,
			       void *data);


#ifdef __cplusplus
}
#endif

#endif	/*  _TAGS_H  */
