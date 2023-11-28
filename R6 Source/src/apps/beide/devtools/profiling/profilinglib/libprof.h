/*	profile.h	*/

#if !defined(PROFILE_H)
#define PROFILE_H

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

/*	call PROFILE_INIT at the start of main() to turn on profiling	*/
void PROFILE_INIT(int maxdepth);
/*	call PROFILE_EXIT at the end of main() to dump data into specified file	*/
void PROFILE_DUMP(const char * path);


/*	
	not fully tested. If you call this, then you don't have
	to call PROFILE_DUMP()
	USE AT YOUR OWN RISK
*/
void PROFILE_INIT_SHARED_LIB(int maxdepth, const char* const path);


#if defined(__cplusplus)
}
#endif


#endif /* PROFILE_H */
