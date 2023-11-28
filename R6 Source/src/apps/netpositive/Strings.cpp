// ===========================================================================
//	Strings.cpp
// 	Copyright 1998 by Be Incorporated, All rights reserved.
// ===========================================================================

#include "HTMLView.h"
#include "HTMLWindow.h"
#include "URLView.h"
#include "NPApp.h"
#include "FontSubstitution.h"
#include "Strings.h"
#include <stdlib.h>
#include <MenuItem.h>

// ===========================================================================
// LOCALIZABLE STRINGS AND CONSTANTS BEGIN HERE
// ===========================================================================
#ifdef ADFILTER
const char *kAdFilterDefaultList[] = {
/*
	"/adlemur/",
	"/Ads/",
	"/ads/",
	"/adverts/",
	"/SmartBanner/",
	"/sponsor/",
	"ad.doubleclick.net",
	"ad.preferences.com/",
	"adbot",
	"adframes/",
	"ntmarkt_",
*/
	NULL
};
const char *kAdFilterActionAttrName =		"Action";
const char *kAdFilterAttrAttrName =			"Attribute";
const char *kAdFilterFilterAttrName =		"Filter";
const char *kAdFilterFolderName =			"NetPositive/Filters";
const char *kAdFilterMIMETypeShortDesc =	"NetPositive Filter";
const char *kAdFilterMIMETypeLongDesc =		"NetPositive Filter";
const char *kAdFilterSiteAttrName = 		"Site";
const char *kAdFilterSubmenuTitle =			"Filter";
const char *kAdFilterTagAttrName =			"Tag";
const char *kAdFilterActionDeleteTag =		"DELETE TAG";
const char *kAdFilterActionDeleteAttr =		"DELETE ATTR";
const char *kPrefEnableAdFilterLabel =		"Enable Filtering";
const char *kPrefShowAdFilters = 			"Show Filters";
const char *kPrefTabAdFilter =				"Filtering";
#endif
const char *kAnonymousPassword =			"ftp@ftp.com";
const char *kAnonymousUserName = 			"anonymous";
const char *kApplicationName = 				"NetPositive";
const char *kBookmarkFolderName =			"NetPositive/Bookmarks";
const char *kByteSizeKBLabel =				"%dK";
const char *kByteSizeManyBytesLabel =		"%d bytes";
const char *kByteSizeMBLabel =				"%2.2f MB";
const char *kByteSizeOneByteLabel =			"1 byte";
const char *kBytesPerSecLabel =				"%d bytes/sec";
const char *kBytesPerSecAbbrevLabel =		"%d B/s";
const char *kBytesPerSecKBLabel = 			"%1.1f KB/sec";
const char *kBytesPerSecKBAbbrevLabel = 	"%1.1f KB/s";
const char *kCacheFileName = 				"%Ld";
const char *kCacheIndexFileName =			"CacheLog";
const char *kDefaultCacheLocation =			"/NetPositive/NetCache/";
const char *kCancelButtonTitle = 			"Cancel";
const char *kCancelDownloadTitle =			"Continue Downloading";
const char *kCantOpenError =				"Can't open '%s'";
const char *kContinueButtonTitle =			"Continue";
const char *kCookieAccept =					"Accept";
const char *kCookieAskMessage =				"Do you wish to accept cookies for %s %s?";
const char *kCookieFileName =				"Cookies";
const char *kCookieReject =					"Reject";
#ifdef DEBUGMENU
const char *kDebugYahooRandomLink =			"http://random.yahoo.com/bin/ryl";
#endif
const char *kDefaultFilterFileName =		"Filter";
const char *kDefaultSavedFileName =			"untitled";
const char *kDefaultDownloadDirectory =		"/Downloads/";
const char *kDefaultStartupURL =			"netpositive:Startup.html";
const char *kDownloadCancelMessage =		"You have file downloads in progress.  Would you like to abort those downloads?";
const char *kDownloadDirLabel =				"Download directory:";
const char *kDownloadErrorTitle =			"Error downloading file!";
const char *kDownloadErrorMessage =			"Error downloading file!";
const char *kDownloadNumDownloading =		"%d Downloading, ";
const char *kDownloadNumWaiting =			"%d Waiting.  ";
const char *kDownloadSimSubmenuTitle =		"Max. Simultaneous";
const char *kDownloadStatusComplete =		"Complete";
const char *kDownloadStatusDownloading =	"Downloading";
const char *kDownloadStatusError =			"Error";
const char *kDownloadStatusStopped =		"Stopped";
const char *kDownloadStatusWaiting =		"Waiting";
const char *kDownloadWaiting =				"Waiting for data.";
const char *kDownloadWaitingSize =			"Waiting for data.  File is %s.";
const char *kDownloadWindowMaxConnections =	"Max. Connections";
const char *kDownloadWindowTitle =			"Downloads";
const char *kErasingCacheMessage =			"Cleaning cache, please wait...";
const char *kErrorDialogTitle =				"Error";
#ifdef NOSSL
const char *kErrorSSLNotImplemented =		"Sorry, secure HTTP connections are not supported at this time.";
const char *kErrorSSLGoThere =				"Go there";
const char *kErrorSSLNetPositiveSite =		"http://www.bedepot.com/Products/Be/NetPositive.asp";
#endif
const char *kErrorUnknownURLType =			"Unknown URL type: ";
const char *kFileCannotBeOpenedError =		"This file cannot be opened by NetPositive.";
const char *kFindButtonTitle =				"Find";
const char *kFindWindowTitle =				"Find";
const char *kFontFolderName =				"NetPositive/FontMappings";
const char *kFontFontAttrName =				"Original Font";
const char *kFontMapsToAttrName =			"Maps to";
const char *kFontMIMETypeShortDesc =		"NetPositive Font";
const char *kFontMIMETypeLongDesc =			"NetPositive Font Substitution Mapping";
const char *kFSHTMLEmptyLabel =				"<TR><TD BGCOLOR=#F0F0FF>Empty</TD></TR>";
const char *kFSHTMLKBLabel =				"%dK</TD>";
const char *kFSHTMLManyBytesLabel =			"%d bytes</TD>";
const char *kFSHTMLMBLabel =				"%2.1f MB</TD>";
const char *kFSHTMLOneByteLabel =			"1 byte</TD>";
const char *kFSHTMLUpLevel =				"..up one level";
const char *kFullScreenCommandTitle =		"Full Screen";
const char *kGenericButtonTitle =			"Button";
const char *kGoBackCommandTitle =			"Back";
const char *kGoForwardCommandTitle =		"Forward";
const char *kHistoryFolderName =			"NetPositive/History";
const char *kInternalURLPrefix =			"netpositive:";
const char *kLoadImagesCommandTitle =		"Load Images";
const char *kNoButtonLabel =				"No";
const char *kOKButtonTitle = 				"OK";
const char *kOpenButtonTitle =				"Open";
const char *kOpenLocationWindowTitle =		"Open Location";
const char *kPasswordDialogPrompt = 		"Please enter the username and password for '%s':";
const char *kPasswordAuthAttrName =			"Authentication";
const char *kPasswordFolderName =			"NetPositive/Passwords";
const char *kPasswordFTPAttrName =			"For FTP";
const char *kPasswordMIMETypeShortDesc =	"NetPositive Password";
const char *kPasswordMIMETypeLongDesc =		"NetPositive Saved Site Password";
const char *kPasswordRealmAttrName = 		"Realm";
const char *kPasswordUserAttrName = 		"User";
const char *kPasswordPrompt = 				"Password";
const char *kPluginDirName =				"Plug-ins";
const char *kPrefCacheClearButtonTitle =	"Clear now";
const char *kPrefCacheLocationLabel =		"Cache location:";
const char *kPrefCacheSizeLabel =			"Cache size (MB):";
const char *kPrefCacheRefreshLabel =		"Refresh cache:";
const char *kPrefCookieOptionLabel =		"When a new cookie is received:";
const char *kPrefEnableProxiesLabel =		"Enable proxies";
const char *kPrefEncodingPopupLabel =		"For the encoding:";
const char *kPrefFontFixLabel =				"Fixed font:";
const char *kPrefFontMinSizeLabel =			"Min. size:";
const char *kPrefFontProLabel =				"Proportional font:";
const char *kPrefFontSizeLabel =			"Size:";
const char *kPrefFormSubmitLabel =			"Warn when sending an unsecure form:";
const char *kPrefHaikuLabel =				"Haiku error messages";
const char *kPrefHistoryLengthLabel =		"Number of days to keep links in the Go menu:";
const char *kPrefHomePageLabel =			"Home page:";
const char *kPrefMaxConnectionsLabel =		"Maximum number of simultaneous connections:";
const char *kPrefNewWindowLabel =			"New windows:";
const char *kPrefAutoLaunchAfterDownload = 	"Automatically launch files after they have finished downloading.";
const char *kPrefOffscreenLabel =			"Flicker-free drawing";
const char *kPrefPlaySoundsLabel =			"Play sounds";
const char *kPrefProxyFTPLabel =			"FTP:";
const char *kPrefProxyHTTPLabel =			"HTTP:";
const char *kPrefProxyPortLabel =			"Port:";
const char *kPrefSearchPageLabel =			"Search page:";
const char *kPrefSecureEnterLabel =			"Warn when entering secure site";
const char *kPrefSecureLeaveLabel =			"Warn when leaving secure site";
const char *kPrefShowAnimationsLabel =		"Show animations";
const char *kPrefShowImagesLabel =			"Show images";
const char *kPrefShowBGImagesLabel =		"Show background images";
const char *kPrefTabCache =					"Cache";
const char *kPrefTabCookies =				"Cookies";
const char *kPrefTabDisplay =				"Display";
const char *kPrefTabGeneral =				"General";
const char *kPrefTabProxies =				"Proxies";
const char *kPrefTabConnections = 			"Connections";	// This replaces Proxies...
const char *kPrefTabSecurity =				"Security";
const char *kPrefUnderlineLinksLabel =		"Underline Links";
const char *kPrefUseBGColorsLabel =			"Use background colors";
const char *kPrefUseFGColorsLabel =			"Use foreground colors";
const char *kPrefUseFontsLabel =			"Use fonts";
const char *kPrefWindowTitle =				"Preferences";
const char *kPrintJobTitle =				"NetPositive Print";
const char *kRemainingLabelSecs =			", %2d secs remaining";
const char *kRemainingLabelSecsAbbrev =		" 0:%02d";
const char *kRemainingLabelMins = 			", %2d:%02d remaining";
const char *kRemainingLabelMinsAbbrev = 	" %2d:%02d";
const char *kRemainingLabelHrs =			", %d:%02d:%02d remaining";
const char *kRemainingLabelHrsAbbrev =		" %d:%02d:%02d";
const char *kRememberPasswordLabel =		"Remember password";
const char *kRememberPasswordWarning =		"Passwords will be saved UNSECURELY on your hard disk.";
const char *kReplicantEmbedMessage =		"To embed this replicant in your page, use the following tag:\n<OBJECT TYPE=\"application/x-vnd.Be-replicant-base64\" WIDTH=%ld HEIGHT=%ld DATA=\"%s\"></OBJECT>\n";
const char *kResetButtonTitle =				"Reset";
const char *kResourceFolderLocation =		"/NetPositive/Resources/";
const char *kSaveError =					"There was an error saving '%s'";
const char *kSaveError2 =					"An error occured saving '%s' as '%s'";
const char *kSavingFileMessage =			" '%s'\nbeing saved as\n '%s'";
const char *kSearchableIndexText = 			"This is a searchable index";
const char *kSearchableKeywordsText = 		"Enter keywords: ";
const char *kSettingsFolderName =			"settings";
const char *kStatusBeginning =				"Opening %s...";
const char *kStatusCancelled =				"Cancelled";
const char *kStatusComplete =				"Complete";
const char *kStatusConnecting =				"Connecting to server %s...";
const char *kStatusDownloading =			"Downloading...";
const char *kStatusDNS =					"Looking up server name %s...";
const char *kStatusLoadingHTML =			"Loading text ";
const char *kStatusLoadingImages =			"Loading images ";
const char *kStatusOpening =				"Opening...";
const char *kStatusRequestSent =			"Requested %s...";
const char *kStopDownloadButtonTitle =		"Stop Downloads";
const char *kSubmitButtonTitle =			"Submit";
const char *kURLViewLabel =					"Location:";
const char *kUsernamePrompt = 				"Username";
const char *kWarningEnteringSSL =			"You are entering a secure site.  All of your transactions with this site will be secured against interception and use by others.";
const char *kWarningInsecureForm =			"The information being sent in this form is not secured and can be intercepted and used by others.\n\nDo you wish to continue?";
const char *kWarningLeavingSSL =			"You are leaving a secure site.  All future transactions are susceptible to interception and use by others.";
const char *kYesButtonLabel =				"Yes";

const char *kErrorPage = "<html><head><title>%s</title></head><body bgcolor=\"#dddddd\"><h1>%s</h1><font size=4>%s</font><p><table><tr><td width=30>&nbsp;</td><td><i><font size=2>%s</font></i></td></tr></table></body></html>";

const int kNumUnknownHostErrors = 19;
const char *kNetErrorUnknownHost[] =
{
	"Web site not found",
	"The web site <b>%s</b> could not be found.  If you have entered the web address manually, "
	"please make sure that you have typed it correctly.  If you have gotten this error by clicking on a link "
	"from another site, you may wish to contact the site administrator about the out-of-date link.",

	"The web site you seek<br>Lies beyond our perception<br>But others await.",
	"Sites you are seeking<br>From your path they are fleeing<br>Their winter has come.",
	"A truth found, be told<br>You are far from the fold, Go<br>Come back yet again.",
	"Wind catches lily<br>Scatt'ring petals to the wind:<br>Your site is not found.",
	"These three are certain:<br>Death, taxes, and site not found.<br>You, victim of one.",
	"Ephemeral site.<br>I am the Blue Screen of Death.<br>No one hears your screams.",
	"Aborted effort:<br>The site, passed this veil of tears.<br>You ask way too much.",
// Ian Hughes at Zibex, inc. (ian807@yahoo.com) claims to be the author and wants credit.  Screw 'im.
//	"Serious error.<br>The site, vanished into dust.<br>Screen.  Mind.  Both are blank.",
	"Morning and sorrow<br>404 not with us now<br>Lost to paradise.",
	"Not a pretty sight<br>When the web dies screaming loud<br>The site is not found.",
	"Site slips through fingers<br>Pulse pounding hard and frantic<br>Vanishing like mist.",
	"The dream is shattered<br>The web site cannot be found<br>Inside the spring rain.",
	"Bartender yells loud<br>Your site cannot be found, boy<br>Buy another drink.",
	"Chrome megaphone barks<br>It's not possible to talk<br>Not yet anyway.",
	"Emptyness of soul<br>Forever aching blackness:<br>\"Blah.com not found.\"",
	"Click exciting link<br>Gossamer threads hold you back<br>404 not found.",
	"With searching comes loss<br>And the presence of absence:<br>The site is not found.",
	"You step in the stream,<br>But the water has moved on<br>The site is not here.",
	"Rather than a beep<br>Or a rude error message,<br>These words: 'Site not found.'",
	"Something you entered<br>Transcended parameters.<br>The site is unknown."
};

const int kNumRefusedErrors = 11;
const char *kNetErrorRefused[] =
{
	"Connection refused",
	"The web site <b>%s</b> refused your connection.  Usually this occurs when the site is temporarily "
	"down.  Try your request again later.",

	"Stay the patient course<br>Of little worth is your ire<br>The server is down",
	"There is a chasm<br>Of carbon and silicon<br>The server can't bridge.",
	"Chaos reigns within.<br>Reflect, repent, and retry.<br>Server shall return.",
	"Won't you please observe<br>A brief moment of silence<br>For the dead server?",
	"First snow, then silence.<br>This expensive server dies<br>So beautifully.",
	"Seeing my great fault<br>Through darkening dead servers<br>I begin again.",
	"Visit the home page<br>It can't be done easily<br>When the site is down.",
	"Cables have been cut<br>Southwest of Northeast somewhere<br>We are not amused.",
	"Site is silent, yes<br>No voices can be heard now<br>The cows roll their eyes.",
	"Silicon shudders<br>The site is down for the count<br>One big knockout punch.",
	"Yesterday it worked<br>Today it is not working<br>The web is like that."
};

const int kNumFileNotFoundErrors = 2;
const char *kNetErrorFileNotFound[] =
{
	"File not found",
	"The file <b>%s</b> could not be found.  If you have entered the address manually, "
	"please make sure that you have typed it correctly.",

	"The ten thousand things<br>How long do any persist?<br>The file, not there.",
	"A file that big?<br>It might be very useful<br>But now it is gone."
};

const int kNumGenericErrors = 4;
const char *kNetErrorGeneric[] =
{
	"Error",
	"An error has occurred trying to access <b>%s</b>.  Sorry that we're not more specific about what the error is.",

	"To have no errors<br>Would be life without meaning<br>No struggle, no joy",
	"Errors have occurred.<br>We won't tell you where or why.<br>Lazy programmers.",
	"The code was willing<br>It considered your request,<br>But the chips were weak.",
	"Error reduces<br>Your expensive computer<br>To a simple stone."
};

const int kNumTimeoutErrors = 1;
const char *kNetErrorTimeout[] =
{
	"Request timed out",
	"Your attempt to access <b>%s</b> failed because the server took too long to respond.  This could be due to "
	"to server being too busy right now.  Try your request again later.",

	"Server's poor response<br>Not quick enough for browser.<br>Timed out, plum blossom."
};

const int kNumUnauthorizedErrors = 1;
const char *kNetErrorUnauthorized[] =
{
	"Unauthorized",
	"Your are not authorized to access <b>%s</b>.  Please make sure that you have entered your username and password correctly.",

	"Login incorrect.<br>Only perfect spellers may<br>Enter this system."
};

// Out of memory.\nWe wish to hold the whole sky,\nBut we never will


// ---------------------------------------------------------------------------
// Menus
// ---------------------------------------------------------------------------

const char *kBookmarksSubmenuTitle =		"Bookmarks";


const char *kEncodingsMenuTitle = "Document Encoding";

const char *kPrefFontSizes[] = {
	"7",
	"9",
	"10",
	"12",
	"14",
	"18",
	"24",
	NULL
};

StaticMenuItem kPrefEncodings[] = {
	{ "Western",	msg_WesternEncoding, 0, 0, 1 },
	{ "Unicode",	msg_UnicodeEncoding, 0, 0, 1 },
	{ "Japanese",	msg_JapaneseEncoding, 0, 0, 1},
	{ "Greek",		msg_GreekEncoding, 0, 0, 1 },
	{ "Cyrillic",	msg_CyrillicEncoding, 0, 0, 1},
	{ "Central European", msg_CEEncoding, 0, 0, 1},
	{ NULL}
};

StaticMenuItem kPrefCacheOptions[] = {
	{ "Every time",			msg_CacheEveryTime		},
	{ "Once per session",	msg_CacheOncePerSession	},
	{ "Once per day",		msg_CacheOncePerDay		},
	{ "Never",				msg_CacheNever			},
	{ NULL, 				0						}
};

StaticMenuItem kPrefNewWindowOptions[] = {
	{ "Clone current window",	msg_NewWindowClone	},
	{ "Open home page",			msg_NewWindowHome	},
	{ "Open blank page",		msg_NewWindowBlank	},
	{ NULL,						0					}
};

StaticMenuItem kPrefFormSubmitOptions[] = {
	{ "Never",					msg_FormSubmitNever	},
	{ "When sending more than one line", msg_FormSubmitOneLine },
	{ "Always",					msg_FormSubmitAlways },
	{ NULL,						0					}
};

StaticMenuItem kPrefCookieOptions[] = {
	{ "Accept",					msg_CookieAccept	},
	{ "Reject",					msg_CookieReject	},
	{ "Ask",					msg_CookieAsk		},
	{ NULL,						0					}
};

FontSubstitutionItem kFontSubstitutionList[] = {
	// Mac fonts
	{ "Chicago",				"Swiss911 XCm BT"		},
	{ "Courier",				"Courier 10 BT"			},
	{ "Geneva",					"Swis721 BT"			},
	{ "Helvetica",				"Swis721 BT"			},
	{ "Monaco",					"Monospac821 BT"		},
	{ "New York",				"Dutch801 Rm BT"		},
	{ "Palatino",				"Dutch801 Rm BT"		},
	{ "Symbol",					"SymbolProp BT"			},
	{ "Times",					"Dutch801 Rm BT"		},
	
	// Windows fonts (Taken from NT 4.0)
	{ "Courier New",			"Courier 10 BT"			},
	{ "MS Sans Serif",			"Swis721 BT"			},
	{ "Lucida Sans Unicode",	"Swis721 BT"			},
	{ "Small Fonts",			"Swis721 BT"			},
//	{ "Symbol",					"SymbolProp BT"			},	// Already defined by Mac list
//	{ "Courier",				"Courier 10 BT"			},	// Already defined by Mac list
	{ "Arial",					"Swis721 BT"			},
	{ "Comic Sans MS",			"VAG Rounded BT"		},
	{ "Arial Black",			"Swiss911 XCm BT"		},
	{ "Roman",					"Dutch801 Rm BT"		},
	{ "Impact",					"CopprplGoth Bd BT"		},
	{ "MS Serif",				"Dutch801 Rm BT"		},
	{ "Modern",					"Humnst777 BT"			},
	{ "Script",					"CommercialScript"		},
	{ "Lucida Console",			"Courier 10 BT"			},
	{ "Verdana",				"Swis721 BT"			},
	
	{ NULL}
};

FontPrefItem kDefaultFontPrefs[] = {
	{"Western",			"Dutch801 Rm BT",		12, 9, "Courier10 BT",			12, 9 },
	{"Japanese",		"NOWGothic Bd BT",		12, 9, "NOWGothic Bd BT",		12, 9 },
	{"Unicode",			"Dutch801 Rm BT",		12, 9, "Courier10 BT",			12, 9 },
	{"Greek",			"Dutch801 Rm BT",		12, 9, "Courier10 BT",			12, 9 },
	{"Cyrillic",		"Dutch801 Rm BT",		12, 9, "Courier10 BT",			12, 9 },
	{"CentralEuropean",	"Dutch801 Rm BT",		12, 9, "Courier10 BT",			12, 9 },
	{ NULL },
};

BoolPref kDefaultBoolPrefs[] = {
	{ "ShowImages",					true },
	{ "ShowBGImages",				true },
	{ "UseBGColors",				true },
	{ "UseFGColors",				true },
	{ "ShowAnimations",				true },
	{ "UseFonts",					true },
	{ "UnderlineLinks",				true },
#ifdef ADFILTER
	{ "FilterEnabled",				false },
#endif
	{ "HTTPProxyActive",			false },
	{ "FTPProxyActive",				false },
	{ "SSLEnterWarning",			true },
	{ "SSLLeaveWarning",			true },
	{ "NewWindFromMouseInFront",	true },
	{ "PlaySounds",					true },
	{ "SaveWindowPos",				true },
	{ "HaikuErrorMessages",			true },
	{ "FullScreen",					false },
	{ "LookupLocalHostnames",		false },
	{ "AutoShowDownloadWindow",		true },
	{ "SupportLayerTag",			true },
	{ "AllowJSPopupWindows",		true },
	{ "AutoLaunchDownloadedFiles", 	true },
	
	// Preferences added for appliance use
	{ "AllowChangeHomePage",		true },
	{ "AllowKBShortcuts",			true },
	{ "AllowRightClick",			true },
	{ "BusyCursor",					false },
	{ "URLViewOnBottom",			false },
	{ "ShowTitleInToolbar",			false },
	
	// DesktopMode flags
	{ "LaunchLocalApps",			false },
	{ "ShowToolbar",				true },
	{ "ShowProgress",				true },
	{ "DesktopMode",				false },
	{ "LimitDirectoryBrowsing",		false },
	
#ifdef JAVASCRIPT
	{ "EnableJavaScript",			true },
#endif
	{ NULL }
};

Int32Pref kDefaultInt32Prefs[] = {
	{ "Version",					2 },
	{ "MaxConnections",				3 },
	{ "HistoryLength",				1 },
	{ "HTTPProxyPort",				0 },
	{ "HTTPSProxyPort",				443 },
	{ "FTPProxyPort",				0 },
	{ "MaxCacheSize",				10 },
	{ "CacheOption",				1 },
	{ "LaunchCount",				0 },
	{ "NewWindowOption",			0 },
	{ "CookieOption",				0 },
	{ "FormSubmitOption",			2 },
	{ "DefaultEncoding",			B_MS_WINDOWS_CONVERSION },
	{ "NumTodaysLinks",				10 },
//	{ "Scale",						100 },
	{ "DrawOffscreen",				offscreenSmart },
	{ "MaxDownloadConnections",		3 },
	{ "NumPreviousDownloads",		5 },
	{ "ToolbarBGColor",				0xd8d8d8 },
	{ "URLViewHeight",				28 },
	{ NULL }
};

StringPref kDefaultStringPrefs[] = {
	{ "DefaultURL",					"netpositive:Startup.html" },
	{ "SearchURL",					"" },
	{ "DownloadDirectory",			"" }, //used by old Download Manager
	{ "DLDirectory",				"" }, //used by new Download Manager
	{ "HTTPProxyName",				"" },
	{ "FTPProxyName",				"" },
//	{ "CacheLocation",				"" },
	{ "BrowserString",				"" },
	
	// Preferences added for appliance use
	{ "WindowTitle",				"" },
	{ NULL }	
};


// ===========================================================================
// LOCALIZABLE STRINGS AND CONSTANTS END HERE
//
// NON-LOCALIZABLE STRINGS AND CONSTANTS BEGIN HERE
// ===========================================================================

const char *kApplicationSig = 				"application/x-vnd.Be-NPOS";
const char *kBookmarkMimeType = 			"application/x-vnd.Be-bookmark";
const char *kTrackerSig =					"application/x-vnd.Be-TRAK";
#ifdef ADFILTER
const char *kAdFilterMimeType =				"application/x-vnd.Be-NPOS-Filter";
const char *kAdFilterSiteAttr = 			"NPOS:filter_site";
const char *kAdFilterFilterAttr =			"NPOS:filter";
const char *kAdFilterTagAttr =				"NPOS:filter_tag";
const char *kAdFilterAttrAttr =				"NPOS_filter_attribute";
const char *kAdFilterActionAttr =			"NPOS_filter_action";
#endif
const char *kPasswordMimeType =				"application/x-vnd.Be-NPOS-Password";
const char *kFontMimeType = 				"application/x-vnd.Be-NPOS-FontSubstitution";
const char *kDocBookmarkMimeType =			"application/x-vnd.Be-doc_bookmark";
const char *kEZDebugOption =				"--ezdebug";
const char *kDumpDataOption = 				"--dump";
const char *kCleanDumpOption = 				"--cleandump";
const char *kFontFontAttr =					"NPOS:font_orig";
const char *kFontMapsToAttr =				"NPOS:font_maps_to";
const char *kHTMLViewName =					"__NetPositive__HTMLView";
const char *kPasswordUserAttr =				"NPOS:password_user";
const char *kPasswordAuthAttr =				"NPOS:password_auth";
const char *kPasswordFTPAttr =				"NPOS:password_ftp";
const char *kPasswordRealmAttr =			"NPOS:password_realm";
const char *kBookmarkLastModifiedAttr =		"NPOS:last_modified";
const char *kBookmarkLastVisitedAttr =		"NPOS:last_visited";
const char *kBookmarkURLAttr =				"META:url";
const char *kBookmarkTitleAttr =			"META:title";
#if __BEOS__
#ifdef LAYERS
const char *kDefaultBrowserString =			"Mozilla/4.0 (compatible; NetPositive/%s; BeOS)";
#else
const char *kDefaultBrowserString =			"Mozilla/3.0 (compatible; NetPositive/%s; BeOS)";
#endif
#else
#ifdef LAYERS
const char *kDefaultBrowserString =			"Mozilla/4.0 (compatible; NetPositive/%s; unknown)";
#else
const char *kDefaultBrowserString =			"Mozilla/3.0 (compatible; NetPositive/%s; unknown)";
#endif
#endif
// MSIE 5 string - 
// Mozilla/4.0 (compatible; MSIE 5.0; Windows 98; DigExt)
#ifdef JAVASCRIPT
const char *kJavaScriptBrowserName =		"NetPositive";
const char *kPrefEnableJavaScriptLabel =	"Enable JavaScript";
const char *kJavaScriptDialogTitle =		"JavaScript";
#endif

const FileTypeSpec kCanHandleList[] = {
{	"text/html",					"HTML file",		".html, .html"	},
{	"text/plain",					"Text file",		".txt"			},
{	"text/text",					"Text file",		""				},
{	"image/gif",					"GIF image",		".gif"			},
{	"image/jpeg",					"JPEG image",		".jpg, .jpeg"	},
{	"image/jpg",					"JPEG image",		""				},
{	"image/png",					"PNG image",		".png"			},
{	"audio/midi",					"MIDI file",		".mid, .midi"	},
{	"audio/x-mid",					"MIDI file",		""				},
{	"audio/x-midi",					"MIDI file",		""				},
{	"application/octet-stream",		"Binary file",		""				},
{	"image/x-bitmap",				"XBM file",			".xbm"			},
{	NULL,							NULL,				NULL			}
};

const char *kDisallowSubstitutionList[] = {
	"3dbelogoicon.gif",
	"3dzooicon.gif",
	"Divider.gif",
	"File.gif",
	"Folder.gif",
	"HTML.gif",
	"Image.gif",
	"Mov.gif",
	"Script.gif",
	"Startup.html",
	"Text.gif",
	"UpArrow.gif",
	"appservericon.gif",
	"blackbar.gif",
	"cubeprefsicon.gif",
	"editdocicon.gif",
	"peopleicon.gif",
	"startup.gif",
	"transicon.gif",
	"whitebar.gif",
	"About.html",
	"NetPositive.gif",
	"OpenLocation.html",
	"rsalogo.gif",
	NULL,
};
// ===========================================================================
// NON-LOCALIZABLE STRINGS AND CONSTANTS END HERE
// ===========================================================================

const char **GetPrefFontSizes() {return kPrefFontSizes;}
const StaticMenuItem *GetPrefEncodings() {return kPrefEncodings;}
const StaticMenuItem *GetPrefCacheOptions() {return kPrefCacheOptions;}
const StaticMenuItem *GetPrefNewWindowOptions() {return kPrefNewWindowOptions;}
const StaticMenuItem *GetPrefFormSubmitOptions() {return kPrefFormSubmitOptions;}
const StaticMenuItem *GetPrefCookieOptions() {return kPrefCookieOptions;}
#ifdef ADFILTER
const char **GetDefaultAdList() {return kAdFilterDefaultList;}
#endif
const FileTypeSpec *GetCanHandleList() {return kCanHandleList;}
const FontSubstitutionItem *GetFontSubstitutionList() {return kFontSubstitutionList;}
const FontPrefItem *GetDefaultFontPrefs() {return kDefaultFontPrefs;}
const BoolPref* GetDefaultBoolPrefs() {return kDefaultBoolPrefs;}
const Int32Pref* GetDefaultInt32Prefs() {return kDefaultInt32Prefs;}
const StringPref* GetDefaultStringPrefs() {return kDefaultStringPrefs;}
const char **DisallowSubstitutionList() {return kDisallowSubstitutionList;}


BString BuildErrorMessage(const char **stringArray, int numStrings)
{
	BString retval = kErrorPage;
	retval.ReplaceFirst("%s", stringArray[0]);
	retval.ReplaceFirst("%s", stringArray[0]);
	retval.ReplaceFirst("%s", stringArray[1]);
	if (gPreferences.FindBool("HaikuErrorMessages")) {
		int j=2+(int) (((float)numStrings)*rand()/(RAND_MAX+1.0));
		retval.ReplaceLast("%s", stringArray[j]);
	} else
		retval.ReplaceLast("%s", "");

	return retval;
}

BString GetErrorUnknownHost()
{
	return BuildErrorMessage(kNetErrorUnknownHost, kNumUnknownHostErrors);
}

BString GetErrorRefused()
{
	return BuildErrorMessage(kNetErrorRefused, kNumRefusedErrors);
}

BString GetErrorFileNotFound()
{
	return BuildErrorMessage(kNetErrorFileNotFound, kNumFileNotFoundErrors);
}

BString GetErrorGeneric()
{
	return BuildErrorMessage(kNetErrorGeneric, kNumGenericErrors);
}

BString GetErrorTimeout()
{
	return BuildErrorMessage(kNetErrorTimeout, kNumTimeoutErrors);
}

BString GetErrorUnauthorized()
{
	return BuildErrorMessage(kNetErrorUnauthorized, kNumUnauthorizedErrors);
}



class MenuResEntry {
public:
	BString	mInternalMenuName;
	BString	mMenuTitle;
	BList	mMenuItems;
};

BList sMenuList;


bool ReadLine(BPositionIO &io, off_t &position, BString& string)
{
	char buffer[81];
	ssize_t length;
	string = "";

	while ((length = io.Read(buffer, 80)) > 0) {
		buffer[length] = 0;
		char *crPos = strchr(buffer, '\n');
		if (crPos) {
			position += crPos - buffer + 1;
			io.Seek(position, SEEK_SET);
			*crPos = 0;
			string += buffer;
			return true;
		}
		position += 80;
		string += buffer;
	}
	return false;
}


bool GetNextToken(BString &line, BString& token)
{
	while (line[0] == '\t')
		line.RemoveFirst("\t");
	if (!line.Length())
		return false;
	token = line;
	int32 pos = token.FindFirst('\t');
	if (pos > 0)
		token.Truncate(pos);
	line.RemoveFirst(token);
	token.RemoveAll("\"");
	return true;
}

MenuItem *ParseRestOfItem(BString command, BString& line)
{
	MenuItem *item = new MenuItem;
	
	BString token;
	
	if (command.Length() < 4)
		item->cmd = 0;
	else {
		item->cmd = (command[0] << 24) + (command[1] << 16) + (command[2] << 8) + command[3];
	}
		
	GetNextToken(line, token);
	item->enabled = atoi(token.String());
	
	GetNextToken(line, token);
	if (token.Length() == 0)
		item->equivalent = 0;
	else if (token.Length() > 1 && token.Compare("\\x", 2) == 0) {
		token.RemoveFirst("\\x");
		item->equivalent = strtol(token.String(), NULL, 16);
	} else
		item->equivalent = token[0];
	
	GetNextToken(line, token);
	item->modifiers = atoi(token.String());
	
	GetNextToken(line, token);
	item->title = token;
	return item;
}


void BuildMenuList()
{
	ResourceIO res("netpositive:menus");
	off_t position = 0;
	BString line;
	BString token;
	MenuResEntry *currentMenu = 0;
	MenuResEntry *currentSubmenu = 0;
	
	enum {
		kLookingForMenu,
		kInMenu,
		kInSubmenu
	};
	
	int state = kLookingForMenu;

	while (ReadLine(res, position, line)) {
		if (line.Length() == 0 || line[0] == '#')
			continue;
		switch(state) {
			case kLookingForMenu:
				if (GetNextToken(line, token)) {
					MenuResEntry *entry = new MenuResEntry;
					entry->mInternalMenuName = token;
					GetNextToken(line, entry->mMenuTitle);
					sMenuList.AddItem(entry);
					state = kInMenu;
					currentMenu = entry;
				}
				break;
			case kInMenu:
			case kInSubmenu:
				if (GetNextToken(line, token)) {
					if (token == "END") {
						if (state == kInMenu) {
							currentMenu = 0;
							state = kLookingForMenu;
						} else if (state == kInSubmenu) {
							currentSubmenu = 0;
							state = kInMenu;
						}
					} else if (token == "SUBMENU") {
						if (state == kInSubmenu)
							break;
						MenuResEntry *entry = new MenuResEntry;
						sMenuList.AddItem(entry);
						
						token = "";
						
						MenuItem *item = ParseRestOfItem(token, line);
						item->submenu = entry;
						
						currentMenu->mMenuItems.AddItem(item);
						state = kInSubmenu;
						currentSubmenu = entry;
					} else {
						MenuItem *item = ParseRestOfItem(token, line);
						item->submenu = 0;
						if (state == kInMenu)
							currentMenu->mMenuItems.AddItem(item);
						else
							currentSubmenu->mMenuItems.AddItem(item);
					}
				}
				break;
		}
	}
}

BMenu *BuildMenu(MenuResEntry *entry);
BMenu *BuildMenu(MenuResEntry *entry, bool desktopMode,	bool isImage, bool isLink, bool isFrame);

void BuildMenu(BMenu *menu, MenuResEntry *entry, bool desktopMode,
	bool isImage, bool isLink, bool isFrame)
{
	for (int i = 0; i < entry->mMenuItems.CountItems(); i++) {
		MenuItem *item = (MenuItem *)entry->mMenuItems.ItemAt(i);
		if (((item->enabled & 0x02) && !gRunningAsReplicant) ||
			((item->enabled & 0x04) && gRunningAsReplicant) ||
			((item->enabled & 0x08) && !desktopMode) ||
			((item->enabled & 0x10) && !isImage) ||
			((item->enabled & 0x20) && !isLink) ||
			((item->enabled & 0x40) && !isFrame) ||
			((item->enabled & 0x30) != 0x30 && (item->enabled & 0x30) != 0x00 && isImage && isLink) ||
			((item->enabled & 0x30) == 0x30 && (!isImage || !isLink)))
				continue;
		if (item->submenu) {
			BMenu *submenu = BuildMenu(item->submenu, desktopMode, isImage, isLink, isFrame);
			submenu->SetName(item->title.String());
			menu->AddItem(new BMenuItem(submenu));
		} else {
			if (item->title == "-")
				menu->AddSeparatorItem();
			else {
				BMenuItem *mItem = new BMenuItem(item->title.String(), new BMessage(item->cmd), item->equivalent, item->modifiers);
				if (!(item->enabled & 0x01))
					mItem->SetEnabled(false);
				menu->AddItem(mItem);
			}
		}
	}
}

BMenu *BuildMenu(MenuResEntry *entry, bool desktopMode,
	bool isImage, bool isLink, bool isFrame)
{
	BMenu *menu = new BMenu(entry->mMenuTitle.String());
	BuildMenu(menu, entry, desktopMode, isImage, isLink, isFrame);
	return menu;
}


BMenu *BuildMenu(const char *internalMenuName, bool desktopMode,
	bool isImage, bool isLink, bool isFrame)
{
	for (int i = 0; i < sMenuList.CountItems(); i++) {
		MenuResEntry *entry = (MenuResEntry *)sMenuList.ItemAt(i);
		if (entry->mInternalMenuName == internalMenuName) {
			return BuildMenu(entry, desktopMode, isImage, isLink, isFrame);
		}
	}
	return 0;
}

void BuildMenu(BMenu *menu, const char *internalMenuName, bool desktopMode,
	bool isImage, bool isLink, bool isFrame)
{
	for (int i = 0; i < sMenuList.CountItems(); i++) {
		MenuResEntry *entry = (MenuResEntry *)sMenuList.ItemAt(i);
		if (entry->mInternalMenuName == internalMenuName) {
			BuildMenu(menu, entry, desktopMode, isImage, isLink, isFrame);
			return;
		}
	}
}
