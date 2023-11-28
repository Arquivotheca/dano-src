//
//	TranslatorStyle.h
//
#ifndef __TRANSLATORSTYLE_H__
#define __TRANSLATORSTYLE_H__

#include <Font.h>
#include "BlockStream.h"

class TTransProperty : public TBlockDataTable
{
	public:
		TTransProperty(int32 kind);
		TTransProperty(const TTransProperty& src);
		virtual ~TTransProperty();
		
		int32	Kind(void) const;
	    bool	operator==(const TTransProperty& propPtr) const;
	
	private:
		int32	mKind;
};

class TTransPropertyMgr
{
	public:
		TTransPropertyMgr();
		~TTransPropertyMgr();
		
		status_t	Read(TBlockStreamReader* reader);
		status_t	Write(TBlockStreamWriter* writer) const;
		
		int32					AddProperty( TTransProperty* prop );
		const TTransProperty*	FindProperty(int32 kind) const;
		const TTransProperty*	operator[](int32 index) const;
		int32					CountProperties(int32 kind) const;
		
	private:
		BList		mProps;
};



class TTranslatorStylesTable;


class TTranslatorStyle
{
	friend class TTranslatorStylesTable;
	
	public:
		// General types.
		typedef enum { kBasicStyle = 'Basi', kParagraphStyle = 'Para', kSpreadsheetStyle = 'SPRD' } StyleType;
		typedef enum { kAlignDefault = 0, kAlignStart, kAlignCenter, kAlignEnd, kAlignDistribute, kAlignFill } AlignType;

		// Word processing and text types.
		typedef enum { kParTabLeft = 0, kParTabCenter, kParTabRight, kParTabAlign, kParTabJust } TabType;
		typedef enum { kNoFill = 0, kPenFill, kCharFill } TabFillType;
		typedef enum { kParAlignLeft = 0, kParAlignCenter, kParAlignRight, kParAlignJust} ParAlignType;
		typedef enum { kNoneLabel = 0, kDiamondLabel, kBulletLabel, kCheckBoxLabel, kHarvardLabel, 
						kLegalLabel, kLeaderLabel, kLetterCapLabel, kLetterLabel, kNumericLabel, 
						kRomanCapLabel, kRomanLabel, kCustomLabel } LabelType;

		// Spreadsheet types.
		typedef enum { kBorderLeft = 0, kBorderTop, kBorderRight, kBorderBottom	} CellBorder;

		enum 
		{
			kGENERAL_FORMATS = 0,
			kNUMBER_FORMATS,
			kCURRENCY_FORMATS,
			kDATE_FORMATS,
			kTIME_FORMATS,
			kPERCENTAGE_FORMATS,
			kFRACTION_FORMATS,
			kSCIENTIFIC_FORMATS,
			kSPECIAL_FORMATS
		};

		enum
		{
			kPropTextColor			= 'TCol',
			kPropPenColor			= 'PCol',
			kPropFillColor			= 'FCol',
			kPropPen 				= 'PEN ',
			kPropPenSize			= 'PENz',
			kPropArrows				= 'Arrw',
			kPropTextSize			= 'TSiz',
			kPropTextFont			= 'TFon',
			kPropHAlign				= 'HAln',
			kPropVAlign				= 'VAln',
			kPropTextFace			= 'TFac',
			kPropNumFormat			= 'SSNm',
			kPropPenOpacity			= 'PTra',
			kPropFillOpacity		= 'FTra',
			kPropTextOpacity		= 'TTra'			
		};
		
		enum
		{
			kPropParLeft			= 'PLef',
			kPropParRight			= 'PRig',
			kPropParIndent			= 'PInd',
			kPropParTab				= 'PTab',
			kPropParAlign			= 'PAlg',
			kPropParLabel			= 'PLab',
			kPropParWidowControl	= 'PWid',
			kPropParLinesTogether	= 'LTog',
			kPropParKeepWithNext	= 'PNxt',
			kPropParBreakBefore		= 'PgBB',
			kPropParHyphenate		= 'PHyp',
			kPropParChecked			= 'PChk',
			kPropParHidden			= 'PHid',
			kPropParExpanded		= 'PExp',
			kPropParLineSpacing		= 'LSpa',
			kPropParSpaceBefore		= 'SpaB',
			kPropParSpaceAfter		= 'SpaA'
		};
		
		enum
		{
			kPropBorderColor		= 'BCol',
			kPropBorderOpacity		= 'BTra',
			kPropBorderPen			= 'BPen',
			kPropBorderSize			= 'BSiz',
			kPropCellWrap			= 'ClWp',
			kPropCellLock			= 'ClLk'
		};
		
		typedef enum 
		{ 
			kPlainFace 				= 0x0001, 
			kBoldFace 				= 0x0002, 
			kItalicFace 			= 0x0004, 
			kUnderlineFace 			= 0x0008, 
			kDoubleUnderlineFace 	= 0x0010,
			kStrikeThruFace 		= 0x0020,
			kSuperScriptFace 		= 0x0040,
			kSubScriptFace 			= 0x0080
		} PropTextFaces;		
		
	public:
		TTranslatorStyle();
		TTranslatorStyle(const TTranslatorStyle& src);
		virtual ~TTranslatorStyle();
		
		// Output style to block stream with given id.
		int32					Index(void) const;
		status_t				Write(TBlockStreamWriter* writer) const;
		status_t				Read(TBlockStreamReader* reader);

		int32					CountProperties(void) const;
		const TTransProperty*	GetProperty(int32 index) const;
		
		status_t				AddProperty( TTransProperty* propPtr );
		const TTransProperty*	FindProperty(int32 kind) const;

		// Style definition methods - use these to create a new style (not to use it)
		void					SetStyleType(StyleType styleType);
		void					SetStyleName(const char* styleName);
		void					SetBaseStyleName(const char* styleName);
		void					SetApplyStyleName(const char* styleName);

		bool					HasStyleName(void) const;
		bool					GetStyleType(StyleType& styleType) const;
		bool					GetStyleName(const char** str) const;
		bool					GetBaseStyleName(const char** str) const;
		bool 					GetBaseStyleIndex(int32& styleIndex) const;
		bool					GetApplyStyleName(const char** str) const;
		bool 					GetApplyStyleIndex(int32& styleIndex) const;
					
		// Basic format properties.
		void					SetTextFont(int32 fontID);
		void					SetTextFont(const font_family fontName, const font_style fontStyle);
		void					SetTextSize(float textSize);
		void					SetTextFace(int32 textFace, bool value = true);
		void					SetTextAlignment(int32 alignment, bool horizontal = true);
		void					SetTextColor(rgb_color high, rgb_color low, pattern pat = B_SOLID_HIGH);
		void					SetFillColor(rgb_color high, rgb_color low, pattern pat = B_SOLID_HIGH);
		void					SetPenColor(rgb_color high, rgb_color low, pattern pat = B_SOLID_HIGH);
		void					SetTextOpacity(float value);
		void					SetFillOpacity(float value);
		void					SetPenOpacity(float value);
		void					SetNumberFormat(const char* formatStr);
		void					SetDashPen(float dashWidth, float gapWidth, const char* name = NULL);

		// Paragraph format properties
		void					SetFirstLineIndent(float value);
		void					SetLeftIndent(float value);
		void					SetRightIndent(float value);
		void					SetLineSpacing(float value, bool lines);
		void					SetSpaceBefore(float value, bool lines);
		void					SetSpaceAfter(float value, bool lines);
		void					SetParagraphAlignment(int32 type);
		void					SetParagraphLabel(int32 type);
		void					SetTab(float pos, int32 type = kParTabLeft, int32 fillType = kNoFill, const uchar* alignChar = NULL, const uchar* fillChar = NULL);

		// Spreadsheet related properties.
		void					SetBorderColor(int32 border, rgb_color high, rgb_color low, pattern pat);
		void					SetBorderSize(int32 border, float size);
		void					SetBorderDashPen(int32 border, float dashWidth, float gapWidth, const char* name = NULL);
		void					SetCellWrap(bool wrap);
		void					SetCellLock(bool lock);

		// Basic format accessors.
		bool					GetFont(BFont& font) const;
		bool					GetTextFont(font_family& fontName, font_style& fontStyle) const;
		bool					GetTextSize(float& textSize) const;
		bool					GetTextFace(int32& textFace, int32& textMask) const;
		bool					GetTextAlignment(int32& align, bool horizontal = true) const;
		bool					GetTextColor(rgb_color& high, rgb_color& low, pattern& pat) const;
		bool					GetFillColor(rgb_color& high, rgb_color& low, pattern& pat) const;
		bool					GetPenColor(rgb_color& high, rgb_color& low, pattern& pat) const;
		bool					GetTextOpacity(float& value) const;
		bool					GetFillOpacity(float& value) const;
		bool					GetPenOpacity(float& value) const;
		bool					GetNumberFormat(const char** str) const;
		
		// Paragraph format accessors.
		bool					GetFirstLineIndent(float& value) const;
		bool					GetLeftIndent(float& value) const;
		bool					GetRightIndent(float& value) const;
		bool					GetLineSpacing(float& value, bool& lines) const;
		bool					GetSpaceBefore(float& value, bool& lines) const;
		bool					GetSpaceAfter(float& value, bool& lines) const;		
		bool 					GetParagraphAlignment(int32& align) const;
		int32					CountTabs(void) const;
		bool 					GetTab(int32 index, float& pos, int32& type, int32& fillType, uchar* alignChar, uchar* fillChar) const;

		// Spreadsheet format accessors.
		bool					GetBorderColor(int32 border, rgb_color& high, rgb_color& low, pattern& pat);
		bool					GetBorderOpacity(int32 border, float& value) const;
		bool					GetBorderSize(int32 border, float& size);
		bool					GetCellWrap(bool& wrap);
		bool					GetCellLock(bool& lock);

		// Helper methods used by the class.
		void					AddColor( block_id id, rgb_color high, rgb_color low, pattern pat );
		void					AddColorToProp( TTransProperty* propPtr, rgb_color high, rgb_color low, pattern pat );
		bool					GetFloatPropertyValue( int32 propType, float& value ) const;
		bool					GetColor( block_id id, rgb_color& high, rgb_color& low, pattern& pat ) const;
		bool					GetColorFromProp( const TTransProperty* prop, rgb_color& high, rgb_color& low, pattern& pat ) const;
		void					AddParagraphSpacing( block_id id, float value, bool lines );
		bool					GetParagraphSpacing( block_id id, float& value, bool& lines ) const;
		long 					ReverseFlattenUTF8(const uchar* textPtr);

		// Access to the underlying tagged data table
		TBlockDataTable&		StyleDataTable(void);
		TTranslatorStylesTable*	StyleSheet(void) const		{ return mStyleSheet; }
		
	private:
		void					SetStyleSheet(TTranslatorStylesTable* styleSheetPtr, int32 index);
		status_t				WriteProperties(TBlockStreamWriter* writer) const;
		status_t				ReadProperties(TBlockStreamReader* reader);
		
		TTranslatorStylesTable*	mStyleSheet;
		TBlockDataTable			mStyleChunk;
		BList					mTransProps;
		BList					mTransPropIDs;
		int32					mIndex;				
		mutable bool			mWritten;
};

class TTranslatorStylesTable
{
	public:
		TTranslatorStylesTable();
		~TTranslatorStylesTable();
		
		int32					AddStyle( TTranslatorStyle* stylePtr );
		int32					AddProperty( TTransProperty* prop );
		
		status_t 				Read(TBlockStreamReader* reader);
		status_t				Write(TBlockStreamWriter* writer);
		
		int32					CountItems(void) const				{ return mStyles.CountItems(); }
		TTranslatorStyle*		operator[](int32 index) const;

		bool					WritenToStream(void) const			{ return mWritten; }
		const TTransProperty*	IndexToProperty(int32 index) const	{ return mPropMgr[index]; }
		const TTransProperty*	FindProperty(int32 kind) const		{ return mPropMgr.FindProperty(kind); }
		
	private:
		status_t				ReadStyles(TBlockStreamReader* reader);
	
		BList					mStyles;
		TTransPropertyMgr		mPropMgr;
		bool					mWritten;
};

#endif // __TRANSLATORSTYLE_H__

// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslatorStyle.h,v 1.24 2000/02/17 00:22:09 tom Exp $

