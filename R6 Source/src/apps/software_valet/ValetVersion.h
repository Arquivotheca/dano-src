//
//  ValetVersion.h
//
//  Contains declarations for various SoftwareValet version information.  The
//  actual symbols are defined in ValetVersion.cpp
//

#ifndef VALET_VERSION_H_
#define VALET_VERSION_H_

#include <AppFileInfo.h> // for version_info struct definition

// strings
extern const char *kInternalVersionNumberString;
extern const char *kProductNameString;
extern const char *kVersionNumberString;
extern const char *kVersionString;
extern const char *kReleaseDateString;

extern const char *kPackageNameString;
extern const char *kPackageDescString;
extern const char *kPackageDevelString;

// numeric
extern const version_info kVersionInfo;

#endif // VALET_VERSION_H_
