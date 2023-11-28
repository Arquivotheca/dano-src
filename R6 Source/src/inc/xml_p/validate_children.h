#ifndef VALIDATE_CHILDREN_H
#define VALIDATE_CHILDREN_H

#include <xml/BContent.h>

namespace B {
namespace XML {

// Give this an elementDecl and it will give you a pointer to the transition
// tables that it uses to do the validating.  This data structure is opaque.
// Don't forget to free it with _free_validate_table_
void		_init_validate_table_(BElementDecl * decl, const void ** tablePtr);

// Give this a pointer to a uint16.  It initializes it.  Do this before you start.
void		_init_validate_state_(uint16 * state);

// Give it the state, the table and an element name.  Will return B_OK if it's
// allowed, and B_XML_CHILD_ELEMENT_NOT_ALLOWED if it's not.
status_t	_check_next_element_(const char * element, uint16 * state, const void * table);

// Give it the state and the table, and it will return B_OK if it's okay to
// be done with children here, and B_XML_CHILD_PATTERN_NOT_FINISHED if it's not.
status_t	_check_end_of_element_(uint16 state, const void * table);

// Free the private table data
void		_free_validate_table_(const void * tableData);

}; // namespace XML
}; // namespace B

#endif // VALIDATE_CHILDREN_H
