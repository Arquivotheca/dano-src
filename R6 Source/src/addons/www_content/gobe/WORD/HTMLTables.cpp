//
// HTMLTables.cpp
//

#include "HTMLconsts.h"
#include "HTMLTables.h"


// mapping table of HTML to tree elements
GIMapping GIMappingTable[] =
{
	{"A", SPACE, HTML_EL_Anchor, NULL},
	{"ADDRESS", SPACE, HTML_EL_Address, NULL},
	{"APPLET", SPACE, HTML_EL_Applet, NULL},
	{"AREA", 'E', HTML_EL_AREA, NULL},
	{"B", SPACE, HTML_EL_Bold_text, NULL},
	{"BASE", 'E', HTML_EL_BASE, NULL},
	{"BASEFONT", 'E', HTML_EL_BaseFont, NULL},
	{"BIG", SPACE, HTML_EL_Big_text, NULL},
	{"BLOCKQUOTE", SPACE, HTML_EL_Block_Quote, NULL},
	{"BODY", SPACE, HTML_EL_BODY, NULL},
	{"BR", 'E', HTML_EL_BR, NULL},
	{"C", SPACE, HTML_EL_TEXT_UNIT, NULL},
	{"CAPTION", SPACE, HTML_EL_CAPTION, NULL},
	{"CENTER", SPACE, HTML_EL_Center, NULL},
	{"CITE", SPACE, HTML_EL_Cite, NULL},
	{"CODE", SPACE, HTML_EL_Code, NULL},
	{"DD", SPACE, HTML_EL_Definition, NULL},
	{"DFN", SPACE, HTML_EL_Def, NULL},
	{"DIR", SPACE, HTML_EL_Directory, NULL},
	{"DIV", SPACE, HTML_EL_Division, NULL},
	{"DL", SPACE, HTML_EL_Definition_List, NULL},
	{"DT", SPACE, HTML_EL_Term, NULL},
	{"EM", SPACE, HTML_EL_Emphasis, NULL},
	{"FONT", SPACE, HTML_EL_Font_, NULL},
	{"FORM", SPACE, HTML_EL_Form, NULL},
	{"H1", SPACE, HTML_EL_H1, NULL},
	{"H2", SPACE, HTML_EL_H2, NULL},
	{"H3", SPACE, HTML_EL_H3, NULL},
	{"H4", SPACE, HTML_EL_H4, NULL},
	{"H5", SPACE, HTML_EL_H5, NULL},
	{"H6", SPACE, HTML_EL_H6, NULL},
	{"HEAD", SPACE, HTML_EL_HEAD, NULL},
	{"HR", 'E', HTML_EL_Horizontal_Rule, NULL},
	{"HTML", SPACE, HTML_EL_HTML, NULL},
	{"I", SPACE, HTML_EL_Italic_text, NULL},
	{"IMG", 'E', HTML_EL_PICTURE_UNIT, NULL},
	{"INPUT", 'E', HTML_EL_Input, NULL},
	{"ISINDEX", 'E', HTML_EL_ISINDEX, NULL},
	{"KBD", SPACE, HTML_EL_Keyboard, NULL},
	{"LI", SPACE, HTML_EL_List_Item, NULL},
	{"LINK", 'E', HTML_EL_LINK, NULL},
	{"LISTING", SPACE, HTML_EL_Preformatted, NULL},		// converted to PRE
	{"MAP", SPACE, HTML_EL_MAP, NULL},
	{"MENU", SPACE, HTML_EL_Menu, NULL},
	{"META", 'E', HTML_EL_META, NULL},
	{"OL", SPACE, HTML_EL_Numbered_List, NULL},
	{"OPTION", SPACE, HTML_EL_Option, NULL},
	{"P", SPACE, HTML_EL_Paragraph, NULL},
	{"PARAM", 'E', HTML_EL_Parameter, NULL},
	{"PRE", SPACE, HTML_EL_Preformatted, NULL},
	{"SAMP", SPACE, HTML_EL_Sample, NULL},
	{"SCRIPT", SPACE, HTML_EL_SCRIPT, NULL},
	{"SELECT", SPACE, HTML_EL_Option_Menu, NULL},
	{"SMALL", SPACE, HTML_EL_Small_text, NULL},
	{"SPAN", SPACE, HTML_EL_TEXT_UNIT, NULL},
	{"STRIKE", SPACE, HTML_EL_Struck_text, NULL},
	{"STRONG", SPACE, HTML_EL_Strong, NULL},
	{"STYLE", SPACE, HTML_EL_Styles, NULL},
	{"SUB", SPACE, HTML_EL_Subscript, NULL},
	{"SUP", SPACE, HTML_EL_Superscript, NULL},
	{"TABLE", SPACE, HTML_EL_Table, NULL},
	{"TBODY", SPACE, HTML_EL_tbody, NULL},
	{"TD", SPACE, HTML_EL_Data_cell, NULL},
	{"TEXTAREA", SPACE, HTML_EL_Text_Area, NULL},
	{"TFOOT", SPACE, HTML_EL_tfoot, NULL},
	{"TH", SPACE, HTML_EL_Heading_cell, NULL},
	{"THEAD", SPACE, HTML_EL_thead, NULL},
	{"TITLE", SPACE, HTML_EL_TITLE, NULL},
	{"TR", SPACE, HTML_EL_Table_row, NULL},
	{"TT", SPACE, HTML_EL_Teletype_text, NULL},
	{"U", SPACE, HTML_EL_Underlined_text, NULL},
	{"UL", SPACE, HTML_EL_Unnumbered_List, NULL},
	{"VAR", SPACE, HTML_EL_Variable, NULL},
	{"XMP", SPACE, HTML_EL_Preformatted, NULL},	// converted to PRE
	{"", SPACE, -1, NULL}						// Last entry. Mandatory
};

// Mapping table of tree elements to HTML and to Squirrel doc.
// This must correspond exactly to the enum for HTML_EL values in HtmlTables.h
IGMapping IGMappingTable[] =
{
	//	HTML_EL_Anchor,						// 0
	{0, "<A", ">", "</A>",
		true},
	//	HTML_EL_Address,			
	{0, "<ADDRESS", ">\n", "</ADDRESS>\n",
		true},
	//	HTML_EL_Applet,
	{0, "<APPLET", ">\n", "</APPLET>\n",
		false},
	//	HTML_EL_AREA, 
	{0, "<AREA", ">\n", "",
		false},
	//	HTML_EL_BASE,			
	{0, "<BASE ", ">\n", "",
		false},
	//	HTML_EL_BaseFont,			
	{0, "<BASEFONT", ">\n", "",
		false},
	//	HTML_EL_Big_text,		
	{0, "<BIG", ">", "</BIG>",
		true},
	// HTML_EL_Block_Quote,				
	{0, "<BLOCKQUOTE", ">\n", "</BLOCKQUOTE>\n",
		true},
	//	HTML_EL_BODY, 
	{0, "<BODY", ">\n", "</BODY>\n",
		true},
	//	HTML_EL_Bold_text,	
	{0, "<B", ">", "</B>",
		true},
	//	HTML_EL_BR,					// 10			
	{0, "<BR", ">\n", "",
		true},
	//	HTML_EL_CAPTION,				
	{0, "<CAPTION", ">", "</CAPTION>\n",
		false},
	//	HTML_EL_Center,	
	{0, "\n<CENTER", ">\n", "</CENTER>\n",
		true},
	//	HTML_EL_Checkbox_Input,	
	{0, "<INPUT type=CHECKBOX", ">", "",
		false},
	//	HTML_EL_Cite,	
	{0, "<CITE", ">", "</CITE>",
		true},
	//	HTML_EL_Code,		
	{0, "<CODE", ">", "</CODE>",
		true},
	//	HTML_EL_Column_head,		
	{0, "<?", "", "</?>",
		false},
	//	HTML_EL_Comment_,				
	{0, "\n<!--", "", "-->\n",
		false},
	//	HTML_EL_Comment_line,		
	{0, "", "", "",
		false},
	//	HTML_EL_Data_cell,					
	{0, "<TD", ">", "",
		true},
	//	HTML_EL_Definition,			// 20				
	{0, "<DD", ">", "</DD>\n",
		false},
	//	HTML_EL_Definition_List,		
	{0, "<DL", ">\n", "</DL>\n",
		false},
	//	HTML_EL_Definition_Item,		
	{0, "<?>", "", "</?>",
		false},
	//	HTML_EL_Def,			
	{0, "<DFN", ">", "</DFN>",
		false},
	//	HTML_EL_Directory,		
	{0, "<DIR", ">\n", "</DIR>\n",
		true},
	//	HTML_EL_Division,		
	{0, "\n<DIV", ">\n", "</DIV>\n",
		true},
	//	HTML_EL_Document_URL,	
	{0, "", "", "",
		false},
	//	HTML_EL_Emphasis,					
	{0, "<EM", ">", "</EM>",
		true},
	//	HTML_EL_File_Input,			
	{0, "<INPUT type=FILE", ">", "",
		false},
	//	HTML_EL_Font_,	
	{0, "<FONT", ">", "</FONT>",
		true},
	//	HTML_EL_Form,				// 30		
	{0, "<FORM", ">\n", "</FORM>\n",
		true},
	//	HTML_EL_Frame,	
	{0, "<?>", "", "</?>",
		false},
	//	HTML_EL_H1,			
	{0, "<H1", ">", "</H1>\n",
		true},
	//	HTML_EL_H2,
	{0, "<H2", ">", "</H2>\n",
		true},
	//	HTML_EL_H3,
	{0, "<H3", ">", "</H3>\n",
		true},
	//	HTML_EL_H4,
	{0, "<H4", ">", "</H4>\n",
		true},
	//	HTML_EL_H5,
	{0, "<H5", ">", "</H5>\n",
		true},
	//	HTML_EL_H6,
	{0, "<H6", ">", "</H6>\n",
		true},
	//	HTML_EL_HEAD,
	{0, "<HEAD>\n", "", "</HEAD>\n",
		false},
	//	HTML_EL_Heading_cell,
	{0, "<TH", ">", "",
		true},
	//	HTML_EL_Hidden_Input,		// 40
	{0, "<INPUT type=HIDDEN", ">", "",
		false},
	//	HTML_EL_Horizontal_Rule,
	{0, "<HR", ">\n", "",
		true},
	//	HTML_EL_HTML,
	{0, "<HTML>\n", "", "</HTML>\n\n",
		true},
	//	HTML_EL_Input,
	{0, "<?>", "", "</?>",
		false},
	//	HTML_EL_Inserted_Text,
	{0, "<?>", "", "</?>",
		false},
	//	HTML_EL_Invalid_element,
	{0, "", "", ">",
		false},
	//	HTML_EL_ISINDEX,
	{1, "<ISINDEX", ">\n", "",
		false},
	//	HTML_EL_Italic_text,
	{0, "<I", ">", "</I>",
		true},
	//	HTML_EL_Keyboard,
	{0, "<KBD", ">", "</KBD>",
		true},
	//	HTML_EL_LINK,
	{0, "<LINK", ">\n", "",
		false},
	//	HTML_EL_Links,				// 50
	{0, "<?>", "", "</?>",
		false},
	//	HTML_EL_List_Item,
	{0, "<LI", ">", "",
		true},
	//	HTML_EL_MAP,
	{0, "<MAP", ">\n", "</MAP>\n",
		false},
	//	HTML_EL_Menu,
	{0, "<MENU", ">\n", "</MENU>\n",
		true},
	//	HTML_EL_Metas,
	{0, "<?>", "", "</?>",
		false},
	//	HTML_EL_META,
	{0, "<META", ">\n", "",
		false},
	//	HTML_EL_Numbered_List,
	{0, "<OL", ">\n", "</OL>\n",
		true},
	//	HTML_EL_Option,
	{0, "\n<OPTION", ">", "",
		false},
	// HTML_EL_Option_Menu,
	{0, "\n<SELECT", ">", "\n</SELECT>\n",
		false},
	//	HTML_EL_Paragraph,
	{0, "<P", ">", "</P>\n",
		true},
	//	HTML_EL_Parameter,			// 60
	{0, "<PARAM", ">\n", "",
		false},
	// HTML_EL_Password_Input,
	{0, "<INPUT type=PASSWORD", ">", "",
		false},
	//	HTML_EL_PICTURE_UNIT,
	{1, "<IMG", ">", "\n",
		true},
	//	HTML_EL_Pre_Line,
	{0, "\n", "", "",
		true},
	//	HTML_EL_Preformatted,
	{0, "<PRE", ">", "\n</PRE>\n",
		true},
	//	HTML_EL_Radio_Input,
	{0, "<INPUT type=RADIO", ">", "",
		false},
	//	HTML_EL_Reset_Input,
	{0, "\n<INPUT type=RESET", ">", "",
		false},
	//	HTML_EL_Sample,
	{0, "<SAMP", ">", "</SAMP>",
		true},
	//	HTML_EL_Scripts,
	{0, "<?>", "", "</?>",
		false},
	//	HTML_EL_SCRIPT,
	{0, "<SCRIPT", ">\n", "",
		false},
	//	HTML_EL_Small_text,			// 70
	{0, "<SMALL", ">", "</SMALL>",
		true},
	//	HTML_EL_Struck_text,
	{0, "<STRIKE", ">", "</STRIKE>",
		true},
	//	HTML_EL_Strong,
	{0, "<STRONG", ">", "</STRONG>",
		true},
	//	HTML_EL_Styles,
	{1, "<STYLE", ">\n", "</STYLE>\n",
		false},
	//	HTML_EL_Subscript,
	{0, "<SUB", ">", "</SUB>",
		true},
	// HTML_EL_Submit_Input,
	{0, "\n<INPUT type=SUBMIT", ">", "",
		false},
	//	HTML_EL_Superscript,
	{0, "<SUP", ">", "</SUP>",
		true},
	// HTML_EL_Table,
	{0, "\n<TABLE", ">\n", "</TABLE>\n",
		true},
	//	HTML_EL_Table_body,
	{0, "", "", "",
		false},
	//	HTML_EL_Table_cell_ghost,
	{0, "<?>", "", "</?>",
		false},
	//	HTML_EL_Table_foot,			// 80
	{0, "<?>", "", "</?>",
		false},
	//	HTML_EL_Table_head,
	{0, "<?>", "", "</?>",
		false},
	//	HTML_EL_Table_row,
	{0, "   <TR", ">\n", "   </TR>\n",
		true},
	//	HTML_EL_tbody,
	{0, "<TBODY", ">\n", "</TBODY>\n",
		false},
	//	HTML_EL_tfoot,
	{0, "<TFOOT", ">\n", "</TFOOT>\n",
		false},
	//	HTML_EL_thead,
	{0, "<THEAD", ">\n", "</THEAD>\n",
		false},
	//	HTML_EL_Teletype_text,
	{0, "<TT", ">", "</TT>",
		true},
	//	HTML_EL_Term,
	{0, "<DT", ">", "</DT>\n",
		false},
	//	HTML_EL_Term_List,
	{0, "<?>", "", "</?>",
		false},
	//	HTML_EL_TEXT_UNIT,
	{1, "", "", "",
		true},
	//HTML_EL_Text_Area,			// 90
	{0, "<TEXTAREA", ">", "</TEXTAREA>",
		false},
	// HTML_EL_Text_Input,
	{0, "<INPUT type=TEXT", ">", "",
		false},
	//	HTML_EL_TITLE,
	{0, "<TITLE>", "", "</TITLE>\n",
		false},
	//	HTML_EL_Underlined_text,
	{0, "<U", ">", "</U>",
		true},
	//	HTML_EL_Unnumbered_List,
	{0, "<UL", ">\n", "</UL>\n",
		true},
	//	HTML_EL_Variable
	{0, "<VAR", ">", "</VAR>",
		true}			
};


// mapping table of HTML to tree attributes
AttributeMapping AttributeMappingTable[] =
{
	// the first entry *must* be unknown_attr
	{"unknown_attr", "", 'A', HTML_ATTR_Invalid_attribute},
	{"ACTION", "", 'A', HTML_ATTR_Script_URL},
	{"CLEAR", "BR", 'A', HTML_ATTR_Clear},
	{"ALIGN", "APPLET", 'A', HTML_ATTR_Alignment},
	{"ALIGN", "CAPTION", 'A', HTML_ATTR_Position},
	{"ALIGN", "DIV", 'A', HTML_ATTR_Align},
	{"ALIGN", "H1", 'A', HTML_ATTR_Align},
	{"ALIGN", "H2", 'A', HTML_ATTR_Align},
	{"ALIGN", "H3", 'A', HTML_ATTR_Align},
	{"ALIGN", "H4", 'A', HTML_ATTR_Align},
	{"ALIGN", "H5", 'A', HTML_ATTR_Align},
	{"ALIGN", "H6", 'A', HTML_ATTR_Align},
	{"ALIGN", "HR", 'A', HTML_ATTR_Align},
	{"ALIGN", "IMG", 'A', HTML_ATTR_Alignment},
	{"ALIGN", "P", 'A', HTML_ATTR_Align},
	{"ALIGN", "TABLE", 'A', HTML_ATTR_Table_align},
	{"ALIGN", "TD", 'A', HTML_ATTR_Cell_align},
	{"ALIGN", "TH", 'A', HTML_ATTR_Cell_align},
	{"ALIGN", "TR", 'A', HTML_ATTR_Row_align},
	{"ALINK", "", 'A', HTML_ATTR_ActiveLinkColor},
	{"ALT", "", 'A', HTML_ATTR_ALT},
	{"BACKGROUND", "", 'A', HTML_ATTR_background_},
	{"BGCOLOR", "", 'A', HTML_ATTR_BackgroundColor},
	{"BORDER", "IMG", 'A', HTML_ATTR_Img_border},
	{"BORDER", "TABLE", 'A', HTML_ATTR_Border},
	{"CELLSPACING", "", 'A', HTML_ATTR_cellspacing},
	{"CELLPADDING", "", 'A', HTML_ATTR_cellpadding},
	{"CHECKED", "", 'A', HTML_ATTR_Checked},
	{"CLASS", "", 'A', HTML_ATTR_Class},
	{"CODE", "", 'A', HTML_ATTR_code},
	{"CODEBASE", "", 'A', HTML_ATTR_codebase},
	{"COLOR", "", 'A', HTML_ATTR_color},
	{"COLS", "", 'A', HTML_ATTR_Columns},
	{"COLSPAN", "", 'A', HTML_ATTR_colspan},
	{"COMPACT", "", 'A', HTML_ATTR_COMPACT},
	{"CONTENT", "", 'A', HTML_ATTR_meta_content},
	{"COORDS", "", 'A', HTML_ATTR_coords},
	{"ENCTYPE", "", 'A', HTML_ATTR_ENCTYPE},
	{"FACE", "FONT", 'A', HTML_ATTR_Font_face},
	{"HEIGHT", "APPLET", 'A', HTML_ATTR_Height_},
	{"HEIGHT", "IMG", 'A', HTML_ATTR_Height_},
	{"HEIGHT", "TD", 'A', HTML_ATTR_Cell_height},
	{"HEIGHT", "TH", 'A', HTML_ATTR_Cell_height},
	{"HREF", "", 'A', HTML_ATTR_HREF_},
	{"HSPACE", "", 'A', HTML_ATTR_hspace},
	{"HTTP-EQUIV", "", 'A', HTML_ATTR_http_equiv},
	{"ISMAP", "", 'A', HTML_ATTR_ISMAP},
	{"LINK", "", 'A', HTML_ATTR_LinkColor},
	{"MAXLENGTH", "", 'A', HTML_ATTR_MaxLength},
	{"METHOD", "", 'A', HTML_ATTR_METHOD},
	{"MULTIPLE", "", 'A', HTML_ATTR_Multiple},
	{"N", "", 'C', 0},
	{"NAME", "APPLET", 'A', HTML_ATTR_applet_name},
	{"NAME", "META", 'A', HTML_ATTR_meta_name},
	{"NAME", "PARAM", 'A', HTML_ATTR_Param_name},
	{"NAME", "", 'A', HTML_ATTR_NAME},
	{"NOHREF", "", 'A', HTML_ATTR_nohref},
	{"NOSHADE", "", 'A', HTML_ATTR_NoShade},
	{"NOWRAP", "", 'A', HTML_ATTR_Word_wrap},
	{"ONCHANGE", "", 'A', HTML_ATTR_OnChange},
	{"PROMPT", "", 'A', HTML_ATTR_Prompt},
	{"REL", "", 'A', HTML_ATTR_REL},
	{"REV", "", 'A', HTML_ATTR_REV},
	{"ROWS", "", 'A', HTML_ATTR_Rows},
	{"ROWSPAN", "", 'A', HTML_ATTR_rowspan},
	{"SELECTED", "", 'A', HTML_ATTR_Selected},
	{"SHAPE", "", 'A', HTML_ATTR_shape},
	{"SIZE", "BASEFONT", 'A', HTML_ATTR_BaseFontSize},
	{"SIZE", "FONT", 'A', HTML_ATTR_Font_size},
	{"SIZE", "HR", 'A', HTML_ATTR_Size_},
	{"SIZE", "INPUT", 'A', HTML_ATTR_Area_Size},
	{"SIZE", "SELECT", 'A', HTML_ATTR_MenuSize},
	{"SRC", "", 'A', HTML_ATTR_SRC},
	{"START", "", 'A', HTML_ATTR_Start},
	{"STYLE", "", 'A', HTML_ATTR_Style_},
	{"TEXT", "", 'A', HTML_ATTR_TextColor},
	{"TITLE", "", 'A', HTML_ATTR_Title},
	{"TYPE", "LI", 'A', HTML_ATTR_ItemStyle},
	{"TYPE", "OL", 'A', HTML_ATTR_NumberStyle},
	{"TYPE", "STYLE", 'A', HTML_ATTR_Notation},
	{"TYPE", "UL", 'A', HTML_ATTR_BulletStyle},
	{"TYPE", "", SPACE, DUMMY_ATTRIBUTE},
	{"USEMAP", "", 'A', HTML_ATTR_USEMAP},
	{"VALIGN", "TD", 'A', HTML_ATTR_Cell_valign},
	{"VALIGN", "TH", 'A', HTML_ATTR_Cell_valign},
	{"VALIGN", "TR", 'A', HTML_ATTR_Row_valign},
	{"VALUE", "LI", 'A', HTML_ATTR_ItemValue},
	{"VALUE", "PARAM", 'A', HTML_ATTR_Param_value},
	{"VALUE", "", 'A', HTML_ATTR_Value_},
	{"VERSION", "", 'A', 0},
	{"VLINK", "", 'A', HTML_ATTR_VisitedLinkColor},
	{"VSPACE", "", 'A', HTML_ATTR_vspace},
	{"WIDTH", "APPLET", 'A', HTML_ATTR_Width_},
	{"WIDTH", "HR", 'A', HTML_ATTR_Width__},
	{"WIDTH", "IMG", 'A', HTML_ATTR_Width_},
	{"WIDTH", "PRE", 'A', HTML_ATTR_WidthElement},
	{"WIDTH", "TABLE", 'A', HTML_ATTR_Width__},
	{"WIDTH", "TD", 'A', HTML_ATTR_Cell_width},
	{"WIDTH", "TH", 'A', HTML_ATTR_Cell_width},
	{"ZZGHOST", "", 'A', HTML_ATTR_Ghost_restruct},
	{"", "", '\0', 0}		// Last entry. Mandatory
};


// Mapping table of tree attributes to HTML.
// This must correspond exactly to the enum for HTML_ATTR values in HtmlTables.h
IGAttrMapping IGAttrMappingTable[] =
{
	{" alink=\"", "\"", 		ATTR_KIND_text},		// HTML_ATTR_ActiveLinkColor,	// 0
	{" align=", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Alignment,
	{" align=", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Align,
	{" alt=\"", "\"", 			ATTR_KIND_text},		// HTML_ATTR_ALT,
	{" name=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_applet_name,
	{" size=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_Area_Size,
	{" background=\"", "\"", 	ATTR_KIND_integer},		// HTML_ATTR_background_,
	{" bgcolor=\"", "\"", 		ATTR_KIND_text},		// HTML_ATTR_BackgroundColor,
	{" size=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_BaseFontSize,
	{" BORDER=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Border,
	{" type=", "", 				ATTR_KIND_enumerate},	// HTML_ATTR_BulletStyle,		// 10
	{" align=", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Cell_align,
	{" width=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Cell_width,
	{" valign=", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Cell_valign,
	{" clear=", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Clear,
	{" cellspacing=\"", "\"", 	ATTR_KIND_integer},		// HTML_ATTR_cellspacing,
	{" cellpadding=\"", "\"", 	ATTR_KIND_integer},		// HTML_ATTR_cellpadding,
	{" checked", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Checked,
	{" class=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Class,
	{" code=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_code,
	{" codebase=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_codebase,			// 20
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_ColExt,
	{" color=\"", "\"", 		ATTR_KIND_text},		// HTML_ATTR_color,
	{" cols=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_Columns,
	{" colspan=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_colspan,
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_Col_width_percent,
	{" ?", "", 					ATTR_KIND_enumerate},	// HTML_ATTR_Col_width_pxl,
	{" compact", "", 			ATTR_KIND_integer},		// HTML_ATTR_COMPACT,
	{" coords=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_coords,
	{" height=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Cell_height,
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_DefaultChecked,	// 30
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_DefaultSelected,
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_Default_Value,
	{" enctype=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_ENCTYPE,				
	{"", "", 					ATTR_KIND_integer},		// HTML_ATTR_Error_type,
	{" face=\"", "\"", 			ATTR_KIND_text},		// HTML_ATTR_Font_face,
	{" size=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_Font_size,
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_Ghost_restruct,
	{" height=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_height_,
	{" height=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Height_,
	{" href=\"", "\"", 			ATTR_KIND_text},		// HTML_ATTR_HREF_,				// 40
	{" hspace=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_hspace,
	{" http-equiv=\"", "\"", 	ATTR_KIND_integer},		// HTML_ATTR_http_equiv,
	{" border=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Img_border,
	{" type=", "", 				ATTR_KIND_integer},		// HTML_ATTR_IntItemStyle,			
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_IntMaxVol,
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_IntSizeDecr,
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_IntSizeIncr,
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_IntSizeRel,
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_IntWidthPercent,
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_IntWidthPxl,		// 50
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_Invalid_attribute,
	{" ismap", "", 				ATTR_KIND_enumerate},	// HTML_ATTR_ISMAP,
	{" type=", "", 				ATTR_KIND_enumerate},	// HTML_ATTR_ItemStyle,
	{" value=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_ItemValue,
	{" link=\"", "\"", 			ATTR_KIND_text},		// HTML_ATTR_LinkColor,			
	{" maxlength=\"", "\"", 	ATTR_KIND_integer},		// HTML_ATTR_MaxLength,
	{" size=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_MenuSize,
	{" content=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_meta_content,
	{" method=", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_METHOD,
	{" multiple", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Multiple,			// 60
	{" name=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_meta_name,
	{" name=\"", "\"", 			ATTR_KIND_text},		// HTML_ATTR_NAME,
	{" nohref", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_nohref,
	{" ?", "", 					ATTR_KIND_enumerate},	// HTML_ATTR_NoShade,
	{" type=", "", 				ATTR_KIND_enumerate},	// HTML_ATTR_NumberStyle,			
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_Notation,
	{" onchange=\"", "\"", 		ATTR_KIND_text},		// HTML_ATTR_OnChange,
	{" name=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_Param_name,
	{" value=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Param_value,
	{" align=", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Position,			// 70
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_Prompt,
	{"", "", 					ATTR_KIND_integer},		// HTML_ATTR_PseudoClass,
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_Ref_column,
	{" rel=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_REL,
	{" rev=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_REV,
	{" rows=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_Rows,					
	{" valign=", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Row_valign,
	{" align=", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Row_align,
	{" ?", "", 					ATTR_KIND_enumerate},	// HTML_ATTR_RowExt,
	{" rowspan=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_rowspan,			// 80
	{" selected", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Selected,
	{" shape=", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_shape,
	{" size=\"", "\"", 			ATTR_KIND_integer},		// HTML_ATTR_Size_,
	{" src=\"", "\"", 			ATTR_KIND_text},		// HTML_ATTR_SRC,
	{" action=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Script_URL,
	{" start=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Start,
	{" style=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Style_,				
	{" align=", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Table_align,
	{" text=\"", "\"", 			ATTR_KIND_text},		// HTML_ATTR_TextColor,
	{" title=\"", "\"", 		ATTR_KIND_text},		// HTML_ATTR_Title,				// 90
	{" usemap=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_USEMAP,
	{" value=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Value_,
	{" vlink=\"", "\"", 		ATTR_KIND_text},		// HTML_ATTR_VisitedLinkColor,
	{" vspace=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_vspace,
	{" width=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_width_,
	{" width=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Width_,
	{" width=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_Width__,				
	{" width=\"", "\"", 		ATTR_KIND_integer},		// HTML_ATTR_WidthElement,
	{" nowrap", "", 			ATTR_KIND_enumerate},	// HTML_ATTR_Word_wrap,
	{" ?", "", 					ATTR_KIND_integer},		// HTML_ATTR_x_coord,			// 100
	{" ?", "", 					ATTR_KIND_integer}		// HTML_ATTR_y_coord
};


// mapping table of HTML attribute values
AttrValueMapping AttrValueMappingTable[] =
{
	{HTML_ATTR_Align, "LEFT", HTML_ATTR_Align_VAL_left_},
	{HTML_ATTR_Align, "CENTER", HTML_ATTR_Align_VAL_center_},
	{HTML_ATTR_Align, "RIGHT", HTML_ATTR_Align_VAL_right_},
	
	{HTML_ATTR_Alignment, "TOP", HTML_ATTR_Alignment_VAL_Top_},
	{HTML_ATTR_Alignment, "MIDDLE", HTML_ATTR_Alignment_VAL_Middle_},
	{HTML_ATTR_Alignment, "BOTTOM", HTML_ATTR_Alignment_VAL_Bottom_},
	{HTML_ATTR_Alignment, "LEFT", HTML_ATTR_Alignment_VAL_Left_},
	{HTML_ATTR_Alignment, "RIGHT", HTML_ATTR_Alignment_VAL_Right_},
	
	{HTML_ATTR_BulletStyle, "DISC", HTML_ATTR_BulletStyle_VAL_disc},
	{HTML_ATTR_BulletStyle, "SQUARE", HTML_ATTR_BulletStyle_VAL_square},
	{HTML_ATTR_BulletStyle, "CIRCLE", HTML_ATTR_BulletStyle_VAL_circle},
	
	{HTML_ATTR_Cell_align, "LEFT", HTML_ATTR_Cell_align_VAL_Cell_left},
	{HTML_ATTR_Cell_align, "CENTER", HTML_ATTR_Cell_align_VAL_Cell_center},
	{HTML_ATTR_Cell_align, "RIGHT", HTML_ATTR_Cell_align_VAL_Cell_right},
	
	{HTML_ATTR_Cell_valign, "TOP", HTML_ATTR_Cell_valign_VAL_Cell_top},
	{HTML_ATTR_Cell_valign, "MIDDLE", HTML_ATTR_Cell_valign_VAL_Cell_middle},
	{HTML_ATTR_Cell_valign, "BOTTOM", HTML_ATTR_Cell_valign_VAL_Cell_bottom},
	
	{HTML_ATTR_Clear, "LEFT", HTML_ATTR_Clear_VAL_Left_},
	{HTML_ATTR_Clear, "RIGHT", HTML_ATTR_Clear_VAL_Right_},
	{HTML_ATTR_Clear, "ALL", HTML_ATTR_Clear_VAL_All_},
	{HTML_ATTR_Clear, "NONE", HTML_ATTR_Clear_VAL_None},
	
	{HTML_ATTR_ItemStyle, "1", HTML_ATTR_ItemStyle_VAL_Arabic_},
	{HTML_ATTR_ItemStyle, "a", HTML_ATTR_ItemStyle_VAL_LowerAlpha},
	{HTML_ATTR_ItemStyle, "A", HTML_ATTR_ItemStyle_VAL_UpperAlpha},
	{HTML_ATTR_ItemStyle, "i", HTML_ATTR_ItemStyle_VAL_LowerRoman},
	{HTML_ATTR_ItemStyle, "I", HTML_ATTR_ItemStyle_VAL_UpperRoman},
	{HTML_ATTR_ItemStyle, "DISC", HTML_ATTR_ItemStyle_VAL_disc},
	{HTML_ATTR_ItemStyle, "SQUARE", HTML_ATTR_ItemStyle_VAL_square},
	{HTML_ATTR_ItemStyle, "CIRCLE", HTML_ATTR_ItemStyle_VAL_circle},
	
	{HTML_ATTR_METHOD, "GET", HTML_ATTR_METHOD_VAL_Get_},
	{HTML_ATTR_METHOD, "POST", HTML_ATTR_METHOD_VAL_Post_},
	
	{HTML_ATTR_NumberStyle, "1", HTML_ATTR_NumberStyle_VAL_Arabic_},
	{HTML_ATTR_NumberStyle, "a", HTML_ATTR_NumberStyle_VAL_LowerAlpha},
	{HTML_ATTR_NumberStyle, "A", HTML_ATTR_NumberStyle_VAL_UpperAlpha},
	{HTML_ATTR_NumberStyle, "i", HTML_ATTR_NumberStyle_VAL_LowerRoman},
	{HTML_ATTR_NumberStyle, "I", HTML_ATTR_NumberStyle_VAL_UpperRoman},
	
	{HTML_ATTR_Position, "TOP", HTML_ATTR_Position_VAL_Position_top},
	{HTML_ATTR_Position, "BOTTOM", HTML_ATTR_Position_VAL_Position_bottom},
	
	{HTML_ATTR_Row_align, "LEFT", HTML_ATTR_Row_align_VAL_Row_left},
	{HTML_ATTR_Row_align, "CENTER", HTML_ATTR_Row_align_VAL_Row_center},
	{HTML_ATTR_Row_align, "RIGHT", HTML_ATTR_Row_align_VAL_Row_right},
	
	{HTML_ATTR_Row_valign, "TOP", HTML_ATTR_Row_valign_VAL_Row_top},
	{HTML_ATTR_Row_valign, "MIDDLE", HTML_ATTR_Row_valign_VAL_Row_middle},
	{HTML_ATTR_Row_valign, "BOTTOM", HTML_ATTR_Row_valign_VAL_Row_bottom},
	
	{HTML_ATTR_shape, "RECT", HTML_ATTR_shape_VAL_rectangle},
	{HTML_ATTR_shape, "CIRCLE", HTML_ATTR_shape_VAL_circle},
	{HTML_ATTR_shape, "POLY", HTML_ATTR_shape_VAL_polygon},
	
	{HTML_ATTR_Table_align, "LEFT", HTML_ATTR_Table_align_VAL_Align_left},
	{HTML_ATTR_Table_align, "CENTER", HTML_ATTR_Table_align_VAL_Center_},
	{HTML_ATTR_Table_align, "RIGHT", HTML_ATTR_Table_align_VAL_Align_right},
	
	// HTML attribute TYPE generates a tree element
	{DUMMY_ATTRIBUTE, "CHECKBOX", HTML_EL_Checkbox_Input},
	{DUMMY_ATTRIBUTE, "HIDDEN", HTML_EL_Hidden_Input},
	{DUMMY_ATTRIBUTE, "FILE", HTML_EL_File_Input},
	{DUMMY_ATTRIBUTE, "IMAGE", HTML_EL_PICTURE_UNIT},
	{DUMMY_ATTRIBUTE, "PASSWORD", HTML_EL_Password_Input},
	{DUMMY_ATTRIBUTE, "RADIO", HTML_EL_Radio_Input},
	{DUMMY_ATTRIBUTE, "RESET", HTML_EL_Reset_Input},
	{DUMMY_ATTRIBUTE, "SUBMIT", HTML_EL_Submit_Input},
	{DUMMY_ATTRIBUTE, "TEXT", HTML_EL_Text_Input},
	
	// The following declarations allow the parser to accept bool attributes
	// written "checked=CHECKED", for instance
	{HTML_ATTR_Checked, "CHECKED", HTML_ATTR_Checked_VAL_Yes_},
	{HTML_ATTR_COMPACT, "COMPACT", HTML_ATTR_COMPACT_VAL_Yes_},
	{HTML_ATTR_ISMAP, "ISMAP", HTML_ATTR_ISMAP_VAL_Yes_},
	{HTML_ATTR_Multiple, "MULTIPLE", HTML_ATTR_Multiple_VAL_Yes_},
	{HTML_ATTR_nohref, "NOHREF", HTML_ATTR_nohref_VAL_Yes_},
	{HTML_ATTR_NoShade, "NOSHADE", HTML_ATTR_NoShade_VAL_NoShade_},
	{HTML_ATTR_Selected, "SELECTED", HTML_ATTR_Selected_VAL_Yes_},
	{HTML_ATTR_Word_wrap, "NOWRAP", HTML_ATTR_Word_wrap_VAL_No_wrap},
	{0, "", 0}			// Last entry. Mandatory
};


ISOlat1entry ISOlat1table[] =
{
	{"AElig", 198},
	{"Aacute", 193},
	{"Acirc", 194},
	{"Agrave", 192},
	{"Aring", 197},
	{"Atilde", 195},
	{"Auml", 196},
	{"Ccedil", 199},
	{"ETH", 208},
	{"Eacute", 201},
	{"Ecirc", 202},
	{"Egrave", 200},
	{"Euml", 203},
	{"Iacute", 205},
	{"Icirc", 206},
	{"Igrave", 204},
	{"Iuml", 207},
	{"Ntilde", 209},
	{"Oacute", 211},
	{"Ocirc", 212},
	{"Ograve", 210},
	{"Oslash", 216},
	{"Otilde", 213},
	{"Ouml", 214},
	{"THORN", 222},
	{"Uacute", 218},
	{"Ucirc", 219},
	{"Ugrave", 217},
	{"Uuml", 220},
	{"Yacute", 221},
	{"aacute", 225},
	{"acirc", 226},
	{"acute", 180},
	{"aelig", 230},
	{"agrave", 224},
	{"amp", 38},
	{"aring", 229},
	{"atilde", 227},
	{"auml", 228},
	{"brvbar", 166},
	{"ccedil", 231},
	{"cedil", 184},
	{"cent", 162},
	{"copy", 169},
	{"curren", 164},
	{"deg", 176},
	{"die", 168},
	{"divide", 247},
	{"eacute", 233},
	{"ecirc", 234},
	{"egrave", 232},
	{"eth", 240},
	{"euml", 235},
	{"frac12", 189},
	{"frac14", 188},
	{"frac34", 190},
	{"gt", 62},			/* Numeric and Special Graphic Entity */
	{"iacute", 237},
	{"icirc", 238},
	{"iexcl", 161},
	{"igrave", 236},
	{"iquest", 191},
	{"iuml", 239},
	{"laquo", 171},		/* added by VQ */
	{"lt", 60},			/* Numeric and Special Graphic Entity */
	{"macr", 175},
	{"micro", 181},
	{"middot", 183},
	{"nbsp", 160},		// Non breaking space.  This *should* be 160, not 32 --SL
	{"not", 172},
	{"ntilde", 241},
	{"oacute", 243},
	{"ocirc", 244},
	{"ograve", 242},
	{"ordf", 170},
	{"ordm", 186},
	{"oslash", 248},
	{"otilde", 245},
	{"ouml", 246},
	{"para", 182},
	{"plusmn", 177},
	{"pound", 163},
	{"quot", 34},		/* Numeric and Special Graphic Entity */
	{"raquo", 187},		/* added by VQ */
	{"reg", 174},
	{"sect", 167},
	{"shy", 173},			/* Soft hyphen */
	{"sup1", 185},
	{"sup2", 178},
	{"sup3", 179},
	{"szlig", 223},
	{"thorn", 254},
	{"times", 215},
	{"uacute", 250},
	{"ucirc", 251},
	{"ugrave", 249},
	{"uuml", 252},
	{"yacute", 253},
	{"yen", 165},
	{"yuml", 255},
	{"zzzz", 0}		// this last entry is required
};


// Look in the GIMapping table for this element type
char *GetGITagName(int elementType)
{
	int	i = 0;
	
	if (elementType >= 0)
	{
		do
		{
			if (GIMappingTable[i].TreeType == elementType &&
					strcmp ((char *)GIMappingTable[i].htmlGI, "LISTING"))	// use PRE
				return (char *) GIMappingTable[i].htmlGI;
			i++;
		}
		while (GIMappingTable[i].htmlGI[0] != '\0');
	}
	
	return "???";
}


// Look in the ISOlat1table for this character
char *GetISOName(int theChar)
{
	if (theChar < 128 || theChar > 255)
		return NULL;
		
	for (long i = 0; ISOlat1table[i].charCode != 0; i++)
	{
		if (ISOlat1table[i].charCode == theChar)
			return (char *) ISOlat1table[i].charName;
	}
	 
	return NULL;
}