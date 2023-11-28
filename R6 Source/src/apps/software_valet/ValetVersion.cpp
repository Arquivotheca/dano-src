//
//  ValetVersion.cpp
//
//  Contains definitions for various SoftwareValet version information. 
//

#include "ValetVersion.h"
#include <string.h>
// strings
const char *kInternalVersionNumberString = "040501"; // 4.51
const char *kProductNameString = "SoftwareValet";
const char *kVersionNumberString = "4.51";
const char *kVersionString = "SoftwareValet v4.51";
const char *kReleaseDateString = "Released June 29, 1999";

const char *kPackageNameString = "BeOS\xE2\x84\xA2"; // BeOS with a TM after it
const char *kPackageDescString = "BeOS\xE2\x84\xA2 operating system";
const char *kPackageDevelString = "Be, Inc.";

// version_info struct
const version_info kVersionInfo = {4, 5, 1, 0, 0, *kVersionNumberString, *kVersionString};

