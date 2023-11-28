//
// HTMLTables.h
//

#ifndef __HTMLTABLES_H__
#define __HTMLTABLES_H__

// HTML tree elements

// DON'T CHANGE ANYTHING IN THIS ENUM WITHOUT CHANGING
// THE CORRESPONDING TABLES IN HTMLTables.c
enum {
		HTML_EL_Anchor = 0,
		HTML_EL_Address,			// 0
		HTML_EL_Applet,
		HTML_EL_AREA,
		HTML_EL_BASE,
		HTML_EL_BaseFont,
		HTML_EL_Big_text,		
		HTML_EL_Block_Quote,
		HTML_EL_BODY,
		HTML_EL_Bold_text,
		HTML_EL_BR,					// 10
		HTML_EL_CAPTION,
		HTML_EL_Center,
		HTML_EL_Checkbox_Input,
		HTML_EL_Cite,
		HTML_EL_Code,
		HTML_EL_Column_head,
		HTML_EL_Comment_,
		HTML_EL_Comment_line,
		HTML_EL_Data_cell,
		HTML_EL_Definition,			// 20
		HTML_EL_Definition_List,
		HTML_EL_Definition_Item,
		HTML_EL_Def,
		HTML_EL_Directory,
		HTML_EL_Division,
		HTML_EL_Document_URL,
		HTML_EL_Emphasis,
		HTML_EL_File_Input,
		HTML_EL_Font_,
		HTML_EL_Form,				// 30
		HTML_EL_Frame,
		HTML_EL_H1,
		HTML_EL_H2,
		HTML_EL_H3,
		HTML_EL_H4,
		HTML_EL_H5,
		HTML_EL_H6,
		HTML_EL_HEAD,
		HTML_EL_Heading_cell,
		HTML_EL_Hidden_Input,		// 40
		HTML_EL_Horizontal_Rule,
		HTML_EL_HTML,
		HTML_EL_Input,
		HTML_EL_Inserted_Text,
		HTML_EL_Invalid_element,
		HTML_EL_ISINDEX,
		HTML_EL_Italic_text,
		HTML_EL_Keyboard,
		HTML_EL_LINK,
		HTML_EL_Links,				// 50
		HTML_EL_List_Item,
		HTML_EL_MAP,
		HTML_EL_Menu,
		HTML_EL_Metas,
		HTML_EL_META,
		HTML_EL_Numbered_List,
		HTML_EL_Option,
		HTML_EL_Option_Menu,
		HTML_EL_Paragraph,
		HTML_EL_Parameter,			// 60
		HTML_EL_Password_Input,
		HTML_EL_PICTURE_UNIT,
		HTML_EL_Pre_Line,
		HTML_EL_Preformatted,
		HTML_EL_Radio_Input,
		HTML_EL_Reset_Input,
		HTML_EL_Sample,
		HTML_EL_Scripts,
		HTML_EL_SCRIPT,
		HTML_EL_Small_text,			// 70
		HTML_EL_Struck_text,
		HTML_EL_Strong,
		HTML_EL_Styles,
		HTML_EL_Subscript,
		HTML_EL_Submit_Input,
		HTML_EL_Superscript,
		HTML_EL_Table,
		HTML_EL_Table_body,
		HTML_EL_Table_cell_ghost,
		HTML_EL_Table_foot,			// 80
		HTML_EL_Table_head,
		HTML_EL_Table_row,
		HTML_EL_tbody,
		HTML_EL_tfoot,
		HTML_EL_thead,
		HTML_EL_Teletype_text,
		HTML_EL_Term,
		HTML_EL_Term_List,
		HTML_EL_TEXT_UNIT,
		HTML_EL_Text_Area,			// 90
		HTML_EL_Text_Input,
		HTML_EL_TITLE,
		HTML_EL_Underlined_text,
		HTML_EL_Unnumbered_List,
		HTML_EL_Variable,
		HTML_EL_LastElement
};

// HTML tree element attributes

// DON'T CHANGE ANYTHING IN THIS ENUM WITHOUT CHANGING
// THE CORRESPONDING TABLES IN HtmlTables.c
enum {
		HTML_ATTR_ActiveLinkColor,		// 0
		HTML_ATTR_Alignment,
		HTML_ATTR_Align,
		HTML_ATTR_ALT,
		HTML_ATTR_applet_name,
		HTML_ATTR_Area_Size,
		HTML_ATTR_background_,
		HTML_ATTR_BackgroundColor,
		HTML_ATTR_BaseFontSize,
		HTML_ATTR_Border,
		HTML_ATTR_BulletStyle,			// 10
		HTML_ATTR_Cell_align,
		HTML_ATTR_Cell_width,
		HTML_ATTR_Cell_valign,
		HTML_ATTR_Clear,
		HTML_ATTR_cellspacing,
		HTML_ATTR_cellpadding,
		HTML_ATTR_Checked,
		HTML_ATTR_Class,
		HTML_ATTR_code,
		HTML_ATTR_codebase,				// 20
		HTML_ATTR_ColExt,
		HTML_ATTR_color,
		HTML_ATTR_Columns,
		HTML_ATTR_colspan,
		HTML_ATTR_Col_width_percent,
		HTML_ATTR_Col_width_pxl,
		HTML_ATTR_COMPACT,
		HTML_ATTR_coords,
		HTML_ATTR_Cell_height,
		HTML_ATTR_DefaultChecked,		// 30
		HTML_ATTR_DefaultSelected,
		HTML_ATTR_Default_Value,
		HTML_ATTR_ENCTYPE,				
		HTML_ATTR_Error_type,
		HTML_ATTR_Font_face,
		HTML_ATTR_Font_size,
		HTML_ATTR_Ghost_restruct,
		HTML_ATTR_height_,
		HTML_ATTR_Height_,
		HTML_ATTR_HREF_,				// 40
		HTML_ATTR_hspace,
		HTML_ATTR_http_equiv,
		HTML_ATTR_Img_border,
		HTML_ATTR_IntItemStyle,			
		HTML_ATTR_IntMaxVol,
		HTML_ATTR_IntSizeDecr,
		HTML_ATTR_IntSizeIncr,
		HTML_ATTR_IntSizeRel,
		HTML_ATTR_IntWidthPercent,
		HTML_ATTR_IntWidthPxl,			// 50
		HTML_ATTR_Invalid_attribute,
		HTML_ATTR_ISMAP,
		HTML_ATTR_ItemStyle,
		HTML_ATTR_ItemValue,
		HTML_ATTR_LinkColor,			
		HTML_ATTR_MaxLength,
		HTML_ATTR_MenuSize,
		HTML_ATTR_meta_content,
		HTML_ATTR_METHOD,
		HTML_ATTR_Multiple,				// 60
		HTML_ATTR_meta_name,
		HTML_ATTR_NAME,
		HTML_ATTR_nohref,
		HTML_ATTR_NoShade,
		HTML_ATTR_NumberStyle,			
		HTML_ATTR_Notation,
		HTML_ATTR_OnChange,
		HTML_ATTR_Param_name,
		HTML_ATTR_Param_value,
		HTML_ATTR_Position,				// 70
		HTML_ATTR_Prompt,
		HTML_ATTR_PseudoClass,
		HTML_ATTR_Ref_column,
		HTML_ATTR_REL,
		HTML_ATTR_REV,
		HTML_ATTR_Rows,					
		HTML_ATTR_Row_valign,
		HTML_ATTR_Row_align,
		HTML_ATTR_RowExt,
		HTML_ATTR_rowspan,				// 80
		HTML_ATTR_Selected,
		HTML_ATTR_shape,
		HTML_ATTR_Size_,
		HTML_ATTR_SRC,
		HTML_ATTR_Script_URL,
		HTML_ATTR_Start,
		HTML_ATTR_Style_,				
		HTML_ATTR_Table_align,
		HTML_ATTR_TextColor,
		HTML_ATTR_Title,			// 90
		HTML_ATTR_USEMAP,
		HTML_ATTR_Value_,
		HTML_ATTR_VisitedLinkColor,
		HTML_ATTR_vspace,
		HTML_ATTR_width_,
		HTML_ATTR_Width_,
		HTML_ATTR_Width__,				
		HTML_ATTR_WidthElement,
		HTML_ATTR_Word_wrap,
		HTML_ATTR_x_coord,			// 100
		HTML_ATTR_y_coord
};


// Attribute Values


// HTML_ATTR_Align
enum {
		HTML_ATTR_Align_VAL_left_,
		HTML_ATTR_Align_VAL_center_,
		HTML_ATTR_Align_VAL_right_
};
		
// HTML_ATTR_Alignment
enum {
		HTML_ATTR_Alignment_VAL_Top_,
		HTML_ATTR_Alignment_VAL_Middle_,
		HTML_ATTR_Alignment_VAL_Bottom_,
		HTML_ATTR_Alignment_VAL_Left_,
		HTML_ATTR_Alignment_VAL_Right_
};
		
// HTML_ATTR_BulletStyle
enum {
		HTML_ATTR_BulletStyle_VAL_disc,
		HTML_ATTR_BulletStyle_VAL_square,
		HTML_ATTR_BulletStyle_VAL_circle
};
		
// HTML_ATTR_Cell_align
enum {
		HTML_ATTR_Cell_align_VAL_Cell_left,
		HTML_ATTR_Cell_align_VAL_Cell_center,
		HTML_ATTR_Cell_align_VAL_Cell_right
};
		
// HTML_ATTR_Cell_valign
enum {
		HTML_ATTR_Cell_valign_VAL_Cell_top,
		HTML_ATTR_Cell_valign_VAL_Cell_middle,
		HTML_ATTR_Cell_valign_VAL_Cell_bottom
};
		
// HTML_ATTR_Clear
enum {
		HTML_ATTR_Clear_VAL_Left_,
		HTML_ATTR_Clear_VAL_Right_,
		HTML_ATTR_Clear_VAL_All_,
		HTML_ATTR_Clear_VAL_None
};

// HTML_ATTR_Error_type
		
enum {
		HTML_ATTR_Error_type_VAL_BadPosition,
		HTML_ATTR_Error_type_VAL_UnknownTag
};

// HTML_ATTR_ItemStyle
enum {
		HTML_ATTR_ItemStyle_VAL_Arabic_,
		HTML_ATTR_ItemStyle_VAL_LowerAlpha,
		HTML_ATTR_ItemStyle_VAL_UpperAlpha,
		HTML_ATTR_ItemStyle_VAL_LowerRoman,
		HTML_ATTR_ItemStyle_VAL_UpperRoman,
		HTML_ATTR_ItemStyle_VAL_disc,
		HTML_ATTR_ItemStyle_VAL_square,
		HTML_ATTR_ItemStyle_VAL_circle
};
		
// HTML_ATTR_IntItemStyle (same as ItemStyle)
enum {
		HTML_ATTR_IntItemStyle_VAL_Arabic_,
		HTML_ATTR_IntItemStyle_VAL_LowerAlpha,
		HTML_ATTR_IntItemStyle_VAL_UpperAlpha,
		HTML_ATTR_IntItemStyle_VAL_LowerRoman,
		HTML_ATTR_IntItemStyle_VAL_UpperRoman,
		HTML_ATTR_IntItemStyle_VAL_disc,
		HTML_ATTR_IntItemStyle_VAL_square,
		HTML_ATTR_IntItemStyle_VAL_circle
};
		
// HTML_ATTR_METHOD
enum {
		HTML_ATTR_METHOD_VAL_Get_,
		HTML_ATTR_METHOD_VAL_Post_
};
		
// HTML_ATTR_NumberStyle
enum {
		HTML_ATTR_NumberStyle_VAL_Arabic_,
		HTML_ATTR_NumberStyle_VAL_LowerAlpha,
		HTML_ATTR_NumberStyle_VAL_UpperAlpha,
		HTML_ATTR_NumberStyle_VAL_LowerRoman,
		HTML_ATTR_NumberStyle_VAL_UpperRoman
};
		
// HTML_ATTR_Position
enum {
		HTML_ATTR_Position_VAL_Position_top,
		HTML_ATTR_Position_VAL_Position_bottom
};
		
// HTML_ATTR_Row_align
enum {
		HTML_ATTR_Row_align_VAL_Row_left,
		HTML_ATTR_Row_align_VAL_Row_center,
		HTML_ATTR_Row_align_VAL_Row_right
};
		
// HTML_ATTR_Row_valign
enum {
		HTML_ATTR_Row_valign_VAL_Row_top,
		HTML_ATTR_Row_valign_VAL_Row_middle,
		HTML_ATTR_Row_valign_VAL_Row_bottom
};
		
// HTML_ATTR_shape
enum {
		HTML_ATTR_shape_VAL_rectangle,
		HTML_ATTR_shape_VAL_circle,
		HTML_ATTR_shape_VAL_polygon
};
		
// HTML_ATTR_Table_align
enum {
		HTML_ATTR_Table_align_VAL_Align_left,
		HTML_ATTR_Table_align_VAL_Center_,
		HTML_ATTR_Table_align_VAL_Align_right
};

// HTML_ATTR_Checked
enum {
		HTML_ATTR_Checked_VAL_Yes_,
		HTML_ATTR_Checked_VAL_No_
};

// HTML_ATTR_   single values
enum {
		HTML_ATTR_COMPACT_VAL_Yes_ = 0,
		HTML_ATTR_DefaultChecked_VAL_Yes_ = 0,
		HTML_ATTR_DefaultSelected_VAL_Yes_ = 0,
		HTML_ATTR_ISMAP_VAL_Yes_ = 0,
		HTML_ATTR_Multiple_VAL_Yes_ = 0,
		HTML_ATTR_nohref_VAL_Yes_ = 0,
		HTML_ATTR_NoShade_VAL_NoShade_ = 0,
		HTML_ATTR_Selected_VAL_Yes_ = 0,
		HTML_ATTR_Word_wrap_VAL_No_wrap = 0
};


// Attribute Kinds
enum 
{
	ATTR_KIND_enumerate = 0,
	ATTR_KIND_integer,
	ATTR_KIND_text,
	ATTR_KIND_reference
};		


#define DUMMY_ATTRIBUTE 	500
#define MAX_GI_LENGTH 		12
typedef unsigned char GI[MAX_GI_LENGTH];


// an element closed by a start tag
typedef struct _ClosedElement *PtrClosedElement;
typedef struct _ClosedElement
{
	int                 tagNum;	 		// rank (in GIMappingTable) of closed element
	PtrClosedElement    nextClosedElem; // next element closed by the same start tag
} ClosedElement;


// Used in mapping of HTML to a tree element 
typedef struct _GIMapping
{				
	GI					htmlGI;				// name of the HTML element
	char				htmlContents;		// info about the contents of the HTML element:
											// 'E'=empty,  space=any contents
	int					TreeType;			// type of the tree element or attribute
	PtrClosedElement	firstClosedElem;	// first element closed by the start tag htmlGI
} GIMapping;

extern GIMapping GIMappingTable[];


// mapping of a HTML attribute
typedef struct _AttributeMapping
{
	char                htmlAttribute[13];	// name of HTML attribute
	GI                  htmlElement;		// name of HTML GI
	char                AttrOrContent;		// info about the corresponding tree thing: 
	   										//'A'=Attribute, 'C'=Content, SPACE= Nothing
	int                 TreeAttribute;		// tree attribute
} AttributeMapping;

extern AttributeMapping AttributeMappingTable[];


// mapping of a HTML attribute value
typedef struct _AttrValueMapping
{
	int		TreeAttr;			// corresponding tree attribute
	char	htmlAttrValue[20];	// HTML value
	int		TreeAttrValue;		// corresponding value of the tree attribute
} AttrValueMapping;

extern AttrValueMapping AttrValueMappingTable[];


// a SGML entity representing an ISO-Latin1 char
typedef struct _ISOlat1entry
{
	unsigned char	charName[10];	// entity name
	int				charCode;		// decimal code of ISO-Latin1 char
} ISOlat1entry;

extern ISOlat1entry ISOlat1table[];

typedef struct _ISOUTF8Entry
{
	unsigned char	charName[10];	// entity name
	unsigned char	utf8[6];
} ISOUTF8Entry;

extern ISOUTF8Entry ISOUTF8Table[];

// map HTML tree elements to their textual output
#define MAX_ELEMENT_LENGTH1		21
#define MAX_ELEMENT_LENGTH2		3
#define MAX_ELEMENT_LENGTH3		15

typedef struct _IGMapping
{
	// These fields are used to get HTML text	
	bool	isSpecial;						// flag non-standard translation				
	char	first[MAX_ELEMENT_LENGTH1];		// string to write when element first encountered
	char	afterAttr[MAX_ELEMENT_LENGTH2]; // string to write after attributes are written
	char	afterAll[MAX_ELEMENT_LENGTH3];	// string to write when done with element children
	
	// These fields are used to get Squirrel parts
	bool	isSupported;					// can Squirrel handle this element type
} IGMapping;

extern IGMapping IGMappingTable[];


// map HTML elements to their textual output
#define MAX_ATTR_LENGTH1		16
#define MAX_ATTR_LENGTH2		2

typedef struct _IGAttrMapping
{
	char	first[MAX_ATTR_LENGTH1];	// string to write when attr first encountered
	char	last[MAX_ATTR_LENGTH2];		// string to write when attr is done
	int		attrKind;					// kind of attribute
} IGAttrMapping;

extern IGAttrMapping IGAttrMappingTable[];



// function protypes
char *GetGITagName(int elementType);
char *GetISOName(int theChar);

#endif //__HTMLTABLES_H__