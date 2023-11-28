/*	$Id: dwarfnames.h,v 1.1 1998/10/21 12:03:12 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 10:58:20
*/

#ifndef DWARFNAMES_H
#define DWARFNAMES_H

// These name arrays are all strictly tied to the numeric values they represent,
// which are used as indices into the arrays.

char *kDWName[] = {
	NULL,
    "DW_AT_sibling",
    "DW_AT_location",
    "DW_AT_name",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
    "DW_AT_ordering",
    "DW_AT_subscr_data",
    "DW_AT_byte_size",
    "DW_AT_bit_offset",
    "DW_AT_bit_size",
	NULL,
    "DW_AT_element_list",
    "DW_AT_stmt_list",
    "DW_AT_low_pc",
    "DW_AT_high_pc",
    "DW_AT_language",
    "DW_AT_member",
    "DW_AT_discr",
    "DW_AT_discr_value",
    "DW_AT_visibility",
    "DW_AT_import",
    "DW_AT_string_length",
    "DW_AT_common_reference",
    "DW_AT_comp_dir",
    "DW_AT_const_value",
    "DW_AT_containing_type",
    "DW_AT_default_value",
	NULL,
    "DW_AT_inline",
    "DW_AT_is_optional",
    "DW_AT_lower_bound",
	NULL,
	NULL,
    "DW_AT_producer",
	NULL,
    "DW_AT_prototyped",
	NULL,
	NULL,
    "DW_AT_return_addr",
	NULL,
    "DW_AT_start_scope",
	NULL,
    "DW_AT_stride_size",
    "DW_AT_upper_bound",
	NULL,
    "DW_AT_abstract_origin",
    "DW_AT_accessibility",
    "DW_AT_address_class",
    "DW_AT_artificial",
    "DW_AT_base_types",
    "DW_AT_calling_convention",
    "DW_AT_count",
    "DW_AT_data_member_location",
    "DW_AT_decl_column",
    "DW_AT_decl_file",
    "DW_AT_decl_line",
    "DW_AT_declaration",
    "DW_AT_discr_list",
    "DW_AT_encoding",
    "DW_AT_external",
    "DW_AT_frame_base",
    "DW_AT_friend",
    "DW_AT_identifier_case",
    "DW_AT_macro_info",
    "DW_AT_namelist_items",
    "DW_AT_priority",
    "DW_AT_segment",
    "DW_AT_specification",
    "DW_AT_static_link",
    "DW_AT_type",
    "DW_AT_use_location",
    "DW_AT_variable_parameter",
    "DW_AT_virtuality",
    "DW_AT_vtable_elem_location"
};


char *kTagName[] = {
    "DW_TAG_padding",
    "DW_TAG_array_type",
    "DW_TAG_class_type",
    "DW_TAG_entry_point",
    "DW_TAG_enumeration_type",
    "DW_TAG_formal_parameter",
	NULL,
	NULL,
    "DW_TAG_imported_declaration",
	NULL,
    "DW_TAG_label",
    "DW_TAG_lexical_block",
	NULL,
    "DW_TAG_member",
	NULL,
    "DW_TAG_pointer_type",
    "DW_TAG_reference_type",
    "DW_TAG_compile_unit",
    "DW_TAG_string_type",
    "DW_TAG_structure_type",
	NULL,
    "DW_TAG_subroutine_type",
    "DW_TAG_typedef",
    "DW_TAG_union_type",
    "DW_TAG_unspecified_parameters",
    "DW_TAG_variant",
    "DW_TAG_common_block",
    "DW_TAG_common_inclusion",
    "DW_TAG_inheritance",
    "DW_TAG_inlined_subroutine",
    "DW_TAG_module",
    "DW_TAG_ptr_to_member_type",
    "DW_TAG_set_type",
    "DW_TAG_subrange_type",
    "DW_TAG_with_stmt",
    "DW_TAG_access_declaration",
    "DW_TAG_base_type",
    "DW_TAG_catch_block",
    "DW_TAG_const_type",
    "DW_TAG_constant",
    "DW_TAG_enumerator",
    "DW_TAG_file_type",
    "DW_TAG_friend",
    "DW_TAG_namelist",
    "DW_TAG_namelist_item",
    "DW_TAG_packed_type",
    "DW_TAG_subprogram",
    "DW_TAG_template_type_param",
    "DW_TAG_template_value_param",
    "DW_TAG_thrown_type",
    "DW_TAG_try_block",
    "DW_TAG_variant_part",
    "DW_TAG_variable",
    "DW_TAG_volatile_type",
};

const char* kFormName[] = {
	NULL,
	"DW_FORM_addr",
	NULL,								// 0x02 is undefined
	"DW_FORM_string",
	"DW_FORM_block4",
	"DW_FORM_data2",
	"DW_FORM_data4",
	"DW_FORM_data8",
	"DW_FORM_string",
	"DW_FORM_block",
	"DW_FORM_block1",
	"DW_FORM_data1",
	"DW_FORM_flag",
	"DW_FORM_sdata",
	"DW_FORM_strp",
	"DW_FORM_udata",
	"DW_FORM_ref_addr",
	"DW_FORM_ref1",
	"DW_FORM_ref2",
	"DW_FORM_ref4",
	"DW_FORM_ref8",
	"DW_FORM_ref_udata",
	"DW_FORM_indirect"
};

#endif
