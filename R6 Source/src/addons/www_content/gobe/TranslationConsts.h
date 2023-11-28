//
//	TranslationConsts.h
//
#ifndef __TRANSLATIONCONSTS_H__
#define __TRANSLATIONCONSTS_H__

#include <Debug.h>

#ifdef ASSERT
	#undef ASSERT
#endif
#ifdef DEBUGGER
	#undef DEBUGGER
#endif

extern char* const DebugStrPtr(void);

#ifdef DEBUG
	#define ASSERT(condition, message)		do {if(!(condition)) {sprintf(DebugStrPtr(), "Assertion failure: %s, File: %s, Line: %d\n", message, __FILE__, __LINE__); debugger(DebugStrPtr());}} while(0)
	#define DEBUGGER()						debugger("Stopped in the debugger.\n")
	#define ASSERTC(condition)				do {if(!(condition)) {sprintf(DebugStrPtr(), "Assertion failure: %s, File: %s, Line: %d\n", #condition, __FILE__, __LINE__); debugger(DebugStrPtr());}} while(0)
	#define DEBUGSTR(msg)					do {if (_rtDebugFlag) debugger(msg);} while(0)
	#define IFDEBUG(exp)					exp
#else
	#define ASSERT(condition, message)
	#define ASSERTC(condition)
	#define DEBUGGER()
	#define DEBUGSTR(msg)
	#define IFDEBUG(exp)
#endif

// FILTER ERRORS // use standard error messages now
// FUCK SHIT BOGUS -- Who commented these out?  They are still in use! sdh 12/3/99
#define FILTER_NO_ERR				0	// Everything's fine!
#define FILTER_OUT_OF_MEMORY		1	// Memory exhausted
#define FILTER_ASSERTION_ERR		2	// Assertion failure
#define FILTER_END_OF_FILE_ERR		3	// Unexpected end of file

enum
{
	WRONG_SHEET_KIND_ERROR		= B_ERROR,
	MISSING_SHEET_KIND_ERROR	= B_ERROR,
	INVALID_STYLESHEET_ERROR	= B_ERROR,
	INVALID_STYLE_ERROR			= B_ERROR,
	INVALID_PROPERTIES_ERROR	= B_ERROR,
	STYLESHEET_WRITTEN_ERROR	= B_ERROR
};


// Don't change these, since they will affect WP file format
enum {
	X_RESERVED = 0, X_LINEBREAK = 1, X_FRAMEBREAK = 2, X_SECTIONBREAK = 3,
	X_MISCCHAR = 4, // DON"T USE THIS!  It has side effects
	X_CELLBREAK = 5,
	X_TAB = 9, X_PARBREAK = 10, X_PAGEBREAK = 12, X_SPACE = ' '
	
};

// Major and minor kinds for gobe compound document stream.
#define COMPOUND_DOCUMENT_KIND			'GOBE'
#define WORDPROCESSING_MINOR_KIND		'WORD'
#define SPREADSHEET_MINOR_KIND			'SPRD'
#define TABLE_MINOR_KIND				'TBLE'

// Top level chunk ids for a gobe compound document kind stream.
enum
{
	kStyleSheet_id			= '.sty',
	kDocumentInfo_id	 	= '.doc',
	kActiveSheetIndex_id	= '.asi',
	kSheet_id				= '.sht',
	kNameRef_id				= '.nam'
};

// General usage ids.
enum
{
	kArrayCount_id			= 'arr#',
	kKind_id				= 'kind',
	kValue_id				= 'valu',
	kIndex_id				= 'indx',
	kName_id				= 'name',
	kDefault_id				= 'def.',
	kPart_id				= 'part',
	kFrame_id				= 'fram',
	kText_id				= 'text',
	kAttachment_id			= 'atch',
	kBitmap_id				= 'bits',
	kData_id				= 'data'
};

// Stylesheet related ids.
enum
{
	kPropsArray_id			= 'prpA',
	kProperty_id			= 'prop',
	kStylesArray_id			= 'styA',
	kPropIndexArray_id		= 'prIA',
	kStyle_id				= 'styl',
	kStyleIndex_id			= 'styI',
	kBaseStyleIndex_id		= 'basI',
	kApplyStyleIndex_id		= 'appI'
};

// Single style level ids.
enum
{
//	kPropertiesChunkID		= 'PRPS',
//	kStyleTypeID			= 'TYPE',
//	kStyleNameID			= 'NAME',
	kBaseStyleNameID		= 'BASE',
	kApplyStyleNameID		= 'APLY'
};


// Sheet level ids
enum
{
	kSheetKind_id			= 'kind',	// INT32 kind of main part - REQUIRED 1st BLOCK
	kMainPart_id			= 'main',	// CHUNK main part stream.
	kHeaderPart_id			= 'head',	// CHUNK header part stream.
	kFooterPart_id			= 'foot',	// CHUNK footer part stream.
	kSheetMargins_id		= 'marg',	// RECT kind.
	kShowPaginated_id		= 'page'	// bool true if sheet should be paginated.
};

// Wordprocessing level ids
enum
{
//	kText_id				= 'text',
	kSpecialChar_id			= 'misc',
	kParStyleRuns_id		= 'pRns',
	kCharStyleRuns_id		= 'cRns',
	kStyleRun_id			= 'sRun',
	kNumColumns_id			= 'col#'
};

// Special character level ids
enum
{
	kFormula_id				= 'spfm',
	kFormulaResult_id		= 'spfr',
	kFormatString_id		= 'spfs'
};

// Word processing special character types.
enum
{
	kDATE 	 				= 'date',
	kSHORTDATE				= 'sdat',
	kLONGDATE				= 'ldat',
	kTIME					= 'time',
	kPAGENUM				= 'page',
	kPAGEBREAK				= 'pgbk',
	kFRAMEBREAK				= 'fbrk',
	kLINENUM				= 'line'
};
		
// StyleRun level ids
enum
{
	kStyleRunStart_id		= 'rbeg',
	kStyleRunEnd_id			= 'rend'
};

// Color level ids.
enum
{
	kHighColor_id			= 'high',
	kLowColor_id			= 'low ',
	kPattern_id				= 'patt'
};

// Font level ids.
enum
{
	kFontName_id			= 'name',
	kFontStyle_id			= 'styl',
	kFontID_id				= 'beID'
};

// Spreadsheet related ids.
enum
{
	kColumnInfo_id			= 'colI',
	kRowInfo_id				= 'rowI',
	kDisplayOptions_id		= 'dspF',
	kSpreadsheetData_id		= 'data'
};

// Spreadsheet data related ids.
enum
{
	kRowData_id				= 'rDat',
	kCellData_id			= 'cell',
		
	kStartRow_id			= 'sRow',
	kEndRow_id				= 'eRow',
	kStartColumn_id			= 'sCol',
	kEndColumn_id			= 'eCol',
	kCellRow_id				= '.row',
	kCellColumn_id			= '.col',

	kCellFlags_id			= 'cFlg',
	kCellText_id			= 'cTxt',
	kCellFormula_id			= 'cFml',
	kCellResult_id			= 'cRES'
};

// Spreadsheet row/column ids.
enum
{
//	kDefaultColumnWidth_id	= 'defW',
//	kDefaultRowHeight_id	= 'defH',
	kColumnFormat_id		= 'colF',
	kRowFormat_id			= 'rowF'
};

// Spreadsheet display options ids.
enum
{
	kShowGrid_id			= 'dGrd',
	kShowLockedCells_id		= 'dLck',
	kShowFormulas_id		= 'dFrm',
	kShowColumnHeaders_id	= 'dCol',
	kShowRowHeaders_id		= 'dRow',
	kShowCircularRefs_id	= 'dCir',
	kShowZeros_id			= 'dZer'
};

enum
{
	kStyleChunkID			= 'STYL'
};

// Table related ids.
enum
{
	kRowSpan_id				= 'row#',
	kColumnSpan_id			= 'col#',
	kTableRow_id			= 'row ',
	kTableCell_id			= 'cell',
	kTableWidth_id			= 'twd#',
	kTableRelWidth_id		= 'twd%',
	kTableHeight_id			= 'tht#',
	kTableCellWidth_id		= 'cwd#',
	kTableCellRelWidth_id	= 'cwd%',
	kTableCellHeight_id		= 'cht#',
	kTableCellRelHeight_id	= 'cht%',
	kTableBorderSize_id		= 'tbdr',
	kTableCellSpacing_id	= 'tspc',
	kTableCellPadding_id	= 'tpad',
	kTableKind_id			= 'tknd',
	kCellPosition_id		= 'cPos'
};

enum
{
	kTableKind_GOBE = 'GOBE',
	kTableKind_HTML = 'HTML',
	kTableKind_WORD = 'MSWD'
};

// Special character related ids.
enum
{
	kAutoNumber_id			= 'auto',
	kIsEndnote_id			= 'endN',
	kCustomMark_id			= 'mark'
};

typedef enum
{
	kSCUnknown			= 'sc??',
	kSCFormula			= 'scFM',
	kSCBreak			= 'scBR',
	kSCPageNumber		= 'scPG',
	kSCHorizontalRule	= 'scHR',
	kSCFrame			= 'scFR',
	kSCFootnote			= 'scFN',
	kSCFootnoteMark		= 'scFM'
} SpecialCharacterTypes;

typedef	enum
{
	kAttachUnknown 		= '????',
	kAttachNamedRef		= 'Name',
	kAttachLink		 	= 'Link'
} AttachmentTypes;
		


// Deprecated constants
// Classes of Squirrel translators
#define SQUIRREL_TRANSLATOR_TEXT		'GPWP'
#define SQUIRREL_TRANSLATOR_SPREADSHEET	'GPSS'

// The structure of the data stream
#define SQUIRREL_TEXT_STREAM			'GPWP'
#define SQUIRREL_SPREADSHEET_STREAM		'GPSS'



// The translator type and the stream type have to be the same.
#define SQUIRREL_TRANSLATOR_NEW_TEXT		WORDPROCESSING_MINOR_KIND
#define SQUIRREL_TRANSLATOR_NEW_SPREADSHEET	SPREADSHEET_MINOR_KIND
#define SQUIRREL_TEXT_NEW_STREAM			WORDPROCESSING_MINOR_KIND
#define SQUIRREL_SPREADSHEET_NEW_STREAM		SPREADSHEET_MINOR_KIND



// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslationConsts.h,v 1.31 2000/02/17 22:18:25 tom Exp $
#endif // __TRANSLATIONCONSTS_H__
