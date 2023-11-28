/* ./config.h.  Generated automatically by configure.  */
/* config.h.  Generated automatically by configure.  */
/* #undef PCSC_DEBUG */
/* #define CPU_ICAP_PC 1 */
/* #undef CPU_MAC_PPC */
/* #undef CPU_SUN_SPARC */

#define DEBUG 0

#if DEBUG > 0
#	define D(_x)	_x
#	define bug		put_msg
#else
#	define D(_x)
#	define bug		
#endif

#define TIOCMGET	TCGETBITS
#define TIOCM_RTS	0x1000
#define TIOCM_DTR	0x2000

#define TIOCM_RI	TCGB_RI
#define TIOCM_CD	TCGB_DCD
#define TIOCM_CTS	TCGB_CTS
#define TIOCM_DSR	TCGB_DSR
