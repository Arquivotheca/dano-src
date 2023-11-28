//
//	WORDstructs.h
//
#ifndef __WORDSTRUCTS_H__
#define __WORDSTRUCTS_H__


// These are the structures used in the WORD file format.
// Be careful when doing a sizeof on these, as the compiler
// may add padding (use the SIZEOF_### to get the size WORD thinks it is).

// These should be in alphabetical order


// ANLD - Autonumbered List Data Descriptor
#define SIZEOF_ANLD		84
typedef struct the_anld
{
	uchar	nfc;
	uchar	cxchTextBefore;
	uchar	cxchTextAfter;
	uchar	bitField1;
	uchar	bitField2;
	uchar	bitField3;
	short	ftc;
	ushort	hps;
	ushort	iStartAt;
	ushort	dxaIndent;
	ushort	dxaSpace;
	uchar	fNumber1;
	uchar	fNumberAcross;
	uchar	fRestartHdn;
	uchar	fSpareX;
	char	rgxch[64];
} ANLD;


// BRC - Border Code
typedef struct the_brc
{
	short	bitField1;
	short	bitField2;
} BRC;

typedef enum 
{ 
	dptLineWidth	= 0x00FF,
	brcType 		= 0xFF00
} BRC_BitField1_Masks;

typedef enum 
{ 
	icoBRC			= 0x00FF,
	dptSpace 		= 0x1F00,
	fShadowBRC		= 0x2000,
	fFrame			= 0x4000
} BRC_BitField2_Masks;


// BTE - bit table entry (long)
#define SIZEOF_BTE		4
typedef enum 
{ 
	btePN		= 0x003FFFFF,		// :22
	bteUNUSED	= 0xFFC00001		// :10
} BTE_BitField1_Mask;


// CHP - Character Properties (not saved in file, but used when CHPX is expanded)
// default CHP is all zeros except hps(20 half-points), fcPic(-1), istd(10), lidDefault(0x0400),
// lidFE(0x0400), wCharScale(100), fUsePgsuSettings(-1)
#define SIZEOF_CHP		136
typedef struct the_chp
{
	ushort		bitField1;
	ushort		bitField2;
	long		reserved1;
	short		ftc;				// no longer used
	short		ftcAscii;			// font for ascii text
	short		ftcFE;				// font for Far East text
	short		ftcOther;			// font for non Far East text
	ushort		hps;				// font size in half-points
	long		dxaSpace;			// space following chars in twips
	short		bitField3;
	short		scriptPos;			// half-points, postive super, negative sub
	ushort		lid;				// language ID
	ushort		lidDefault;
	ushort		lidFE;				// language for Far East
	uchar		idct;				// internal to WORD
	uchar		idctHint;
	ushort		wCharScale;
	long		fcPicObjTagObj;		// offset in data stream for Pic and Ole objects
	short		ibstRMark;
	short		ibstRMarkDel;
	long		dttmRMark;
	long		dttmRMarkDel;
	short		reserved2;
	ushort		istd;
	short		ftcSym;
	char		xchSym[2];
	short		idslRMReason;
	short		idslReasonDel;
	uchar		ysr;
	uchar		chYsr;
	ushort		cpg;
	ushort		hpsKern;
	ushort		bitField4;
	ushort		fPropMark;
	short		ibstPropRMark;
	long		dttmPropRMark;
	uchar		sfxtText;
	uchar		reserved3;
	uchar		reserved4;
	ushort		reserved5;
	short		reserved6;
	long		dttmReserved;
	uchar		fDispFldRMark;
	short		ibstDispFldRMark;
	long		dttmDispFldRMark;
	char		xstDispFldRMark[32];
	short		shd;
	BRC			brc;
} CHP;

typedef enum 
{ 
	fBold				= 0x0001,
	fItalic				= 0x0002,
	fRMarkDel			= 0x0004,	// strikethru when revision displayed
	fOutline			= 0x0008,
	fFldVanish			= 0x0010,
	fSmallCaps			= 0x0020,
	fCaps				= 0x0040,
	fVanish				= 0x0080,
	fRMark				= 0x0100,	// underline when revision displayed
	fSpec				= 0x0200,	// WORD special character
	fStrike				= 0x0400,
	fObj				= 0x0800,	// embedded object
	fShadowCHP			= 0x1000,
	fLowerCase			= 0x2000,
	fData				= 0x4000,	// form field
	fOle2				= 0x8000
} CHP_BitField1_Mask;

typedef enum 
{ 
	fEmboss				= 0x0001,
	fImprint			= 0x0002,	// engraved
	fDStrike			= 0x0004,	// double strikethru
	fUsePgsuSettings	= 0x0008
	//unused			= 0xFFF0
} CHP_BitField2_Mask;

typedef enum 
{ 
	iss					= 0x0007,	// super,subscript indices (0-none, 1-super, 2-sub)
	kul					= 0x0078,	// underline code
	fSpecSymbol			= 0x0080,	// internal to WORD
	icoCHP					= 0x1F00,	// text color
	//unused			= 0x2000,
	fSysVanish			= 0x4000,	// internal to WORD
	hpsPos				= 0x8000	// internal to WORD
} CHP_BitField3_Mask;

typedef enum 
{ 
	icoHighlight		= 0x001F,	// highlight color
	fHighlight			= 0x0020,
	kcd					= 0x01C0,
	fNavHighlight		= 0x0200,	// internal to WORD
	fChsDiff			= 0x0400,
	fMacChs				= 0x0800,
	fFtcAsciSym			= 0x1000
	//unused			= 0xE000
} CHP_BitField4_Mask;

// TC Table Cell Descriptors. found in the sprmTDefTable in the last style of a table row
#define SIZEOF_TC		20
typedef struct the_tc
{
	ushort		bitField1;
	ushort		reserved1;
	BRC			brcTop;
	BRC			brcLeft;
	BRC			brcBottom;
	BRC			brcRight;
} TC ;

typedef enum 
{ 
	tc_fFirstMerged			= 0x0001,	
	tc_fMerged				= 0x0002,
	tc_fVertical			= 0x0004,
	tc_fBackward			= 0x0008,
	tc_fRotateFont			= 0x0010,
	tc_fVertMerged			= 0x0020,
	tc_fVertRestart			= 0x0040,
	tc_vertAlign			= 0x0180
} TC_BitField1_Mask;

// DCS - Drop Cap Specifier (a short)
#define SIZEOF_DCS		2
typedef enum 
{ 
	dcType			= 0x0007,
	dcCount 		= 0x00F8
} DCS_BitField1_Masks;


// DOP - document properties
#define SIZEOF_DOP		500
typedef struct dop
{
	ushort		bitField1;
	ushort		bitField2;
	ushort		bitField3;
	ushort		bitField4;
	ushort		bitField5;
	ushort		dxaTab;		// def 720 twips
	ushort		wSpare;
	ushort		dxaHotZ;
	ushort		cConsecHypLim;
	ushort		wSpare2;
	long		dttmCreated;
	long		dttmRevised;
	long		dttmLastPrint;
	short		nRevision;
	long		tmEdited;
	long		cWords;
	long		cCh;
	short		cPg;
	long		cParas;
	ushort		bitField6;	// same mask as bitField2
	ushort		bitField7;
	long		cLines;
	long		cWordsFtnEdn;
	long		cChFtnEdn;
	short		cPgFtnEdn;
	long		cParasFtnEdn;
	long		cLinesFtnEdn;
	long		lKeyProtDoc;
	ushort		bitField8;
	
	// if nFib < 103, DOP stops with above (84 bytes)
	ulong		bitField9;
	
	// if nFib < 106, DOP stops with above (88 bytes)
	short		adt;
	char		doptypography[310];		// DOPTYPOGRAPHY
	char		dogrid[10];				// DOGRID
	ushort		bitField10;
	ushort		bitField11;
	char		asumyi[12];					// ASUMYI
	long		cChWS;
	long		cChWSFtnEdn;
	long		grfDocEvents;
	long		bitField12;
	char		spare[30];
	long		reserved1;
	long		reserved2;
	long		cDBC;
	long		cDBCFtnEdn;
	long		reserved3;
	short		nfcFtnRef;
	short		nfcEdnRef;
	short		hpsZoonFontPag;
	short		dywDispPag;
} DOP;

typedef enum 
{ 
	fFacingPages	 	= 0x0001,
	fWidowControl		= 0x0002,
	fPMHMainDoc			= 0x0004,
	grfSuppression		= 0x0018,
	fpc					= 0x0060,
	//unused			= 0x0080,
	grpfIhdt			= 0xFF00	 
} DOP_BitField1_Mask;

typedef enum 
{ 
	rncFtn			 	= 0x0003,
	nFtn				= 0xFFFC
} DOP_BitField2_Mask;

typedef enum 
{ 
	fOutlineDirtySave	= 0x0001,
	//unused			= 0x00FE,
	fOnlyMacPics		= 0x0100,
	fOnlyWinPics		= 0x0200,
	fLabelDoc			= 0x0400,
	fHyphCapitals		= 0x0800,
	fAutoHyphen			= 0x1000,
	fFormNoFields		= 0x2000,
	fLinkStyles			= 0x4000,
	fRevMarking			= 0x8000
} DOP_BitField3_Mask;

typedef enum 
{ 
	fBackup			 	= 0x0001,
	fExactCWords		= 0x0002,
	fPagHidden			= 0x0004,
	fPagResults			= 0x0008,
	fLockAtn			= 0x0010,
	fMirrorMargins		= 0x0020,
	//unused			= 0x0040,
	fDfltTrueType		= 0x0080,
	fPagSuppressTopSpacing = 0x0100,
	fProtEnabled		= 0x0200,
	fDispFormFldSel		= 0x0400,
	fRMView				= 0x0800,
	fRMPrint			= 0x1000,
	//unused			= 0x2000,
	fLockRev			= 0x4000,
	fEmbedFonts			= 0x8000
} DOP_BitField4_Mask;

typedef enum 
{ 
	wfNoTabForInd		= 0x0001,
	wfNoSpaceRaiseLower	= 0x0002,
	wfSuppressSpbfAfterPageBreak	= 0x0004,
	wfWrapTrailSpaces	= 0x0008,
	wfMapPrintTextColor	= 0x0010,
	wfNoColumnBalance	= 0x0020,
	wfConvMailMergeEsc	= 0x0040,
	wfSuppressTopSpacing	= 0x0080,
	wfOrigWordTableRules = 0x0100,
	wfTransparentMetafiles = 0x0200,
	wfShowBreaksInFrames	= 0x0400,
	wfSwapBordersFacingPgs = 0x0800
	//unused			= 0xF000
} DOP_BitField5_Mask;

typedef enum 
{ 
	epc			 		= 0x0003,
	nfcFtnRef			= 0x003C,
	nfcEdnRef			= 0x03C0,
	fPrintFormData		= 0x0400,
	fSaveFormData		= 0x0800,
	fShadeFormData		= 0x1000,
	//unused			= 0x6000,
	fWCFtnEdn			= 0x8000
} DOP_BitField7_Mask;

typedef enum 
{ 
	wvkSaved			= 0x0007,
	wScaleSaved			= 0x0FF8,
	zkSaved				= 0x3000,
	fRotateFontW6		= 0x4000,
	iGutterPos			= 0x8000
} DOP_BitField8_Mask;

typedef enum 
{ 
	fNoTabForInd				= 0x00000001,
	fNoSpaceRaiseLower			= 0x00000002,
	fSuppressSpbfAfterPageBreak	= 0x00000004,
	fWrapTrailSpaces			= 0x00000008,
	fMapPrintTextColor			= 0x00000010,
	fNoColumnBalance			= 0x00000020,
	fConvMailMergeEsc			= 0x00000040,
	fSuppressTopSpacing			= 0x00000080,
	fOrigWordTableRules 		= 0x00000100,
	fTransparentMetafiles 		= 0x00000200,
	fShowBreaksInFrames			= 0x00000400,
	fSwapBordersFacingPgs 		= 0x00000800,
	//reserved					= 0x0000F000,
	fSuppressTopSpacingMac		= 0x00010000,
	fTruncDxaExpand				= 0x00020000,
	fPrintBodyBeforeHdr			= 0x00040000,
	fNoLeading					= 0x00080000,
	//reserved					= 0x00100000,
	fMWSmallCaps				= 0x00200000
	//reserved					= 0xFFC00000
} DOP_BitField9_Mask;


// DTTM - date time (a long)
#define SIZEOF_DTTM		4


// FFN - Font Family Name
#define SIZEOF_PANOSE			10
#define SIZEOF_FONTSIGNATURE	24
#define SIZEOF_FFN_FIXED		(5 + SIZEOF_PANOSE + SIZEOF_FONTSIGNATURE)
#define FFN_MAX_NAME_BYTES		131
typedef struct the_ffn
{
	// START FIXED PORTION
	uchar	cbFfnM1;					// length of FFN - 1
	uchar	bitField;			
	short	wWeight;					// base weight of font
	uchar	chs;						// character set identifier
	uchar	ixchSzAlt;					// index into ffn.szFfn to name of alternate font
	char	panose[SIZEOF_PANOSE];		// PANOSE
	char	fs[SIZEOF_FONTSIGNATURE]; 	// FONTSIGNATURE
	// END FIXED PORTION
	char	xszFfn[FFN_MAX_NAME_BYTES];	// XCHAR[] (unicode) font name (may have second name)
										// max of 65 characters (not clear whether that's chars or bytes)
} FFN;

typedef enum 
{ 
	prq				= 0x03,				// pitch request
	fTrueType		= 0x04,
	ffnReserved1	= 0x08,
	ff				= 0x70,				// font family id
	ffnReserved2	= 0x80
} FFN_BitField_Mask;


//
// FIB - this is the File Information Block, and is the first thing in the file
// For Word 8.0, it consists of the FIBHeader, an array of shorts, an array of longs, and
// finally an array of long pairs. 
//
typedef struct fibheader
{
	ushort		wIdent;		// magic number
	ushort		nFib;		// FIB version - Word 6.0 and later will be >= 101
	ushort		nProduct;	// product version written by
	ushort		lid;		// language stamp - localized version
	short		pnNext;		
	ushort		bitField1;
	ushort		nFibBack;	// compatible with readers that understand nFib >= this value
	ulong		lKey;		// encryption key
	uchar		envr;		// 0 - Win Word, 1 - Mac Word
	uchar		bitField2;
	ushort		chs;		// default char set for text stream, 0 - Windows ANSI, 256 - Mac char set
	ushort		chsTables;	// default char set for text in internal structures, 0 - Windows ANSI, 256 - Mac char set
	long		fcMin;		// file offset of first char of text
	long		fcMac;		// file offset of last character of text in doc text stream + 1
} FIBHeader;

typedef enum 
{ 
	fDot	 				= 0x0001,	// set if template 
	fGlsy	 				= 0x0002,	// set if glossary 
	fComplex	 			= 0x0004,	// set if complex, fast-save 
	fHasPic		 			= 0x0008, 	// set if contains 1 or more pictures
	cQuickSaves			 	= 0x00F0,	// count of times file was quick-saved
	fEncrypted		 		= 0x0100,	// set if encrypted
	fWhichTblStm	 		= 0x0200,	// which table stream, 0 is 0Table, 1 is 1Table
	fReadOnlyRecommended 	= 0x0400,	// set when user recommends read-only
	fWriteReservation		= 0x0800,	// set when owner has made it write reserved
	fExtChar				= 0x1000,	// set when using extended character set
	fLoadOverride			= 0x2000,	// REVIEW
	fFarEast				= 0x4000,	// REVIEW
	fCrypto					= 0x8000	// REVIEW
} FIB_BitField1_Masks;

typedef enum 
{ 
	fMac	 				= 0x01,		// set if last saved in Mac environment 
	fEmptySpecial	 		= 0x02, 
	fLoadOverridePage	 	= 0x04, 
	fFutureSavedUndo		= 0x08,
	fWord97Saved			= 0xF0,
	fSpare0					= 0xFE
} FIB_BitField2_Masks;


// FIB Shorts
typedef enum
{
	wMagicCreated = 0,		// file's creator id, 0x6A62 is Word and is reserved
	wMagicRevised,			// file's last modifier
	wMagicCreatedPrivate,
	wMagicRevisedPrivate,
	unused1, unused2, unused3, unused4, unused5, unused6, unused7, unused8, unused9,
	lidFE,					// language id if doc in FarEast Word (FIB.fFarEast is on)
	NUM_FIB_SHORTS
} FIB_Shorts_Items;


// FIB Longs
typedef enum
{
	cbMac = 0,				// file offset of last byte written to file + 1
	lProductCreated,		// build date of creator.  10695 is Jan 6, 1995
	lProductRevised,		// build date of file's last modifier
	ccpText,				// length of main document text stream
	ccpFtn,					// length of footnote subdocument text stream
	ccpHdr,					// length of header subdocument text stream
	ccpMcr,					// length of macro subdocument text stream
	ccpAtn,					// length of annotation subdocument text stream
	ccpEdn,					// length of endnote subdocument text stream
	ccpTxbx,				// length of textbox subdocument text stream
	ccpHdrTxbx,				// length of header textbox subdocument text stream
	pnFbpChpFirst,
	pnChpFirst,				// page num of lowest numbered page that records CHPX FKP info
	cpnBteChp,				// count of CHPX FKPs
	pnFbpPapFirst,
	pnPapFirst,				// page num of lowest numbered page that records PAPX FKP info
	cpnBtePap,				// count of PAPX FKPs
	pnFbpLvcFirst,
	pnLvcFirst,				// page num of lowest numbered page that records LVC FKP info
	cpnBteLvc,				// count of LVC FKPs
	fcIslandFirst,
	fcIslandLim,
	NUM_FIB_LONGS
} FIB_Longs_Items;


// FIB Pairs
typedef enum
{
	StshfOrig = 0,			// offset and bytes of original STSH
	Stshf,					// offset and bytes of STSH
	PlcffndRef,				// offset and bytes of footnote FRD structures
	PlcffndTxt,				// offset and bytes of footnote text
	PlcfandRef,				// offset and bytes of annotation ATRD 
	PlcfandTxt,				// offset and bytes of annotation text 
	Plcfsed,				// offset and bytes of section descriptors SED
	Plcpad,					// no longer used 
	Plcfphe,				// offset and bytes of paragraph heights PHE 
	Sttbfglsy,				// offset and bytes of glossary string table (pascal strings) 
	Plcfglsy,				// offset and bytes of glossary 
	Plcfhdd,				// offset and bytes of header HDD 
	PlcfbteChpx,			// offset and bytes of character property bin table 
	PlcfbtePapx,			// offset and bytes of paragraph property bin table 
	Plcfsea,				// private 
	Sttbfffn,				// offset and bytes of font information STTBF (FFN) 
	PlcffldMom,				// offset and bytes of field positions 
	PlcffldHdr,				// offset and bytes of field positions in header 
	PlcffldFtn,				// offset and bytes of field positions in footnote 
	PlcffldAtn,				// offset and bytes of field positions in annotation subdocument 
	PlcffldMcr,				// no longer used 
	Sttbfbkmk,				// offset and bytes of bookmarks STTBF 
	Plcfbkf,				// offset and bytes of bookmarks PLCF 
	Plcfbkl,				// offset and bytes of ending bookmarks PLCF 
	Cmds,					// offset and bytes of macros 
	Plcmcr,					// no longer used
	Sttbfmcr,				// no longer used
	PrDrvr,					// offset and bytes of printer driver information 
	PrEnvPort,				// offset and bytes of print environment in portrait mode 
	PrEnvLand,				// offset and bytes of print environment in landscape mode 
	Wss,					// offset and bytes of Window Save State WSS 
	Dop,					// offset and bytes of document property 
	SttbfAssoc,				// offset and bytes of associated strings (doc summary et al)
	Clx,					// offset and bytes of complex files 
	PlcfpgdFtn,				// no longer used
	AutosaveSource,			// offset and bytes of name of original file 
	GrpXstAtnOwners,		// offset and bytes of annotation owners 
	SttbfAtnbkmk,			// offset and bytes of names of bookmarks for annotation subdocument 
	PlcdoaMom,				// no longer used 
	PlcdoaHdr,				// no longer used 
	PlcspaMom,				// offset and bytes of office art objects FSPA 
	PlcspaHdr,				// offset and bytes of office art objects FSPA in header 
	PlcfAtnbkf,				// offset and bytes of bookmark first BKF in annotation subdocument 
	PlcfAtnbkl,				// offset and bytes of bookmark last BKL in annotation subdocument 
	Pms,					// offset and bytes of print merge state PMS 
	FormFldSttbs,			// offset and bytes of form field dropdown controls
	PlcfednRef,				// offset and bytes of endnote references
	PlcfednTxt,				// offset and bytes of endnote text
	PlcffldEdn,				// offset and bytes of field positions in endnotes
	PlcfpgdEdn,				// no longer used 
	DggInfo,				// offset and bytes of office art object table data
	SttbfRMark,				// offset and bytes of author abbreviations of revisors
	SttbCaption,			// offset and bytes of caption titles
	SttbAutoCaption,		// offset and bytes of auto caption titles
	Plcfwkb,				// offset and bytes of boundaries in master doc WKB
	Plcfspl,				// offset and bytes of spell check state SPLS	
	PlcftxbxTxt,			// offset and bytes of text box entries
	PlcffldTxBx,			// offset and bytes of text box boundaries
	PlcfhdrtxbxTxt,			// offset and bytes of header text box entries
	PlcffldHdrTxBx,			// offset and bytes of header text box boundaries
	StwUser,				// offset and bytes of Macro User storage
	Sttbttmbd,				// offset and bytes of embedded TT font info
	Unused,					// no longer used 
	PgdMother,				// offset and bytes of page descriptors
	BkdMother,				// offset and bytes of break descriptors
	PgdFtn,					// offset and bytes of page descriptors for footnotes
	BkdFtn,					// offset and bytes of break descriptors for footnotes
	PgdEdn,					// offset and bytes of page descriptors for endnotes
	BkdEdn,					// offset and bytes of break descriptors for endnotes
	SttbfIntlFld,			// offset and bytes of intl field keywords
	RouteSlip,				// offset and bytes of mailer routing slip
	SttbSavedBy,			// offset and bytes of names of savers
	SttbFnm,				// offset and bytes of referenced filenames
	PlcfLst,				// offset and bytes of list format information
	PlfLfo,					// offset and bytes of list format override information
	PlcftxbxBkd,			// offset and bytes of textbox break table
	PlcftxbxHdrBkd,			// offset and bytes of textbox break table for headers
	DocUndo,				// offset and bytes of undo / versioning
	Rgbuse,					// offset and bytes of undo / versioning
	Usp,					// offset and bytes of undo / versioning
	Uskf,					// offset and bytes of undo / versioning
	PlcupcRgbuse,			// offset and bytes of undo / versioning
	PlcupcUsp,				// offset and bytes of undo / versioning
	SttbGlsyStyle,			// offset and bytes of style names for glossary
	Plgosl,					// offset and bytes of grammar options
	Plcocx,					// offset and bytes of ocx
	PlcfbteLvc,				// offset and bytes of character property bin table
	Modified,				// lowDateTime and highDateTime
	Plcflvc,				// offset and bytes of LVC
	Plcasumy,				// offset and bytes of auto-summary
	Plcfgram,				// offset and bytes of grammar check state SPLS
	SttbListNames,			// offset and bytes of list names
	SttbfUssr,				// offset and bytes of undo / versioning
	NUM_FIB_PAIRS
} FIB_Pairs_Items;

typedef struct fibpair
{
	long		theLong;
	ulong		theULong;
} FIB_Pair;


// FSPA - File Shape Address
#define SIZEOF_FSPA		26
typedef struct the_fspa
{
	long	spid;
	long	xaLeft;
	long	yaTop;
	long	xaRight;
	long	yaBottom;
	ushort	bitField1;
	long	cTxbx;
} FSPA;


// HDD indexes into plcfhdd
enum
{
	// Word doc is wrong, as it says these start at 0
	hddHeaderEven = 6,
	hddHeaderOdd,
	hddFooterEven,
	hddFooterOdd,
	hddHeaderFirst,
	hddFooterFirst
};


// LSPD - Line Spacing Descriptor
typedef struct the_lspd
{
	short	dyaLine;			// see sprmPDyaLine
	short	fMultLinespace;
} LSPD;


// NUMRM - Number Revision Mark Data
#define SIZEOF_NUMRM		128
typedef struct the_numrm
{
	uchar	fNumRM;
	uchar	spare1;
	short	ibstNumRM;
	long	dttmNumRM;
	uchar	rgbxchNums[9];
	uchar	rgnfc[9];
	short	spare2;
	long	pnbr[9];
	char	xst[64];
} NUMRM;


// PHE - Paragraph Height
// out of order, used by PAP
typedef struct the_phe
{
	short	bitField1;
	short	bitField2;
	long	dymLine;
	long	dymHeight;
} PHE;

// FKP - used for in memory version.
typedef struct thefkp
{
	char	fkp[512];	// always 512 byte pages
} FKP;


// BX - used in FKP for PAPXs
#define SIZEOF_BX		13
typedef struct the_bx
{
	uchar	bxOffset;
	PHE		bxPHE;
} BX;


// PAP - Paragraph Properties (not saved in file, but used when expanding PAPX)
// default PAP is all 0s, except fWidowCtl(1), fMultLineSpace(1), dyaLine(240twips), lvl(9)
#define SIZEOF_PAP	610
typedef struct pap
{
	ushort			istd;				// index to STD in STSH
	uchar			jc;					// justification (0-left, 1-center, 2-right, 3-full)
	uchar			fKeep;				// keep para on one page if possible
	uchar			fKeepFollow;		// keep para on same page with next para if possible
	uchar			fPageBreakBefore;	// start this para on new page
	uchar			bitField1;			// see PAP_BitField1_Masks
	uchar			brcp;				// superseded by brcLeft, brcTop, etc
	uchar			brcl;				// ""
	uchar			reserved1;
	uchar			ilvl;				// list level
	uchar			fNoLnn;				// no line numbering
	short			ilfo;				// 1-based index into pllfo id
	uchar			reserved2;
	uchar			reserved3;
	uchar			fSideBySide;		// side-by-side paragraph
	uchar			reserved4;
	uchar			fNoAutoHyph;		// when 0, may be auto hyph
	uchar			fWidowControl;		// when 1, prevent widowed lines at beginning of page
	long			dxaRight;			// indent from right margin
	long			dxaLeft;			// indent from left margin
	long			dxaLeft1;			// first line indent, relative to dxaLeft
	LSPD			lspd;				// line spacing descriptor
	ulong			dyaBefore;			// vertical spacing before
	ulong			dyaAfter;			// vertical spacing after
	PHE				phe;				// height of paragraph
	uchar			fCrLf;				//
	uchar			fUsePgsuSettings;
	uchar			reserved5;
	char			fKinsoku;			// apply kinsoku rules
	uchar			fWordWrap;
	uchar			fOverflowPunct;
	uchar			fTopLinePunct;
	uchar			fAutoSpaceDE;
	uchar			fAutoSpaceDN;
	ushort			wAlignFont;			// 0-hanging, 1-centered, 2-roman, 3-variable, 4-auto
	ushort			bitField2;
	short			reserved6;
	char			fInTable;			// para is in a table row
	char			fTtp;				// para consists only of row marks and is end of table row
	uchar			wrapCode;
	uchar			fLocked;
	long			reserved7;
	long			dxaAbs;
	long			dyaAbs;
	long			dxaWidth;
	BRC				brcTop;
	BRC				brcLeft;
	BRC				brcBottom;
	BRC				brcRight;
	BRC				brcBetween;
	BRC				brcBar;
	long			dxaFromText;
	long			dyaFromText;
	ushort			bitField3;			// dyaHeight and fMinHeight
	ushort			shd;				// SHD - shading
	ushort			dcs;				// DCS - drop cap specifier
	char			lvl;
	char			fNumRMIns;
	ANLD			anld;				// autonumber list descriptor
	short			fPropRMark;
	short			ibstPropRMark;
	long			dttmPropRMark;
	NUMRM			numrm;
	short			itbdMac;			// must be 64
	short			rgdxaTab[64];
	char			rgtbd[128];
} PAP;

typedef enum 
{ 
	fBrLnAbove		= 0x01,
	fBrLnBelow 		= 0x02,
	//unused		= 0x06,
	pcVert			= 0x30,			// vertical position code
	pcHorz			= 0xC0			// horizontal position code
} PAP_BitField1_Masks;

typedef enum 
{ 
	fVertical		= 0x0001,
	fBackward 		= 0x0002,
	fRotateFont		= 0x0004
} PAP_BitField2_Masks;

typedef enum 
{ 
	dyaHeight		= 0x7FFF,
	fMinHeight 		= 0x1000
} PAP_BitField3_Masks;


// PCD - Piece Descriptor
#define SIZEOF_PCD		8
typedef struct pcd
{
	ushort		bitField1;
	long		fc;			// file offset of beginning of piece
							// if 2nd MSD is off, then we're pointing at unicode,
							// else it's CP1252 (clear and divide by 2)
	ushort		prm;		// see PRM_BitField1_Mask
} PCD;

typedef enum 
{ 
	fNoParaLast	 	= 0x0001		// piece contains no end of paragraph marks 
} PCD_BitField1_Mask;


// PRM - Property Modifier (short)
#define SIZEOF_PRM		2
typedef enum 
{ 
	fComplexPRM	 	= 0x0001,		// 0 for variant 1, 1 for variant 2
	isprm			= 0x00FE,		// var1: index to entry into rgsprmPrm 
	val				= 0xFF00,		// var1: sprm's operand
	igrpprl			= 0xFFFE		// var2: index to a grpprl stored in CLX portion of file 
} PRM_BitField1_Mask;


// SHD - Shading Descriptor (a short)
#define SIZEOF_SHD		2
typedef enum 
{ 
	icoFore			= 0x001F,
	icoBack 		= 0x03E0,
	ipat			= 0xFC00
} SHD_BitField1_Masks;


// STD - Style Sheet Descriptor
typedef struct std_base
{
	ushort			bitField1;		// sti
	ushort			bitField2;		// style type code
	ushort			bitField3;		// num of UPXs
	ushort			bchUpe;			// offset to end of UPXs, start of UPEs
	ushort			bitField4;		// auto redefine
} STD_BASE;

typedef enum 
{ 
	sti				= 0x0FFF,		// invariant style identifier
	fScratch 		= 0x1000,		// spare, always reset to 0
	fInvalHeight	= 0x2000,		// PHEs of all text with this style are wrong
	fHasUpe 		= 0x4000,		// UPEs have been generated
	fMassCopy 		= 0x8000 		// std has been mass copied; if unused at save time, style should be deleted
} STD_BASE_BitField1_Masks;

typedef enum 
{ 
	sgc				= 0x000F,		// style type code
	istdBase 		= 0xFFF0,		// base style
	sgcPara			= 1,
	sgcChp			= 2,
	nullBaseStyle	= 4095
} STD_BASE_BitField2_Masks;

typedef enum 
{ 
	cupx			= 0x000F,		// number of UPXs
	istdNext 		= 0xFFF0		// next style
} STD_BASE_BitField3_Masks;

typedef enum 
{ 
	fAutoRedef		= 0x0001,		// auto redefine style when appropriate
	fHidden 		= 0x0002		// hidden from UI?
} STD_BASE_BitField4_Masks;


// STSHI - Style Sheet Header
typedef struct stshi
{
	ushort		cstd;						// count of styles in stylesheet
	ushort		cbSTDBaseInFile;			// length of STD base
	ushort		bitField1;
	ushort		stiMaxWhenSaved;			// Max sti known when file saved
	ushort		istdMaxFixedWhenSaved;		// count of fixed index istds
	ushort		nVerBuiltInNamesWhenSaved;	// Current version of built-in stylesheets
	ushort		rgftcStandardChpStsh[3];	// ftc used by StandardChpStsh	
} STSHI;

typedef enum 
{ 
	fStdStyleNamesWritten	= 0x01		// are built-in style names stored? 
} STSHI_BitField1_Masks;


// TBD - tab descriptor
#define SIZEOF_TBD		1
typedef enum 
{ 
	tbd_jc			= 0x07,		// justification code
	tbd_jc_left 	= 0x00,		// left tab
	tbd_jc_center 	= 0x01,		// center tab
	tbd_jc_right 	= 0x02,		// right tab
	tbd_jc_decimal 	= 0x03,		// decimal tab
	tbd_jc_bar 		= 0x04,		// bar tab
	
	tbd_tlc 		= 0x38,		// tab leader code
	tbd_tlc_none 	= 0x00,		// 0 - no leader
	tbd_tlc_dotted 	= 0x08,		// 1 - dotted leader
	tbd_tlc_hyph 	= 0x10,		// 2 - hyphenated leader
	tbd_tlc_single 	= 0x18,		// 3 - single line leader
	tbd_tlc_heavy 	= 0x20		// 4 - heavy line leader
} TBD_Mask;

//
// SPRM DEFINITIONS
//

typedef enum 
{ 
	sprm_ispmd		= 0x01FF,		// unique identifier within sgc group
	sprm_fSpec 		= 0x0200,		// requires special handling
	sprm_sgc 		= 0x1C00,		// sprm group; type of sprm (PAP, CHP, etc.  see below)
	sprm_spra 		= 0xE000		// size of sprm argument (see below)
} SPRM_Mask;

typedef enum 
{ 
	sgcPAP		= 1,
	sgcCHP		= 2,
	sgcPIC		= 3,
	sgcSEP		= 4,
	sgcTAP		= 5
} SPRM_sgc;

// spra operand sizes
typedef enum 
{ 
	sprm_0_1		= 0x0000,		// 0 - 1 byte (operand affects 1 bit)
	sprm_1_1		= 0x2000,		// 1 - 1 byte
	sprm_2_2		= 0x4000,		// 2 - 2 bytes
	sprm_3_4		= 0x6000,		// 3 - 4 bytes
	sprm_4_2		= 0x8000,		// 4 - 2 bytes
	sprm_5_2		= 0xA000,		// 5 - 2 bytes
	sprm_6_v		= 0xC000,		// 6 - variable - following byte is size of operand
	sprm_7_3		= 0xE000		// 7 - 3 bytes
} SPRM_spra_sizes;

typedef enum 
{ 
	sprmNoop			= 0x0000,
	//sprmCFStrikeRM		= 0x0000,	// BOGUS - what is this (in rgsprmPRM)?	
	
	oneByteSprm			= sprm_0_1, //sprm_0_1;
	twoByteSprm			= sprm_2_2,
	threeByteSprm		= sprm_7_3,
	fourByteSprm		= sprm_3_4,
	variableLengthSprm	= sprm_6_v,
	
	// sprms from word 6 that we won't handle
	sprmCFStrikeRM			= oneByteSprm, // docs say 1 bit, but I imagine this has to be at least a byte
	sprmPIncLv1				= oneByteSprm,
	sprmPPageBreakBefore	= oneByteSprm,
	sprmPNLvlAnm			= oneByteSprm,
	sprmPTtp				= oneByteSprm,
	//sprmPBrcBetween10		= twoByteSprm,
	sprmPFromText10			= twoByteSprm,
	//sprmPFWidowControl		= oneByteSprm,
	sprmCRMReason			= twoByteSprm,
	sprmCChse				= threeByteSprm,
	//sprmCFtc				= twoByteSprm, // -BOGUS this is defined below
	sprmCCondHyhen			= twoByteSprm,
	sprmSScnsPgn			= oneByteSprm,
	//sprmSDxaColSpacing		= threeByteSprm,
	//sprmSFEvenlySpaced		= oneByteSprm,
	//sprmSDyaHdrBottom		= twoByteSprm,
	//sprmSBOrientation		= oneByteSprm,
	sprmSDMPaperReq			= twoByteSprm,
	sprmTTableBorder		= variableLengthSprm,
//	sprmTDyaRowHeigh		= twoByteSprm,
	sprmMax					= twoByteSprm,
	//sprmPILvl
	
	
	sprmPIstd	= 0x4600,	// short
	sprmPIstdPermute	= 0xC601,	// variable length
	sprmPIncLvl	= 0x2602,	// byte
	sprmPJc	= 0x2403,	// byte
	sprmPFSideBySide	= 0x2404,	// byte
	sprmPFKeep	= 0x2405,	// byte
	sprmPFKeepFollow	= 0x2406,	// byte
	sprmPFPageBreakBefore	= 0x2407,	// byte
	sprmPBrcl	= 0x2408,	// byte
	sprmPBrcp	= 0x2409,	// byte
	sprmPIlvl	= 0x260A,	// byte
	sprmPIlfo	= 0x460B,	// short
	sprmPFNoLineNumb	= 0x240C,	// byte
	sprmPChgTabsPapx	= 0xC60D,	// variable length
	sprmPDxaRight	= 0x840E,	// word
	sprmPDxaLeft	= 0x840F,	// word
	sprmPNest	= 0x4610,	// word
	sprmPDxaLeft1	= 0x8411,	// word
	sprmPDyaLine	= 0x6412,	// long
	sprmPDyaBefore	= 0xA413,	// word
	sprmPDyaAfter	= 0xA414,	// word
	sprmPChgTabs	= 0xC615,	// variable length
	sprmPFInTable	= 0x2416,	// byte
	sprmPFTtp	= 0x2417,	// byte
	sprmPDxaAbs	= 0x8418,	// word
	sprmPDyaAbs	= 0x8419,	// word
	sprmPDxaWidth	= 0x841A,	// word
	sprmPPc	= 0x261B,	// byte
	sprmPBrcTop10	= 0x461C,	// word
	sprmPBrcLeft10	= 0x461D,	// word
	sprmPBrcBottom10	= 0x461E,	// word
	sprmPBrcRight10	= 0x461F,	// word
	sprmPBrcBetween10	= 0x4620,	// word
	sprmPBrcBar10	= 0x4621,	// word
	sprmPDxaFromText10	= 0x4622,	// word
	sprmPWr	= 0x2423,	// byte
	sprmPBrcTop	= 0x6424,	// long
	sprmPBrcLeft	= 0x6425,	// long
	sprmPBrcBottom	= 0x6426,	// long
	sprmPBrcRight	= 0x6427,	// long
	sprmPBrcBetween	= 0x6428,	// long
	sprmPBrcBar	= 0x6629,	// long
	sprmPFNoAutoHyph	= 0x242A,	// byte
	sprmPWHeightAbs	= 0x442B,	// word
	sprmPDcs	= 0x442C,	// short
	sprmPShd	= 0x442D,	// word
	sprmPDyaFromText	= 0x842E,	// word
	sprmPDxaFromText	= 0x842F,	// word
	sprmPFLocked	= 0x2430,	// byte
	sprmPFWidowControl	= 0x2431,	// byte
	sprmPRuler	= 0xC632,	// variable length
	sprmPFKinsoku	= 0x2433,	// byte
	sprmPFWordWrap	= 0x2434,	// byte
	sprmPFOverflowPunct	= 0x2435,	// byte
	sprmPFTopLinePunct	= 0x2436,	// byte
	sprmPFAutoSpaceDE	= 0x2437,	// byte
	sprmPFAutoSpaceDN	= 0x2438,	// byte
	sprmPWAlignFont	= 0x4439,	// word
	sprmPFrameTextFlow	= 0x443A,	// word
	sprmPISnapBaseLine	= 0x243B,	// byte
	sprmPAnld	= 0xC63E,	// variable length
	sprmPPropRMark	= 0xC63F,	// variable length
	sprmPOutLvl	= 0x2640,	// byte
	sprmPFBiDi	= 0x2441,	// byte
	sprmPFNumRMIns	= 0x2443,	// bit
	sprmPCrLf	= 0x2444,	// byte
	sprmPNumRM	= 0xC645,	// variable length
	sprmPHugePapx	= 0x6645,	// long
	sprmPFUsePgsuSettings	= 0x2447,	// byte
	sprmPFAdjustRight	= 0x2448,	// byte
	sprmCFRMarkDel	= 0x0800,	// bit
	sprmCFRMark	= 0x0801,	// bit
	sprmCFFldVanish	= 0x0802,	// bit
	sprmCPicLocation	= 0x6A03,	// variable length, length recorded is always 4
	sprmCIbstRMark	= 0x4804,	// short
	sprmCDttmRMark	= 0x6805,	// long
	sprmCFData	= 0x0806,	// bit
	sprmCIdslRMark	= 0x4807,	// short
	sprmCChs	= 0xEA08,	// 3 bytes
	sprmCSymbol	= 0x6A09,	// variable length, length recorded is always 4
	sprmCFOle2	= 0x080A,	// bit
	sprmCIdCharType	= 0x480B,	// &nbsp;
	sprmCHighlight	= 0x2A0C,	// byte
	sprmCObjLocation	= 0x680E,	// long
	sprmCFFtcAsciSymb	= 0x2A10,	// &nbsp;
	sprmCIstd	= 0x4A30,	// short
	sprmCIstdPermute	= 0xCA31,	// variable length
	sprmCDefault	= 0x2A32,	// variable length
	sprmCPlain	= 0x2A33,	// 0
	sprmCKcd	= 0x2A34,	// &nbsp;
	sprmCFBold	= 0x0835,	// byte
	sprmCFItalic	= 0x0836,	// byte
	sprmCFStrike	= 0x0837,	// byte
	sprmCFOutline	= 0x0838,	// byte
	sprmCFShadow	= 0x0839,	// byte
	sprmCFSmallCaps	= 0x083A,	// byte
	sprmCFCaps	= 0x083B,	// byte
	sprmCFVanish	= 0x083C,	// byte
	sprmCFtcDefault	= 0x4A3D,	// word
	sprmCKul	= 0x2A3E,	// byte
	sprmCSizePos	= 0xEA3F,	// 3 bytes
	sprmCDxaSpace	= 0x8840,	// word
	sprmCLid	= 0x4A41,	// word
	sprmCIco	= 0x2A42,	// byte
	sprmCHps	= 0x4A43,	// byte
	sprmCHpsInc	= 0x2A44,	// byte
	sprmCHpsPos	= 0x4845,	// byte
	sprmCHpsPosAdj	= 0x2A46,	// byte
	sprmCMajority	= 0xCA47,	// variable length, length byte plus size of following grpprl
	sprmCIss	= 0x2A48,	// byte
	sprmCHpsNew50	= 0xCA49,	// variable width, length always recorded as 2
	sprmCHpsInc1	= 0xCA4A,	// variable width, length always recorded as 2
	sprmCHpsKern	= 0x484B,	// short
	sprmCMajority50	= 0xCA4C,	// variable length
	sprmCHpsMul	= 0x4A4D,	// short
	sprmCYsri	= 0x484E,	// short
	sprmCRgFtc0	= 0x4A4F,	// short
	sprmCRgFtc1	= 0x4A50,	// short
	sprmCRgFtc2	= 0x4A51,	// short
	sprmCCharScale	= 0x4852,	// &nbsp;
	sprmCFDStrike	= 0x2A53,	// byte
	sprmCFImprint	= 0x0854,	// bit
	sprmCFSpec	= 0x0855,	// bit
	sprmCFObj	= 0x0856,	// bit
	sprmCPropRMark	= 0xCA57,	// variable length always recorded as 7 bytes
	sprmCFEmboss	= 0x0858,	// bit
	sprmCSfxText	= 0x2859,	// byte
	sprmCFBiDi	= 0x085A,	// &nbsp;
	sprmCFDiacColor	= 0x085B,	// &nbsp;
	sprmCFBoldBi	= 0x085C,	// &nbsp;
	sprmCFItalicBi	= 0x085D,	// &nbsp;
	sprmCFtcBi	= 0x4A5E,	// &nbsp;
	sprmCLidBi	= 0x485F,	// &nbsp;
	sprmCIcoBi	= 0x4A60,	// &nbsp;
	sprmCHpsBi	= 0x4A61,	// &nbsp;
	sprmCDispFldRMark	= 0xCA62,	// variable length always recorded as 39 bytes
	sprmCIbstRMarkDel	= 0x4863,	// short
	sprmCDttmRMarkDel	= 0x6864,	// long
	sprmCBrc	= 0x6865,	// long
	sprmCShd	= 0x4866,	// short
	sprmCIdslRMarkDel	= 0x4867,	// short
	sprmCFUsePgsuSettings	= 0x0868,	// bit
	sprmCCpg	= 0x486B,	// word
	sprmCRgLid0	= 0x486D,	// word
	sprmCRgLid1	= 0x486E,	// word
	sprmCIdctHint	= 0x286F,	// byte
	sprmPicBrcl	= 0x2E00,	// byte
	sprmPicScale	= 0xCE01,	// length byte plus 12 bytes
	sprmPicBrcTop	= 0x6C02,	// long
	sprmPicBrcLeft	= 0x6C03,	// long
	sprmPicBrcBottom	= 0x6C04,	// long
	sprmPicBrcRight	= 0x6C05,	// long
	sprmScnsPgn	= 0x3000,	// byte
	sprmSiHeadingPgn	= 0x3001,	// byte
	sprmSOlstAnm	= 0xD202,	// variable length
	sprmSDxaColWidth	= 0xF203,	// 3 bytes
	sprmSDxaColSpacing	= 0xF204,	// 3 bytes
	sprmSFEvenlySpaced	= 0x3005,	// byte
	sprmSFProtected	= 0x3006,	// byte
	sprmSDmBinFirst	= 0x5007,	// word
	sprmSDmBinOther	= 0x5008,	// word
	sprmSBkc	= 0x3009,	// byte
	sprmSFTitlePage	= 0x300A,	// byte
	sprmSCcolumns	= 0x500B,	// word
	sprmSDxaColumns	= 0x900C,	// word
	sprmSFAutoPgn	= 0x300D,	// byte
	sprmSNfcPgn	= 0x300E,	// byte
	sprmSDyaPgn	= 0xB00F,	// short
	sprmSDxaPgn	= 0xB010,	// short
	sprmSFPgnRestart	= 0x3011,	// byte
	sprmSFEndnote	= 0x3012,	// byte
	sprmSLnc	= 0x3013,	// byte
	sprmSGprfIhdt	= 0x3014,	// byte
	sprmSNLnnMod	= 0x5015,	// word
	sprmSDxaLnn	= 0x9016,	// word
	sprmSDyaHdrTop	= 0xB017,	// word
	sprmSDyaHdrBottom	= 0xB018,	// word
	sprmSLBetween	= 0x3019,	// byte
	sprmSVjc	= 0x301A,	// byte
	sprmSLnnMin	= 0x501B,	// word
	sprmSPgnStart	= 0x501C,	// word
	sprmSBOrientation	= 0x301D,	// byte
	sprmSBCustomize	= 0x301E,	// &nbsp;
	sprmSXaPage	= 0xB01F,	// word
	sprmSYaPage	= 0xB020,	// word
	sprmSDxaLeft	= 0xB021,	// word
	sprmSDxaRight	= 0xB022,	// word
	sprmSDyaTop	= 0x9023,	// word
	sprmSDyaBottom	= 0x9024,	// word
	sprmSDzaGutter	= 0xB025,	// word
	sprmSDmPaperReq	= 0x5026,	// word
	sprmSPropRMark	= 0xD227,	// variable length always recorded as 7 bytes
	sprmSFBiDi	= 0x3228,	// &nbsp;
	sprmSFFacingCol	= 0x3229,	// &nbsp;
	sprmSFRTLGutter	= 0x322A,	// &nbsp;
	sprmSBrcTop	= 0x702B,	// long
	sprmSBrcLeft	= 0x702C,	// long
	sprmSBrcBottom	= 0x702D,	// long
	sprmSBrcRight	= 0x702E,	// long
	sprmSPgbProp	= 0x522F,	// word
	sprmSDxtCharSpace	= 0x7030,	// long
	sprmSDyaLinePitch	= 0x9031,	// long
	sprmSClm	= 0x5032,	// &nbsp;
	sprmSTextFlow	= 0x5033,	// short
	sprmTJc	= 0x5400,	// word (low order byte is significant)
	sprmTDxaLeft	= 0x9601,	// word
	sprmTDxaGapHalf	= 0x9602,	// word
	sprmTFCantSplit	= 0x3403,	// byte
	sprmTTableHeader	= 0x3404,	// byte
	sprmTTableBorders	= 0xD605,	// 24 bytes
	sprmTDefTable10	= 0xD606,	// variable length
	sprmTDyaRowHeight	= 0x9407,	// word
	sprmTDefTable	= 0xD608,	// &nbsp;
	sprmTDefTableShd	= 0xD609,	// &nbsp;
	sprmTTlp	= 0x740A,	// 4 bytes
	sprmTFBiDi	= 0x560B,	// &nbsp;
	sprmTHTMLProps	= 0x740C,	// &nbsp;
	sprmTSetBrc	= 0xD620,	// 5 bytes
	sprmTInsert	= 0x7621,	// 4 bytes
	sprmTDelete	= 0x5622,	// word
	sprmTDxaCol	= 0x7623,	// 4 bytes
	sprmTMerge	= 0x5624,	// word
	sprmTSplit	= 0x5625,	// word
	sprmTSetBrc10	= 0xD626,	// 5 bytes
	sprmTSetShd	= 0x7627,	// 4 bytes
	sprmTSetShdOdd	= 0x7628,	// 4 bytes
	sprmTTextFlow	= 0x7629,	// word
	sprmTDiagLine	= 0xD62A,	// &nbsp;
	sprmTVertMerge	= 0xD62B,	// variable length always recorded as 2 bytes
	sprmTVertAlign	= 0xD62C,	// variable length always recorded as 3 byte


	sprmCFtc				= sprmCRgFtc0
/*	sprmCFRMark			= 0x0801,
	sprmCFFldVanish		= 0x0802,
	sprmCFData			= 0x0806,
	sprmCFOle2			= 0x080A,
	sprmCFBold			= 0x0835,
	sprmCFItalic		= 0x0836,
	sprmCFStrike		= 0x0837,
	sprmCFOutline		= 0x0838,
	sprmCFShadow		= 0x0839,
	sprmCFSmallCaps		= 0x083A,
	sprmCFCaps			= 0x083B,
	sprmCFVanish		= 0x083C,
	sprmCFImprint		= 0x0854,
	sprmCFSpec			= 0x0855,
	sprmCFObj			= 0x0856,
	sprmCFEmboss		= 0x0858,
	sprmPJc				= 0x2403,
	sprmPFSideBySide	= 0x2404,
	sprmPFKeep			= 0x2405,
	sprmPFKeepFollow	= 0x2406,
	sprmPFPageBreakBefore = 0x2407,
	sprmPBrcl			= 0x2408,
	sprmPBrcp			= 0x2409,
	sprmPIlvl			= 0x240A,
	sprmPFNoLineNumb	= 0x240C,
	sprmPFInTable		= 0x2416,
	sprmPFTtp			= 0x2417,
	sprmPWr				= 0x2423,
	sprmPFNoAutoHyph	= 0x242A,
	sprmPFLocked		= 0x2430,
	sprmPFWidowControl	= 0x2431,
	sprmPFKinsoku		= 0x2433,
	sprmPFWordWrap		= 0x2434,
	sprmPFOverflowPunct	= 0x2435,
	sprmPFTopLinePunct	= 0x2436,
	sprmPFAutoSpaceDE	= 0x2437,
	sprmPFAutoSpaceDN	= 0x2438,
	sprmPISnapBaseLine	= 0x243B,
	sprmPIncLvl			= 0x2602,
	sprmPILvl			= 0x260A,
	sprmPPc				= 0x261B,
	sprmPOutLvl			= 0x2640,
	sprmCSfxText		= 0x2859,
	sprmCHighlight		= 0x2A0C,
	sprmCPlain			= 0x2A33,
	sprmCKul			= 0x2A3E,
	sprmCIco			= 0x2A42,
	sprmCHpsInc			= 0x2A44,
	sprmCHpsPosAdj		= 0x2A46,
	sprmCIss			= 0x2A48,
	sprmCFDStrike		= 0x2A53,
	sprmPicBrcl			= 0x2E00,
	sprmPIstd			= 0x4600,
	sprmPIlfo			= 0x460B,
	sprmCHpsKern		= 0x484B,
	sprmCRgLid0			= 0x486D,
	sprmCRgLid1			= 0x486E,
	sprmCIstd			= 0x4A30,
	sprmCFtcDefault		= 0x4A3D,
	sprmCHps			= 0x4A43,
	sprmCRgFtc0			= 0x4A4F,
	sprmCRgFtc1			= 0x4A50,
	sprmCRgFtc2			= 0x4A51,
	sprmCPicLocation	= 0x6A03,
	sprmPDxaRight		= 0x840E,
	sprmPDxaLeft		= 0x840F,
	sprmPDxaLeft1		= 0x8411,
	sprmTDxaLeft		= 0x9601,
	sprmTDxaGapHalf		= 0x9602,
	sprmPDyaBefore		= 0xA413,
	sprmPDyaAfter		= 0xA414,
	sprmPDyaLine		= 0x6412,
	sprmPIstdPermute 	= 0xC601,
	sprmPChgTabsPapx 	= 0xC60D,
	sprmPChgTabs		= 0xC615,
	sprmTTableBorders	= 0xD605,
	sprmTDefTable10		= 0xD606,
	sprmTDefTable		= 0xD608*/
} SPRM;

#define WORD_COLOR_COUNT 17

extern const rgb_color word_colors[WORD_COLOR_COUNT];



#define STYLE_COUNT 31

extern const char* StyleNames[STYLE_COUNT];

extern const uint16 StyleIndexes[STYLE_COUNT];


#endif // __WORDSTRUCTS_H__

