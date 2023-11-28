// ===========================================================================
//	PP_Types.h				   ©1993-1996 Metrowerks Inc. All rights reserved.
// ===========================================================================

#ifndef _PP_TYPES_H
#define _PP_TYPES_H

#if defined(__CFM68K__) && !defined(__USING_STATIC_LIBS__)
	#pragma import on
#endif

	// Integer types for when it's important to know the number
	// of bits occupied by the value. These should be changed
	// depending on the compiler.

typedef		signed char		Int8;
typedef		signed short	Int16;
typedef		signed long		Int32;

typedef		unsigned char	Uint8;
typedef		unsigned short	Uint16;
typedef		unsigned long	Uint32;

typedef		Uint16			Char16;

typedef		unsigned char	Uchar;
typedef		const unsigned char *ConstStringPtr;


enum	ETriState {
	triState_Off,
	triState_Latent,
	triState_On
};


typedef		Int32			CommandT;
typedef		Int32			MessageT;

typedef		Int16			ResIDT;
typedef		Int32			PaneIDT;
typedef		Int32			ClassIDT;
typedef		Int32			DataIDT;


#ifndef topLeft
#define topLeft(r)	(((Point *) &(r))[0])
#endif

#ifndef botRight
#define botRight(r)	(((Point *) &(r))[1])
#endif

#if defined(__CFM68K__) && !defined(__USING_STATIC_LIBS__)
	#pragma import reset
#endif

#endif
