// ===========================================================================
//	Strings.h
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#ifndef __STRINGS__
#define __STRINGS__

#include <InterfaceDefs.h>
#include <String.h>

class MenuResEntry;
class BMenuItem;
class BMenu;

typedef struct {
//  FLAGS (sum all constants which apply)
//	Enabled							1
//	Replicant only					2
//	Non-replicant only				4
//	Fullscreen DesktopMode only		8
//	Right-click on images only		16	Note that something can be both an image
//	Right-click on links only		32	and a link.  There, both of these apply.
//	Right-click on frames only		64
	BString		title;
	int32		cmd;
	char		equivalent;
	uint32		modifiers;
	uint32		enabled;
	MenuResEntry*submenu;
} MenuItem;


typedef struct {
	const char*	title;
	int32		cmd;
	char		equivalent;
	uint32		modifiers;
	uint32		enabled;
} StaticMenuItem;

typedef struct {
	const char *mFont;
	const char *mMapsTo;
} FontSubstitutionItem;


typedef struct {
	const char *encName;
	
	const char *proFace;
	int32		proSize;
	int32		proMinSize;
	
	const char *fixFace;
	int32		fixSize;
	int32		fixMinSize;
} FontPrefItem;


typedef struct {
	const char *prefName;
	bool		value;
} BoolPref;


typedef struct {
	const char *prefName;
	int32		value;
} Int32Pref;


typedef struct {
	const char *prefName;
	const char *value;
} StringPref;

typedef enum {
	offscreenNever,
	offscreenAlways,
	offscreenSmart
} OffscreenOption;

typedef struct {
	const char *mimeType;
	const char *description;
	const char *suffixes;
} FileTypeSpec;

BMenuItem *AddMenuItem(BMenu *menu, const char *title, int32 command, const char equivalent = 0, uint32 modifiers = B_COMMAND_KEY, bool enabled = true);
void AddMenuItems(BMenu *menu, const StaticMenuItem *items);


#ifdef ADFILTER
extern const char *kAdFilterFilterAttrName;
extern const char *kAdFilterFolderName;
extern const char *kAdFilterMIMETypeShortDesc;
extern const char *kAdFilterMIMETypeLongDesc;
extern const char *kAdFilterSiteAttrName;
extern const char *kAdFilterSubmenuTitle;
extern const char *kPrefEnableAdFilterLabel;
extern const char *kPrefShowAdFilters;
extern const char *kPrefTabAdFilter;
extern const char *kAdFilterActionDeleteTag;
extern const char *kAdFilterActionDeleteAttr;
#endif
extern const char *kAnonymousPassword;
extern const char *kAnonymousUserName;
extern const char *kApplicationName;
extern const char *kBookmarkFolderName;
extern const char *kBookmarksMenuTitle;
extern const char *kBookmarksSubmenuTitle;
extern const char *kByteSizeKBLabel;
extern const char *kByteSizeManyBytesLabel;
extern const char *kByteSizeMBLabel;
extern const char *kByteSizeOneByteLabel;
extern const char *kBytesPerSecLabel;
extern const char *kBytesPerSecAbbrevLabel;
extern const char *kBytesPerSecKBLabel;
extern const char *kBytesPerSecKBAbbrevLabel;
extern const char *kCacheFileName;
extern const char *kCacheIndexFileName;
extern const char *kDefaultCacheLocation;
extern const char *kCancelButtonTitle;
extern const char *kCancelDownloadTitle;
extern const char *kCantOpenError;
extern const char *kContinueButtonTitle;
extern const char *kCookieAccept;
extern const char *kCookieAskMessage;
extern const char *kCookieFileName;
extern const char *kCookieReject;
#ifdef DEBUGMENU
extern const char *kDebugYahooRandomLink;
#endif
extern const char *kDefaultFilterFileName;
extern const char *kDefaultSavedFileName;
extern const char *kDefaultDownloadDirectory;
extern const char *kDefaultStartupURL;
extern const char *kDownloadCancelMessage;
extern const char *kDownloadDirLabel;
extern const char *kDownloadErrorTitle;
extern const char *kDownloadErrorMessage;
extern const char *kDownloadNumDownloading;
extern const char *kDownloadNumWaiting;
extern const char *kDownloadSimSubmenuTitle;
extern const char *kDownloadStatusComplete;
extern const char *kDownloadStatusDownloading;
extern const char *kDownloadStatusError;
extern const char *kDownloadStatusStopped;
extern const char *kDownloadStatusWaiting;
extern const char *kDownloadWaiting;
extern const char *kDownloadWaitingSize;
extern const char *kDownloadWindowTitle;
extern const char *kDownloadWindowMaxConnections;
extern const char *kEditMenuTitle;
extern const char *kEncodingsMenuTitle;
extern const char *kErasingCacheMessage;
extern const char *kErrorDialogTitle;
#ifdef NOSSL
extern const char *kErrorSSLNotImplemented;
extern const char *kErrorSSLGoThere;
extern const char *kErrorSSLNetPositiveSite;
#endif
extern const char *kErrorUnknownURLType;
extern const char *kFileCannotBeOpenedError;
extern const char *kFileMenuTitle;
extern const char *kFindButtonTitle;
extern const char *kFindWindowTitle;
extern const char *kFontFolderName;
extern const char *kFontFontAttrName;
extern const char *kFontMapsToAttrName;
extern const char *kFontMIMETypeShortDesc;
extern const char *kFontMIMETypeLongDesc;
extern const char *kFullScreenCommandTitle;
extern const char *kFSHTMLEmptyLabel;
extern const char *kFSHTMLKBLabel;
extern const char *kFSHTMLManyBytesLabel;
extern const char *kFSHTMLMBLabel;
extern const char *kFSHTMLOneByteLabel;
extern const char *kFSHTMLUpLevel;
extern const char *kGenericButtonTitle;
extern const char *kGoBackCommandTitle;
extern const char *kGoForwardCommandTitle;
extern const char *kGoMenuTitle;
extern const char *kHistoryFolderName;
extern const char *kHTMLViewName;
extern const char *kInternalURLPrefix;
extern const char *kLoadImagesCommandTitle;
extern const char *kNoButtonLabel;
extern const char *kOKButtonTitle;
extern const char *kOpenButtonTitle;
extern const char *kOpenLocationWindowTitle;
extern const char *kOptionsMenuTitle;
extern const char *kPasswordDialogPrompt;
extern const char *kPasswordAuthAttrName;
extern const char *kPasswordFolderName;
extern const char *kPasswordFTPAttrName;
extern const char *kPasswordMIMETypeShortDesc;
extern const char *kPasswordMIMETypeLongDesc;
extern const char *kPasswordRealmAttrName;
extern const char *kPasswordUserAttrName;
extern const char *kPasswordPrompt;
extern const char *kPluginDirName;
extern const char *kPrefCacheClearButtonTitle;
extern const char *kPrefCacheLocationLabel;
extern const char *kPrefCacheSizeLabel;
extern const char *kPrefCacheRefreshLabel;
extern const char *kPrefCookieOptionLabel;
extern const char *kPrefEnableProxiesLabel;
extern const char *kPrefEncodingPopupLabel;
extern const char *kPrefFontFixLabel;
extern const char *kPrefFontMinSizeLabel;
extern const char *kPrefFontProLabel;
extern const char *kPrefFontSizeLabel;
extern const char *kPrefFormSubmitLabel;
extern const char *kPrefHaikuLabel;
extern const char *kPrefHistoryLengthLabel;
extern const char *kPrefHomePageLabel;
extern const char *kPrefMaxConnectionsLabel;
extern const char *kPrefNewWindowLabel;
extern const char *kPrefAutoLaunchAfterDownload;
extern const char *kPrefOffscreenLabel;
extern const char *kPrefPlaySoundsLabel;
extern const char *kPrefProxyFTPLabel;
extern const char *kPrefProxyHTTPLabel;
extern const char *kPrefProxyPortLabel;
extern const char *kPrefSearchPageLabel;
extern const char *kPrefSecureEnterLabel;
extern const char *kPrefSecureLeaveLabel;
extern const char *kPrefShowAnimationsLabel;
extern const char *kPrefShowImagesLabel;
extern const char *kPrefShowBGImagesLabel;
extern const char *kPrefTabCache;
extern const char *kPrefTabCookies;
extern const char *kPrefTabDisplay;
extern const char *kPrefTabGeneral;
extern const char *kPrefTabProxies;
extern const char *kPrefTabConnections;		// This replaces Proxies...
extern const char *kPrefTabSecurity;
extern const char *kPrefUnderlineLinksLabel;
extern const char *kPrefUseBGColorsLabel;
extern const char *kPrefUseFGColorsLabel;
extern const char *kPrefUseFontsLabel;
extern const char *kPrefWindowTitle;
extern const char *kPrintJobTitle;
extern const char *kRemainingLabelSecs;
extern const char *kRemainingLabelSecsAbbrev;
extern const char *kRemainingLabelMins;
extern const char *kRemainingLabelMinsAbbrev;
extern const char *kRemainingLabelHrs;
extern const char *kRemainingLabelHrsAbbrev;
extern const char *kRememberPasswordLabel;
extern const char *kRememberPasswordWarning;
extern const char *kReplicantEmbedMessage;
extern const char *kResetButtonTitle;
extern const char *kResourceFolderLocation;
extern const char *kSaveError;
extern const char *kSaveError2;
extern const char *kSavingFileMessage;
extern const char *kSearchableIndexText;
extern const char *kSearchableKeywordsText;
extern const char *kSettingsFolderName;
extern const char *kStatusBeginning;
extern const char *kStatusCancelled;
extern const char *kStatusComplete;
extern const char *kStatusConnecting;
extern const char *kStatusDownloading;
extern const char *kStatusDNS;
extern const char *kStatusLoadingHTML;
extern const char *kStatusLoadingImages;
extern const char *kStatusOpening;
extern const char *kStatusRequestSent;
extern const char *kStopDownloadButtonTitle;
extern const char *kSubmitButtonTitle;
extern const char *kTrackerSig;
extern const char *kURLViewLabel;
extern const char *kUsernamePrompt;
extern const char *kWarningEnteringSSL;
extern const char *kWarningInsecureForm;
extern const char *kWarningLeavingSSL;
extern const char *kYesButtonLabel;

extern const char *kApplicationSig;
extern const char *kBookmarkMimeType;
extern const char *kDocBookmarkMimeType;
extern const char *kTrackerSig;
#ifdef ADFILTER
extern const char *kAdFilterMimeType;
extern const char *kAdFilterSiteAttr;
extern const char *kAdFilterFilterAttr;
extern const char *kAdFilterTagAttr;
extern const char *kAdFilterAttrAttr;
extern const char *kAdFilterActionAttr;
extern const char *kAdFilterActionAttrName;
extern const char *kAdFilterAttrAttrName;
extern const char *kAdFilterTagAttrName;
#endif
extern const char *kPasswordMimeType;
extern const char *kFontMimeType;
extern const char *kEZDebugOption;
extern const char *kDumpDataOption;
extern const char *kCleanDumpOption;
extern const char *kFontFontAttr;
extern const char *kFontMapsToAttr;
extern const char *kPasswordUserAttr;
extern const char *kPasswordAuthAttr;
extern const char *kPasswordFTPAttr;
extern const char *kPasswordRealmAttr;
extern const char *kBookmarkLastModifiedAttr;
extern const char *kBookmarkLastVisitedAttr;
extern const char *kBookmarkURLAttr;
extern const char *kBookmarkTitleAttr;
extern const char *kDefaultBrowserString;
#ifdef JAVASCRIPT
extern const char *kJavaScriptBrowserName;
extern const char *kPrefEnableJavaScriptLabel;
extern const char *kJavaScriptDialogTitle;
#endif


extern const MenuItem *GetFileMenuItems();
extern const MenuItem *GetTextFileMenuItems();
extern const MenuItem *GetEditMenuItems();
extern const MenuItem *GetTextEditMenuItems();
extern const MenuItem *GetGoMenuItems();
extern const MenuItem *GetBookmarksMenuItems();
extern const MenuItem *GetEncodingsMenuItems();
extern const MenuItem *GetOptionsMenuItems();
extern const char **GetPrefFontSizes();
extern const StaticMenuItem *GetPrefEncodings();
extern const StaticMenuItem *GetPrefCacheOptions();
extern const StaticMenuItem *GetPrefNewWindowOptions();
extern const StaticMenuItem *GetPrefFormSubmitOptions();
extern const StaticMenuItem *GetPrefCookieOptions();
extern const char **DisallowSubstitutionList();
#ifdef ADFILTER
extern const char **GetDefaultAdList();
#endif
extern const FontSubstitutionItem *GetFontSubstitutionList();
extern const FontPrefItem *GetDefaultFontPrefs();
extern const BoolPref* GetDefaultBoolPrefs();
extern const Int32Pref* GetDefaultInt32Prefs();
extern const StringPref* GetDefaultStringPrefs();
extern BString GetErrorFileNotFound();
extern BString GetErrorGeneric();
extern BString GetErrorUnknownHost();
extern BString GetErrorTimeout();
extern BString GetErrorRefused();
extern BString GetErrorUnauthorized();
extern const FileTypeSpec* GetCanHandleList();
extern void BuildMenuList();
extern BMenu* BuildMenu(const char *internalMenuName, bool desktopMode = false,
	bool isImage = false, bool isLink = false, bool isFrame = false);
extern void BuildMenu(BMenu *menu, const char *internalMenuName, bool desktopMode = false,
	bool isImage = false, bool isLink = false, bool isFrame = false);
#endif
