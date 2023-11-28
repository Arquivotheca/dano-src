// ---------------------------------------------------------------------------
/*
	Profile.h
	
	Copyright (c) 2000 Be Inc. All Rights Reserved.
	
	Profile starting/stopping for both C and C++.

	Please note that you can profile your entire application from startup to quit by
	just compiling and linking with -p.  The results of the profile will be written
	to a file named profile_log.<threadID> in the application's directory.  You do 
	not need to use anything in this header to profile your application.  However,
	there are times when you might want more control over when profiling starts or
	stops.  Use the functionality defined here for these times.

	Use start_profile/stop_profile or Profile to profile a section of code.  
	The profile for this section will be named profile_log.<threadID>.<profileName>
		
	Normally profiling starts automatically.  To delay profiling until the start/stop
	define the following environment variable before running your application.
	
	export B_PROFILE_ON=0
		
	Notice...
	Multiple threads can cause some timing irregularities when stopping the profiler.
	If a function in a second thread doesn't return until after the profiler has been
	stopped, none of the time in that function will be recorded.  If another profile
	is started and then the function returns, the entire time will be attributed to
	the second profile log.  These issues do not arise for functions in the thread
	where the profiler is started and stopped.

	Multiple threads interlacing calls to Profile will cause unreliable
	timing results.
	
	start_profile and stop_profile should be called at the same level.
	(ie: call stop_profile in the same function as you called start_profile)
	This is handled for you automatically when using a Profile object.
 
*/
// ---------------------------------------------------------------------------

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

	status_t start_profile();
	status_t stop_profile(char* profile_name);
	
#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#include <String.h>

class Profile {
public:
	Profile(const char* name) : fProfileName(name)	
	{	
		// stop_profile is a c function and doesn't know about const
		// Set it up here before profiling that we don't get a spurious 
		// BString call in our profiling results
		fString = const_cast<char *> (fProfileName.String());
		start_profile(); 
	}

	~Profile()
	{
		stop_profile(fString);
	}
	
private:
	BString fProfileName;
	char*	fString;
};

#endif
