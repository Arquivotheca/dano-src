

/*
 * This is C-source code for the derived variables:
 *	minnow834.dbase1.GTscore_OUT  (type N)
 *
 * Host: hercules  Date: Sat Mar 27 10:25:54 1993
 * Note that this code uses ANSI function prototypes.
 */

#include	<math.h>
#include	<string.h>
#include	<stdio.h>
#include	<stdarg.h>

/*
 * Declarations of functions for the derived variables:
 */
double	Var_GTscore_OUT( void);

/*
 * Declarations of variables referenced by the derived variable expressions:
 */
extern double	Var_mycalah;
extern double	Var_mymoves;
extern double	Var_yourcalah;
extern double	Var_yourdistr;
extern double	Var_mydistl;
extern double	Var_mycaptures;
extern double	Var_yourmoves;
extern int	Var_mustpass;
extern double	Var_yourdistl;
extern double	Var_mydistr;

/*
 * The following declaration may be needed by the user who wishes to set the
 * value of a numeric variable to missing.
 */
#define MIS_NUM		(1.e30)		/* missing numeric value	*/

/*
 * Type and structure for a data value.  If valc == 0, then the value is
 * numeric and in valn; else the value is categorical and in valc.
 */
typedef struct Val {
	double	valn;
	char	*valc;
} Val;

double		Var_GTscore_OUT( void);
int		MissingNumQ( double valn);

static Val	fnMember( Val val, int nargs, ...),
		opGt( Val val1, Val val2),
		valOfBool( int valb),
		valOfCat( char *valc),
		valOfNum( double valn);
static char	*field( char *s, int n),
		*sknsp( char *s),
		*skwsp( char *s);
static double	numOfVal( Val val);
static		boolOfVal( Val val),
		fieldCount( char *s),
		strEq( char *s1, char *s2),
		valEq( Val val1, Val val2);
static void	chkProg( void);


/*
 * int boolOfVal( val) - return result of coercing val to a value of
 * boolean type and extracting the boolean value.
 */
static int
boolOfVal( Val val)
{
	char	*s;

	if (val.valc) {
		val.valn = strtod( val.valc, &s);
		if (fieldCount( val.valc) != 1 ||
					s != sknsp( skwsp( val.valc))) {
			return (0);
		}
	}

	if (MissingNumQ( val.valn))
		return (0);
	else
		return (val.valn != 0.0);
}


/*
 * void chkProg( ) - check certain aspects of program functioning for
 * anomalies that preclude successful execution and take an error exit
 * if any are found.  This version checks for correct operation of the
 * variable args support in stdarg.h, which was found to contain apparent
 * errors in Ultrix 4.1 on the DECstation.  It also verifies that an integer
 * arg to a function whose prototype specifies a double arg is correctly
 * converted to double.
 */
static void
chkProg( void)
{
	static int	first;

	if (! first) {
		first = 1;
		if (! boolOfVal( fnMember( valOfCat( "red"), 3,
				valOfCat( "red"), valOfCat( "green"),
				valOfCat( "blue"))) ||
		    ! boolOfVal( fnMember( valOfCat( "green"), 3,
				valOfCat( "red"), valOfCat( "green"),
				valOfCat( "blue"))) ||
		    ! boolOfVal( fnMember( valOfCat( "blue"), 3,
				valOfCat( "red"), valOfCat( "green"),
				valOfCat( "blue")))) {
			fprintf( stderr, "Fatal error: stdarg.h failure!\n");
			exit( 1);
		}
		if (numOfVal( valOfNum( 5)) != 5.0) {
			fprintf( stderr, "Fatal error: failure to convert int arg to double!\n");
			exit( 1);
		}
	}
}


/*
 * char *field( s, n) - Return a pointer to the n'th white-space separated
 * field of s.  The first field is number 1.  A pointer to the terminating
 * null char is returned if there are fewer than n fields.  NULL is returned
 * if s is NULL.
 */
static char *
field( char *s, int n)
{
	int	f;

	if (! s)
		return (NULL);
	s = skwsp( s);

	for (f = 1; f < n; ++f)
		s = skwsp( sknsp( s));

	return (s);
}


/*
 * int fieldCount( s) - return count of white-space-separated fields in s,
 * or 0 if s == NULL.
 */
static int
fieldCount( char *s)
{
	int	n;

	if (! s)
		return (0);
	for (n = 0; *field( s, 1); s = field( s, 2))
		++n;
	return (n);
}


/*
 * Val fnMember( val, nargs, ...) - return value of expression:
 * member( val, elem1, elem2, .. elemn) - where nargs = n and the variable
 * args ... are elem1, etc.
 */
static Val
fnMember( Val val, int nargs, ...)
{
	va_list args;
	int	iarg;

	va_start( args, nargs);

	for (iarg = 0; iarg < nargs; ++iarg) {
		if (valEq( val, va_arg( args, Val))) {
			va_end( args);
			return (valOfBool( 1));
		}
	}

	va_end( args);
	return (valOfBool( 0));
}


/*
 * double numOfVal( val) - return result of coercing val to a value of
 * numeric type and returning its numeric value.
 */
static double
numOfVal( Val val)
{
	char	*s;

	if (val.valc) {
		val.valn = strtod( val.valc, &s);
		if (fieldCount( val.valc) != 1 ||
					s != sknsp( skwsp( val.valc)))
			 return (MIS_NUM);
	}

	if (MissingNumQ( val.valn))
		return (MIS_NUM);

	return (val.valn);
}


/*
 * Val opGt( val1, val2) - return value of expression: val1 > val2.
 */
static Val
opGt( Val val1, Val val2)
{
	double	valn1,
		valn2;

	valn1 = numOfVal( val1);
	valn2 = numOfVal( val2);
	if (MissingNumQ( valn1) || MissingNumQ( valn2))
		return (valOfBool( 0));
	else
		return (valOfBool( valn1 > valn2));
}


/*
 * char *sknsp( s) - returns a pointer to the first tab, space, return, or
 * null char in the string pointed to by s.  Returns NULL if s is NULL.
 */
static char *
sknsp( char *s)
{
	char	c;

	if (! s)
		return (NULL);
	while ( (c = *s) != ' ' && c != '\t' && c != '\n' && c != '\0')
		s++;
	return (s);
}


/*
 * char *skwsp( s) - returns a pointer to the first non-(tab space return)
 * char in the string pointed to by s.  Returns NULL if s is NULL.
 */
static char *
skwsp( char *s)
{
	char	c;

	if (! s)
		return (NULL);
	while ( (c = *s) == ' ' || c == '\t' || c == '\n')
		s++;
	return (s);
}


/*
 * int strEq( s1, s2) - return 1 if s1 and s2 are both non-NULL and the
 * strings they point to are equal, else 0.
 */
static int
strEq( char *s1, char *s2)
{
	return (s1 && s2 && strcmp( s1, s2) == 0);
}


/*
 * int valEq( val1, val2) - return 1 if val1 and val2 are equal, else 0.
 * String comparison is used if both val1 and val2 are strings; otherwise
 * numeric comparison is used.
 */
static int
valEq( Val val1, Val val2)
{
	double	valn1,
		valn2;

	if (val1.valc && val2.valc) 
		return (strEq( val1.valc, val2.valc));
	else {
		valn1 = numOfVal( val1);
		valn2 = numOfVal( val2);
		if (MissingNumQ( valn1) || MissingNumQ( valn2))
			return (0);
		else
			return (valn1 == valn2);
	}
}


/*
 * Val valOfBool( valb) - return a Val struct of boolean type and value valb,
 * which must be 1 or 0.
 */
static Val
valOfBool( int valb)
{
	Val	val;

	val.valc = NULL;
	val.valn = valb;
	return (val);
}


/*
 * Val valOfCat( valc) - return a Val struct of categorical type and value
 * valc.
 */
static Val
valOfCat( char *valc)
{
	Val	val;

	val.valc = valc;
	return (val);
}


/*
 * Val valOfNum( valn) - return a Val struct of numeric type and value valn.
 * Missing values are standardized.
 */
static Val
valOfNum( double valn)
{
	Val	val;

	val.valc = NULL;
	if (MissingNumQ( valn))
		val.valn = MIS_NUM;
	else
		val.valn = valn;
	return (val);
}


/*
 * int MissingNumQ( valn) - return 1 if valn is a missing numeric value,
 * else 0.
 */
int
MissingNumQ( double valn)
{
	return (valn >= 0.9 * MIS_NUM);
}


/*
 * Following are the functions for the specified derived variables:
 */


/*
 * double Var_GTscore_OUT( ) - return value of derived numeric variable GTscore_OUT.
 */
double
Var_GTscore_OUT( void)
{
	chkProg( );
	return (numOfVal(

	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 17))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mymoves), valOfNum( 0))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mymoves), valOfNum( 1))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 21))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourcalah), valOfNum( 8))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 22))) ? valOfNum( 0.6406936) : valOfNum( 0.6059637)) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistr), valOfNum( 0))) ? valOfNum( 0.6822) : valOfNum( 0.6435442))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistl), valOfNum( 1))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourcalah), valOfNum( 7))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistl), valOfNum( 2))) ? valOfNum( 0.5650773) : valOfNum( 0.600268)) : valOfNum( 0.6312018)) : valOfNum( 0.6297958))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mycaptures), valOfNum( 0))) ? valOfNum( 0.7700617) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistl), valOfNum( 0))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 22))) ? valOfNum( 0.6634364) : valOfNum( 0.6279026)) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourmoves), valOfNum( 1))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 22))) ? valOfNum( 0.7721409) : valOfNum( 0.7202855)) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 19))) ? valOfNum( 0.6625234) : valOfNum( 0.7050988)))))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 18))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourmoves), valOfNum( 0))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourmoves), valOfNum( 1))) ? valOfNum( 0.9839093) : valOfNum( 0.9659775)) : valOfNum( 1)) : valOfNum( 0.5))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourcalah), valOfNum( 17))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourmoves), valOfNum( 0))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourmoves), valOfNum( 1))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourcalah), valOfNum( 21))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 8))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourcalah), valOfNum( 22))) ? valOfNum( 0.367392) : valOfNum( 0.3996585)) : 
	(boolOfVal( valOfBool( Var_mustpass)) ? valOfNum( 0.3192853) : valOfNum( 0.3596902))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistr), valOfNum( 2))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 7))) ? valOfNum( 0.4406223) : valOfNum( 0.3788793)) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistl), valOfNum( 0))) ? valOfNum( 0.3683924) : valOfNum( 0.401588)))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mymoves), valOfNum( 1))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistl), valOfNum( 0))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistl), valOfNum( 0))) ? valOfNum( 0.2520958) : valOfNum( 0.3072325)) : valOfNum( 0.3392292)) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mymoves), valOfNum( 0))) ? valOfNum( 0.3403134) : valOfNum( 0.3683184)))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mymoves), valOfNum( 0))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 13))) ? valOfNum( 0.3576833) : valOfNum( 0.2697837)) : valOfNum( 0.00450742))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourcalah), valOfNum( 15))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourmoves), valOfNum( 1))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 11))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mycaptures), valOfNum( 0))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourmoves), valOfNum( 2))) ? valOfNum( 0.4105101) : 
	(boolOfVal( valOfBool( Var_mustpass)) ? valOfNum( 0.3731656) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourcalah), valOfNum( 16))) ? valOfNum( 0.4989919) : valOfNum( 0.6938559)))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistl), valOfNum( 2))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourmoves), valOfNum( 3))) ? valOfNum( 0.3806818) : valOfNum( 0.193299)) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourcalah), valOfNum( 16))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistr), valOfNum( 0))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistr), valOfNum( 2))) ? valOfNum( 0.4367681) : valOfNum( 0.3731727)) : valOfNum( 0.2559289)) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistr), valOfNum( 0))) ? valOfNum( 0.4148839) : valOfNum( 0.4985015))))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistr), valOfNum( 7))) ? valOfNum( 0.3567674) : valOfNum( 0.456533))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourcalah), valOfNum( 16))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistl), valOfNum( 1))) ? valOfNum( 0.1695906) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourmoves), valOfNum( 0))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistr), valOfNum( 1))) ? valOfNum( 0.3503741) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mycaptures), valOfNum( 0))) ? valOfNum( 0.6553846) : valOfNum( 0.5005336))) : 
	(boolOfVal( valOfBool( Var_mustpass)) ? valOfNum( 0.6775777) : valOfNum( 0.5598765)))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistl), valOfNum( 1))) ? valOfNum( 0.4822946) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistr), valOfNum( 1))) ? valOfNum( 0.4258621) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistr), valOfNum( 1))) ? valOfNum( 0.7056484) : valOfNum( 0.6091198)))))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 13))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mymoves), valOfNum( 1))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourcalah), valOfNum( 11))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistl), valOfNum( 3))) ? valOfNum( 0.5694152) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 15))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistl), valOfNum( 0))) ? valOfNum( 0.6110775) : valOfNum( 0.6814614)) : valOfNum( 0.5735372))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistl), valOfNum( 7))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistr), valOfNum( 1))) ? valOfNum( 0.7278287) : valOfNum( 0.588293)) : valOfNum( 0.5509718))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mycalah), valOfNum( 16))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistr), valOfNum( 1))) ? valOfNum( 0.8861607) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mycaptures), valOfNum( 0))) ? valOfNum( 0.7043011) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mymoves), valOfNum( 0))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistl), valOfNum( 1))) ? valOfNum( 0.6423077) : valOfNum( 0.4958231)) : valOfNum( 0.3232143)))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mycaptures), valOfNum( 1))) ? valOfNum( 0.7315436) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistl), valOfNum( 4))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistl), valOfNum( 0))) ? valOfNum( 0.4642857) : valOfNum( 0.2083333)) : valOfNum( 0.4334764))))) : 
	(boolOfVal( 
	opGt( valOfNum( Var_yourdistr), valOfNum( 0))) ? 
	(boolOfVal( 
	opGt( valOfNum( Var_mymoves), valOfNum( 3))) ? valOfNum( 0.5274463) : valOfNum( 0.4489031)) : 
	(boolOfVal( 
	opGt( valOfNum( Var_mydistr), valOfNum( 4))) ? valOfNum( 0.7083333) : valOfNum( 0.5549188)))))))

	));
}
