// ===========================================================================
//	MessageWindow.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#ifdef  DEBUGMENU
void ShowMessages();

// ===========================================================================

void 	pprint(const char *format, ...);
void 	pprintBig(const char *format, ...);
void 	pprintHex(const void *data, long count);

// ============================================================================


#else

inline void ShowMessages() {}
inline void pprint(const char *, ...) {}
inline void pprintBig(const char *, ...) {}
inline void pprintHex(const void *, long) {}

#endif

#endif
