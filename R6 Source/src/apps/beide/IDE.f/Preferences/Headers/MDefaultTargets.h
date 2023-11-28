//========================================================================
//	MDefaultTargets.h
//	Copyright 1996 - 98 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MDEFAULTTARGETS_H
#define _MDEFAULTTARGETS_H

#include "MTargetTypes.h"

#include <Mime.h>

#define anytexttype "text/*"
#define xcofftype "application/x-xcoff"
#define noMimeType ""


#ifndef __GNUC__

#if (B_HOST_IS_BENDIAN)
#define mwccname "mwccppc"
#define mwldname "mwldppc"
#define mwlibtype "application/x-mw-library"
#define mwexecutabletype "application/x-be-executable"
#else
#define mwccname "mwccx86"
#define mwldname "mwldx86"
#define mwlibtype "application/x-mw-library-x86"
#define mwlinklibtype "application/x-mw-linklibrary"
#define mwexecutabletype "application/x-vnd.Be-peexecutable"
#endif

#else

#define mwccname "gcc"
#define mwldname "gcc_link"
// we should use B_APP_MIME_TYPE from mime.h but they way TargetRec is set up we can't
#define mwexecutabletype "application/x-vnd.Be-elfexecutable"
#define archiveType "application/x-vnd.Be.ar-archive"
#endif

#define projectMimeType "application/x-mw-project"

const TargetRec sDefaultTargets[] = {
#ifdef __GNUC__

{ 	0, linkStage, noResources, "", mwldname, mwexecutabletype},
{	0, precompileStage, noResources, "", "", projectMimeType},
{ 	0, linkStage, noResources, "", mwldname, archiveType},
{ 	0, linkStage, noResources, "a", mwldname, noMimeType},
{ 	0, linkStage, noResources, "o", mwldname, noMimeType},
{ 	0, compileStage, noResources, "c", mwccname, anytexttype },
{ 	0, compileStage, noResources, "cc", mwccname, anytexttype },
{ 	0, compileStage, noResources, "c++", mwccname, anytexttype },
{ 	0, compileStage, noResources, "cp", mwccname, anytexttype },
{ 	0, compileStage, noResources, "cpp", mwccname, anytexttype },
{ 	0, ignoreStage, noResources, "doc", "", anytexttype },
{ 	0, ignoreStage, noResources, "exp", "", anytexttype },
{ 	0, ignoreStage, noResources, "h", "", anytexttype },
{ 	0, ignoreStage, noResources, "html", "", anytexttype },
{ 	0, linkStage, noResources, "so", mwldname, noMimeType },
{ 	0, postLinkStage, hasResources, "rsrc", "", noMimeType },
{ 	0, ignoreStage, noResources, "", "", },	// sentinel
};

#else

{ 	0, linkStage, noResources, "", mwldname, mwexecutabletype},
{	0, precompileStage, noResources, "", "", projectMimeType},
#if (B_HOST_IS_BENDIAN)
{ 	0, linkStage, noResources, "a", mwldname, mwlibtype},
#else
{ 	0, linkStage, noResources, "a", mwldname, mwlinklibtype},
{ 	0, linkStage, noResources, "LIB", mwldname, mwlinklibtype },
#endif
{ 	0, linkStage, noResources, "o", mwldname, mwlibtype},
{ 	0, linkStage, noResources, "lib", mwldname, mwlibtype},
{ 	0, compileStage, noResources, "c", mwccname, anytexttype },
{ 	0, compileStage, noResources, "cc", mwccname, anytexttype },
{ 	0, compileStage, noResources, "c++", mwccname, anytexttype },
{ 	0, linkStage, noResources, "class", "javalink", noMimeType },
{ 	0, compileStage, noResources, "c++", mwccname, anytexttype },
{ 	0, compileStage, noResources, "cp", mwccname, anytexttype },
{ 	0, compileStage, noResources, "cpp", mwccname, anytexttype },
{ 	0, linkStage, noResources, "def", mwldname, anytexttype },
{ 	0, ignoreStage, noResources, "doc", "", anytexttype },
{ 	0, ignoreStage, noResources, "exp", "", anytexttype },
{ 	0, ignoreStage, noResources, "h", "", anytexttype },
{ 	0, ignoreStage, noResources, "html", "", anytexttype },
{ 	0, compileStage, noResources, "java", "javac", anytexttype },
{ 	0, precompileStage, noResources, "pch", mwccname, anytexttype },
{ 	0, precompileStage, noResources, "pch++", mwccname, anytexttype },
{ 	0, linkStage, noResources, "zip", "javalink", noMimeType },
{ 	0, linkStage, noResources, "xcoff", mwldname, noMimeType },
{ 	0, linkStage, noResources, "xcof", mwldname, noMimeType },
{ 	0, linkStage, noResources, "", mwldname, xcofftype },
{ 	0, linkStage, noResources, "so", mwldname, noMimeType },
{ 	0, postLinkStage, hasResources, "rsrc", "", noMimeType },
{ 	0, ignoreStage, noResources, "", "", },	// sentinel
};

#endif

#if 0
const TargetRecOld sDefaultTargetsOld[] = {
{ 	'BAPP', linkStage, hasResources, "", "mwld", },
{ 	'IRES', postLinkStage, hasResources, "", "", },			// IconWorld files
{ 	'MPLF', linkStage, noResources, "a", "mwld", },
{ 	'MPLF', linkStage, noResources, "o", "mwld", },
{ 	'MPLF', linkStage, noResources, "lib", "mwld", },
{	'TEXT', compileStage, noResources, "c", "mwcc", },
{	'TEXT', compileStage, noResources, "cc", "mwcc", },
{	'TEXT', compileStage, noResources, "C", "mwcc", },
{	'TEXT', compileStage, noResources, "c++", "mwcc", },
{	'TEXT', compileStage, noResources, "cp", "mwcc", },
{ 	'TEXT', compileStage, noResources, "cpp", "mwcc", },
{ 	'TEXT', ignoreStage, noResources, "doc", "", },
{ 	'TEXT', ignoreStage, noResources, "exp", "", },
{ 	'TEXT', ignoreStage, noResources, "h", "mwcc", },
{ 	'TEXT', precompileStage, noResources, "pch", "mwcc", },
{ 	'TEXT', precompileStage, noResources, "pch++", "mwcc", },
{ 	'XCOF', linkStage, noResources, "", "mwld", },
{ 	noFileType, linkStage, noResources, "a", "mwld", },
{ 	noFileType, linkStage, hasResources, "so", "mwld", },
{ 	noFileType, linkStage, noResources, "xcoff", "mwld", },
{ 	noFileType, linkStage, noResources, "xcof", "mwld", },
{ 	noFileType, postLinkStage, hasResources, "rsrc", "", },
{ 	noFileType, ignoreStage, noResources, "", "", },	// sentinel
};
#endif

const long kDefaultTargetCount = (sizeof(sDefaultTargets) / sizeof(TargetRec)) - 1;

#endif
