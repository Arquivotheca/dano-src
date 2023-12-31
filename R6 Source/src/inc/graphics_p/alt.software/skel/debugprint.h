//////////////////////////////////////////////////////////////////////////////
// Debug Printing Header
//////////////////////////////////////////////////////////////////////////////


#if DEBUG_ACCEL

// From user space, use _kdprintf_ for debug output.
_IMPORT void _kdprintf_(const char *, ...);
#define ddprintf(a) _kdprintf_ a

#elif DEBUG_KDRIVER

// From kernel space, use dprintf for debug output.
#define ddprintf(a) dprintf a

#else

// If no debug flags are defined, ignore debug output statements.
#define ddprintf(a)

#endif


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
