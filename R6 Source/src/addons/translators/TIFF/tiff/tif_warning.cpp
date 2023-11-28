#ifndef lint
static char rcsid[] = "$Header: /net/bally/be/rcs_rel/src/apps/3dmov/tiff/tif_warning.cpp,v 1.1 1996/12/22 23:45:33 pierre Exp $";
#endif

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992 Sam Leffler
 * Copyright (c) 1991, 1992 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library.
 */
#include "tiffiop.h"
#include <stdio.h>

static void
defaultHandler(const char* module, const char* fmt, va_list ap)
{
/*
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	fprintf(stderr, "Warning, ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
*/
}

static TIFFErrorHandler _warningHandler = defaultHandler;

TIFFErrorHandler
TIFFSetWarningHandler(TIFFErrorHandler handler)
{
	TIFFErrorHandler prev = _warningHandler;
	_warningHandler = handler;
	return (prev);
}

void
TIFFWarning(const char* module, const char* fmt, ...)
{
	if (_warningHandler) {
		va_list ap;
		va_start(ap, fmt);
		(*_warningHandler)(module, fmt, ap);
		va_end(ap);
	}
}
