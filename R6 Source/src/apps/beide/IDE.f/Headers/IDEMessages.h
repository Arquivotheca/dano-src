//========================================================================
//	IDEMessages.h
//	Copyright 1995 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _IDEMESSAGES_H
#define _IDEMESSAGES_H

#include "IDEConstants.h"


// Custom messages
// These messages must be only used within the application and
// not written to disk because of the endian issue
const uint32 msgPanelOpenMessage		= 'POpn';
const uint32 msgProjectClosed			= 'PrGA';
const uint32 msgProjectOpened			= 'PrOp';
const uint32 msgProjectActivated		= 'PrAc';
const uint32 msgProjectSelected			= 'PrSl';
const uint32 msgOpenSourceFile			= 'OpFi';
const uint32 msgCompileOne				= 'CMPo';
const uint32 msgPreCompileOne			= 'PCMp';
const uint32 msgPreProcessOne			= 'PPss';
const uint32 msgCheckSyntaxOne			= 'ChSn';
const uint32 msgDisassembleOne			= 'DsOn';
const uint32 msgAddToMessageWindow				= 'AddM';
const uint32 msgAddInfoToMessageWindow			= 'AddI';
const uint32 msgAddDefinitionToMessageWindow	= 'AddD';
const uint32 msgAddErrorsToMessageWindow		= 'AddE';
const uint32 msgAddDocInfoToMessageWindow		= 'Addo';
const uint32 msgDoneWithMessageWindow			= 'LTdn';
const uint32 msgOK						= 'OKOk';
const uint32 msgCancel					= 'CANc';
const uint32 msgClear					= 'CLer';
const uint32 msgDontSave				= 'DONt';
const uint32 msgSetName					= 'SNam';
const uint32 msgCompileDone				= 'CoDn';
const uint32 msgOneCompileDone			= 'OCDn';
const uint32 msgAddToProject			= 'AddP';
const uint32 msgFileSaved				= 'FSav';
const uint32 msgTextWindowClosed		= 'FClo';
const uint32 msgWindowsAllGone			= 'WGon';
const uint32 msg_MessageWindowsAllGone	= 'MGon';
const uint32 msgFindWindowHidden		= 'FWHd';
const uint32 msgPreferencesWindowHidden	= 'PWHd';
const uint32 msgProjectWindowClosed		= 'PWCl';
const uint32 msgSetCaptionText			= 'SCap';
const uint32 msgSetStatusText			= 'SSta';
const uint32 msgAddFiles				= 'AddF';
const uint32 msgRequestAddFiles			= 'AdRF';
const uint32 msgSaveACopyAs				= 'SaCp';
const uint32 msgGoToLine				= 'GTLn';
const uint32 msgGoToWindowClosed		= 'GTCo';
const uint32 msgFind					= 'Find';
const uint32 msgFindFromTextWindow		= 'FTxt';
const uint32 msgReplace					= 'FRep';
const uint32 msgReplaceAndFind			= 'RepF';
const uint32 msgReplaceAll				= 'RepA';
const uint32 msgSetFindString			= 'SFnd';
const uint32 msgFindSelection			= 'FndS';
const uint32 msgFindRegExp				= 'FRex';
const uint32 msgReplaceRegExp			= 'RRex';
const uint32 msgCheckBoxChanged			= 'CBCh';
const uint32 msgWindowActivated			= 'WAct';
const uint32 msgWindowDeActivated		= 'WDAc';
const uint32 msgWindowMenuClicked		= 'WMen';
const uint32 msgFactorySettings			= 'SFac';
const uint32 msgRevertSettings			= 'SRev';
const uint32 msgCancelSettings			= 'SCan';
const uint32 msgSaveSettings			= 'SSav';
const uint32 msgPrefsWindowClosed		= 'PrCl';
const uint32 msgRadioButtonClicked		= 'RBCl';
const uint32 msgButtonPushed			= 'BuCl';
const uint32 msgFontChosen				= 'ChFn';
const uint32 msgSizeChosen				= 'ChSz';
const uint32 msgStyleChosen				= 'ChSt';
const uint32 msgApplyToChanged			= 'ApTo';
const uint32 msgSetData					= 'SetD';
const uint32 msgGetData					= 'GetD';
const uint32 msgCompileCountChanged		= 'CCCh';
const uint32 msgBalanceWhileTyping		= 'MBWt';
const uint32 msgDoOpenSelection			= 'OpSt';
const uint32 msgOpenSelectionClosed		= 'OpCC';
const uint32 msgDoAndyFeature			= 'Andy';
const uint32 msgSetFileType				= 'SetF';
const uint32 msgNull					= 'nuLL';
const uint32 msgTouchFile				= 'Tuch';
const uint32 msgRawTextWindow			= 'Raww';
const uint32 msgAddAccessPath			= 'AApt';
const uint32 msgDoAccessPath			= 'DApt';
const uint32 msgRemoveAccessPath		= 'RApt';
const uint32 msgAddDefaultAccessPath	= 'ADAp';
const uint32 msgChangeAccessPath		= 'CApt';
const uint32 msgListBecameFocus			= 'LFoc';
const uint32 msgListSelectionChanged	= 'LSel';
const uint32 msgPopupChanged			= 'PopC';
const uint32 msgValueChanged			= 'ValC';
const uint32 msgDragThings				= 'Drgg';
const uint32 msgUpdateLine				= 'UpLn';
const uint32 msgSaveAsForFile			= 'SAFf';
const uint32 msgCmdOptUpArrow			= 'COUp';
const uint32 msgCmdOptDownArrow			= 'CODn';
const uint32 msgSources					= 'Srcs';
const uint32 msgSystemHeaders			= 'SyHd';
const uint32 msgProjectHeaders			= 'PrHd';
const uint32 msgOther					= 'Oter';
const uint32 msgStartOther				= 'StOt';
const uint32 msgAddOtherFile			= 'AdOt';
const uint32 msgMultiChanged			= 'Mtii';
const uint32 msgTriangleChanged			= 'TriC';
const uint32 msgRecentFindString		= 'FSCh';
const uint32 msgRecentReplaceString		= 'RSCh';
const uint32 msgBlueRowChanged			= 'BLue';
const uint32 msgSaveFileSet				= 'SFst';
const uint32 msgRemoveFileSet			= 'RFst';
const uint32 msgSaveFileSetWindowClosed		= 'SFCl';
const uint32 msgRemoveFileSetWindowClosed	= 'RFCl';
const uint32 msgFileSetChosen			= 'FSDe';
const uint32 msgShowAndActivate			= 'ShAc';
const uint32 msgCommandRightArrow		= 'CMRa';
const uint32 msgCommandLeftArrow		= 'CMLa';
const uint32 msgCommandUpArrow			= 'CMUa';
const uint32 msgCommandDownArrow		= 'CMDa';
const uint32 msgFindDefinition			= 'FDef';
const uint32 msgFindDocumentation		= 'FDoc';
const uint32 msgOpenRecent				= 'ORec';
const uint32 msgPictureButtonChanged	= 'PBCh';
const uint32 msgFindDefinitionClosed	= 'FDCl';
const uint32 msgFindDocumentationClosed	= 'FCCl';
const uint32 msgAddTarget				= 'AddT';
const uint32 msgChangeTarget			= 'ChnT';
const uint32 msgRemoveTarget			= 'RemT';
const uint32 msgFlagChosen				= 'FLgC';
const uint32 msgToolChosen				= 'TllC';
const uint32 msgTargetChosen			= 'TrgC';
const uint32 msgShowIdleStatus			= 'IdSt';
const uint32 msgUpdateButtons			= 'UpBt';
const uint32 msgPrefsViewModified		= 'PrMd';
const uint32 msgSetSyntaxColoring		= 'SynC';
const uint32 msgViewClicked				= 'VwCk';
const uint32 msgColorControlClicked		= 'CCCk';
const uint32 msgBuildWindow				= 'BWin';
const uint32 msgClearMessages			= 'MsCl';
const uint32 msgHideMessages			= 'MsHd';
const uint32 msgFilePanelRefsReceived 	= 'RRCv';
const uint32 msgCustomQuitRequested 	= 'QRec';
const uint32 msgBindingWindowClosed		= 'BWCo';
const uint32 msgSetBinding				= 'SBin';
const uint32 msgUpdateBinding			= 'UBin';
const uint32 msgSpecialKeydown			= 'SKey';
const uint32 msgUpdateMenus				= 'UMen';
const uint32 msgLinkerChanged			= 'LnCh';
const uint32 msgColorsChanged			= 'ClCh';
const uint32 msgBuildExtrasChanged		= 'BECh';
const uint32 msgNewProjectClosed		= 'NPCl';
const uint32 msgAllFilesFound			= 'FAll';
const uint32 msgSelectionChanged		= 'SelC';
const uint32 msgCreateProject			= 'CrPr';
const uint32 msgCreateEmptyProject		= 'CrEP';
const uint32 msgOpenProjectAndReply		= 'OPrR';
const uint32 msgSaveProject				= 'SavP';
const uint32 msgRowSelected				= 'RSel';
const uint32 msgMakeAndReply			= 'MkRp';


// Data types
const uint32 kErrorType				= 'ErrR';
const uint32 kInfoType				= 'InfO';
const uint32 kPathType				= 'PaTh';
const uint32 kTokenIDType			= 'Toke';
const uint32 kBindInfoType			= 'BInf';
const uint32 kStationeryType		= 'Sttt';

// Names of message fields
// (see MPrefsStruct.h for more of these)
const char kFileName[] 				= "File";
const char kFileObject[] 			= "FileObj";		// A BFile*
const char kErrorMessageName[]		= "Error";
const char kPlugInErrorName[] 		= "PlugInError";
const char kName[] 					= "Name";
const char kSection[] 				= "Section";
const char kProjectMID[] 			= "Project";		// MID == message ID
const char kBuilderProject[]		= "MProject";		// A MProject*
const char kTextWindow[] 			= "TextWindow";		// A MTextWindow*
const char kTextView[] 				= "TextView";		// A MIDETextView*
const char kText[] 					= "Text";			// A C String
const char kObjectDirectoryName[] 	= "(Objects)";		// Name of the Object File directory
const char kLineNumber[]		 	= "LineNumber";
const char kAsciiText[]		 		= "Ascii";			// Raw text
const char kTextFileRef[]		 	= "TextRef";		// entry ref for a text file
const char kInfoStruct[]		 	= "InfoStruct";		// an info struct for the message window
const char kDocInfo[]				= "DocInfo";		// an info struct for documentation lookup
const char kWhichView[]		 		= "WhichView";		// view number in the preferences window
const char kFlashDelay[]	 		= "Flash";			// flash delay when balancing
const char kSystemInclude[]	 		= "Tree";			// bool for open selection message
const char kProjectWindow[]			= "ProjectWind";	// a MProjectWindow*
const char kEOLType[]				= "EOLType";		// Be, Mac, or Dos
const char kSize[]					= "Size";			// The size of something
const char kAreaID[]				= "AreaID";			// an area_id
const char kAddress[]				= "Address";		// a pointer to the address of something
const char kButtonLabel[]			= "ButtonLabel";	// label for the button in the access paths panel
const char kWindowLabel[]			= "WindowLabel";	// label for the access paths panel
const char kCloseFilePanel[]		= "ClosePanel";		// Do we close the open panel first?
const char kSystemPath[]			= "SysPath";		// A system path for the access path
const char kProjectPath[]			= "ProjPath";		// A project path for the access path
const char kNeedsUpdating[]			= "UpdateProj";		// The project needs to be recompiled or relinked
const char kIndex[]					= "Index";			// An index in a list of some sort
const char kProjectLine[]			= "ProjLine";		// A project line object
const char kDragType[]				= "DragType";		// Does the drag contain files or sections?
const char kTokenIdentifier[]		= "Token";			// A token identifier struct
const char kListObject[]			= "ListObject";		// An access path list object
const char kIsInProject[]			= "IsProjFile";		// The file is part of the project
const char kSemID[]					= "SemID";			// A sem_id being passed to syncronize an operation
const char kForward[]				= "Forward";		// Forward or reverse find
const char kFindIt[]				= "Findit";			// Find or replace
const char kMessagesType[]			= "MessKind";		// Kind of messages for the message window
const char kFontFamilyAndStyle[]	= "FontFamStyle";	// A font family and style id
const char kFramePrefsName[]		= "frame";			// A window frame attribute
const char kFramePrefsNameLE[]		= "leframe";		// A window frame attribute little endian
const char kOKToQuit[]				= "OKQuit";			// is it ok to quit?
const char kUseOptionKey[]			= "Option";			// simulated option keydown.
const char kBindingInfo[]			= "BindInfo";		// a BindingInfo struct
const char kBindingContext[]		= "BindContext";	// a BindingContext enum
const char kIsPrefixKey[]			= "Prefix";			// is the BindingInfo struct for a prefix key
const char kNewLinkerName[]			= "NewLinkerName";	// new linker was chosen in the targets prefs panel
const char kCreateFolder[]			= "NewFolder";		// create a folder, from the new projects window
const char kFolder[]				= "Folder";			// ref of a folder
const char kEmptyProject[]			= "EmptyProject";	// build an empty project?

// message names for communication with external editor
const char kBeLineNumber[]			= "be:line";				// line number (external editor)
const char kBeSelectionOffset[]		= "be:selection_offset";	// selection offset (external editor)
const char kBeSelectionLength[]	 	= "be:selection_length";	// selection length (external editor)
const char kBeSelectionText[]		= "be:selection_text";		// actual selection text (external editor)

// Default names for the system tree and the project tree
const char kSystemPathName[]		= "{beide}";		// System tree Access path name
const char kProjectPathName[]		= "{project}";		// Project tree Access path name

// MIME Types
const char kTextPlain[] = "text/plain";
const char kIDETextMimeType[] = "text/x-source-code";
const char kTextHTML[] = "text/html";

const char kProjectMimeType[] = "application/x-mw-project";
const char kCWLibMimeType[] = "application/x-mw-library";
const char kCWLibx86MimeType[] = "application/x-mw-library-x86";
const char kCWLinkLibx86MimeType[] = "application/x-mw-linklibrary";
const char kKeyBindingsMimeType[] = "application/x-mw-keybindings";
const char kPreCompiledMimeType[] = "application/x-mw-dump";
const char kXCOFFMimeType[] = "application/x-xcoff";
#define kSharedLibMimeType B_APP_MIME_TYPE
const char kClassFileMimeType[] = "application/java-byte-code";
const char kZipFileMimeType[] = "application/zip";

#ifdef __GNUC__
const char kDebuggerSigMimeType[] = "application/x-vnd.Be-debugger";
#else
const char kDebuggerSigMimeType[] = "application/x-mw-debugger";
#endif

const char kTrackerSig[] = "application/x-vnd.Be-TRAK";

#if DEBUG
const char kIDESigMimeType[] = "application/x-mw-BeIDE-debug";
#else
const char kIDESigMimeType[] = "application/x-mw-BeIDE";
#endif

// Application wide constants
const CommandT cmdSetDirty 			= 'Drty';

// Resource names and types
const char kFontPrefsName[] 		= "FontPrefs";
const char kFontPrefsNewName[] 		= "NFontPrefs";
const uint32 kFontPrefsType			= 'FPrf';

const char kToolsFolderName[]		= "tools";
const char kStationeryFolderName[]	= "stationery";

// Menu item names
const char kDisableDebugger[] = "Disable Debugger";
const char kEnableDebugger[] = "Enable Debugger";
const char kDebug[] = "Debug";
const char kRun[] = "Run";

// File and Folder names
const char kPluginsFolderName[] = "plugins";
const char kPrefsAddOnsName[] = "Prefs_add_ons";
const char kEditorAddOnsName[] = "Editor_add_ons";

// This is only used for the limited version
#if __INTEL__
const char kMWPluginName[] = "MWPrefsPluginx86";
#else
const char kMWPluginName[] = "MWPrefsPlugin";
#endif

const char kNewProjectName[] = "New Project";

// Constants for font size popups 
const long kFirstFontSize = 6;
const long kLastFontSize = 24;
const long kDefaultFontSize = 12;

#endif
