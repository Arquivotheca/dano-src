#include <SupportDefs.h>
#include "Replace.h"

// Computes whether or not a given replacement option is compatible with a given
// version of the package file format
bool IsCompatibleReplacementOption(uint16 version, uint32 replacement)
{
	bool compat = false;
	// the following construct works because all version 1 replacement options
	// are valid version 2 replacement options.
	switch (version) {
	case 2: // version 2 (R4.5 compatible)
		switch (replacement) {
		case R_ASK_VERSION_NEWER:
		case R_ASK_CREATION_NEWER:
		case R_ASK_MODIFICATION_NEWER:
			compat = true;
			break;
		}
		// fall through
	case 1: // version 1 (R4 compatible)
		switch (replacement) {
		case R_ASK_USER: 					// fall through
		case R_NEVER_REPLACE:				// fall through
		case R_RENAME:						// fall through
		case R_REPLACE_VERSION_NEWER:		// fall through
		case R_REPLACE_CREATION_NEWER:		// fall through
		case R_REPLACE_MODIFICATION_NEWER:	// fall through
		case R_MERGE_FOLDER:				// fall through
		case R_INSTALL_IF_EXISTS:			// fall through
			compat = true;
			break;
		}
		break;
	}		
	return compat;
}
