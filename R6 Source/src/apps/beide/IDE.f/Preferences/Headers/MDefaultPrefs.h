//========================================================================
//	MDefaultPrefs.h
//	Copyright 1995 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MDEFAULTPREFS_H
#define _MDEFAULTPREFS_H

#include "MPrefsStruct.h"

//const ulong kCurrentVersion = 0x0001;		// d1, d2a, d2b
//const ulong kCurrentVersion = 0x0002;		// 1.0
//const ulong kCurrentVersion = 0x0003;		// 1.1
const ulong kDR8lateVersion = 0x0004;			// 1.1.1, 1.2
const ulong kDR9Version = 0x0005;		// 1.3, 1.3.1, 1.3.2
// next version will be 7. see access paths version issue.
//const ulong kCurrentVersion = 0x0007;		// R3
const ulong kCurrentVersion = 0x0008;		// R4.1

struct TargetRec;

class MDefaultPrefs
{
public:

	static void					SetEditorDefaults(
									EditorPrefs& 	outPrefs);
	static void					SetFontDefaults(
									FontPrefs& 	outPrefs);
	static void					SetAppEditorDefaults(
									AppEditorPrefs& 	outPrefs);
	static void					SetProjectDefaults(
									ProjectPrefs& 	outPrefs);
	static void					SetLanguageDefaults(
									LanguagePrefs& 	outPrefs);
	static void					SetLinkerDefaults(
									LinkerPrefs& 	outPrefs);
	static void					SetAccessPathsDefaults(
									AccessPathsPrefs& 	outPrefs);
	static void					SetWarningsDefaults(
									WarningsPrefs& 	outPrefs);
	static void					SetProcessorDefaults(
									ProcessorPrefs& 	outPrefs);
	static void					SetPEFDefaults(
									PEFPrefs& 	outPrefs);
	static void					SetTargetsDefaults(
									TargetPrefs& 	outPrefs);
	static void					UpdateTargets(
									TargetPrefs& 	outPrefs);
	static void					SetDisassemblerDefaults(
									DisassemblePrefs& 	outPrefs);
	static void					SetFontInfoDefaults(
									FontInfo& 	outPrefs);
	static void					SetSyntaxStylingDefaults(
									SyntaxStylePrefs& 	outPrefs);
	static void					SetOpenSelectionDefaults(
									OpenSelectionPrefs& outPrefs);
	static void					SetPrivateDefaults(
									PrivateProjectPrefs& outPrefs);
	static void					SetBuildExtraDefaults(
									BuildExtrasPrefs& outPrefs);
	static void					SetGlobalOptsDefaults(
									GlobalOptimizations& 	outPrefs);
	// x86 defaults
	static void					SetProjectDefaultsx86(
									ProjectPrefsx86& 	outPrefs);
	static void					SetLinkerDefaultsx86(
									LinkerPrefsx86& 	outPrefs);
	static void					SetCodeGenx86Defaults(
									CodeGenx86& 	outPrefs);
};

#endif
