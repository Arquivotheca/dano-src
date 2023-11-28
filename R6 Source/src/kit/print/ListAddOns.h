#ifndef LISTADDONS_H_
#define LISTADDONS_H_

namespace BPrivate
{

// returns a new'ed char* containing a list of colon-separated partial paths.
extern char* ListAddOns(char* subpath=NULL);

char* parse_add_on_path(char* path);
void add_names(const char*,const char*,char**);
void add_unique_name(const char*,char**);

}

using namespace BPrivate;

#endif // LISTADDONS_H_
